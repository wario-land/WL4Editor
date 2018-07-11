#include "RoomConfigDialog.h"
#include "ui_RoomConfigDialog.h"

RoomConfigDialog::RoomConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RoomConfigDialog)
{
    ui->setupUi(this);
}

RoomConfigDialog::~RoomConfigDialog()
{
    delete ui;
}

void RoomConfigDialog::ShowTilesetDetails(int _tilesetIndex)
{
    // Set up tileset
    int _tilesetPtr = WL4Constants::TilesetDataTable + _tilesetIndex * 36;
    LevelComponents::Tileset *tmpTileset = new LevelComponents::Tileset(_tilesetPtr, _tilesetIndex);

    // Set up scene
    QGraphicsScene *tmpTile16MAPScene = new QGraphicsScene(0, 0, 8 * 16, (48 * 2) * 16);
    QPixmap layerPixmap(8 * 16, (48 * 2) * 16);
    layerPixmap.fill(Qt::transparent);

    // Draw the tiles to the QPixmap
    for(int i = 0; i < (48 * 2); ++i)
    {
        for(int j = 0; j < 8; ++j)
        {
            tmpTileset->GetMap16Data()[i * 8 + j]->DrawTile(&layerPixmap, j * 16, i * 16);
        }
    }
    tmpTile16MAPScene->addPixmap(layerPixmap);

    ui->graphicsView->setScene(tmpTile16MAPScene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
}

void RoomConfigDialog::ShowMappingType20LayerDetails(int _layerdataAddr)
{

}
