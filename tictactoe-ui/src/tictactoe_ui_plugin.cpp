#include "tictactoe_ui_plugin.h"
#include "tictactoe_backend.h"
#include "logos_api.h"
#include <QDebug>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QTimer>
#include <memory>

// Cell values from libtictactoe
static const int CELL_EMPTY = 0;
static const int CELL_X     = 1;
static const int CELL_O     = 2;

// Game status from libtictactoe
static const int STATUS_ONGOING = 0;
static const int STATUS_X_WINS  = 1;
static const int STATUS_O_WINS  = 2;
static const int STATUS_DRAW    = 3;

TicTacToeUiPlugin::TicTacToeUiPlugin(QObject* parent) : QObject(parent) {}
TicTacToeUiPlugin::~TicTacToeUiPlugin() {}

void TicTacToeUiPlugin::initLogos(LogosAPI* api)
{
    m_logosAPI = api;
}

static QString cellText(int cell)
{
    if (cell == CELL_X) return "X";
    if (cell == CELL_O) return "O";
    return "";
}

static QString statusText(int status, int current)
{
    if (status == STATUS_X_WINS) return "X wins!";
    if (status == STATUS_O_WINS) return "O wins!";
    if (status == STATUS_DRAW)   return "Draw!";
    return (current == CELL_X) ? "X's turn" : "O's turn";
}

QWidget* TicTacToeUiPlugin::createWidget(LogosAPI* logosAPI)
{
    auto* backend = new TicTacToeBackend(logosAPI);

    auto* widget = new QWidget();
    widget->setMinimumSize(360, 440);
    widget->setStyleSheet("QWidget { color: #e0e0e0; } "
                          "QPushButton { background-color: #2a2a2a; border: 1px solid #555; border-radius: 4px; color: #e0e0e0; } "
                          "QPushButton:hover { background-color: #3a3a3a; } "
                          "QPushButton:disabled { background-color: #1a1a1a; border-color: #333; color: #666; }");
    auto* root = new QVBoxLayout(widget);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // Title
    auto* title = new QLabel("Tic-Tac-Toe");
    title->setAlignment(Qt::AlignHCenter);
    QFont titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);
    root->addWidget(title);

    // Status label
    auto* statusLabel = new QLabel();
    statusLabel->setAlignment(Qt::AlignHCenter);
    QFont statusFont = statusLabel->font();
    statusFont.setPointSize(14);
    statusLabel->setFont(statusFont);
    root->addWidget(statusLabel);

    // Board grid
    auto* grid = new QGridLayout();
    grid->setSpacing(6);
    root->addLayout(grid);

    QFont cellFont;
    cellFont.setPointSize(32);
    cellFont.setBold(true);

    QPushButton* cells[3][3];
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            auto* btn = new QPushButton();
            btn->setFixedSize(96, 96);
            btn->setFont(cellFont);
            cells[r][c] = btn;
            grid->addWidget(btn, r, c);
        }
    }

    // Mode toggle: 2 Players / vs Computer
    auto* modeBtn = new QPushButton("Mode: 2 Players");
    QFont modeBtnFont = modeBtn->font();
    modeBtnFont.setPointSize(11);
    modeBtn->setFont(modeBtnFont);
    modeBtn->setStyleSheet("QPushButton { background-color: #333; border: 1px solid #666; padding: 6px; }");
    root->addWidget(modeBtn);

    // Track whether computer mode is on (shared_ptr for safe lambda capture)
    auto aiMode = std::make_shared<bool>(false);

    // New Game button
    auto* newGameBtn = new QPushButton("New Game");
    QFont btnFont = newGameBtn->font();
    btnFont.setPointSize(12);
    newGameBtn->setFont(btnFont);
    root->addWidget(newGameBtn);

    root->addStretch();

    // Refresh all cells and status from backend state
    auto refresh = [=]() {
        int s = backend->status();
        int cur = backend->currentPlayer();
        QString st = statusText(s, cur);
        if (*aiMode && s == STATUS_ONGOING)
            st = (cur == CELL_X) ? "Your turn (X)" : "Computer thinking...";
        statusLabel->setText(st);

        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                int cell = backend->getCell(r, c);
                cells[r][c]->setText(cellText(cell));
                // In computer mode, disable cells during computer's turn
                bool canClick = (cell == CELL_EMPTY && s == STATUS_ONGOING);
                if (*aiMode && cur == CELL_O && s == STATUS_ONGOING)
                    canClick = false;
                cells[r][c]->setEnabled(canClick);

                if (cell == CELL_X)
                    cells[r][c]->setStyleSheet("color: #4a9eff; font-weight: bold; background-color: #2a2a2a; border: 1px solid #555;");
                else if (cell == CELL_O)
                    cells[r][c]->setStyleSheet("color: #ff6b6b; font-weight: bold; background-color: #2a2a2a; border: 1px solid #555;");
                else
                    cells[r][c]->setStyleSheet("");
            }
        }
    };

    // Wire cell clicks — in computer mode, auto-play O after human X
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            QObject::connect(cells[r][c], &QPushButton::clicked, [=]() {
                backend->play(r, c);
                refresh();

                // If computer mode is on and the game continues, delay so
                // "Computer thinking..." is visible before the move lands
                if (*aiMode && backend->status() == STATUS_ONGOING
                    && backend->currentPlayer() == CELL_O) {
                    QTimer::singleShot(150, widget, [=]() {
                        backend->aiMove();
                        refresh();
                    });
                }
            });
        }
    }

    // Wire mode toggle — preserves current game state
    QObject::connect(modeBtn, &QPushButton::clicked, [=]() {
        *aiMode = !(*aiMode);
        modeBtn->setText(*aiMode ? "Mode: vs Computer" : "Mode: 2 Players");
        refresh();

        // If switching to computer mode mid-game and it's O's turn, delay move
        if (*aiMode && backend->status() == STATUS_ONGOING
            && backend->currentPlayer() == CELL_O) {
            QTimer::singleShot(150, widget, [=]() {
                backend->aiMove();
                refresh();
            });
        }
    });

    // Wire new game button
    QObject::connect(newGameBtn, &QPushButton::clicked, [=]() {
        backend->newGame();
        refresh();
    });

    // Initial state
    backend->newGame();
    refresh();

    return widget;
}

void TicTacToeUiPlugin::destroyWidget(QWidget* widget)
{
    delete widget;
}
