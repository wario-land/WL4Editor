#include "MainGraphicsView.h"
#include "Operation.h"
#include "LevelComponents/Tile.h"
#include "LevelComponents/Door.h"

#include <QScrollBar>
#include <QMouseEvent>

#include <iostream>

extern WL4EditorWindow *singleton;

// TODO why is this event not getting called?
void MainGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if(!singleton->FirstROMIsLoaded()) return;

    // Get the ID of the tile that was clicked
    int X = event->x() + horizontalScrollBar()->sliderPosition();
    int Y = event->y() + verticalScrollBar()->sliderPosition();
    int tileX = X / 32;
    int tileY = Y / 32;

    // Different cases for different editting mode
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    if(tileX < room->GetWidth() && tileY < room->GetHeight())
    {
        enum Ui::EditMode editMode = singleton->GetEditModeWidgetPtr()->GetEditModeParams().editMode;

        if(editMode == Ui::LayerEditMode) // Change textmaps and layer graphic
        {
            unsigned short selectedTile = singleton->GetTile16DockWidgetPtr()->GetSelectedTile();
            if(selectedTile == 0xFFFF) return;
            int selectedLayer = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer;
            LevelComponents::Layer *layer = room->GetLayer(selectedLayer);
            int selectedTileIndex = tileX + tileY * room->GetWidth();
            struct OperationParams *params = new struct OperationParams();
            params->type = ChangeTileOperation;
            params->tileChangeParams.push_back(TileChangeParams::Create(
                tileX,
                tileY,
                selectedLayer,
                selectedTile,
                layer->GetLayerData()[selectedTileIndex]
            ));
            ExecuteOperation(params);
            // delete params; //shall we?
        }
        else if(editMode == Ui::DoorEditMode) // select a door
        {
            if(room->CountDoors() > 0)
            {
                for(int i = 0; i < room->CountDoors(); i++)
                {
                    if((room->Door(i)->GetX1() <= tileX) &&
                            (room->Door(i)->GetX2() >= tileX) &&
                            (room->Door(i)->GetY1() <= tileY) &&
                            (room->Door(i)->GetY2() >= tileY))
                    {
                        SelectedDoorID = i;
                        break;
                    }
                }
            }
        }
        // TODO add more cases for other edit mode types
    }
}
