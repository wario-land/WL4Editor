#ifndef MAINGRAPHICSVIEW_H
#define MAINGRAPHICSVIEW_H

#include "Dialog/DoorConfigDialog.h"
#include "LevelComponents/Level.h"
#include "WL4EditorWindow.h"
#include <QGraphicsView>

class MainGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    MainGraphicsView(QWidget *param) : QGraphicsView(param) {}
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    int GetSelectedDoorID() { return SelectedDoorID; }
    void DeselectDoorAndEntity();

private:
    int SelectedDoorID = -1;
    int SelectedEntityID = -1;
    int drawingTileX = -1;
    int drawingTileY = -1;

    void SetTile(int tileX, int tileY);
};

#endif // MAINGRAPHICSVIEW_H
