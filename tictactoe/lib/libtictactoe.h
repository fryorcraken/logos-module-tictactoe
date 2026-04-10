#ifndef LIBTICTACTOE_H
#define LIBTICTACTOE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Cell states for each board position.
 */
typedef enum {
    TICTACTOE_CELL_EMPTY = 0,
    TICTACTOE_CELL_X     = 1,
    TICTACTOE_CELL_O     = 2
} TicTacToeCell;

/**
 * Overall game status.
 */
typedef enum {
    TICTACTOE_STATUS_ONGOING = 0,
    TICTACTOE_STATUS_X_WINS  = 1,
    TICTACTOE_STATUS_O_WINS  = 2,
    TICTACTOE_STATUS_DRAW    = 3
} TicTacToeStatus;

/**
 * Error codes returned by tictactoe_play().
 */
typedef enum {
    TICTACTOE_OK                  = 0,
    TICTACTOE_ERR_INVALID_POSITION = 1, /* row or col out of [0,2] */
    TICTACTOE_ERR_CELL_OCCUPIED   = 2,  /* target cell is not empty */
    TICTACTOE_ERR_GAME_OVER       = 3,  /* game has already ended */
    TICTACTOE_ERR_NULL_GAME       = 4   /* game pointer is NULL */
} TicTacToeError;

/**
 * Opaque handle to a tic-tac-toe game instance.
 */
typedef struct TicTacToeGame TicTacToeGame;

/**
 * Allocate and initialise a new game. X always moves first.
 * The caller must eventually pass the returned pointer to tictactoe_free().
 *
 * @return Pointer to the new game, or NULL on allocation failure.
 */
TicTacToeGame* tictactoe_new(void);

/**
 * Release all resources owned by the game.
 * Passing NULL is safe and has no effect.
 *
 * @param game  Game handle returned by tictactoe_new().
 */
void tictactoe_free(TicTacToeGame* game);

/**
 * Reset the board to the initial empty state and restore X as the
 * first player, without reallocating the game object.
 *
 * @param game  Game handle.
 */
void tictactoe_reset(TicTacToeGame* game);

/**
 * Place the current player's mark at (row, col).
 *
 * @param game  Game handle.
 * @param row   Row index in [0, 2], top to bottom.
 * @param col   Column index in [0, 2], left to right.
 * @return      TICTACTOE_OK on success, or an error code.
 */
TicTacToeError tictactoe_play(TicTacToeGame* game, int row, int col);

/**
 * Return the current game status (ongoing, won, or draw).
 *
 * @param game  Game handle.
 * @return      TicTacToeStatus value.
 */
TicTacToeStatus tictactoe_status(const TicTacToeGame* game);

/**
 * Return the mark stored in a board cell.
 *
 * @param game  Game handle.
 * @param row   Row index in [0, 2].
 * @param col   Column index in [0, 2].
 * @return      TICTACTOE_CELL_X, TICTACTOE_CELL_O, or TICTACTOE_CELL_EMPTY.
 *              Returns TICTACTOE_CELL_EMPTY for out-of-bounds indices.
 */
TicTacToeCell tictactoe_get_cell(const TicTacToeGame* game, int row, int col);

/**
 * Return whose turn it is (TICTACTOE_CELL_X or TICTACTOE_CELL_O).
 * If the game is over, the value is the player who would have moved next.
 *
 * @param game  Game handle.
 * @return      TICTACTOE_CELL_X or TICTACTOE_CELL_O.
 */
TicTacToeCell tictactoe_current_player(const TicTacToeGame* game);

/**
 * Compute the best move for the current player using minimax.
 * Tic-tac-toe is small enough for an exhaustive search with no
 * depth limit; this always plays optimally.
 *
 * @param game       Game handle (must be ONGOING with at least one empty cell).
 * @param out_row    Receives the best row [0, 2].
 * @param out_col    Receives the best column [0, 2].
 * @return           TICTACTOE_OK on success, TICTACTOE_ERR_GAME_OVER if the
 *                   game has already ended, TICTACTOE_ERR_NULL_GAME if game
 *                   is NULL.
 */
TicTacToeError tictactoe_ai_move(const TicTacToeGame* game,
                                  int* out_row, int* out_col);

#ifdef __cplusplus
}
#endif

#endif /* LIBTICTACTOE_H */
