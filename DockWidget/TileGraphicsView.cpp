#include "TileGraphicsView.h"
#include "LevelComponents/Tileset.h"

#include <QScrollBar>
#include <QMouseEvent>

void TileGraphicsView::mousePressEvent(QMouseEvent *event)
{
    // Get the ID of the tile that was clicked
    int X = event->x() + horizontalScrollBar()->sliderPosition();
    int Y = event->y() + verticalScrollBar()->sliderPosition();
    int tileX = X / 32;
    int tileY = Y / 32;
    int tileID = tileX + tileY * 8;

    // Get the event information about the selected tile
    LevelComponents::Tileset *Selectedtileset = DockWidget->GetSelectedTileset();
    unsigned short eventIndex = Selectedtileset->Map16EventTable[tileID];
    int tmpWarioAnimationSlotID = (int) Selectedtileset->Map16WarioAnimationSlotIDTable[tileID];

    // Print information about the tile to the user
    QString infoText = QString::asprintf("Tile ID: %d\nEvent ID: 0x%04X\nWario Animation Slot ID: %d", tileID, eventIndex, tmpWarioAnimationSlotID);
    DockWidget->SetTileInfoText(infoText);

    // Set the selected tile location for the graphics view
    DockWidget->SetSelectedTile(tileID);
}
