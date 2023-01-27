#include "OutputDockWidget.h"
#include "ui_OutputDockWidget.h"
#include <QQmlEngine>

#ifndef WINDOW_INSTANCE_SINGLETON
#define WINDOW_INSTANCE_SINGLETON
#include "WL4EditorWindow.h"
extern WL4EditorWindow *singleton;
#endif

/// <summary>
/// Construct the instance of the OutputDockWidget and initialize jsEngine.
/// </summary>
OutputDockWidget::OutputDockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::OutputDockWidget) // TODO memory leak
{
    ui->setupUi(this);

    // Initialize jsEngine
    // use "WL4EditorInterface" to call its regular member functions
    if (!interface)
    {
        interface = new ScriptInterface();
    }
    QJSValue funcInterface = jsEngine.newQObject(interface);
    jsEngine.globalObject().setProperty("WL4EditorInterface", funcInterface);

    // use "CurrentRoomHintLayer" to manipulate the hint layer
    if (!CurrentRoomHintLayer)
    {
        CurrentRoomHintLayer = new HintLayer();
    }
    QJSValue JSObject_hintlayer = jsEngine.newQObject(CurrentRoomHintLayer);
    jsEngine.globalObject().setProperty("CurrentRoomHintLayer", JSObject_hintlayer);

    // use "TileUtils" to do PCG (procedual contents generation) Tile's graphic job
    if (!tileUtils)
    {
        tileUtils = new PCG::GFXUtils::TileUtils();
    }
    QJSValue JSObject_tileUtils = jsEngine.newQObject(tileUtils);
    jsEngine.globalObject().setProperty("TileUtils", JSObject_tileUtils);
}

/// <summary>
/// Execute JS scripts and output the result to textEdit_Output.
/// </summary>
QJSValue OutputDockWidget::ExecuteJSScript(QString scriptSourceCode, bool silenceFinishInfo)
{
    // execute scripts and output
    QTextCursor logCursor = ui->textEdit_Output->textCursor();
    QJSValue result = jsEngine.evaluate(scriptSourceCode/*, windowFilePath()*/);
    if(result.isError()) { // Only output error if needed
            QTextCharFormat errFormat;
            logCursor.insertText(tr("Exception at line %1:\n").arg(result.property("lineNumber").toInt()), errFormat);
            logCursor.insertText(result.toString(), errFormat);
            logCursor.insertBlock();
            logCursor.insertText(result.property("stack").toString(), errFormat);
    } else {
        if (!silenceFinishInfo) ui->textEdit_Output->append("Script processing finished.\n");
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
    delete tileUtils;
    delete ui;
}

/// <summary>
/// Print String in the textEdit_Output.
/// </summary>
void OutputDockWidget::PrintString(QString str)
{
    ui->textEdit_Output->append(str); // append function add a new paragraph to the textedit, no need to add an extra \n
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

/// <summary>
/// slot function when clicking pushButton_Abort.
/// </summary>
void OutputDockWidget::on_pushButton_Abort_clicked()
{
    jsEngine.setInterrupted(true);
}

