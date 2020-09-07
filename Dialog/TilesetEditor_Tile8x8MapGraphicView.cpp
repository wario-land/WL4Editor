#include "TilesetEditor_Tile8x8MapGraphicView.h"

/// <summary>
/// this function will be called when the TilesetEditor_Tile8x8MapGraphicView is clicked.
/// </summary>
/// <param name="event">
/// The mouse click event.
/// </param>
void TilesetEditor_Tile8x8MapGraphicView::mousePressEvent(QMouseEvent *event)
{
    mouseX_Press = event->x() + horizontalScrollBar()->sliderPosition();
    mouseY_Press = event->y() + verticalScrollBar()->sliderPosition();
    int Tile8x8Id = qMin((mouseX_Press >> 4), 0xF) + (qMin((mouseY_Press >> 4), 0x5F) << 4); // 0x600 = 0x10 * 0x60
    TilesetEditor->SetSelectedTile8x8(Tile8x8Id, false);
}

/// <summary>
/// this function will be called when key-press happens in TilesetEditor_Tile8x8MapGraphicView.
/// </summary>
/// <param name="event">
/// The key-press event.
/// </param>
void TilesetEditor_Tile8x8MapGraphicView::keyPressEvent(QKeyEvent *event)
{
    int selectedtile = TilesetEditor->GetSelectedTile8x8();
    if(selectedtile <= 0x40 || (selectedtile > (TilesetEditor->GetFGTile8x8Num() + 0x40)))
    {
        return;
    }
    if(event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)
    {
        TilesetEditor->DeleteFGTile8x8(selectedtile);
    }
}
