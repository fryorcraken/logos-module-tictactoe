#ifndef TICTACTOE_UI_INTERFACE_H
#define TICTACTOE_UI_INTERFACE_H

#include <QObject>
#include <QString>
#include "interface.h"

class TicTacToeUiInterface : public PluginInterface
{
public:
    virtual ~TicTacToeUiInterface() = default;
};

#define TicTacToeUiInterface_iid "org.logos.TicTacToeUiInterface"
Q_DECLARE_INTERFACE(TicTacToeUiInterface, TicTacToeUiInterface_iid)

#endif // TICTACTOE_UI_INTERFACE_H
