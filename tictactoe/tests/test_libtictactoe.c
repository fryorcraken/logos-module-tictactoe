#include "libtictactoe.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
            failures++;                                                      \
        }                                                                    \
    } while (0)

static void test_new_game_is_empty(void)
{
    TicTacToeGame* g = tictactoe_new();
    CHECK(g != NULL);
    CHECK(tictactoe_status(g) == TICTACTOE_STATUS_ONGOING);
    CHECK(tictactoe_current_player(g) == TICTACTOE_CELL_X);
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
            CHECK(tictactoe_get_cell(g, r, c) == TICTACTOE_CELL_EMPTY);
    tictactoe_free(g);
}

static void test_turn_alternation(void)
{
    TicTacToeGame* g = tictactoe_new();
    CHECK(tictactoe_current_player(g) == TICTACTOE_CELL_X);
    CHECK(tictactoe_play(g, 0, 0) == TICTACTOE_OK);
    CHECK(tictactoe_get_cell(g, 0, 0) == TICTACTOE_CELL_X);
    CHECK(tictactoe_current_player(g) == TICTACTOE_CELL_O);
    CHECK(tictactoe_play(g, 1, 1) == TICTACTOE_OK);
    CHECK(tictactoe_get_cell(g, 1, 1) == TICTACTOE_CELL_O);
    CHECK(tictactoe_current_player(g) == TICTACTOE_CELL_X);
    tictactoe_free(g);
}

static void test_invalid_position(void)
{
    TicTacToeGame* g = tictactoe_new();
    CHECK(tictactoe_play(g, -1, 0) == TICTACTOE_ERR_INVALID_POSITION);
    CHECK(tictactoe_play(g, 0, -1) == TICTACTOE_ERR_INVALID_POSITION);
    CHECK(tictactoe_play(g, 3, 0)  == TICTACTOE_ERR_INVALID_POSITION);
    CHECK(tictactoe_play(g, 0, 3)  == TICTACTOE_ERR_INVALID_POSITION);
    // After invalid attempts, it's still X's turn and board is empty.
    CHECK(tictactoe_current_player(g) == TICTACTOE_CELL_X);
    CHECK(tictactoe_get_cell(g, 0, 0) == TICTACTOE_CELL_EMPTY);
    tictactoe_free(g);
}

static void test_cell_occupied(void)
{
    TicTacToeGame* g = tictactoe_new();
    CHECK(tictactoe_play(g, 1, 1) == TICTACTOE_OK);  // X
    CHECK(tictactoe_play(g, 1, 1) == TICTACTOE_ERR_CELL_OCCUPIED);  // O tries same cell
    // Turn did not advance.
    CHECK(tictactoe_current_player(g) == TICTACTOE_CELL_O);
    tictactoe_free(g);
}

static void test_null_game(void)
{
    CHECK(tictactoe_play(NULL, 0, 0) == TICTACTOE_ERR_NULL_GAME);
    tictactoe_free(NULL);  // must not crash
}

static void test_row_win(void)
{
    TicTacToeGame* g = tictactoe_new();
    // X row 0, O sprinkled on row 1
    CHECK(tictactoe_play(g, 0, 0) == TICTACTOE_OK); // X
    CHECK(tictactoe_play(g, 1, 0) == TICTACTOE_OK); // O
    CHECK(tictactoe_play(g, 0, 1) == TICTACTOE_OK); // X
    CHECK(tictactoe_play(g, 1, 1) == TICTACTOE_OK); // O
    CHECK(tictactoe_play(g, 0, 2) == TICTACTOE_OK); // X wins
    CHECK(tictactoe_status(g) == TICTACTOE_STATUS_X_WINS);
    tictactoe_free(g);
}

static void test_col_win(void)
{
    TicTacToeGame* g = tictactoe_new();
    CHECK(tictactoe_play(g, 0, 2) == TICTACTOE_OK); // X
    CHECK(tictactoe_play(g, 0, 0) == TICTACTOE_OK); // O
    CHECK(tictactoe_play(g, 1, 2) == TICTACTOE_OK); // X
    CHECK(tictactoe_play(g, 1, 0) == TICTACTOE_OK); // O
    CHECK(tictactoe_play(g, 2, 1) == TICTACTOE_OK); // X
    CHECK(tictactoe_play(g, 2, 0) == TICTACTOE_OK); // O wins col 0
    CHECK(tictactoe_status(g) == TICTACTOE_STATUS_O_WINS);
    tictactoe_free(g);
}

