#ifndef TICTACTOE_BACKEND_H
#define TICTACTOE_BACKEND_H

#include <QObject>
#include "logos_sdk.h"

class LogosAPI;

class TicTacToeBackend : public QObject
{
    Q_OBJECT

public:
    explicit TicTacToeBackend(LogosAPI* api, QObject* parent = nullptr);

    Q_INVOKABLE void newGame();
    Q_INVOKABLE int  play(int row, int col);
    Q_INVOKABLE int  status();
    Q_INVOKABLE int  getCell(int row, int col);
    Q_INVOKABLE int  currentPlayer();
    Q_INVOKABLE int  aiMove();

private:
    LogosModules* m_logos;
};

#endif // TICTACTOE_BACKEND_H
