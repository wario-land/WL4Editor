#include "MainGraphicsView.h"
#include "WL4EditorWindow.h"
#include "Operation.h"

#include <QMessageBox>
#include <QMouseEvent>
#include <QScrollBar>

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
    if (!singleton->FirstROMIsLoaded())
        return;

    holdingmouse = true;

    // Get the ID of the tile that was clicked
    uint scalerate = singleton->GetGraphicViewScalerate();
    unsigned int X = (unsigned int) event->x() + horizontalScrollBar()->sliderPosition();
    unsigned int Y = (unsigned int) event->y() + verticalScrollBar()->sliderPosition();
    unsigned int tileX = X / (16 * scalerate);
    unsigned int tileY = Y / (16 * scalerate);

    // Different cases for different editting mode
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    if (tileX < qMax(room->GetWidth(), room->GetLayer0Width()) && tileY < qMax(room->GetHeight(), room->GetLayer0Height()))
    {
        if (singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer != 0)
        {
            if (tileX >= room->GetWidth() || tileY >= room->GetHeight())
                return;
        }
        else // singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer == 0
        {
            if (tileX >= room->GetLayer0Width() || tileY >= room->GetLayer0Height())
                return;
        }

        enum Ui::EditMode editMode = singleton->GetEditModeWidgetPtr()->GetEditModeParams().editMode;

        if (editMode == Ui::LayerEditMode)
        {
            if(!rectSelectMode)
            {
                // Use right click then copy the tile
                if (event->button() == Qt::RightButton)
                {
                    CopyTile(tileX, tileY);
                }
                else // Otherwise just place the tile
                {
                    // Change textmaps and layer graphics
                    SetTiles(tileX, tileY);
                }
            } else {
                if (event->button() == Qt::LeftButton)
                {
                    if(!singleton->Getgraphicview()->scene()) return;

                    if(has_a_rect)
                    {
                        // De-rect-select if need
                        if(tileX < rectx || tileX > (rectx + rectwidth - 1) ||
                                tileY < recty || tileY > (recty + rectheight - 1))
                        {

                            ResetRectPixmaps();
                            has_a_rect = false;

                            // Do Operation (and update layer data)
                            int selectedLayer = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer;
                            unsigned short *Layerdata = singleton->GetCurrentRoom()->GetLayer(selectedLayer)->GetLayerData();
                            int layerwidth = singleton->GetCurrentRoom()->GetLayer(selectedLayer)->GetLayerWidth();
                            int layerheight = singleton->GetCurrentRoom()->GetLayer(selectedLayer)->GetLayerHeight();
                            struct OperationParams *params = new struct OperationParams();
                            params->type = ChangeTileOperation;
                            params->tileChange = true;
                            int rectcuttopheight, rectcutleftsidewidth;
                            int rectcutbottomheight, rectcutrightsidewidth;
                            rectcuttopheight = qMax(-recty, 0);
                            rectcutleftsidewidth = qMax(-rectx, 0);
                            rectcutbottomheight = qMax(recty + rectheight - 1 - (layerheight - 1), 0);
                            rectcutrightsidewidth = qMax(rectx + rectwidth - 1 - (layerwidth - 1), 0);
                            for(int j = rectcuttopheight; j < (rectheight - rectcutbottomheight); ++j) // paste rect
                            {
                                for(int i = rectcutleftsidewidth; i < (rectwidth - rectcutrightsidewidth); ++i)
                                {
                                    params->tileChangeParams.push_back(
                                        TileChangeParams::Create(rectx + i, recty + j, selectedLayer,
                                                                 rectdata[i + j * rectwidth],
                                                                 Layerdata[rectx + i + (recty + j) * layerwidth]));
                                }
                            }
                            ExecuteOperation(params);
                            // Reset variables
                            ResetRect();
                            return;
                        }

                        // Start holding the mouse inside the rect
                        Isdraggingrect = true;
                        dragInitmouseX = tileX;
                        dragInitmouseY = tileY;
                    } else {
                        // reset rect
                        QPixmap selectionPixmap(16, 16);
                        selectionPixmap.fill(highlightColor);
                        if(rect == nullptr)
                        {
                            rect = singleton->Getgraphicview()->scene()->addPixmap(selectionPixmap);
                        } else {
                            rect->setPixmap(selectionPixmap);
                        }
                        rectx = tmpLTcornerTileX = rectselectstartTileX = tileX;
                        recty = tmpLTcornerTileY = rectselectstartTileY = tileY;
                        rectwidth = rectheight = 1;
                        rect->setPos(tileX * 16, tileY * 16);
                        rect->setZValue(14); // assume every layer in room is enabled, and rect should be above selectedrectgraphic
                        rect->setVisible(true);
                        has_a_rect = true;
                    }
                }
            }

        }
        else if (editMode == Ui::DoorEditMode) // select a door
        {
            auto doorsInRoom = singleton->GetCurrentLevel()->GetDoorList().GetDoorsByRoomID(room->GetRoomID());
            unsigned int doorCount = doorsInRoom.size();
            if (doorCount)
            {
                // Select a Door if possible
                for (unsigned int i = 0; i < doorCount; i++)
                {
                    bool b1 = doorsInRoom[i].x1 <= (int) tileX;
                    bool b2 = doorsInRoom[i].x2 >= (int) tileX;
                    bool b3 = doorsInRoom[i].y1 <= (int) tileY;
                    bool b4 = doorsInRoom[i].y2 >= (int) tileY;
                    if (b1 && b2 && b3 && b4) {

                        // If the door that was clicked was not already selected, then select it
                        SelectedDoorID = i;
                        // Let the Entityset change with the last selected Door
                        singleton->GetCurrentRoom()->SetCurrentEntitySet(doorsInRoom[i].EntitySetID);
                        singleton->ResetEntitySetDockWidget();
                        holdingEntityOrDoor = true;
                        objectInitialX = doorsInRoom[i].x1;
                        objectInitialY = doorsInRoom[i].y1;
                        goto DOOR_FOUND_MousePress;
                    }
                }
                SelectedDoorID = -1;
            DOOR_FOUND_MousePress:;
            }
            singleton->RenderScreenElementsLayersUpdate((unsigned int) SelectedDoorID, -1);
        }
        else if (editMode == Ui::EntityEditMode) // select or add an Entity
        {
            SelectedEntityID = room->FindEntity(tileX, tileY);
            int difficulty = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedDifficulty;
            if (SelectedEntityID == -1)
            {
                // Add the new entity
                bool success =
                    room->AddEntity(tileX, tileY, singleton->GetEntitySetDockWidgetPtr()->GetCurrentEntityLocalId());
                if(!success)
                {
                    singleton->GetOutputWidgetPtr()->PrintString("Cannot add more entity under the current difficulty in this room");
                    return;
                }
                room->SetEntityListDirty(difficulty, true);
                singleton->SetUnsavedChanges(true);
                SelectedEntityID = room->FindEntity(tileX, tileY); // reset SelectedEntityID
            }
            singleton->RenderScreenElementsLayersUpdate(0xFFFFFFFFu, SelectedEntityID);
            if (event->button() == Qt::RightButton)
            {
                singleton->GetEntitySetDockWidgetPtr()->SetCurrentEntity(room->GetEntityListData(difficulty)[SelectedEntityID].EntityID);
            }
            holdingEntityOrDoor = true;
            objectInitialX = tileX;
            objectInitialY = tileY;
        }
    }
}