static void test_diag_win(void)
{
    TicTacToeGame* g = tictactoe_new();
    CHECK(tictactoe_play(g, 0, 0) == TICTACTOE_OK); // X
    CHECK(tictactoe_play(g, 0, 1) == TICTACTOE_OK); // O
    CHECK(tictactoe_play(g, 1, 1) == TICTACTOE_OK); // X
    CHECK(tictactoe_play(g, 0, 2) == TICTACTOE_OK); // O
    CHECK(tictactoe_play(g, 2, 2) == TICTACTOE_OK); // X wins main diag
    CHECK(tictactoe_status(g) == TICTACTOE_STATUS_X_WINS);
    tictactoe_free(g);
}

static void test_anti_diag_win(void)
{
    TicTacToeGame* g = tictactoe_new();
    CHECK(tictactoe_play(g, 0, 2) == TICTACTOE_OK); // X
    CHECK(tictactoe_play(g, 0, 0) == TICTACTOE_OK); // O
    CHECK(tictactoe_play(g, 1, 1) == TICTACTOE_OK); // X
    CHECK(tictactoe_play(g, 0, 1) == TICTACTOE_OK); // O
    CHECK(tictactoe_play(g, 2, 0) == TICTACTOE_OK); // X wins anti-diag
    CHECK(tictactoe_status(g) == TICTACTOE_STATUS_X_WINS);
    tictactoe_free(g);
}

static void test_draw(void)
{
    TicTacToeGame* g = tictactoe_new();
    // Classic cat's-game layout (no three-in-a-row possible):
    //  X O X
    //  X O O
    //  O X X
    // Move order alternates X, O and never completes a line before move 9.
    int moves[9][2] = {
        {0,0}, {0,1}, {0,2},  // X O X
        {1,1}, {1,0}, {1,2},  // O X O
        {2,1}, {2,0}, {2,2}   // X O X  (X plays last — no line)
    };
    for (int i = 0; i < 9; i++)
        CHECK(tictactoe_play(g, moves[i][0], moves[i][1]) == TICTACTOE_OK);
    CHECK(tictactoe_status(g) == TICTACTOE_STATUS_DRAW);
    tictactoe_free(g);
}

static void test_game_over_rejection(void)
{
    TicTacToeGame* g = tictactoe_new();
    CHECK(tictactoe_play(g, 0, 0) == TICTACTOE_OK); // X
    CHECK(tictactoe_play(g, 1, 0) == TICTACTOE_OK); // O
    CHECK(tictactoe_play(g, 0, 1) == TICTACTOE_OK); // X
    CHECK(tictactoe_play(g, 1, 1) == TICTACTOE_OK); // O
    CHECK(tictactoe_play(g, 0, 2) == TICTACTOE_OK); // X wins
    // Any further move must be rejected with GAME_OVER.
    CHECK(tictactoe_play(g, 2, 2) == TICTACTOE_ERR_GAME_OVER);
    tictactoe_free(g);
}

static void test_reset(void)
{
    TicTacToeGame* g = tictactoe_new();
    CHECK(tictactoe_play(g, 0, 0) == TICTACTOE_OK);
    CHECK(tictactoe_play(g, 1, 1) == TICTACTOE_OK);
    tictactoe_reset(g);
    CHECK(tictactoe_status(g) == TICTACTOE_STATUS_ONGOING);
    CHECK(tictactoe_current_player(g) == TICTACTOE_CELL_X);
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
            CHECK(tictactoe_get_cell(g, r, c) == TICTACTOE_CELL_EMPTY);
    // Can play again after reset
    CHECK(tictactoe_play(g, 0, 0) == TICTACTOE_OK);
    tictactoe_free(g);
}

static void test_get_cell_out_of_bounds(void)
{
    TicTacToeGame* g = tictactoe_new();
    CHECK(tictactoe_get_cell(g, -1, 0) == TICTACTOE_CELL_EMPTY);
    CHECK(tictactoe_get_cell(g, 0, -1) == TICTACTOE_CELL_EMPTY);
    CHECK(tictactoe_get_cell(g, 3, 0)  == TICTACTOE_CELL_EMPTY);
    CHECK(tictactoe_get_cell(g, 0, 3)  == TICTACTOE_CELL_EMPTY);
    tictactoe_free(g);
}

int main(void)
{
    test_new_game_is_empty();
    test_turn_alternation();
    test_invalid_position();
    test_cell_occupied();
    test_null_game();
    test_row_win();
    test_col_win();
    test_diag_win();
    test_anti_diag_win();
    test_draw();
    test_game_over_rejection();
    test_reset();
    test_get_cell_out_of_bounds();

    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("all libtictactoe tests passed\n");
    return 0;
}
