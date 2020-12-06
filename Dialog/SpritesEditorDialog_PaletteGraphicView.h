#ifndef SPRITESEDITORDIALOG_PALETTEGRAPHICVIEW_H
#define SPRITESEDITORDIALOG_PALETTEGRAPHICVIEW_H

#include "Dialog/SpritesEditorDialog.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QMouseEvent>

class SpritesEditorDialog_PaletteGraphicView : public QGraphicsView
{
    Q_OBJECT
public:
    SpritesEditorDialog_PaletteGraphicView(QWidget *param) : QGraphicsView(param) {}
    void mouseReleaseEvent(QMouseEvent *event);
    void SetCurrentSpritesEditor(SpritesEditorDialog *currentEditor) { spritesEditor = currentEditor; }

private:
    SpritesEditorDialog* spritesEditor = nullptr;
    int mouseX_Release = 0;
    int mouseY_Release = 0;
};

#endif // SPRITESEDITORDIALOG_PALETTEGRAPHICVIEW_H
