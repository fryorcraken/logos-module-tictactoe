#include "tictactoe_backend.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include "logos_object.h"
#include <QDebug>

TicTacToeBackend::TicTacToeBackend(LogosAPI* api, QObject* parent)
    : QObject(parent), m_logos(new LogosModules(api)), m_logosAPI(api)
{
    // Subscribe to tictactoe core module events for multiplayer updates
    m_tttClient = m_logosAPI->getClient("tictactoe");
    if (m_tttClient) {
        m_tttObject = m_tttClient->requestObject("tictactoe");
        if (m_tttObject) {
            m_tttClient->onEvent(m_tttObject, "remoteMove",
                [this](const QString& /*eventName*/, const QVariantList& /*data*/) {
                    refreshMpState();
                    emit remoteMovePlayed();
                });

            m_tttClient->onEvent(m_tttObject, "remoteNewGame",
                [this](const QString& /*eventName*/, const QVariantList& /*data*/) {
                    refreshMpState();
                    emit remoteMovePlayed();
                });

            m_tttClient->onEvent(m_tttObject, "mpStatusChanged",
                [this](const QString& /*eventName*/, const QVariantList& /*data*/) {
                    refreshMpState();
                    emit deliveryChanged();
                });
        }
    }
}

void TicTacToeBackend::newGame()
{
    m_logos->tictactoe.newGame();
    refreshMpState();
}

int TicTacToeBackend::play(int row, int col)
{
    int result = m_logos->tictactoe.play(row, col);
    refreshMpState();
    return result;
}

int  TicTacToeBackend::status()                { return m_logos->tictactoe.status(); }
int  TicTacToeBackend::getCell(int row, int col) { return m_logos->tictactoe.getCell(row, col); }
int  TicTacToeBackend::currentPlayer()         { return m_logos->tictactoe.currentPlayer(); }

void TicTacToeBackend::enableMultiplayer()
{
    if (m_multiplayerEnabled) return;
    m_logos->tictactoe.enableMultiplayer();
    m_multiplayerEnabled = true;
    refreshMpState();
    emit multiplayerChanged();
    emit deliveryChanged();
}

void TicTacToeBackend::disableMultiplayer()
{
    if (!m_multiplayerEnabled) return;
    m_logos->tictactoe.disableMultiplayer();
    m_multiplayerEnabled = false;
    m_deliveryConnected = false;
    m_messagesSent = 0;
    m_messagesReceived = 0;
    m_deliveryError.clear();
    emit multiplayerChanged();
    emit deliveryChanged();
}

void TicTacToeBackend::refreshMpState()
{
    int status = m_logos->tictactoe.mpStatus();
    m_multiplayerEnabled = (status != 0);
    m_deliveryConnected  = (m_logos->tictactoe.mpConnected() != 0);
    m_messagesSent       = m_logos->tictactoe.mpMessagesSent();
    m_messagesReceived   = m_logos->tictactoe.mpMessagesReceived();
    m_deliveryError      = m_logos->tictactoe.mpError();
}
