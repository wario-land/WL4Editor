#include "OutputDockWidget.h"
#include "ui_OutputDockWidget.h"
#include "WL4EditorWindow.h"
#include <QQmlEngine>

extern WL4EditorWindow *singleton;

/// <summary>
/// Construct the instance of the OutputDockWidget and initialize jsEngine.
/// </summary>
OutputDockWidget::OutputDockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::OutputDockWidget) // TODO memory leak
{
    ui->setupUi(this);

    // Initialize jsEngine with QObject
    interface = new ScriptInterface();
    QJSValue funcInterface = jsEngine.newQObject(interface);
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
    if(result.isError()) { // Only output error if needed
            QTextCharFormat errFormat;
            logCursor.insertText(tr("Exception at line %1:\n").arg(result.property("lineNumber").toInt()), errFormat);
            logCursor.insertText(result.toString(), errFormat);
            logCursor.insertBlock();
            logCursor.insertText(result.property("stack").toString(), errFormat);
    } else {
        ui->textEdit_Output->append("Script processing finished.\n");
    }
    QQmlEngine::setObjectOwnership(interface, QQmlEngine::CppOwnership);
    //jsEngine.collectGarbage();
    return result;
}

/// <summary>
/// Deconstruct the instance of the OutputDockWidget.
/// </summary>
OutputDockWidget::~OutputDockWidget()
{
    singleton->InvalidOutputWidgetPtr();
    delete interface;
    delete ui;
}

/// <summary>
/// Print String in the textEdit_Output.
/// </summary>
void OutputDockWidget::PrintString(QString str)
{
    ui->textEdit_Output->append(str + QString('\n'));
}

/// <summary>
/// Clear the textEdit_Output.
/// </summary>
void OutputDockWidget::ClearTextEdit()
{
    ui->textEdit_Output->clear();
}

/// <summary>
/// slot function when clicking pushButton_Execute.
/// </summary>
void OutputDockWidget::on_pushButton_Execute_clicked()
{
    if(ui->lineEdit_JsCode->text().length())
    {
        ExecuteJSScript(ui->lineEdit_JsCode->text());
        ui->lineEdit_JsCode->clear();
    }
}
