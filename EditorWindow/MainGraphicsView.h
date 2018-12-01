#ifndef MAINGRAPHICSVIEW_H
#define MAINGRAPHICSVIEW_H

#include <QGraphicsView>
#include "WL4EditorWindow.h"
#include "LevelComponents/Level.h"
#include "Dialog/DoorConfigDialog.h"

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
    bool IsDrawing = false;

    void SetTile(int tileX, int tileY);
};

#endif // MAINGRAPHICSVIEW_H
