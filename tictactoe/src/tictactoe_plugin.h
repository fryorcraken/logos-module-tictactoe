#ifndef TICTACTOE_PLUGIN_H
#define TICTACTOE_PLUGIN_H

#include <QObject>
#include <QString>
#include "tictactoe_interface.h"
#include "logos_api.h"
#include "logos_sdk.h"
#include "lib/libtictactoe.h"

namespace tictactoe { class GameMessage; }

class TicTacToePlugin : public QObject, public TicTacToeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID TicTacToeInterface_iid FILE "metadata.json")
    Q_INTERFACES(TicTacToeInterface PluginInterface)

public:
    explicit TicTacToePlugin(QObject* parent = nullptr);
    ~TicTacToePlugin() override;

    QString name()    const override { return "tictactoe"; }
    QString version() const override { return "1.0.0"; }

    Q_INVOKABLE void initLogos(LogosAPI* logosAPIInstance);

    // Core game
    Q_INVOKABLE void newGame()              override;
    Q_INVOKABLE int  play(int row, int col) override;
    Q_INVOKABLE int  status()               override;
    Q_INVOKABLE int  getCell(int row, int col) override;
    Q_INVOKABLE int  currentPlayer()        override;

    // Multiplayer
    Q_INVOKABLE void enableMultiplayer()    override;
    Q_INVOKABLE void disableMultiplayer()   override;
    Q_INVOKABLE int  mpStatus()             override;
    Q_INVOKABLE int  mpConnected()          override;
    Q_INVOKABLE int  mpMessagesSent()       override;
    Q_INVOKABLE int  mpMessagesReceived()   override;
    Q_INVOKABLE QString mpError()           override;

signals:
    void eventResponse(const QString& eventName, const QVariantList& args);

private:
    void broadcastMove(int row, int col, int player);
    void broadcastNewGame();
    void sendGameMessage(const tictactoe::GameMessage& msg);
    void setMpError(const QString& err);

    TicTacToeGame* m_game = nullptr;
    LogosModules*  logos  = nullptr;

    bool    m_mpEnabled           = false;
    bool    m_mpConnected         = false;
    bool    m_handlersRegistered  = false;
    int     m_msgSent     = 0;
    int     m_msgReceived = 0;
    QString m_mpError;
    QString m_contentTopic = "/tictactoe/1/moves/proto";
};

#endif // TICTACTOE_PLUGIN_H
