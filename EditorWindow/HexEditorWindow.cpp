#include "HexEditorWindow.h"
#include "ui_HexEditorWindow.h"
#include "ROMUtils.h"
#include "document/qhexcursor.h"

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
/// Goto an offset and refresh the hexview and reset the cursor
/// </summary>
/// <param name="offset">
/// Offset data is needed for the hexview update
/// </param>
void HexEditorWindow::gotoOffset(unsigned int offset)
{
    QHexCursor *hexCursor = currentDocument->cursor();
    hexCursor->moveTo(offset >> 4, static_cast<int>(offset & 0xF), 1);
}

/// <summary>
/// highlight data in hexview, background mode
/// </summary>
/// <param name="offset">
/// Offset data is needed for the data highlight
/// </param>
/// <param name="length">
/// Length of data will be highlighted
/// </param>
/// <param name="color">
/// Pick a color to highlight data
/// </param>
void HexEditorWindow::hightlightData_bg(unsigned int offset, unsigned length, QColor color)
{
    currentDocument->metadata()->background(offset >> 4, static_cast<int>(offset & 0xF), length, color);
}

/// <summary>
/// highlight data in hexview, foreground mode
/// </summary>
void HexEditorWindow::hightlightData_fg(unsigned int offset, unsigned length, QColor color)
{
    currentDocument->metadata()->foreground(offset >> 4, static_cast<int>(offset & 0xF), length, color);
}

/// <summary>
/// Clear all styling in hexview
/// </summary>
void HexEditorWindow::highlightClear()
{
    currentDocument->metadata()->clear();
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
