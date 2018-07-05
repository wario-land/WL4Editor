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
Tile16DockWidget::Tile16DockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::Tile16DockWidget)
{
    ui->setupUi(this);
    int scalerate = ui->graphicsView->width() / (16 * 8 * 2);
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
}

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
    if(SelectedTileset) { delete SelectedTileset; }
    if(Tile16MAPScene) { delete Tile16MAPScene; }

    // Set up tileset
    int _tilesetPtr = WL4Constants::TilesetDataTable + _tilesetIndex * 36;
    SelectedTileset = new LevelComponents::Tileset(_tilesetPtr, _tilesetIndex);

    // Set up scene
    Tile16MAPScene = new QGraphicsScene(0, 0, 8 * 16, (48 * 2) * 16);
    QPixmap layerPixmap(8 * 16, (48 * 2) * 16);
    layerPixmap.fill(Qt::transparent);

    // Draw the tiles to the QPixmap
    for(int i = 0; i < (48 * 2); ++i)
    {
        for(int j = 0; j < 8; ++j)
        {
            SelectedTileset->GetMap16Data()[i * 8 + j]->DrawTile(&layerPixmap, j * 16, i * 16);
        }
    }
    Tile16MAPScene->addPixmap(layerPixmap);
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
    SelectedTile = -1;
    SetTileInfoText(QString()); // clear tile info text

    return 0;
}

/// <summary>
/// Set the text of the tile info box.
/// </summary>
/// <param name="str">
/// The string to display in the text box.
/// </param>
void Tile16DockWidget::SetTileInfoText(QString str)
{
    ui->tileInfoTextBox->setText(str);
}

/// <summary>
/// Set the selected tile index for the dock widget, and update the position of the highlight square.
/// </summary>
/// <remarks>
/// The SelectedTile instance variable must be set so it can be obtained when writing tiles to the main graphics view.
/// </remarks>
/// <param name="tile">
/// The map16 tile index that was selected in the graphics view.
/// </param>
void Tile16DockWidget::SetSelectedTile(int tile)
{
    int X = tile & 7;
    int Y = tile >> 3;
    SelectionBox->setPos(X * 16, Y * 16);
    SelectionBox->setVisible(true);
    SelectedTile = tile;
}
