#ifndef TILEGRAPHICSVIEW_H
#define TILEGRAPHICSVIEW_H

#include "Tile16DockWidget.h"
#include <QGraphicsView>

class TileGraphicsView : public QGraphicsView
{
    Q_OBJECT

private:
    Tile16DockWidget *Map16DockWidget;

protected:
    void showEvent(QShowEvent *event);

public:
    TileGraphicsView(QWidget *param) : QGraphicsView(param) {}
    void SetDockWidget(Tile16DockWidget *_dockWidget) { Map16DockWidget = _dockWidget; }
    void Resetmembers() { tx = ty = 0; holdingmouse = false;}

    // mouse events
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    bool holdingmouse = false;
    int tx = 0;
    int ty = 0;
};

#endif // TILEGRAPHICSVIEW_H
