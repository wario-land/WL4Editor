#ifndef MAINGRAPHICSVIEW_H
#define MAINGRAPHICSVIEW_H

#include <QGraphicsView>

class MainGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    MainGraphicsView(QWidget *param) : QGraphicsView(param) {}
    void mousePressEvent(QMouseEvent *event);
};

#endif // MAINGRAPHICSVIEW_H
