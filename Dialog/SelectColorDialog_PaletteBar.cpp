#include "SelectColorDialog_PaletteBar.h"

void SelectColorDialog_PaletteBar::mouseReleaseEvent(QMouseEvent *event)
{
    mouseX_Release = event->x() + horizontalScrollBar()->sliderPosition();
    int selectingColorId = qMin((mouseX_Release / 20), 0xF);
    if(event->button() == Qt::LeftButton)
    {
        SelectedColorId = selectingColorId;
        pdialog->SetColor(SelectedColorId);
    } else {
        return;
    }
}
