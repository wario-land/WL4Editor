#include "OutputDockWidget.h"
#include "ui_OutputDockWidget.h"

/// <summary>
/// Construct the instance of the OutputDockWidget and initialize jsEngine.
/// </summary>
OutputDockWidget::OutputDockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::OutputDockWidget)
{
    ui->setupUi(this);

    // Initialize jsEngine with QObject
    interface = new ScriptInterface();
    QJSValue funcInterface= jsEngine.newQObject(interface);
    jsEngine.globalObject().setProperty("interface", funcInterface);
}

/// <summary>
/// Execute JS scripts and output the result to textEdit_Output.
/// </summary>
QJSValue OutputDockWidget::ExecuteJSScript(QString scriptSourceCode)
{
    // execute scripts and output
    QTextCursor logCursor = ui->textEdit_Output->textCursor();
    QJSValue result = jsEngine.evaluate(scriptSourceCode, windowFilePath());
    if(result.isError()) {
            QTextCharFormat errFormat;
            logCursor.insertText(tr("Exception at line %1:\n").arg(result.property("lineNumber").toInt()), errFormat);
            logCursor.insertText(result.toString(), errFormat);
            logCursor.insertBlock();
            logCursor.insertText(result.property("stack").toString(), errFormat);
    } else {
            QTextCharFormat resultFormat;
            logCursor.insertText(result.toString(), resultFormat);
    }
    return result;
}

/// <summary>
/// Deconstruct the instance of the OutputDockWidget.
/// </summary>
OutputDockWidget::~OutputDockWidget()
{
    delete interface;
    delete ui;
}
