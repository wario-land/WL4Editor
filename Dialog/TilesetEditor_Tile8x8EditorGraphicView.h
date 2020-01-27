#ifndef TILESETEDITOR_TILE8X8EDITORGRAPHICVIEW_H
#define TILESETEDITOR_TILE8X8EDITORGRAPHICVIEW_H

#include <LevelComponents/Tileset.h>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QMouseEvent>
#include "Dialog/TilesetEditDialog.h"

class TilesetEditor_Tile8x8EditorGraphicView : public QGraphicsView
{
    Q_OBJECT

public:
    TilesetEditor_Tile8x8EditorGraphicView(QWidget *param) : QGraphicsView(param) {}
    void mouseReleaseEvent(QMouseEvent *event);
    void SetCurrentTilesetEditor(TilesetEditDialog *currentEditor) { TilesetEditor = currentEditor; }

private:
    TilesetEditDialog *TilesetEditor = nullptr;
    int mouseX_Release = 0;
    int mouseY_Release = 0;
};

#endif // TILESETEDITOR_TILE8X8EDITORGRAPHICVIEW_H
