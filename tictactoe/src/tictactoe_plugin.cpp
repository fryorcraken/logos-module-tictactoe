#include "tictactoe_plugin.h"
#include "tictactoe.pb.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include <QByteArray>
#include <QDebug>
#include <QVariantList>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static FILE* ttt_trace_file()
{
    static FILE* f = []() {
        const char* path = std::getenv("TICTACTOE_TRACE_PATH");
        if (!path || !*path) path = "/tmp/tictactoe-trace.log";
        FILE* fp = std::fopen(path, "a");
        if (fp) setvbuf(fp, nullptr, _IOLBF, 0);
        return fp;
    }();
    return f;
}

#define TTT_TRACE(...) do { \
    if (FILE* _f = ttt_trace_file()) { \
        fprintf(_f, "[tictactoe pid=%d] ", (int)getpid()); \
        fprintf(_f, __VA_ARGS__); \
        fprintf(_f, "\n"); \
    } \
    fprintf(stderr, "[tictactoe] " __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    fflush(stderr); \
} while (0)

// Pick a free OS-assigned port on 0.0.0.0 for `type` (SOCK_STREAM or
// SOCK_DGRAM). Workaround for logos-delivery-module#24: createNode fails
// silently when tcpPort or discv5UdpPort is 0, so we pick ephemeral ports
// ourselves and pass concrete numbers. Inherent TOCTOU window between close()
// and delivery_module's bind(), but acceptable for local dev multiplayer.
//
// Ideally delivery_module would handle this itself — either by accepting
// port 0 as "OS-assigned" per POSIX convention (its own factory code at
// waku/factory/waku.nim:316-319 already claims to support this), or by
// picking free ports by default so modules don't need to care about port
// assignment at all. Once that's fixed upstream, this helper and the
// pickFreePort calls in enableMultiplayer() can go away.
static int pickFreePort(int type)
{
    int fd = ::socket(AF_INET, type, 0);
    if (fd < 0) return 0;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = 0;
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        return 0;
    }
    socklen_t len = sizeof(addr);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
        ::close(fd);
        return 0;
    }
    int port = ntohs(addr.sin_port);
    ::close(fd);
    return port;
}

TicTacToePlugin::TicTacToePlugin(QObject* parent)
    : QObject(parent)
    , m_game(tictactoe_new())
{
}

TicTacToePlugin::~TicTacToePlugin()
{
    tictactoe_free(m_game);
    delete logos;
}

void TicTacToePlugin::initLogos(LogosAPI* logosAPIInstance)
{
    // `logosAPI` is the member inherited from `PluginInterface` (see
    // core/interface.h in the SDK) — the host owns its lifetime, so we
    // only assign, never delete. `logos` is ours.
    if (logos) {
        delete logos;
        logos = nullptr;
    }
    logosAPI = logosAPIInstance;
    if (logosAPI) {
        logos = new LogosModules(logosAPI);
    }
}

// ── Core game methods ─────────────────────────────────────────────

void TicTacToePlugin::newGame()
{
    tictactoe_reset(m_game);
    broadcastNewGame();
    emit eventResponse("newGame", {});
}

int TicTacToePlugin::play(int row, int col)
{
    int currentP = static_cast<int>(tictactoe_current_player(m_game));
    TicTacToeError err = tictactoe_play(m_game, row, col);
    if (err == TICTACTOE_OK) {
        TicTacToeStatus s = tictactoe_status(m_game);
        broadcastMove(row, col, currentP);
        emit eventResponse("played", QVariantList() << row << col << static_cast<int>(s));
    }
    return static_cast<int>(err);
}

int TicTacToePlugin::status()
{
    return static_cast<int>(tictactoe_status(m_game));
}

int TicTacToePlugin::getCell(int row, int col)
{
    return static_cast<int>(tictactoe_get_cell(m_game, row, col));
}

int TicTacToePlugin::currentPlayer()
{
    return static_cast<int>(tictactoe_current_player(m_game));
}

// ── Multiplayer ───────────────────────────────────────────────────

