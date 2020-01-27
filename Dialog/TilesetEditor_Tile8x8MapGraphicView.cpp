#include "TilesetEditor_Tile8x8MapGraphicView.h"

void TilesetEditor_Tile8x8MapGraphicView::mousePressEvent(QMouseEvent *event)
{
    mouseX_Press = event->x() + horizontalScrollBar()->sliderPosition();
    mouseY_Press = event->y() + verticalScrollBar()->sliderPosition();
    int Tile8x8Id = qMin((mouseX_Press >> 4), 0xF) + (qMin((mouseY_Press >> 4), 0x5F) << 4); // 0x600 = 0x10 * 0x60
    TilesetEditor->SetSelectedTile8x8(Tile8x8Id, false);
}