/// <summary>
/// This function will be called when the graphic view in the main window is double clicked
/// For now it used on door to open the dialog
/// <param name="event">
/// The mouse click event.
/// </param>
void MainGraphicsView::mouseDoubleClickEvent(QMouseEvent *event) {
    if (!singleton->FirstROMIsLoaded())
        return;

    // Get the ID of the tile that was clicked
    uint scalerate = singleton->GetGraphicViewScalerate();
    unsigned int X = (unsigned int) event->x() + horizontalScrollBar()->sliderPosition();
    unsigned int Y = (unsigned int) event->y() + verticalScrollBar()->sliderPosition();
    unsigned int tileX = X / (16 * scalerate);
    unsigned int tileY = Y / (16 * scalerate);

    // Different cases for different editting mode
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    if (tileX < room->GetWidth() && tileY < room->GetHeight())
    {
        enum Ui::EditMode editMode = singleton->GetEditModeWidgetPtr()->GetEditModeParams().editMode;

        if (editMode == Ui::DoorEditMode) // select a door
        {
            auto doorsInRoom = singleton->GetCurrentLevel()->GetDoorList().GetDoorsByRoomID(room->GetRoomID());
            unsigned int doorCount = doorsInRoom.size();
            if (doorCount)
            {
                // Select a Door if possible
                for (unsigned int i = 0; i < doorCount; i++)
                {
                    bool b1 = doorsInRoom[i].x1 <= (int) tileX;
                    bool b2 = doorsInRoom[i].x2 >= (int) tileX;
                    bool b3 = doorsInRoom[i].y1 <= (int) tileY;
                    bool b4 = doorsInRoom[i].y2 >= (int) tileY;
                    if (b1 && b2 && b3 && b4)
                    {
                        // Door "i" was selected
                        if ((int) i == SelectedDoorID)
                        {
                            // If the door that was clicked is already selected, open the door config dialog
                            DoorConfigDialog _doorconfigdialog(singleton, room, i, singleton->GetCurrentLevel());
                            if (_doorconfigdialog.exec() == QDialog::Accepted)
                            {
                                // Apply changes
                                // TODO: put the logic to the operation class to support Undo and Redo of Door things
                                singleton->GetCurrentLevel()->SetDoorVec(_doorconfigdialog.GetChangedDoorVectorResult());
                                singleton->ResetEntitySetDockWidget();
                                singleton->SetUnsavedChanges(true);
                            }
                        }
                        else
                        {
                            // If the door that was clicked was not already selected, then select it
                            SelectedDoorID = i;
                            // Let the Entityset change with the last selected Door
                            singleton->GetCurrentRoom()->SetCurrentEntitySet(doorsInRoom[i].EntitySetID);
                            singleton->ResetEntitySetDockWidget();
                            holdingEntityOrDoor = true;
                            objectInitialX = doorsInRoom[i].x1;
                            objectInitialY = doorsInRoom[i].y1;
                        }
                        goto DOOR_FOUND_DoubleClick;
                    }
                }
                SelectedDoorID = -1;
                DOOR_FOUND_DoubleClick:;
            }
            singleton->RenderScreenElementsLayersUpdate((unsigned int) SelectedDoorID, -1);
        }
    }
}


