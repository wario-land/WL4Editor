#include "MainGraphicsView.h"
#include "Operation.h"
#include "LevelComponents/Tile.h"
#include "LevelComponents/Door.h"

#include <QScrollBar>
#include <QMouseEvent>

#include <iostream>

extern WL4EditorWindow *singleton;

/// <summary>
/// this function will be called when the graphic view in the main window is clicked.
/// </summary>
/// <param name="event">
/// The mouse click event.
/// </param>
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
            if(layer->IsEnabled() == false) return;
            int selectedTileIndex = tileX + tileY * room->GetWidth();
            if(layer->GetLayerData()[selectedTileIndex] == selectedTile) return;
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
            if(room->CountDoors())
            {
                // Select a Door if possible
                for(int i = 0; i < room->CountDoors(); i++)
                {
                    LevelComponents::Door *door = room->GetDoor(i);
                    bool b1 = door->GetX1() <= tileX;
                    bool b2 = door->GetX2() >= tileX;
                    bool b3 = door->GetY1() <= tileY;
                    bool b4 = door->GetY2() >= tileY;
                    if(b1 && b2 && b3 && b4)
                    {
                        if(i == SelectedDoorID)
                        {
                            DoorConfigDialog _doorconfigdialog(singleton, room, i, singleton->GetCurrentLevel());
                            if(_doorconfigdialog.exec() == QDialog::Accepted)
                            {
                                // TODO
                            }
                        }
                        else
                        {
                            SelectedDoorID = i;
                        }
                        goto DOOR_FOUND;
                    }
                }
                SelectedDoorID = -1;
                DOOR_FOUND:;
            }
            singleton->RenderScreenElementsLayersUpdate((unsigned int) SelectedDoorID, -1);
        }
        else if(editMode == Ui::EntityEditMode) // select or add an Entity
        {
            SelectedEntityID = room->FindEntity(tileX, tileY);
            if(SelectedEntityID == -1)
            {
                bool success = room->AddEntity(tileX, tileY, singleton->GetEntitySetDockWidgetPtr()->GetCurrentEntityLocalId());
                // TODO: Show information if unsuccess
            }
            singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, SelectedEntityID);
        }
        // TODO add more cases for other edit mode types
    }
}

/// <summary>
/// this function will be called when key-press happens.
/// </summary>
/// <param name="event">
/// The key-press event.
/// </param>
void MainGraphicsView::keyPressEvent(QKeyEvent *event)
{
    if((SelectedEntityID != -1) && ((event->key() == Qt::Key_Backspace) || (event->key() == Qt::Key_Delete)))
    {
        singleton->DeleteEntity(SelectedEntityID);
        SelectedEntityID = -1;
        singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
    }
    else if((SelectedDoorID != -1) && ((event->key() == Qt::Key_Backspace) || (event->key() == Qt::Key_Delete)))
    {
        singleton->DeleteDoor(singleton->GetCurrentRoom()->GetDoor(SelectedDoorID)->GetGlobalDoorID());
        SelectedDoorID = -1;
        singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
    }
}

void MainGraphicsView::UnSelectDoorAndEntity()
{
    SelectedDoorID = -1;
    SelectedEntityID = -1;
    singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
}
