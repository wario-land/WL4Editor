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
    void mousePressEvent(QMouseEvent *event);
    void SetDockWidget(Tile16DockWidget *_dockWidget) { Map16DockWidget = _dockWidget; }
};

#endif // TILEGRAPHICSVIEW_H