/// <summary>
/// this function will be called when the mouse is moving on the graphic view in the main window.
/// </summary>
/// <param name="event">
/// The mouse click event.
/// </param>
void MainGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (!singleton->FirstROMIsLoaded())
        return;

    // Get the ID of the tile that was clicked
    uint scalerate = singleton->GetGraphicViewScalerate();
    unsigned int X = (unsigned int) event->x() + horizontalScrollBar()->sliderPosition();
    unsigned int Y = (unsigned int) event->y() + verticalScrollBar()->sliderPosition();
    unsigned int tileX = X / (16 * scalerate);
    unsigned int tileY = Y / (16 * scalerate);

    // Print Mouse Position
    singleton->PrintMousePos(X / scalerate, Y / scalerate);
    if(!holdingmouse) return;

    // If we have moved within the same tile, do nothing
    if (tileX == (unsigned int) drawingTileX && tileY == (unsigned int) drawingTileY)
    {
        return;
    }

    // main part
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    if (tileX < qMax(room->GetWidth(), room->GetLayer0Width()) && tileY < qMax(room->GetHeight(), room->GetLayer0Height()))
    {
        if (singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer != 0)
        {
            if (tileX >= room->GetWidth() || tileY >= room->GetHeight())
                return;
        }
        else // singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer == 0
        {
            if (tileX >= room->GetLayer0Width() || tileY >= room->GetLayer0Height())
                return;
        }

        enum Ui::EditMode editMode = singleton->GetEditModeWidgetPtr()->GetEditModeParams().editMode;

        if (editMode == Ui::LayerEditMode)
        {
            if(!rectSelectMode)
            {
                // If we hold right click then copy the tile
                // event->button() cannot work, event->buttons() return the correct mouseState according to the Qt code
                if (event->buttons() == Qt::RightButton)
                {
                    CopyTile(tileX, tileY);
                }
                else // Otherwise just place the tile
                {
                    // Change textmaps and layer graphics
                    SetTiles(tileX, tileY);
                }
            }
            else
            {
                if(!has_a_rect) return;

                if(!Isdraggingrect)
                {
                    if(tileX < rectselectstartTileX || tileY < rectselectstartTileY)
                        return;

                    // reset rect
                    rectwidth = tileX - rectselectstartTileX + 1;
                    rectheight = tileY - rectselectstartTileY + 1;
                    QPixmap selectionPixmap(16 * rectwidth, 16 * rectheight);
                    selectionPixmap.fill(highlightColor);
                    rect->setPixmap(selectionPixmap);
                    rect->setPos(rectx * 16, recty * 16);
                    rect->setZValue(14); // assume every layer in room is enabled, and rect should be above selectedrectgraphic
                    rect->setVisible(true);
                }
                else
                {
                    rectx = tmpLTcornerTileX - dragInitmouseX + tileX;
                    recty = tmpLTcornerTileY - dragInitmouseY + tileY;
                    rect->setPos(rectx * 16, recty * 16);
                    selectedrectgraphic->setPos(rectx * 16, recty * 16);
                }
            }
        }
        else if(editMode == Ui::EntityEditMode)
        {
            if (holdingEntityOrDoor && SelectedEntityID != -1)
            {
                LevelComponents::Room *currentRoom = singleton->GetCurrentRoom();
                // If the entity position has changed
                int px = currentRoom->GetEntityX(SelectedEntityID);
                int py = currentRoom->GetEntityY(SelectedEntityID);

                // If the entity position has changed
                if (px != tileX || py != tileY)
                {
                    if (currentRoom->IsNewEntityPositionInsideRoom(tileX, tileY))
                    {
                        currentRoom->SetEntityPosition(tileX, tileY, SelectedEntityID);
                        singleton->RenderScreenElementsLayersUpdate(0xFFFFFFFFu, SelectedEntityID);
                        int difficulty = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedDifficulty;
                        singleton->GetCurrentRoom()->SetEntityListDirty(difficulty, true);
                        singleton->SetUnsavedChanges(true);
                    }
                }
            }
        }
        else if(editMode == Ui::DoorEditMode)
        {
            if (holdingEntityOrDoor && SelectedDoorID != -1)
            {
                auto doorsInRoom = singleton->GetCurrentLevel()->GetDoorList().GetDoorsByRoomID(room->GetRoomID());
                auto curDoor = doorsInRoom[SelectedDoorID];

                // The new positions
                int px1 = curDoor.x1;
                int py1 = curDoor.y1;
                int deltaX = curDoor.x2 - px1;
                int deltaY = curDoor.y2 - py1;

                // If the door position has changed
                if (px1 != tileX || py1 != tileY)
                {
                    if (room->IsNewDoorPositionInsideRoom(tileX, tileX + deltaX, tileY, tileY + deltaY))
                    {
                        int globalDoorId = singleton->GetCurrentLevel()->GetDoorList().GetGlobalIDByLocalID(room->GetRoomID(), SelectedDoorID);
                        singleton->GetCurrentLevel()->GetDoorListRef().SetDoorPlace(globalDoorId, tileX, tileX + deltaX, tileY, tileY + deltaY);
                        singleton->RenderScreenElementsLayersUpdate((unsigned int) SelectedDoorID, -1);
                    }
                }
            }
        }
    }
}

