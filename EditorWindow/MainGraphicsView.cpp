#include "MainGraphicsView.h"

#include <QScrollBar>
#include <QMouseEvent>

#include <iostream>
extern WL4EditorWindow *singleton;
extern int selectedRoom;
extern LevelComponents::Level *CurrentLevel;

// TODO why is this event not getting called?
void MainGraphicsView::mousePressEvent(QMouseEvent *event)
{
    // Get the ID of the tile that was clicked
    int X = event->x() + horizontalScrollBar()->sliderPosition();
    int Y = event->y() + verticalScrollBar()->sliderPosition();
    int tileX = X / 32;
    int tileY = Y / 32;
    std::cout << "(" << tileX << ", " << tileY << ")" << std::endl;

    // TEST: change the tile (unfinished)
    if((CurrentLevel->GetRooms()[selectedRoom]->GetWidth() > tileX) && (CurrentLevel->GetRooms()[selectedRoom]->GetHeight() > tileY))
    {
        if(singleton->GetEditModeWidgetPtr()->GetEditModeParams().editMode == Ui::LayerEditMode)
        {
            CurrentLevel->GetRooms()[selectedRoom]->ChangeTile(singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer,
                                                          tileX, tileY, (unsigned short)singleton->GetTile16DockWidgetPtr()->GetSelectedTile());
            singleton->RenderScreenVisibilityChange();
        }
    }

    // TODO

}
