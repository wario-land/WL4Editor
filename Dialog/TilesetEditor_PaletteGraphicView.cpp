#include "TilesetEditor_PaletteGraphicView.h"


void TilesetEditor_PaletteGraphicView::mouseReleaseEvent(QMouseEvent *event)
{
    mouseX_Release = event->x() + horizontalScrollBar()->sliderPosition();
    int selectingColorId = qMin((mouseX_Release >> 4), 0xF);
    if(event->button() == Qt::LeftButton)
    {
        TilesetEditor->SetSelectedColorId(selectingColorId);
    }
    else if(event->button() == Qt::RightButton)
    {
        if(selectingColorId == 0) return;
        TilesetEditor->SetSelectedColorId(selectingColorId);
        TilesetEditor->SetColor(selectingColorId);
    }
    else
    {
        return;
    }
}