/// <summary>
/// this function will be called when the mouse is dragging out from the graphic view in the main window.
/// </summary>
/// <param name="event">
/// The drag leave event.
/// </param>
void MainGraphicsView::dragLeaveEvent(QDragLeaveEvent *event)
{
    (void) event;
    holdingmouse = false;
}

/// <summary>
/// This is a helper function for setting the tile at a tile position to the currently
/// selected tile from the UI.
/// </summary>
/// <param name="tileX">
/// The X position of the tile (unit: map16)
/// </param>
/// <param name="tileY">
/// The Y position of the tile (unit: map16)
/// </param>
void MainGraphicsView::SetTiles(int tileX, int tileY)
{
    // Update which tile has last been drawn, for the tile painting functionality
    drawingTileX = tileX;
    drawingTileY = tileY;

    // Execute a tile change operation for the changed tile
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    unsigned short selectedTile = singleton->GetTile16DockWidgetPtr()->GetSelectedTile();
    if (selectedTile == 0xFFFF)
        return;
    int selectedLayer = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer;
    LevelComponents::Layer *layer = room->GetLayer(selectedLayer);
    if (layer->IsEnabled() == false)
        return;
    if (layer->GetMappingType() == LevelComponents::LayerTile8x8)
        return; // temporarily skip the condition when the current Layer's MappingType is 0x20 to avoid incorrect
                // rendering
    int selectedTileIndex;
    int drawwidth = singleton->GetTile16DockWidgetPtr()->getrw();
    int drawheight = singleton->GetTile16DockWidgetPtr()->getrh();
    int drawlayerwidth = 0;
    if (selectedLayer)
    {
        drawlayerwidth = room->GetWidth();
        selectedTileIndex = tileX + tileY * drawlayerwidth;
        drawwidth = qMin(drawlayerwidth - tileX, drawwidth);
        drawheight = qMin(static_cast<int>(room->GetHeight()) - tileY, drawheight);
    } else {
        drawlayerwidth = room->GetLayer0Width();
        selectedTileIndex = tileX + tileY * drawlayerwidth;
        drawwidth = qMin(drawlayerwidth - tileX, drawwidth);
        drawheight = qMin(static_cast<int>(room->GetLayer0Height()) - tileY, drawheight);
    }
    if (layer->GetLayerData()[selectedTileIndex] == selectedTile && drawwidth == 1 && drawheight == 1)
        return;
    struct OperationParams *params = new struct OperationParams();
    params->type = ChangeTileOperation;
    params->tileChange = true;
    for(int j = 0; j < drawheight; ++j)
    {
        for(int i = 0; i < drawwidth; ++i)
        {
            params->tileChangeParams.push_back(
                TileChangeParams::Create(tileX + i, tileY + j, selectedLayer, selectedTile + i + 8 * j,
                                         layer->GetLayerData()[selectedTileIndex + i + j * drawlayerwidth]));
        }
    }
    ExecuteOperation(params);
}

