#include "TileGraphicsView.h"
#include "LevelComponents/Tileset.h"

#include <QScrollBar>
#include <QMouseEvent>
#include <QTextStream>

static bool initialized = false; // Used to allow the vertical scrollbar to initialize to the top once.

/// <summary>
/// Select the tile that was clicked.
/// </summary>
/// <remarks>
/// Update the tile info text.
/// Set the tile ID that is now being highlighted by the parent dock widget.
/// If the user clicks out of bounds of the selectable tiles, do nothing.
/// </remarks>
/// <param name="event">
/// The object from which the mouse press position can be obtained.
/// </param>
void TileGraphicsView::mousePressEvent(QMouseEvent *event)
{
    // Get the ID of the tile that was clicked
    int X = event->x() + horizontalScrollBar()->sliderPosition();
    int Y = event->y() + verticalScrollBar()->sliderPosition();
    if(X > 32 * 8 || Y > 32 * 0x60) return;
    int tileX = X / 32;
    int tileY = Y / 32;
    int tileID = tileX + tileY * 8;

    // Get the event information about the selected tile
    LevelComponents::Tileset *Selectedtileset = Map16DockWidget->GetSelectedTileset();
    unsigned short eventIndex = Selectedtileset->Map16EventTable[tileID];
    int tmpWarioAnimationSlotID = (int) Selectedtileset->Map16WarioAnimationSlotIDTable[tileID];

    // Print information about the tile to the user
    QString infoText;
    infoText.sprintf("Tile ID: %d\nEvent ID: 0x%04X\nWario Animation Slot ID: %d", tileID, eventIndex, tmpWarioAnimationSlotID);
    Map16DockWidget->SetTileInfoText(infoText);

    // Set the selected tile location for the graphics view
    Map16DockWidget->SetSelectedTile((unsigned short) tileID);
}

/// <summary>
/// Show the graphics view, and set the vertical scrollbar to the top of the view.
/// </summary>
/// <remarks>
/// Without this override, the graphics view will initialize with the vertical scrollbar halfway down.
/// </remarks>
/// <param name="event">
/// Resize event information which is sent to the parent implementation of this function.
/// </param>
void TileGraphicsView::showEvent(QShowEvent *event)
{
    QGraphicsView::showEvent(event);
    if(!initialized)
    {
        initialized = true;
        verticalScrollBar()->setValue(0);
    }
}