// NOTE: the RPC chain below (createNode → start → subscribe, and the symmetric
// unsubscribe/stop in disableMultiplayer, and send() in sendGameMessage) uses
// the SYNCHRONOUS `invokeRemoteMethod`. Each call blocks the host thread,
// which means the UI freezes in "connecting…" until createNode+start+subscribe
// all complete (often hundreds of ms combined). We'd prefer
// `invokeRemoteMethodAsync` with callbacks so status could progress
// incrementally, but that API only exists on `logos-module-builder` master —
// we're pinned to `tutorial-v1` because basecamp v0.1.1's bundled
// delivery_module speaks the pre-LogosResult wire format. Same branch split
// that keeps onEvent broken for C++ (see tictactoe-ui-cpp's poll workaround).
// Resolution path: once basecamp ships a build with the newer delivery_module
// wrapper we can bump to master, switch these calls to
// `invokeRemoteMethodAsync`, and drop the QTimer poll in tictactoe-ui-cpp.
// Until then the sync path is intentional, not an oversight.
void TicTacToePlugin::enableMultiplayer()
{
    TTT_TRACE("enableMultiplayer() entered");
    emit eventResponse("mpStatusChanged", QVariantList() << 1); // 1=connecting (early)
    if (m_mpEnabled) { TTT_TRACE("already enabled, returning"); return; }
    if (!logos) {
        TTT_TRACE("logos is null");
        setMpError("Logos SDK not initialized");
        return;
    }

    // Register delivery_module event handlers before the start/subscribe
    // RPCs so Qt has them in place when events start arriving. Lambdas
    // gate on m_mpEnabled so they stay inert until the enable sequence
    // succeeds and during/after disableMultiplayer. Registration itself
    // runs once per plugin lifetime — LogosAPIConsumer::onEvent appends
    // without dedup and exposes no .off(), so a naive enable → disable →
    // enable cycle would otherwise stack duplicate handlers. Events
    // arrive via Qt queued connections from the host's RPC thread, so
    // they cannot fire synchronously inside our start() call — the
    // m_mpEnabled gate will not drop a same-stack event.
    if (!m_handlersRegistered) {
        logos->delivery_module.on("messageReceived",
            [this](const QString& /*eventName*/, const QVariantList& data) {
                if (!m_mpEnabled) return;
                if (data.size() < 3) {
                    qWarning() << "TicTacToePlugin: messageReceived payload too short";
                    return;
                }
                // messageReceived delivers data[2] as base64. We encode our own
                // payload as base64 before send(), so the full path is
                // decode(delivery base64) → decode(our base64) → protobuf.
                QByteArray deliveryPayload = QByteArray::fromBase64(data[2].toString().toUtf8());
                QByteArray protoBytes = QByteArray::fromBase64(deliveryPayload);

                tictactoe::GameMessage msg;
                if (!msg.ParseFromArray(protoBytes.data(), protoBytes.size())) {
                    qWarning() << "TicTacToePlugin: failed to parse protobuf message";
                    return;
                }

                if (msg.has_move()) {
                    int row = msg.move().row();
                    int col = msg.move().col();
                    qDebug() << "TicTacToePlugin: received remote move" << row << col
                             << "player" << msg.move().player();
                    TicTacToeError err = tictactoe_play(m_game, row, col);
                    if (err != TICTACTOE_OK) {
                        qWarning() << "TicTacToePlugin: remote move rejected by libtictactoe, err=" << err;
                        return;
                    }
                    m_msgReceived++;
                    TicTacToeStatus s = tictactoe_status(m_game);
                    emit eventResponse("remoteMove", QVariantList() << row << col << static_cast<int>(s));
                } else if (msg.has_new_game()) {
                    qDebug() << "TicTacToePlugin: received remote newGame";
                    tictactoe_reset(m_game);
                    m_msgReceived++;
                    emit eventResponse("remoteNewGame", {});
                }
            });

        logos->delivery_module.on("connectionStateChanged",
            [this](const QString& /*eventName*/, const QVariantList& data) {
                if (!m_mpEnabled) return;
                m_mpConnected = data.size() > 0 && !data[0].toString().isEmpty();
                qDebug() << "TicTacToePlugin: connection state changed, connected:" << m_mpConnected;
                int status = m_mpConnected ? 2 : 1; // 2=connected, 1=connecting
                emit eventResponse("mpStatusChanged", QVariantList() << status);
            });

        logos->delivery_module.on("messageError",
            [this](const QString& /*eventName*/, const QVariantList& data) {
                if (!m_mpEnabled) return;
                m_mpError = data.size() >= 3 ? data[2].toString() : "send failed";
                qWarning() << "TicTacToePlugin: message error:" << m_mpError;
                emit eventResponse("mpStatusChanged", QVariantList() << 3);
            });

        m_handlersRegistered = true;
    }

    // The generated DeliveryModule wrapper decodes RPC replies as
    // LogosResult, but delivery_module's Qt slots return plain bool on the
    // wire. QVariant(bool).value<LogosResult>() yields {success=false}, so
    // every typed-wrapper RPC reports a spurious failure. Bypass it: go
    // straight to invokeRemoteMethod + toBool(). Events still flow through
    // the typed wrapper's .on() which delegates to client->onEvent.
    LogosAPIClient* client = logosAPI->getClient("delivery_module");
    if (!client) {
        TTT_TRACE("getClient returned null");
        setMpError("delivery_module client unavailable");
        return;
    }

    auto callBool = [&](const char* what, const QVariant& v) {
        TTT_TRACE("RPC %s: raw QVariant type=%d, toString=%s",
                  what, (int)v.userType(), v.toString().toUtf8().constData());
        if (!v.isValid()) {
            setMpError(QStringLiteral("%1: no response").arg(what));
            return false;
        }
        if (!v.toBool()) {
            setMpError(QStringLiteral("%1 returned false").arg(what));
            return false;
        }
        return true;
    };

    // 1. Create the delivery node.
    // Pick free ports ourselves because delivery_module rejects port 0 in the
    // JSON config (logos-co/logos-delivery-module#24). Lets two basecamps run
    // on one host without hardcoded port assignments. This really ought to be
    // delivery_module's responsibility — it should ship with sensible defaults
    // (ephemeral ports, or "port 0 = OS picks") so modules don't have to
    // reinvent socket probing. Remove once upstream fixes the port-0 path.
    int tcpPort = pickFreePort(SOCK_STREAM);
    int udpPort = pickFreePort(SOCK_DGRAM);
    if (tcpPort == 0 || udpPort == 0) {
        TTT_TRACE("pickFreePort failed: tcp=%d udp=%d errno=%d (%s)",
                  tcpPort, udpPort, errno, std::strerror(errno));
        setMpError("failed to pick free ports for delivery_module");
        return;
    }
    TTT_TRACE("picked tcpPort=%d discv5UdpPort=%d", tcpPort, udpPort);
    QString config = QStringLiteral(
        R"({"logLevel":"INFO","mode":"Core","preset":"logos.dev","tcpPort":%1,"discv5UdpPort":%2})"
    ).arg(tcpPort).arg(udpPort);
    TTT_TRACE("calling createNode");
    if (!callBool("createNode",
                  client->invokeRemoteMethod("delivery_module", "createNode", config))) {
        return;
    }

    // 2. Start the node.
    TTT_TRACE("calling start");
    if (!callBool("start",
                  client->invokeRemoteMethod("delivery_module", "start"))) {
        return;
    }

    // 3. Subscribe. If this fails we still have a running node, so stop it
    // before bailing out.
    TTT_TRACE("calling subscribe");
    if (!callBool("subscribe",
                  client->invokeRemoteMethod("delivery_module", "subscribe", m_contentTopic))) {
        client->invokeRemoteMethod("delivery_module", "stop");
        return;
    }

    m_mpEnabled = true;
    TTT_TRACE("multiplayer enabled");
    emit eventResponse("mpStatusChanged", QVariantList() << 1); // 1=connecting
}

