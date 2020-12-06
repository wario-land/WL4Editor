#include "SpritesEditorDialog_SpriteTileMapGraphicView.h"

/// <summary>
/// this function will be called when the SpritesEditorDialog_SpriteTileMapGraphicView is clicked.
/// </summary>
/// <param name="event">
/// The mouse click event.
/// </param>
void SpritesEditorDialog_SpriteTileMapGraphicView::mouseReleaseEvent(QMouseEvent *event)
{
    mouseX_Release = event->x() + horizontalScrollBar()->sliderPosition();
    mouseY_Release = event->y() + verticalScrollBar()->sliderPosition();
    int Tile8x8Id = qMin((mouseX_Release >> 4), 0x1F) + (qMin((mouseY_Release >> 4), 0x1F) << 5); // 0x600 = 0x10 * 0x60
    spritesEditor->SetSelectedSpriteTile(Tile8x8Id);
}
