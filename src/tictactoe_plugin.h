#ifndef TICTACTOE_PLUGIN_H
#define TICTACTOE_PLUGIN_H

#include <QObject>
#include "tictactoe_interface.h"
#include "logos_api.h"
#include "logos_sdk.h"
#include "lib/libtictactoe.h"

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

    Q_INVOKABLE void newGame()           override;
    Q_INVOKABLE int  play(int row, int col) override;
    Q_INVOKABLE int  status()            override;
    Q_INVOKABLE int  getCell(int row, int col) override;
    Q_INVOKABLE int  currentPlayer()     override;

signals:
    void eventResponse(const QString& eventName, const QVariantList& args);

private:
    TicTacToeGame* m_game = nullptr;
    LogosModules*  logos  = nullptr;
};

#endif // TICTACTOE_PLUGIN_H
