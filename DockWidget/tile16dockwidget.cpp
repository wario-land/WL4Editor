#include "tile16dockwidget.h"
#include "ui_tile16dockwidget.h"

Tile16DockWidget::Tile16DockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::Tile16DockWidget)
{
    ui->setupUi(this);
}

Tile16DockWidget::~Tile16DockWidget()
{
    delete ui;
}

int Tile16DockWidget::SetTileset(int _tilesetIndex)
{
    // Set up tileset
    int _tilesetPtr = WL4Constants::TilesetDataTable + _tilesetIndex * 36;
    LevelComponents::Tileset *Selectedtileset = new LevelComponents::Tileset(_tilesetPtr, _tilesetIndex);
    QGraphicsScene *Tile16MAPScene = new QGraphicsScene(0, 0, 8 * 16, (48 * 2) * 16);
    ui->graphicsView->setScene(Tile16MAPScene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    QPixmap layerPixmap(8 * 16, (48 * 2) * 16);
    layerPixmap.fill(Qt::transparent);

    // Draw the tiles to the QPixmap
    for(int i = 0; i < (48 * 2); ++i)
    {
        for(int j = 0; j < 8; ++j)
        {
            Selectedtileset->GetMap16Data()[i * 8 + j]->DrawTile(&layerPixmap, j * 16, i * 16);
        }
    }
    Tile16MAPScene->addPixmap(layerPixmap);
    ui->graphicsView->show();
    ui->graphicsView->scale(2, 2);
    ui->label_TIlesetID->setText("Tileset ID: 0x" + QString::number(_tilesetIndex, 16).toUpper());

    return 0;
}