/// <summary>
/// This is a helper function for copying a tile at a tile position to the clipboard
/// </summary>
/// <param name="tileX">
/// The X position of the tile to be copied  (unit: map16)
/// </param>
/// <param name="tileY">
/// The Y position of the tile to be copied (unit: map16)
/// </param>
void MainGraphicsView::CopyTile(int tileX, int tileY)
{
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    int selectedLayer = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer;
    LevelComponents::Layer *layer = room->GetLayer(selectedLayer);
    int selectedTileIndex;
    if (selectedLayer)
    {
        selectedTileIndex = tileX + tileY * room->GetWidth();
    } else {
        selectedTileIndex = tileX + tileY * room->GetLayer0Width();
    }
    unsigned short placedTile = layer->GetLayerData()[selectedTileIndex];
    singleton->GetTile16DockWidgetPtr()->SetSelectedTile(placedTile, true);
}

/// <summary>
/// this function will be called when the graphic view in the main window is clicked then mouse release.
/// </summary>
/// <param name="event">
/// The mouse click event.
/// </param>
void MainGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    holdingmouse = false;

    // Get the ID of the tile where the mouse was released
    uint scalerate = singleton->GetGraphicViewScalerate();
    unsigned int X = (unsigned int) event->x() + horizontalScrollBar()->sliderPosition();
    unsigned int Y = (unsigned int) event->y() + verticalScrollBar()->sliderPosition();
    unsigned int tileX = X / (16 * scalerate);
    unsigned int tileY = Y / (16 * scalerate);

    enum Ui::EditMode editMode = singleton->GetEditModeWidgetPtr()->GetEditModeParams().editMode;

    //Temporary Remove because of a bug
    //Add a move operation for door (for CTRL-Z)
    if (editMode == Ui::LayerEditMode) {
        if(!has_a_rect)
        {
            return;
        }

        if(!Isdraggingrect)
        {
            // Update layer rect graphic
            int selectedLayer = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer;
            QPixmap layerrectpixmap(singleton->GetCurrentRoom()->GetLayerPixmap(selectedLayer,
                                                                                rectselectstartTileX,
                                                                                rectselectstartTileY,
                                                                                rectwidth,
                                                                                rectheight));
            if(selectedrectgraphic == nullptr)
            {
                selectedrectgraphic = singleton->Getgraphicview()->scene()->addPixmap(layerrectpixmap);
            } else {
                selectedrectgraphic->setPixmap(layerrectpixmap);
            }
            selectedrectgraphic->setPos(rectx * 16, recty * 16);
            selectedrectgraphic->setZValue(13);
            selectedrectgraphic->setVisible(true);
            // Update rectdata
            unsigned short *Layerdata = singleton->GetCurrentRoom()->GetLayer(selectedLayer)->GetLayerData();
            int layerwidth = singleton->GetCurrentRoom()->GetLayer(selectedLayer)->GetLayerWidth();
            for (int j = rectselectstartTileY; j < (rectselectstartTileY + rectheight); ++j)
            {
                for (int i = rectselectstartTileX; i < (rectselectstartTileX + rectwidth); ++i)
                {
                    rectdata.push_back(Layerdata[i + j * layerwidth]);
                }
            }
        } else {
            dragInitmouseX = dragInitmouseY = -1;
            tmpLTcornerTileX = rectx; tmpLTcornerTileY = recty;
            Isdraggingrect = false;
        }
    } else if (editMode == Ui::DoorEditMode) { // Add a move operation for entity (for CTRL-Z)
        if (holdingEntityOrDoor && SelectedDoorID != -1) {
            struct OperationParams *params = new struct OperationParams();
            params->type = ObjectMoveOperation;
            params->objectPositionChange = true;
            auto curDoor = singleton->GetCurrentLevel()->GetDoorListRef().GetDoor(singleton->GetCurrentRoom()->GetRoomID(), SelectedDoorID);
            params->objectMoveParams = ObjectMoveParams::Create(objectInitialX, objectInitialY, curDoor.x1, curDoor.y1, ObjectMoveParams::DOOR_TYPE, SelectedDoorID);

            // TODO: Only perform and not execute because not support undo redo yet
            PerformOperation(params);
        }
    } else if (editMode == Ui::EntityEditMode) { // Add a move operation for entity (for CTRL-Z)
        if (holdingEntityOrDoor && SelectedEntityID != -1) {
            struct OperationParams *params = new struct OperationParams();
            params->type = ObjectMoveOperation;
            params->objectPositionChange = true;
            params->objectMoveParams = ObjectMoveParams::Create(objectInitialX, objectInitialY, tileX, tileY, ObjectMoveParams::ENTITY_TYPE, SelectedEntityID);

            // TODO: Only perform and not execute because not support undo redo yet
            PerformOperation(params);
        }
    }
    // We are no longer drawing or holding an object
    drawingTileX = -1;
    drawingTileY = -1;
    holdingEntityOrDoor = false;
}




