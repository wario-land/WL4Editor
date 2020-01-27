#ifndef TILESETEDITOR_PALETTEGRAPHICVIEW_H
#define TILESETEDITOR_PALETTEGRAPHICVIEW_H

#include <LevelComponents/Tileset.h>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QMouseEvent>
#include "Dialog/TilesetEditDialog.h"

class TilesetEditor_PaletteGraphicView : public QGraphicsView
{
    Q_OBJECT

public:
    TilesetEditor_PaletteGraphicView(QWidget *param) : QGraphicsView(param) {}
    void mouseReleaseEvent(QMouseEvent *event);
    void SetCurrentTilesetEditor(TilesetEditDialog *currentEditor) { TilesetEditor = currentEditor; }

private:
    TilesetEditDialog *TilesetEditor = nullptr;
    int mouseX_Release = 0;
    int mouseY_Release = 0;
};

#endif // TILESETEDITOR_PALETTEGRAPHICVIEW_H
