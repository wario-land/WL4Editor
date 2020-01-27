#include "TilesetEditor_Tile16MapGraphicView.h"

void TilesetEditor_Tile16MapGraphicView::mousePressEvent(QMouseEvent *event)
{
    if(Inmouseslotfunction) return;
    Inmouseslotfunction = true;
    mouseX_Press = event->x() + horizontalScrollBar()->sliderPosition();
    mouseY_Press = event->y() + verticalScrollBar()->sliderPosition();
    Inmouseslotfunction = false;
}

void TilesetEditor_Tile16MapGraphicView::mouseReleaseEvent(QMouseEvent *event)
{
    if(Inmouseslotfunction) return;
    Inmouseslotfunction = true;
    mouseX_Release = event->x() + horizontalScrollBar()->sliderPosition();
    mouseY_Release = event->y() + verticalScrollBar()->sliderPosition();
    int Tile16Id_first = qMin((mouseX_Press >> 5), 7) + (qMin((mouseY_Press >> 5), 0x5F) << 3);  // 0x300 = 0x60 * 8
    int Tile16Id_second = qMin((mouseX_Release >> 5), 7) + (qMin((mouseY_Release >> 5), 0x5f) << 3);
    if(TilesetEditor->PaletteBrushValue() == -1)
    {
        if(Tile16Id_first == Tile16Id_second)
        {
            TilesetEditor->SetSelectedTile16(Tile16Id_second, false);
        }
        else
        {
            TilesetEditor->CopyTile16AndUpdateGraphic(Tile16Id_first, Tile16Id_second);
        }
    }
    else
    {
        TilesetEditor->SetTile16PaletteId(Tile16Id_second);
    }
    Inmouseslotfunction = false;
}