/// <summary>
/// this function will be called when key-press happens.
/// </summary>
/// <param name="event">
/// The key-press event.
/// </param>
void MainGraphicsView::keyPressEvent(QKeyEvent *event)
{
    // If an entity is selected
    enum Ui::EditMode editMode = singleton->GetEditModeWidgetPtr()->GetEditModeParams().editMode;
    LevelComponents::Room *currentRoom = singleton->GetCurrentRoom();

    switch(editMode)
    {
    case(Ui::EntityEditMode):
    {
        if(SelectedEntityID == -1) return;
        switch (event->key())
        {
        // Delete selected entity if BSP or DEL is pressed
        case Qt::Key_Backspace:
        case Qt::Key_Delete:
        {
            singleton->DeleteEntity(SelectedEntityID);
            SelectedEntityID = -1;
            singleton->RenderScreenElementsLayersUpdate(0xFFFFFFFFu, -1);
            int difficulty = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedDifficulty;
            currentRoom->SetEntityListDirty(difficulty, true);
            singleton->SetUnsavedChanges(true);
        }; break;

        // Move selected entity when a direction key is pressed
        case Qt::Key_Right:
        case Qt::Key_Left:
        case Qt::Key_Up:
        case Qt::Key_Down:
        {
            // The new positions
            int pX = currentRoom->GetEntityX(SelectedEntityID);
            int pY = currentRoom->GetEntityY(SelectedEntityID);

            // The old positions
            int oldX=pX;
            int oldY=pY;

            if (event->key() == Qt::Key_Left)
            {
                pX = pX - 1;
            }
            else if (event->key() == Qt::Key_Right)
            {
                pX = pX + 1;
            }
            else if (event->key() == Qt::Key_Up)
            {
                pY = pY - 1;
            }
            else // Qt::Key_Down
            {
                pY = pY + 1;
            }

            if (currentRoom->IsNewEntityPositionInsideRoom(pX, pY))
            {
                struct OperationParams *params = new struct OperationParams();
                params->type = ObjectMoveOperation;
                params->objectPositionChange = true;
                params->objectMoveParams=ObjectMoveParams::Create(oldX, oldY, pX, pY,ObjectMoveParams::ENTITY_TYPE,SelectedEntityID);

                // Only perform and not execute because of a bug after deletion and undo
                PerformOperation(params);
            }
        }; break;
        }
    }; break;
    case(Ui::DoorEditMode):
    {
        if(SelectedDoorID == -1) return;
        switch (event->key())
        {
        // Delete selected door if BSP or DEL is pressed
        case Qt::Key_Backspace:
        case Qt::Key_Delete:
        {
            if (singleton->DeleteDoor(singleton->GetCurrentLevel()->GetDoorListRef().GetGlobalIDByLocalID(currentRoom->GetRoomID(), SelectedDoorID)))
            {
                SelectedDoorID = -1;
                singleton->RenderScreenElementsLayersUpdate(0xFFFFFFFFu, -1);
                singleton->ResetEntitySetDockWidget();
                singleton->SetUnsavedChanges(true);
            }
        }; break;
        // Move selected door when a direction key is pressed
        case Qt::Key_Right:
        case Qt::Key_Left:
        case Qt::Key_Up:
        case Qt::Key_Down:
        {
            auto curDoor = singleton->GetCurrentLevel()->GetDoorListRef().GetDoor(currentRoom->GetRoomID(), SelectedDoorID);

            // The new positions
            int pX1 = curDoor.x1;
            int pY1 = curDoor.y1;
            int pX2 = curDoor.x2;
            int pY2 = curDoor.y2;

            // Old positions (x1 and y1)
            int oldX=pX1;
            int oldY=pY1;

            if (event->key() == Qt::Key_Left)
            {
                pX1 = pX1 - 1;
                pX2 = pX2 - 1;
            }
            else if (event->key() == Qt::Key_Right)
            {
                pX1 = pX1 + 1;
                pX2 = pX2 + 1;
            }
            else if (event->key() == Qt::Key_Up)
            {
                pY1 = pY1 - 1;
                pY2 = pY2 - 1;
            }
            else // Qt::Key_Down
            {
                pY1 = pY1 + 1;
                pY2 = pY2 + 1;
            }

            if (currentRoom->IsNewDoorPositionInsideRoom(pX1, pX2, pY1, pY2))
            {
                struct OperationParams *params = new struct OperationParams();
                params->type = ObjectMoveOperation;
                params->objectPositionChange = true;
                params->objectMoveParams = ObjectMoveParams::Create(oldX, oldY, pX1, pY1, ObjectMoveParams::DOOR_TYPE,SelectedDoorID);

                // Only perform and not execute because of a bug after deletion and undo
                PerformOperation(params);
            }
        }; break;
        }
    }; break;
    case(Ui::LayerEditMode):
    {
        if (!rectSelectMode) {
            // Reset selected tiles
            unsigned short tile = singleton->GetTile16DockWidgetPtr()->GetSelectedTile();
            if (event->key() == Qt::Key_Left)
            {
                singleton->GetTile16DockWidgetPtr()->SetSelectedTile((tile > 0) ? tile - 1: tile, true);
            }
            else if (event->key() == Qt::Key_Right)
            {
                singleton->GetTile16DockWidgetPtr()->SetSelectedTile(qMin(tile + 1, 0x2FF), true);
            }
            else if (event->key() == Qt::Key_Up)
            {
                singleton->GetTile16DockWidgetPtr()->SetSelectedTile((tile > 7) ? tile - 8: tile, true);
            }
            else if (event->key() == Qt::Key_Down)
            {
                if(tile <= (0x2FF - 8))
                {
                    singleton->GetTile16DockWidgetPtr()->SetSelectedTile(tile + 8, true);
                }
            }
        } else {
            if (has_a_rect) {
                if(event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)
                {
                    if (rectselectstartTileX == rectx)
                    {
                        int selectedLayer = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer;
                        struct OperationParams *params = new struct OperationParams();
                        params->type = ChangeTileOperation;
                        params->tileChange = true;
                        for(int j = 0; j < rectheight; ++j) // delete rect
                        {
                            for(int i = 0; i < rectwidth; ++i)
                            {
                                params->tileChangeParams.push_back(
                                            TileChangeParams::Create(rectselectstartTileX + i, rectselectstartTileY + j, selectedLayer,
                                                                     0,
                                                                     rectdata[i + j * rectwidth]));
                            }
                        }
                        ExecuteOperation(params);
                    }
                    // Reset variables
                    ResetRectPixmaps();
                    ResetRect();
                    return;
                }
            }
        }
    }; break;
    default:;
    }
}

