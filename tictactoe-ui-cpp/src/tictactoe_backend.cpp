#include "tictactoe_backend.h"
#include "logos_api.h"
#include <QDebug>

TicTacToeBackend::TicTacToeBackend(LogosAPI* api, QObject* parent)
    : QObject(parent), m_logos(new LogosModules(api)), m_logosAPI(api)
{
    // Why we poll instead of subscribing via LogosAPIClient::onEvent
    // ------------------------------------------------------------------
    // The logos-cpp-sdk shipped via `logos-module-builder` on the
    // `tutorial-v1` branch has a broken LogosAPIConsumer::onEvent: calling
    // it from a UI plugin's backend constructor dereferences uninitialized
    // state and segfaults the basecamp GUI. Observed stack from coredump:
    //
    //   #0 LogosAPIConsumer::onEvent
    //   #1 LogosAPIClient::onEvent
    //   #2 TicTacToeBackend::TicTacToeBackend
    //   #3 TicTacToeUiPlugin::createWidget
    //
    // We can't just bump to `logos-module-builder` default (master), where
    // event subscription works, because master ships a newer delivery_module
    // SDK whose RPC wire format uses `LogosResult` instead of plain `bool` —
    // incompatible with the `delivery_module` 1.0.0 bundled in basecamp
    // AppImage v0.1.1. See the pin comment in ../tictactoe/flake.nix and
    // PR https://github.com/logos-co/logos-delivery-module/pull/23 for the
    // full reasoning.
    //
    // Until either (a) the onEvent fix is backported to tutorial-v1, or
    // (b) basecamp ships a build with the newer delivery wrapper so we can
    // move off tutorial-v1, we poll the core module for multiplayer state
    // changes and emit our own signals when sent/received counters or the
    // connection flag change. 500ms is fine for turn-based gameplay.
    // TODO: switch to onEvent once upstream is fixed.
    m_mpPollTimer = new QTimer(this);
    m_mpPollTimer->setInterval(500);
    QObject::connect(m_mpPollTimer, &QTimer::timeout, this, [this]() {
        int prevReceived = m_messagesReceived;
        bool prevConnected = m_deliveryConnected;
        refreshMpState();
        if (m_messagesReceived != prevReceived) emit remoteMovePlayed();
        if (m_deliveryConnected != prevConnected) emit deliveryChanged();
    });
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
    if (m_mpPollTimer) m_mpPollTimer->start();
    emit multiplayerChanged();
    emit deliveryChanged();
}

void TicTacToeBackend::disableMultiplayer()
{
    if (!m_multiplayerEnabled) return;
    if (m_mpPollTimer) m_mpPollTimer->stop();
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
