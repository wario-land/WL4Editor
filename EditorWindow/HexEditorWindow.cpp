#include "HexEditorWindow.h"
#include "ui_HexEditorWindow.h"
#include "ROMUtils.h"

/// <summary>
/// Construct the instance of the HexEditorWindow.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
HexEditorWindow::HexEditorWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::HexEditorWindow)
{
    ui->setupUi(this);
    ui->hexview->setFont(QFont("Timers", 8, QFont::Bold));
}

/// <summary>
/// Deconstruct the HexEditorWindow.
/// </summary>
HexEditorWindow::~HexEditorWindow()
{
    delete ui;
}

/// <summary>
/// (Re)Load Current File into the hexview
/// </summary>
/// <param name="filePath">
/// An optional param if you want to load other rom into the hexview
/// </param>
void HexEditorWindow::ReLoadFile(QString filePath)
{
    if (currentDocument)
    {
        delete currentDocument;
    }
    if (!filePath.size() || QFile::exists(filePath))
    {
        filePath = ROMUtils::ROMFileMetadata->FilePath;
    }
    currentDocument = QHexDocument::fromFile<QMemoryBuffer>(filePath);
    ui->hexview->setDocument(currentDocument);
}

/// <summary>
/// Save changes to the current ROM file
/// </summary>
void HexEditorWindow::on_actionSave_changes_triggered()
{
    QByteArray romByteArray = currentDocument->read(0, currentDocument->length());
    QFile file(ROMUtils::ROMFileMetadata->FilePath);
    file.open(QIODevice::WriteOnly);
    file.write(romByteArray);
    file.close();
}
