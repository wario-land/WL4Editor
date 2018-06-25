#include "MainGraphicsView.h"

#include <QScrollBar>
#include <QMouseEvent>

#include <iostream>

// TODO why is this event not getting called?
void MainGraphicsView::mousePressEvent(QMouseEvent *event)
{
    // Get the ID of the tile that was clicked
    int X = event->x() + horizontalScrollBar()->sliderPosition();
    int Y = event->y() + verticalScrollBar()->sliderPosition();
    int tileX = X / 32;
    int tileY = Y / 32;
    std::cout << "(" << tileX << ", " << tileY << ")" << std::endl;

    // TODO change the tile

}
