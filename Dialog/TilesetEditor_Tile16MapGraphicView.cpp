#include "TilesetEditor_Tile16MapGraphicView.h"

void TilesetEditor_Tile16MapGraphicView::mousePressEvent(QMouseEvent *event)
{
     mouseX_Press = event->x() + horizontalScrollBar()->sliderPosition();
     mouseY_Press = event->y() + verticalScrollBar()->sliderPosition();
}

void TilesetEditor_Tile16MapGraphicView::mouseReleaseEvent(QMouseEvent *event)
{
    mouseX_Release = event->x() + horizontalScrollBar()->sliderPosition();
    mouseY_Release = event->y() + verticalScrollBar()->sliderPosition();
    int Tile16Id_first = (mouseX_Press >> 5) + ((mouseY_Press >> 5) << 3);
    int Tile16Id_second = (mouseX_Release >> 5) + ((mouseY_Release >> 5) << 3);
    if(Tile16Id_first == Tile16Id_second)
    {
        TilesetEditor->SetSelectedTile16(Tile16Id_second, false);
    }
    else
    {
        TilesetEditor->CopyTile16AndUpdateGraphic(Tile16Id_first, Tile16Id_second);
    }
}
