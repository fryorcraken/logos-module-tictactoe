#include "tictactoe_backend.h"
#include "tictactoe.pb.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include "logos_object.h"
#include <QByteArray>
#include <QDebug>

TicTacToeBackend::TicTacToeBackend(LogosAPI* api, QObject* parent)
    : QObject(parent), m_logos(new LogosModules(api)), m_logosAPI(api) {}

void TicTacToeBackend::newGame()
{
    m_logos->tictactoe.newGame();
    if (m_multiplayerEnabled && m_deliveryStarted) {
        tictactoe::GameMessage msg;
        msg.set_new_game(true);
        std::string serialized;
        msg.SerializeToString(&serialized);
        QString payload = QString::fromLatin1(
            QByteArray(serialized.data(), serialized.size()).toBase64());
        m_deliveryClient->invokeRemoteMethod(
            "delivery_module", "send", m_contentTopic, payload);
        m_messagesSent++;
        emit deliveryChanged();
    }
}

int TicTacToeBackend::play(int row, int col)
{
    int currentP = m_logos->tictactoe.currentPlayer();
    int result = m_logos->tictactoe.play(row, col);
    if (result == 0) {
        broadcastMove(row, col, currentP);
    }
    return result;
}

int  TicTacToeBackend::status()        { return m_logos->tictactoe.status(); }
int  TicTacToeBackend::getCell(int row, int col) { return m_logos->tictactoe.getCell(row, col); }
int  TicTacToeBackend::currentPlayer() { return m_logos->tictactoe.currentPlayer(); }

void TicTacToeBackend::broadcastMove(int row, int col, int player)
{
    if (!m_multiplayerEnabled || !m_deliveryStarted) return;

    tictactoe::GameMessage msg;
    auto* move = msg.mutable_move();
    move->set_row(row);
    move->set_col(col);
    move->set_player(player);

    std::string serialized;
    msg.SerializeToString(&serialized);
    // Base64-encode protobuf bytes so they survive the delivery module's
    // QString → UTF-8 → base64 pipeline without corruption.
    QString payload = QString::fromLatin1(
        QByteArray(serialized.data(), serialized.size()).toBase64());
    m_deliveryClient->invokeRemoteMethod(
        "delivery_module", "send", m_contentTopic, payload);
    m_messagesSent++;
    emit deliveryChanged();
}

void TicTacToeBackend::enableMultiplayer()
{
    if (m_multiplayerEnabled) return;

    // Get a client for the delivery_module (pre-installed in basecamp)
    m_deliveryClient = m_logosAPI->getClient("delivery_module");
    if (!m_deliveryClient) {
        qWarning() << "TicTacToeBackend: failed to get delivery_module client";
        return;
    }

    QString config = R"({"logLevel":"INFO","mode":"Core","preset":"logos.dev","relay":true})";
    m_deliveryClient->invokeRemoteMethod("delivery_module", "createNode", config);
    m_deliveryClient->invokeRemoteMethod("delivery_module", "start");
    m_deliveryClient->invokeRemoteMethod("delivery_module", "subscribe", m_contentTopic);

    // Get a LogosObject handle for event subscription
    m_deliveryObject = m_deliveryClient->requestObject("delivery_module");
    if (!m_deliveryObject) {
        qWarning() << "TicTacToeBackend: failed to get delivery_module object for events";
    } else {
        // Subscribe to incoming messages (same pattern as logos-chat-legacy-ui)
        m_deliveryClient->onEvent(m_deliveryObject, "messageReceived",
            [this](const QString& /*eventName*/, const QVariantList& data) {
                if (data.size() < 3) {
                    qWarning() << "TicTacToeBackend: messageReceived payload too short";
                    return;
                }
                // data[2] is base64-encoded by delivery module; decode to get our base64 payload
                QByteArray deliveryPayload = QByteArray::fromBase64(data[2].toString().toUtf8());
                // Decode our base64 layer to get raw protobuf bytes
                QByteArray protoBytes = QByteArray::fromBase64(deliveryPayload);

                tictactoe::GameMessage msg;
                if (!msg.ParseFromArray(protoBytes.data(), protoBytes.size())) {
                    qWarning() << "TicTacToeBackend: failed to parse protobuf message";
                    return;
                }

                if (msg.has_move()) {
                    int row = msg.move().row();
                    int col = msg.move().col();
                    qDebug() << "TicTacToeBackend: received remote move" << row << col
                             << "player" << msg.move().player();
                    m_logos->tictactoe.play(row, col);
                    m_messagesReceived++;
                    emit deliveryChanged();
                    emit remoteMovePlayed();
                } else if (msg.has_new_game()) {
                    qDebug() << "TicTacToeBackend: received remote newGame";
                    m_logos->tictactoe.newGame();
                    m_messagesReceived++;
                    emit deliveryChanged();
                    emit remoteMovePlayed();
                }
            });
    }

    m_deliveryStarted = true;
    m_multiplayerEnabled = true;
    emit multiplayerChanged();
    emit deliveryChanged();
    qDebug() << "TicTacToeBackend: multiplayer enabled, topic:" << m_contentTopic;
}

void TicTacToeBackend::disableMultiplayer()
{
    if (!m_multiplayerEnabled) return;

    if (m_deliveryClient) {
        m_deliveryClient->invokeRemoteMethod("delivery_module", "unsubscribe", m_contentTopic);
        m_deliveryClient->invokeRemoteMethod("delivery_module", "stop");
    }
    m_deliveryObject = nullptr;
    m_deliveryStarted = false;
    m_multiplayerEnabled = false;
    m_messagesSent = 0;
    m_messagesReceived = 0;
    emit multiplayerChanged();
    emit deliveryChanged();
    qDebug() << "TicTacToeBackend: multiplayer disabled";
}
