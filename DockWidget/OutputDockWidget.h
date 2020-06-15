#ifndef OUTPUTDOCKWIDGET_H
#define OUTPUTDOCKWIDGET_H

#include "ScriptInterface.h"
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
    QJSValue ExecuteJSScript(QString scriptSourceCode);
    ~OutputDockWidget();

    // Functions
    void PrintString(QString str);
    void ClearTextEdit();

private slots:
    void on_pushButton_Execute_clicked();

private:
    Ui::OutputDockWidget *ui;
    ScriptInterface *interface;
    QJSEngine jsEngine;
};

#endif // OUTPUTDOCKWIDGET_H
