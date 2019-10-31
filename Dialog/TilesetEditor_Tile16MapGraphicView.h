#ifndef TILESETEDITOR_TILE16MAPGRAPHICVIEW_H
#define TILESETEDITOR_TILE16MAPGRAPHICVIEW_H

#include <LevelComponents/Tileset.h>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QMouseEvent>
#include "Dialog/TilesetEditDialog.h"

class TilesetEditor_Tile16MapGraphicView : public QGraphicsView
{
    Q_OBJECT

public:
    TilesetEditor_Tile16MapGraphicView(QWidget *param) : QGraphicsView(param) {}
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void SetCurrentTilesetEditor(TilesetEditDialog *currentEditor) { TilesetEditor = currentEditor; }

private:
    TilesetEditDialog *TilesetEditor = nullptr;
    bool Inmouseslotfunction = false;
    int mouseX_Press = 0;
    int mouseY_Press = 0;
    int mouseX_Release = 0;
    int mouseY_Release = 0;
};

#endif // TILESETEDITOR_TILE16MAPGRAPHICVIEW_H
