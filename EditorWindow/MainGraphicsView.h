#ifndef MAINGRAPHICSVIEW_H
#define MAINGRAPHICSVIEW_H

#include "Dialog/DoorConfigDialog.h"
#include "LevelComponents/Level.h"
#include "WL4EditorWindow.h"
#include <QtGlobal>
#include <QGraphicsView>

class MainGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    MainGraphicsView(QWidget *param) : QGraphicsView(param) {}
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    int GetSelectedDoorID() { return SelectedDoorID; }
    void DeselectDoorAndEntity(bool updateRenderArea = false);
    void SetRectSelectMode(bool state);
    void ClearRectPointer() { rect = nullptr; }

private:
    int SelectedDoorID = -1;
    int SelectedEntityID = -1;
    int drawingTileX = -1;
    int drawingTileY = -1;
    int objectInitialX = -1;
    int objectInitialY = -1;
    bool holdingEntityOrDoor = false;
    bool holdingmouse = false;
    bool rectSelectMode = false;
    const QColor highlightColor = QColor(0xFF, 0, 0, 0x7F);
    QGraphicsPixmapItem *rect = nullptr;
    QGraphicsPixmapItem *selectedrect = nullptr;

    void SetTiles(int tileX, int tileY);
    void CopyTile(int tileX, int tileY);
};

#endif // MAINGRAPHICSVIEW_H
