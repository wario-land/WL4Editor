#include "OutputDockWidget.h"
#include "ui_OutputDockWidget.h"

OutputDockWidget::OutputDockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::OutputDockWidget)
{
    ui->setupUi(this);
}

QJSValue OutputDockWidget::ExecuteJSScript(QString scriptSourceCode)
{
    // Initialize jsEngine with QObject
    ScriptInterface *interface = new ScriptInterface();
    QJSValue funcInterface= jsEngine.newQObject(interface);
    jsEngine.globalObject().setProperty("interface", funcInterface);

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

    delete interface;
    return result;
}

OutputDockWidget::~OutputDockWidget()
{
    delete ui;
}
