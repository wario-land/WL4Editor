#ifndef ROOMPREVIEWGRAPHICSVIEW_H
#define ROOMPREVIEWGRAPHICSVIEW_H

#include <LevelComponents/Tileset.h>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QLabel>

namespace Ui
{
    class RoomPreviewGraphicsView;
}

class RoomPreviewGraphicsView : public QGraphicsView
{
    Q_OBJECT

private:
    bool initialized = false;               // Used for initializing the scrollbars
    void *dataPointers[4] = { 0, 0, 0, 0 }; // Tilset, BG, L0
    QGraphicsPixmapItem *pixmapItems[3];
    unsigned int displayPixmap;
    void DisplayNextPixmap();
    void EnableSelectedPixmap();
    ~RoomPreviewGraphicsView();

protected:
    void showEvent(QShowEvent *event);

public:
    QLabel *infoLabel;
    RoomPreviewGraphicsView(QWidget *param);
    void mousePressEvent(QMouseEvent *event);
    void UpdateGraphicsItems(LevelComponents::Tileset *tileset, int BGptr, int L0ptr);
};

#endif // ROOMPREVIEWGRAPHICSVIEW_H
