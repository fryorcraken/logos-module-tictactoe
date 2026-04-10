#ifndef TICTACTOE_UI_PLUGIN_H
#define TICTACTOE_UI_PLUGIN_H

#include <QObject>
#include <QWidget>
#include <QVariantList>
#include <IComponent.h>
#include "tictactoe_ui_interface.h"

class TicTacToeUiPlugin : public QObject, public TicTacToeUiInterface, public IComponent
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IComponent_iid FILE "metadata.json")
    Q_INTERFACES(TicTacToeUiInterface PluginInterface IComponent)

public:
    explicit TicTacToeUiPlugin(QObject* parent = nullptr);
    ~TicTacToeUiPlugin() override;

    QString name()    const override { return "tictactoe_ui"; }
    QString version() const override { return "1.0.0"; }

    Q_INVOKABLE void initLogos(LogosAPI* api);

    Q_INVOKABLE QWidget* createWidget(LogosAPI* logosAPI = nullptr);
    Q_INVOKABLE void destroyWidget(QWidget* widget);

signals:
    void eventResponse(const QString& eventName, const QVariantList& args);

private:
    LogosAPI* m_logosAPI = nullptr;
};

#endif // TICTACTOE_UI_PLUGIN_H
