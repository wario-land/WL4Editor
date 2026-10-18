#include "TilesetEditor_PaletteGraphicView.h"

/// <summary>
/// Mouse release event in Palette graphicsview.
/// </summary>
/// <param name="event">
/// QMouseEvent signal sent by the Qt kernal.
/// </param>
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
        // Every color of the palette is editable: the palette color 0 is real data as well, even
        // though the hardware draws it as a transparent tile pixel (and uses the color 0 of BG
        // palette 0 as the backdrop color)
        TilesetEditor->SetSelectedColorId(selectingColorId);
        TilesetEditor->SetColor(selectingColorId);
    }
    else
    {
        return;
    }
}
