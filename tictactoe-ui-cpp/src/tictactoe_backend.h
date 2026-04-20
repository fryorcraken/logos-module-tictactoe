#ifndef TICTACTOE_BACKEND_H
#define TICTACTOE_BACKEND_H

#include <QObject>
#include <QString>
#include "logos_sdk.h"

class LogosAPI;
class LogosAPIClient;
class LogosObject;

class TicTacToeBackend : public QObject
{
    Q_OBJECT

public:
    explicit TicTacToeBackend(LogosAPI* api, QObject* parent = nullptr);

    // Core game methods
    Q_INVOKABLE void newGame();
    Q_INVOKABLE int  play(int row, int col);
    Q_INVOKABLE int  status();
    Q_INVOKABLE int  getCell(int row, int col);
    Q_INVOKABLE int  currentPlayer();

    // Multiplayer — delegates to core module
    Q_INVOKABLE void enableMultiplayer();
    Q_INVOKABLE void disableMultiplayer();

    bool multiplayerEnabled() const { return m_multiplayerEnabled; }
    bool deliveryConnected()  const { return m_deliveryConnected; }
    int  messagesSent()       const { return m_messagesSent; }
    int  messagesReceived()   const { return m_messagesReceived; }
    QString deliveryError()   const { return m_deliveryError; }

signals:
    void multiplayerChanged();
    void deliveryChanged();
    void remoteMovePlayed();

private:
    void refreshMpState();

    LogosModules*   m_logos;
    LogosAPI*       m_logosAPI;
    LogosAPIClient* m_tttClient  = nullptr;
    LogosObject*    m_tttObject  = nullptr;

    // Cached multiplayer state (from core module)
    bool m_multiplayerEnabled = false;
    bool m_deliveryConnected  = false;
    int  m_messagesSent       = 0;
    int  m_messagesReceived   = 0;
    QString m_deliveryError;
};

#endif // TICTACTOE_BACKEND_H