/// <summary>
/// This function will deselect doors and entities.
/// </summary>
void MainGraphicsView::DeselectDoorAndEntity(bool updateRenderArea)
{
    SelectedDoorID = -1;
    SelectedEntityID = -1;
    if(!updateRenderArea)
        return;
    singleton->RenderScreenElementsLayersUpdate(0xFFFFFFFFu, -1);
}

void MainGraphicsView::SetRectSelectMode(bool state)
{
    rectSelectMode = state;
    singleton->RefreshRectSelectHint(rectSelectMode);
    ResetRectPixmaps();
    ResetRect();
}


/// <summary>
/// This function will reset rectangle selection pixmaps variables if needed
/// </summary>
void MainGraphicsView::ResetRectPixmaps() {
    if(rect != nullptr)
    {
        delete rect;
        rect = nullptr;
    }
    if(selectedrectgraphic != nullptr)
    {
        delete selectedrectgraphic;
        selectedrectgraphic = nullptr;
    }
}

/// <summary>
/// This function will reset rectangle selection variables (but it doesn't reset the rect graphics)
/// </summary>
void MainGraphicsView::ResetRect() {
    Isdraggingrect=false;
    rectx = recty = -1;
    tmpLTcornerTileX = tmpLTcornerTileY = rectselectstartTileX = rectselectstartTileY = -1;
    rectwidth = rectheight = 0;
    has_a_rect = false;
    dragInitmouseX = dragInitmouseY = -1;
    holdingmouse = false;
    rectdata.clear();
}
