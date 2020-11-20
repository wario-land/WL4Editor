#include "SpritesEditorDialog_PaletteGraphicView.h"

#include <QScrollBar>

/// <summary>
/// Mouse release event in Palette graphicsview.
/// </summary>
/// <param name="event">
/// QMouseEvent signal sent by the Qt kernal.
/// </param>
void SpritesEditorDialog_PaletteGraphicView::mouseReleaseEvent(QMouseEvent *event)
{
    mouseX_Release = event->x() + horizontalScrollBar()->sliderPosition();
    mouseY_Release = event->y() + verticalScrollBar()->sliderPosition();
    int selectingColorId = qMin((mouseX_Release >> 4), 0xF);
    int selectingPalId = qMin((mouseY_Release >> 4), 0xF);
    if(event->button() == Qt::LeftButton)
    {
        spritesEditor->SetSelectedEntityPaletteId(selectingPalId);
        spritesEditor->SetSelectedEntityColorId(selectingColorId);
    }
    else if(event->button() == Qt::RightButton)
    {
        if(selectingColorId == 0) return;
        spritesEditor->SetSelectedEntityPaletteId(selectingPalId);
        spritesEditor->SetSelectedEntityColorId(selectingColorId);
        spritesEditor->SetColor(selectingPalId, selectingColorId);
    }
    else
    {
        return;
    }
}
