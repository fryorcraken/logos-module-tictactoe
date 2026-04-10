#include "tictactoe_backend.h"

TicTacToeBackend::TicTacToeBackend(LogosAPI* api, QObject* parent)
    : QObject(parent), m_logos(new LogosModules(api)) {}

void TicTacToeBackend::newGame()           { m_logos->tictactoe.newGame(); }
int  TicTacToeBackend::play(int row, int col) { return m_logos->tictactoe.play(row, col); }
int  TicTacToeBackend::status()            { return m_logos->tictactoe.status(); }
int  TicTacToeBackend::getCell(int row, int col) { return m_logos->tictactoe.getCell(row, col); }
int  TicTacToeBackend::currentPlayer()     { return m_logos->tictactoe.currentPlayer(); }
