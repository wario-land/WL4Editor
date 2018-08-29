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
    int GetSelectedDoorID() { return SelectedDoorID; }
    void UnSelectDoor();

private:
    int SelectedDoorID = -1;
};

#endif // MAINGRAPHICSVIEW_H
