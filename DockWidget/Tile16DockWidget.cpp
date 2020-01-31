#include "Tile16DockWidget.h"
#include "ui_Tile16DockWidget.h"

#include <QMouseEvent>
#include <QScrollBar>

/// <summary>
/// Construct the instance of the Tile16DockWidget.
/// </summary>
/// <remarks>
/// The graphics view is hardcoded to scale at 2x size.
/// </remarks>
/// <param name="parent">
/// The parent QWidget.
/// </param>
Tile16DockWidget::Tile16DockWidget(QWidget *parent) : QDockWidget(parent), ui(new Ui::Tile16DockWidget)
{
    ui->setupUi(this);
    scalerate = ui->graphicsView->width() / (16 * 8 * 2);
    ui->graphicsView->scale(scalerate, scalerate);
    ui->graphicsView->SetDockWidget(this);

    // Set the fixed height of the info box
    QFontMetrics fontMetrics(ui->tileInfoTextBox->font());
    int rowHeight = fontMetrics.lineSpacing();
    ui->tileInfoGroupBox->setFixedHeight(6 * rowHeight); // TODO: Make this exact, calculate using margins
}

/// <summary>
/// Deconstruct the WL4EditorWindow and clean up its instance objects on the heap.
/// </summary>
Tile16DockWidget::~Tile16DockWidget()
{
    delete ui;
    if (Tile16MAPScene)
    {
        delete Tile16MAPScene;
    }
}

/// <summary>
/// This function will be triggered when the dock widget get focus.
/// </summary>
void Tile16DockWidget::FocusInEvent(QFocusEvent *e) { SetSelectedTile(0, true); }

/// <summary>
/// Set the tileset for the dock widget.
/// </summary>
/// <remarks>
/// Clean up heap objects from last time this funcction was called.
/// Set up and draw the graphics for the selectable map16 tiles.
/// Add the semi-transparent square used to highlight the selected tile (initialized to an invisible position)
/// </remarks>
/// <param name="_tilesetIndex">
/// The index of the tileset to display.
/// </param>
int Tile16DockWidget::SetTileset(int _tilesetIndex)
{
    // Clean up heap objects from previous invocations
    if (Tile16MAPScene)
    {
        delete Tile16MAPScene;
    }

    // Set up tileset
    SelectedTileset = ROMUtils::singletonTilesets[_tilesetIndex];

    // Set up scene
    Tile16MAPScene = new QGraphicsScene(0, 0, 8 * 16, (48 * 2) * 16);
    Tile16MAPScene->addPixmap(SelectedTileset->RenderAllTile16(1));
    ui->tileSetIDLabel->setText("Tileset ID: 0x" + QString::number(_tilesetIndex, 16).toUpper());

    // Add the highlighted tile rectangle
    QPixmap selectionPixmap(16, 16);
    const QColor highlightColor(0xFF, 0, 0, 0x7F);
    selectionPixmap.fill(highlightColor);
    SelectionBox = Tile16MAPScene->addPixmap(selectionPixmap);
    SelectionBox->setVisible(false);

    ui->graphicsView->setScene(Tile16MAPScene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // Re-initialize other settings
    SetSelectedTile(0, true);

    return 0;
}

/// <summary>
/// Set the text of the tile info box.
/// </summary>
/// <param name="str">
/// The string to display in the text box.
/// </param>
void Tile16DockWidget::SetTileInfoText(QString str) { ui->tileInfoTextBox->setText(str); }

/// <summary>
/// Set the selected tile index for the dock widget, and update the position of the highlight square.
/// </summary>
/// <remarks>
/// The SelectedTile instance variable must be set so it can be obtained when writing tiles to the main graphics view.
/// </remarks>
/// <param name="tile">
/// The map16 tile index that was selected in the graphics view.
/// </param>
void Tile16DockWidget::SetSelectedTile(unsigned short tile, bool resetscrollbar)
{
    // Paint red Box to show selected Tile16
    int X = tile & 7;
    int Y = tile >> 3;
    SelectionBox->setPos(X * 16, Y * 16);
    SelectionBox->setVisible(true);
    SelectedTile = tile;

    // Get the event information about the selected tile
    unsigned short eventIndex = SelectedTileset->GetEventTablePtr()[tile];
    int tmpTerrainTypeID = SelectedTileset->GetTerrainTypeIDTablePtr()[tile];

    // Print information about the tile to the user
    QString infoText;
    infoText.sprintf("Tile ID: %d\nEvent ID: 0x%04X\nTerrain type ID: %d", tile, eventIndex, tmpTerrainTypeID);
    SetTileInfoText(infoText);

    // Set vertical scrollbar of braphicview
    if (resetscrollbar)
        ui->graphicsView->verticalScrollBar()->setValue(scalerate * 16 * (tile / 8));
}
