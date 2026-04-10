#include "tictactoe_plugin.h"
#include "logos_api.h"
#include "logos_api_client.h"

TicTacToePlugin::TicTacToePlugin(QObject* parent)
    : QObject(parent)
    , m_game(tictactoe_new())
{
}

TicTacToePlugin::~TicTacToePlugin()
{
    tictactoe_free(m_game);
    delete logos;
}

void TicTacToePlugin::initLogos(LogosAPI* logosAPIInstance)
{
    if (logos) {
        delete logos;
        logos = nullptr;
    }
    if (logosAPI) {
        delete logosAPI;
        logosAPI = nullptr;
    }
    logosAPI = logosAPIInstance;
    if (logosAPI) {
        logos = new LogosModules(logosAPI);
    }
}

void TicTacToePlugin::newGame()
{
    tictactoe_reset(m_game);
    emit eventResponse("newGame", {});
}

int TicTacToePlugin::play(int row, int col)
{
    TicTacToeError err = tictactoe_play(m_game, row, col);
    if (err == TICTACTOE_OK) {
        TicTacToeStatus s = tictactoe_status(m_game);
        emit eventResponse("played", QVariantList() << row << col << static_cast<int>(s));
    }
    return static_cast<int>(err);
}

int TicTacToePlugin::status()
{
    return static_cast<int>(tictactoe_status(m_game));
}

int TicTacToePlugin::getCell(int row, int col)
{
    return static_cast<int>(tictactoe_get_cell(m_game, row, col));
}

int TicTacToePlugin::currentPlayer()
{
    return static_cast<int>(tictactoe_current_player(m_game));
}

int TicTacToePlugin::aiMove()
{
    int row, col;
    TicTacToeError err = tictactoe_ai_move(m_game, &row, &col);
    if (err != TICTACTOE_OK)
        return static_cast<int>(err);

    return play(row, col); // reuse play() which also emits eventResponse
}
