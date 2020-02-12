#ifndef TILESETEDITOR_TILE8X8MAPGRAPHICVIEW_H
#define TILESETEDITOR_TILE8X8MAPGRAPHICVIEW_H

#include "Dialog/TilesetEditDialog.h"
#include <LevelComponents/Tileset.h>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QMouseEvent>

class TilesetEditor_Tile8x8MapGraphicView : public QGraphicsView
{
    Q_OBJECT

public:
    TilesetEditor_Tile8x8MapGraphicView(QWidget *param) : QGraphicsView(param) {}
    void mousePressEvent(QMouseEvent *event);
    void SetCurrentTilesetEditor(TilesetEditDialog *currentEditor) { TilesetEditor = currentEditor; }

private:
    TilesetEditDialog *TilesetEditor = nullptr;
    int mouseX_Press                 = 0;
    int mouseY_Press                 = 0;
};

#endif // TILESETEDITOR_TILE8X8MAPGRAPHICVIEW_H
