#include "Tile16DockWidget.h"
#include "ui_tile16dockwidget.h"

#include <QMouseEvent>

Tile16DockWidget::Tile16DockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::Tile16DockWidget)
{
    ui->setupUi(this);
    ui->graphicsView->scale(2, 2);
    ui->graphicsView->SetDockWidget(this);
}

Tile16DockWidget::~Tile16DockWidget()
{
    delete ui;
}

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
    ui->graphicsView->setScene(Tile16MAPScene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
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
    ui->label_TIlesetID->setText("Tileset ID: 0x" + QString::number(_tilesetIndex, 16).toUpper());

    // Add the highlighted tile rectangle
    QPixmap selectionPixmap(16, 16);
    const QColor highlightColor(0xFF, 0, 0, 0x7F);
    selectionPixmap.fill(highlightColor);
    SelectionBox = Tile16MAPScene->addPixmap(selectionPixmap);
    ui->graphicsView->show();

    // Re-initialize other settings
    SelectedTile = -1;
    ui->tileInfoTextBox->clear();

    return 0;
}

void Tile16DockWidget::SetTileInfoText(QString str)
{
    ui->tileInfoTextBox->setText(str);
}

void Tile16DockWidget::SetSelectedTile(int tile)
{
    int X = tile & 7;
    int Y = tile >> 3;
    SelectionBox->setPos(X * 16, Y * 16);
    SelectedTile = tile;
}
