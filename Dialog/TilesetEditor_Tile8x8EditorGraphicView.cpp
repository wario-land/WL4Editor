#include "TilesetEditor_Tile8x8EditorGraphicView.h"

void TilesetEditor_Tile8x8EditorGraphicView::mouseReleaseEvent(QMouseEvent *event)
{
    mouseX_Release = event->x() + horizontalScrollBar()->sliderPosition();
    mouseY_Release = event->y() + verticalScrollBar()->sliderPosition();
}
