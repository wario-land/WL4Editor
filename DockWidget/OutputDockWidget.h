#ifndef OUTPUTDOCKWIDGET_H
#define OUTPUTDOCKWIDGET_H

#include "ScriptInterface.h"
#include "PCG/Graphics/TileUtils.h"
#include <QDockWidget>
#include <QJSEngine>

namespace Ui {
class OutputDockWidget;
}

class OutputDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit OutputDockWidget(QWidget *parent = nullptr);
    QJSValue ExecuteJSScript(QString scriptSourceCode, bool silenceFinishInfo = false);
    ~OutputDockWidget();

    // Functions
    void PrintString(QString str);
    void ClearTextEdit();

private slots:
    void on_pushButton_Execute_clicked();
    void on_pushButton_Abort_clicked();

private:
    Ui::OutputDockWidget *ui;
    QJSEngine jsEngine;

    // Qt meta object
    ScriptInterface *interface = nullptr;
    PCG::GFXUtils::TileUtils *tileUtils = nullptr;
};

#endif // OUTPUTDOCKWIDGET_H
