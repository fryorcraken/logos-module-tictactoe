#include "tictactoe_plugin.h"
#include "tictactoe.pb.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include "logos_object.h"
#include <QByteArray>
#include <QDebug>

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
    if (logos) {
        delete logos;
        logos = nullptr;
    }
    if (logosAPI) {
        delete logosAPI;
        logosAPI = nullptr;
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

void TicTacToePlugin::enableMultiplayer()
{
    if (m_mpEnabled) return;

    m_deliveryClient = logosAPI->getClient("delivery_module");
    if (!m_deliveryClient) {
        qWarning() << "TicTacToePlugin: failed to get delivery_module client";
        m_mpError = "delivery_module not available";
        emit eventResponse("mpStatusChanged", QVariantList() << 3);
        return;
    }

    // 1. Create the delivery node
    QString config = R"({"logLevel":"INFO","mode":"Core","preset":"logos.dev"})";
    m_deliveryClient->invokeRemoteMethod("delivery_module", "createNode", config);

    // 2. Register event handlers before start
    m_deliveryObject = m_deliveryClient->requestObject("delivery_module");
    if (!m_deliveryObject) {
        qWarning() << "TicTacToePlugin: failed to get delivery_module object for events";
    } else {
        m_deliveryClient->onEvent(m_deliveryObject, "messageReceived",
            [this](const QString& /*eventName*/, const QVariantList& data) {
                if (data.size() < 3) {
                    qWarning() << "TicTacToePlugin: messageReceived payload too short";
                    return;
                }
                // data[2] is the base64-encoded message payload (delivery module convention)
                QByteArray deliveryPayload = QByteArray::fromBase64(data[2].toString().toUtf8());
                // Decode our base64 layer to get raw protobuf bytes
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
                    // Apply move directly to game state (no broadcast)
                    tictactoe_play(m_game, row, col);
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

        m_deliveryClient->onEvent(m_deliveryObject, "connectionStateChanged",
            [this](const QString& /*eventName*/, const QVariantList& data) {
                m_mpConnected = data.size() > 0 && !data[0].toString().isEmpty();
                qDebug() << "TicTacToePlugin: connection state changed, connected:" << m_mpConnected;
                int status = m_mpConnected ? 2 : 1; // 2=connected, 1=connecting
                emit eventResponse("mpStatusChanged", QVariantList() << status);
            });

        m_deliveryClient->onEvent(m_deliveryObject, "messageError",
            [this](const QString& /*eventName*/, const QVariantList& data) {
                m_mpError = data.size() >= 3 ? data[2].toString() : "send failed";
                qWarning() << "TicTacToePlugin: message error:" << m_mpError;
                emit eventResponse("mpStatusChanged", QVariantList() << 3);
            });
    }

    // 3. Start the node
    m_deliveryClient->invokeRemoteMethod("delivery_module", "start");
    // 4. Subscribe to content topic
    m_deliveryClient->invokeRemoteMethod("delivery_module", "subscribe", m_contentTopic);

    m_mpEnabled = true;
    emit eventResponse("mpStatusChanged", QVariantList() << 1); // 1=connecting
    qDebug() << "TicTacToePlugin: multiplayer enabled, topic:" << m_contentTopic;
}

void TicTacToePlugin::disableMultiplayer()
{
    if (!m_mpEnabled) return;

    if (m_deliveryClient) {
        m_deliveryClient->invokeRemoteMethod("delivery_module", "unsubscribe", m_contentTopic);
        m_deliveryClient->invokeRemoteMethod("delivery_module", "stop");
    }
    m_deliveryObject = nullptr;
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
    m_mpError.clear();

    tictactoe::GameMessage msg;
    auto* move = msg.mutable_move();
    move->set_row(row);
    move->set_col(col);
    move->set_player(player);

    std::string serialized;
    msg.SerializeToString(&serialized);
    QString payload = QString::fromLatin1(
        QByteArray(serialized.data(), serialized.size()).toBase64());
    m_deliveryClient->invokeRemoteMethod(
        "delivery_module", "send", m_contentTopic, payload);
    m_msgSent++;
}

void TicTacToePlugin::broadcastNewGame()
{
    if (!m_mpEnabled || !m_mpConnected) return;
    m_mpError.clear();

    tictactoe::GameMessage msg;
    msg.set_new_game(true);
    std::string serialized;
    msg.SerializeToString(&serialized);
    QString payload = QString::fromLatin1(
        QByteArray(serialized.data(), serialized.size()).toBase64());
    m_deliveryClient->invokeRemoteMethod(
        "delivery_module", "send", m_contentTopic, payload);
    m_msgSent++;
}
