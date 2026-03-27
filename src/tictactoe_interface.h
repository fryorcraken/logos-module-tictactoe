#ifndef TICTACTOE_INTERFACE_H
#define TICTACTOE_INTERFACE_H

#include <QObject>
#include "interface.h"

/**
 * @brief Interface for the TicTacToe module.
 *
 * Exposes a single game instance managed by the plugin.
 * Enum values from libtictactoe are passed as plain int so that callers
 * do not need to include the C header directly.
 *
 * TicTacToeCell  : 0=EMPTY, 1=X, 2=O
 * TicTacToeStatus: 0=ONGOING, 1=X_WINS, 2=O_WINS, 3=DRAW
 * TicTacToeError : 0=OK, 1=INVALID_POSITION, 2=CELL_OCCUPIED,
 *                  3=GAME_OVER, 4=NULL_GAME
 */
class TicTacToeInterface : public PluginInterface
{
public:
    virtual ~TicTacToeInterface() = default;

    /**
     * @brief Start a new game, resetting the board. X always moves first.
     */
    Q_INVOKABLE virtual void newGame() = 0;

    /**
     * @brief Place the current player's mark at (row, col).
     * @param row  Row index in [0, 2].
     * @param col  Column index in [0, 2].
     * @return     TicTacToeError as int (0 = success).
     */
    Q_INVOKABLE virtual int play(int row, int col) = 0;

    /**
     * @brief Return the current game status as a TicTacToeStatus int.
     */
    Q_INVOKABLE virtual int status() = 0;

    /**
     * @brief Return the cell content at (row, col) as a TicTacToeCell int.
     */
    Q_INVOKABLE virtual int getCell(int row, int col) = 0;

    /**
     * @brief Return whose turn it is as a TicTacToeCell int (1=X, 2=O).
     */
    Q_INVOKABLE virtual int currentPlayer() = 0;
};

#define TicTacToeInterface_iid "org.logos.TicTacToeInterface"
Q_DECLARE_INTERFACE(TicTacToeInterface, TicTacToeInterface_iid)

#endif // TICTACTOE_INTERFACE_H
