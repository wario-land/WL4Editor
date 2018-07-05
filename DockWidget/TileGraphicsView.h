#ifndef TILEGRAPHICSVIEW_H
#define TILEGRAPHICSVIEW_H

#include <QGraphicsView>
#include "Tile16DockWidget.h"

class TileGraphicsView : public QGraphicsView
{
    Q_OBJECT

private:
    Tile16DockWidget *DockWidget;

protected:
    void showEvent(QShowEvent *event);

public:
    TileGraphicsView(QWidget *param) : QGraphicsView(param) {}
    void mousePressEvent(QMouseEvent *event);
    void SetDockWidget(Tile16DockWidget *_dockWidget) { DockWidget = _dockWidget; }
};

#endif // TILEGRAPHICSVIEW_H