void TicTacToePlugin::disableMultiplayer()
{
    if (!m_mpEnabled) return;

    if (logosAPI) {
        if (LogosAPIClient* client = logosAPI->getClient("delivery_module")) {
            // Best-effort teardown — same wrapper-bypass as enableMultiplayer.
            client->invokeRemoteMethod("delivery_module", "unsubscribe", m_contentTopic);
            client->invokeRemoteMethod("delivery_module", "stop");
        }
    }
    m_mpConnected = false;
    m_mpEnabled = false;
    m_msgSent = 0;
    m_msgReceived = 0;
    m_mpError.clear();
    emit eventResponse("mpStatusChanged", QVariantList() << 0); // 0=off
    qDebug() << "TicTacToePlugin: multiplayer disabled";
}

int TicTacToePlugin::mpStatus()
{
    if (!m_mpEnabled)   return 0; // off
    if (!m_mpError.isEmpty()) return 3; // error
    if (m_mpConnected)  return 2; // connected
    return 1; // connecting
}

int     TicTacToePlugin::mpConnected()      { return m_mpConnected ? 1 : 0; }
int     TicTacToePlugin::mpMessagesSent()   { return m_msgSent; }
int     TicTacToePlugin::mpMessagesReceived() { return m_msgReceived; }
QString TicTacToePlugin::mpError()          { return m_mpError; }

// ── Private helpers ───────────────────────────────────────────────

void TicTacToePlugin::broadcastMove(int row, int col, int player)
{
    if (!m_mpEnabled || !m_mpConnected) return;
    tictactoe::GameMessage msg;
    auto* move = msg.mutable_move();
    move->set_row(row);
    move->set_col(col);
    move->set_player(player);
    sendGameMessage(msg);
}

void TicTacToePlugin::broadcastNewGame()
{
    if (!m_mpEnabled || !m_mpConnected) return;
    tictactoe::GameMessage msg;
    msg.set_new_game(true);
    sendGameMessage(msg);
}

void TicTacToePlugin::sendGameMessage(const tictactoe::GameMessage& msg)
{
    m_mpError.clear();

    std::string serialized;
    if (!msg.SerializeToString(&serialized)) {
        qWarning() << "TicTacToePlugin: protobuf serialization failed";
        setMpError("protobuf serialization failed");
        return;
    }
    QString payload = QString::fromLatin1(
        QByteArray(serialized.data(), serialized.size()).toBase64());

    LogosAPIClient* client = logosAPI ? logosAPI->getClient("delivery_module") : nullptr;
    if (!client) {
        setMpError("delivery_module client unavailable");
        return;
    }
    QVariant v = client->invokeRemoteMethod("delivery_module", "send",
                                            m_contentTopic, payload);
    if (!v.isValid()) {
        setMpError("send: no response");
        return;
    }
    m_msgSent++;
}

void TicTacToePlugin::setMpError(const QString& err)
{
    m_mpError = err;
    qWarning() << "TicTacToePlugin:" << err;
    emit eventResponse("mpStatusChanged", QVariantList() << 3); // 3=error
}
