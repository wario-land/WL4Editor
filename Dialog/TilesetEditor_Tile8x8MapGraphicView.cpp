#include "TilesetEditor_Tile8x8MapGraphicView.h"

void TilesetEditor_Tile8x8MapGraphicView::mousePressEvent(QMouseEvent *event)
{
    mouseX_Press = event->x() + horizontalScrollBar()->sliderPosition();
    mouseY_Press = event->y() + verticalScrollBar()->sliderPosition();
    int Tile8x8Id = (mouseX_Press >> 4) + ((mouseY_Press >> 4) << 4);
    TilesetEditor->SetSelectedTile8x8(Tile8x8Id, false);
}
