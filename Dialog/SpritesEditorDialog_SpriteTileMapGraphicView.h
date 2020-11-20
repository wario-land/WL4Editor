#ifndef SPRITESEDITORDIALOG_SPRITETILEMAPGRAPHICVIEW_H
#define SPRITESEDITORDIALOG_SPRITETILEMAPGRAPHICVIEW_H

#include "Dialog/SpritesEditorDialog.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QScrollBar>

class SpritesEditorDialog_SpriteTileMapGraphicView : public QGraphicsView
{
    Q_OBJECT
public:
    SpritesEditorDialog_SpriteTileMapGraphicView(QWidget *param) : QGraphicsView(param) {}
    void mouseReleaseEvent(QMouseEvent *event);
    void SetCurrentSpritesEditor(SpritesEditorDialog *currentEditor) { spritesEditor = currentEditor; }

private:
    SpritesEditorDialog* spritesEditor = nullptr;
    int mouseX_Release = 0;
    int mouseY_Release = 0;
};

#endif // SPRITESEDITORDIALOG_SPRITETILEMAPGRAPHICVIEW_H
