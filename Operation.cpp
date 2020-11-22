#include "Operation.h"
#include "WL4EditorWindow.h"

#include <deque>

extern WL4EditorWindow *singleton;

// Globals used by the undo system
static std::deque<struct OperationParams *> operationHistory[16];
static unsigned int operationIndex[16]; // For room-specific changes
static std::deque<struct OperationParams *> operationHistoryGlobal;
static unsigned int operationIndexGlobal; // For level-wide changes

/// <summary>
/// Perform an operation based on its parameters.
/// </summary>
/// <remarks>
/// This function does not take into consideration the undo deque. It only performs an operation.
/// </remarks>
/// <param name="operation">
/// The operation to perform.
/// </param>
void PerformOperation(struct OperationParams *operation)
{
    LevelComponents::Room *room;
    if (operation->tileChange)
    {
        room = singleton->GetCurrentRoom();
        int tl1 = -1, tl2 = -1; // there are 2 target layers only when doing cross layer rect-copy
        QVector<LevelComponents::Tileinfo> tilechangelist, tilechangelist2;
        for (auto iter = operation->tileChangeParams.begin(); iter != operation->tileChangeParams.end(); ++iter)
        {
            struct TileChangeParams *tcp = *iter;
            LevelComponents::Layer *layer = room->GetLayer(tcp->targetLayer);
            unsigned int index;
            if (!tcp->targetLayer) // i.e. targetLayer is Layer 0
            {
                index = tcp->tileX + tcp->tileY * room->GetLayer0Width();
            }
            else
            {
                index = tcp->tileX + tcp->tileY * room->GetWidth();
            }
            layer->GetLayerData()[index] = tcp->newTile;

            if(tl1 == -1)
            {
                tl1 = tcp->targetLayer;
            }
            if(tl1 != -1 && tl2 == -1 && tl1 != tcp->targetLayer)
            {
                tl2 = tcp->targetLayer;
            }
            struct LevelComponents::Tileinfo tinfo;
            tinfo.tileX = tcp->tileX;
            tinfo.tileY = tcp->tileY;
            tinfo.tileID = tcp->newTile;
            if(tl1 == tcp->targetLayer)
            {
                tilechangelist.push_back(tinfo);
                continue;
            }
            if(tl2 == tcp->targetLayer)
            {
                tilechangelist2.push_back(tinfo);
                continue;
            }
        }
        // Update graphic changes
        singleton->RenderScreenTilesChange(tilechangelist, tl1);
        if(tl2 == -1) return;
        singleton->RenderScreenTilesChange(tilechangelist2, tl2);
    }
    if (operation->roomConfigChange)
    {
        // change the width and height for all layers
        singleton->RoomConfigReset(operation->lastRoomConfigParams, operation->newRoomConfigParams);
        singleton->RenderScreenFull();
        singleton->SetEditModeDockWidgetLayerEditability();
        singleton->SetEditModeWidgetDifficultyRadioBox(1);
        singleton->ResetEntitySetDockWidget();
        singleton->SetUnsavedChanges(true);
    }
    if (operation->objectPositionChange)
    {
        struct ObjectMoveParams *om=operation->objectMoveParams;
        /*om->previousPositionX = pX;
        om->previousPositionY = pY;*/
        if (om->type == ObjectMoveParams::DOOR_TYPE)
        {
            LevelComponents::Room *currentRoom = singleton->GetCurrentRoom();
            LevelComponents::Door *selectedDoor = currentRoom->GetDoor(om->objectID);

            // Calculating the deltas
            int px1 = selectedDoor->GetX1();
            int py1 = selectedDoor->GetY1();
            int deltaX = selectedDoor->GetX2()-px1;
            int deltaY = selectedDoor->GetY2()-py1;

            // If the door exists and if it is still in the room
            if (om->objectID != -1 && selectedDoor)
            {
                if (currentRoom->IsNewDoorPositionInsideRoom(om->nextPositionX, om->nextPositionX+deltaX, om->nextPositionY, om->nextPositionY + deltaY))
                {
                    selectedDoor->SetDoorPlace(om->nextPositionX, om->nextPositionX+deltaX, om->nextPositionY, om->nextPositionY + deltaY);
                    singleton->RenderScreenElementsLayersUpdate((unsigned int) om->objectID, -1);
                }
            }
        }
        else if (om->type == ObjectMoveParams::ENTITY_TYPE)
        {
            LevelComponents::Room *currentRoom = singleton->GetCurrentRoom();

            // If the entity exists
            if (om->objectID != -1)
            {
                if (currentRoom->IsNewEntityPositionInsideRoom(om->nextPositionX, om->nextPositionY))
                {
                    currentRoom->SetEntityPosition(om->nextPositionX, om->nextPositionY, om->objectID);
                    singleton->RenderScreenElementsLayersUpdate(0xFFFFFFFFu, om->objectID);
                    int difficulty = singleton->GetEditModeWidgetPtr()->GetEditModeParams().seleteddifficulty;
                    singleton->GetCurrentRoom()->SetEntityListDirty(difficulty, true);
                    singleton->SetUnsavedChanges(true);
                }
            }
        }
    }
    if (operation->TilesetChange)
    {
        // Update Rooms's Tileset in CurrentLevel
        int roomnum = singleton->GetCurrentLevel()->GetRooms().size();
        int tilesetId = operation->newTilesetEditParams->currentTilesetIndex;
        ROMUtils::singletonTilesets[tilesetId] = operation->newTilesetEditParams->newTileset;
        for(int i = 0; i < roomnum; ++i)
        {
            if(singleton->GetCurrentLevel()->GetRooms()[i]->GetTilesetID() == tilesetId)
            {
                singleton->GetCurrentLevel()->GetRooms()[i]->SetTileset(operation->newTilesetEditParams->newTileset, tilesetId);
            }
        }

        singleton->GetTile16DockWidgetPtr()->SetTileset(tilesetId);
        singleton->RenderScreenFull();
        singleton->SetUnsavedChanges(true);
    }
    if (operation->SpritesSpritesetChange)
    {
        // Update new Entities and Entitysets to global singeltons
        for (LevelComponents::Entity *entityIter: operation->newSpritesAndSetParam->entities)
        {
            ROMUtils::entities[entityIter->GetEntityGlobalID()] = entityIter;
        }
        for (LevelComponents::EntitySet *entitySetIter: operation->newSpritesAndSetParam->entitySets)
        {
            ROMUtils::entitiessets[entitySetIter->GetEntitySetId()] = entitySetIter;
        }

        // Update Rooms's Entities and Entitysets in CurrentLevel
        int roomnum = singleton->GetCurrentLevel()->GetRooms().size();
        for(int i = 0; i < roomnum; ++i)
        {
            LevelComponents::Room *curRoom = singleton->GetCurrentLevel()->GetRooms()[i];
            curRoom->SetCurrentEntitySet(curRoom->GetCurrentEntitySetID());
        }

        singleton->GetEntitySetDockWidgetPtr()->ResetEntitySet(singleton->GetCurrentRoom());
        singleton->RenderScreenFull();
        singleton->SetUnsavedChanges(true);
    }
}

/// <summary>
/// Backtrack an operation based on its parameters.
/// </summary>
/// <remarks>
/// This function does not take into consideration the undo deque. It only resets the effect of an operation.
/// </remarks>
/// <param name="operation">
/// The operation to backtrack.
/// </param>
void BackTrackOperation(struct OperationParams *operation)
{
    LevelComponents::Room *room;
    if (operation->tileChange)
    {
        room = singleton->GetCurrentRoom();
        int tl1 = -1, tl2 = -1; // there are 2 target layers only when doing cross layer rect-copy
        QVector<LevelComponents::Tileinfo> tilechangelist, tilechangelist2;
        for (auto iter = operation->tileChangeParams.begin(); iter != operation->tileChangeParams.end(); ++iter)
        {
            struct TileChangeParams *tcp = *iter;
            LevelComponents::Layer *layer = room->GetLayer(tcp->targetLayer);
            unsigned int index;
            if (!tcp->targetLayer) // i.e. targetLayer is Layer 0
            {
                index = tcp->tileX + tcp->tileY * room->GetLayer0Width();
            }
            else
            {
                index = tcp->tileX + tcp->tileY * room->GetWidth();
            }
            layer->GetLayerData()[index] = tcp->oldTile;

            if(tl1 == -1)
            {
                tl1 = tcp->targetLayer;
            }
            if(tl1 != -1 && tl2 == -1 && tl1 != tcp->targetLayer)
            {
                tl2 = tcp->targetLayer;
            }
            struct LevelComponents::Tileinfo tinfo;
            tinfo.tileX = tcp->tileX;
            tinfo.tileY = tcp->tileY;
            tinfo.tileID = tcp->oldTile;
            if(tl1 == tcp->targetLayer)
            {
                tilechangelist.push_back(tinfo);
                continue;
            }
            if(tl2 == tcp->targetLayer)
            {
                tilechangelist2.push_back(tinfo);
                continue;
            }
        }
        // Update graphic changes
        singleton->RenderScreenTilesChange(tilechangelist, tl1);
        if(tl2 == -1) return;
        singleton->RenderScreenTilesChange(tilechangelist2, tl2);
    }
    if (operation->roomConfigChange)
    {
        // new to last
        singleton->RoomConfigReset(operation->newRoomConfigParams, operation->lastRoomConfigParams);
        singleton->RenderScreenFull();
        singleton->SetEditModeDockWidgetLayerEditability();
        singleton->SetEditModeWidgetDifficultyRadioBox(1);
        singleton->ResetEntitySetDockWidget();
    }
    if (operation->objectPositionChange)
    {
        struct ObjectMoveParams *om=operation->objectMoveParams;
        /*om->previousPositionX = pX;
        om->previousPositionY = pY;*/
        if (om->type == ObjectMoveParams::DOOR_TYPE)
        {
            LevelComponents::Room *currentRoom = singleton->GetCurrentRoom();
            LevelComponents::Door *selectedDoor = currentRoom->GetDoor(om->objectID);

            // Calculating the deltas
            int px1 = selectedDoor->GetX1();
            int py1 = selectedDoor->GetY1();
            int deltaX = selectedDoor->GetX2()-px1;
            int deltaY = selectedDoor->GetY2()-py1;

            // If the door exists and if it is still in the room
            if (om->objectID != -1)
            {
                if (currentRoom->IsNewDoorPositionInsideRoom(om->previousPositionX, om->previousPositionX+deltaX, om->previousPositionY, om->previousPositionY + deltaY))
                {
                    selectedDoor->SetDoorPlace(om->previousPositionX, om->previousPositionX+deltaX, om->previousPositionY, om->previousPositionY + deltaY);
                    singleton->RenderScreenElementsLayersUpdate((unsigned int) om->objectID, -1);
                }
            }
        } else if (om->type == ObjectMoveParams::ENTITY_TYPE)
        {
            LevelComponents::Room *currentRoom = singleton->GetCurrentRoom();

            // If the entity exists and if it is still in the room
            if (om->objectID != -1)
            {
                if (currentRoom->IsNewEntityPositionInsideRoom(om->previousPositionX, om->previousPositionY))
                {
                    currentRoom->SetEntityPosition(om->previousPositionX, om->previousPositionY, om->objectID);
                    singleton->RenderScreenElementsLayersUpdate(0xFFFFFFFFu, om->objectID);
                    int difficulty = singleton->GetEditModeWidgetPtr()->GetEditModeParams().seleteddifficulty;
                    singleton->GetCurrentRoom()->SetEntityListDirty(difficulty, true);
                }
            }
        }
    }
    if (operation->TilesetChange)
    {
        // Update Rooms's Tileset in CurrentLevel
        int roomnum = singleton->GetCurrentLevel()->GetRooms().size();
        int tilesetId = operation->lastTilesetEditParams->currentTilesetIndex;
        ROMUtils::singletonTilesets[tilesetId] = operation->lastTilesetEditParams->newTileset;
        for(int i = 0; i < roomnum; ++i)
        {
            if(singleton->GetCurrentLevel()->GetRooms()[i]->GetTilesetID() == tilesetId)
            {
                singleton->GetCurrentLevel()->GetRooms()[i]->SetTileset(operation->lastTilesetEditParams->newTileset, tilesetId);
            }
        }

        singleton->GetTile16DockWidgetPtr()->SetTileset(tilesetId);
        singleton->RenderScreenFull();
    }
    if (operation->SpritesSpritesetChange)
    {
        // Update old Entities and Entitysets to global singeltons
        for (LevelComponents::Entity *entityIter: operation->lastSpritesAndSetParam->entities)
        {
            ROMUtils::entities[entityIter->GetEntityGlobalID()] = entityIter;
        }
        for (LevelComponents::EntitySet *entitySetIter: operation->lastSpritesAndSetParam->entitySets)
        {
            ROMUtils::entitiessets[entitySetIter->GetEntitySetId()] = entitySetIter;
        }

        // Update Rooms's Entities and Entitysets in CurrentLevel
        int roomnum = singleton->GetCurrentLevel()->GetRooms().size();
        for(int i = 0; i < roomnum; ++i)
        {
            LevelComponents::Room *curRoom = singleton->GetCurrentLevel()->GetRooms()[i];
            curRoom->SetCurrentEntitySet(curRoom->GetCurrentEntitySetID());
        }

        singleton->GetEntitySetDockWidgetPtr()->ResetEntitySet(singleton->GetCurrentRoom());
        singleton->RenderScreenFull();
        singleton->SetUnsavedChanges(true);
    }
}

/// <summary>
/// Perform an operation based on its parameters, and add it to the undo deque.
/// </summary>
/// <param name="operation">
/// The operation to perform.
/// </param>
/// <param name="operationHist">
/// The the history deque from which to execute an operation.
/// </param>
/// <param name="operationIdx">
/// The operation indexer to modify.
/// </param>
static void ExecuteOperationImpl(struct OperationParams *operation, std::deque<struct OperationParams *> &operationHist, unsigned int *operationIdx)
{
    PerformOperation(operation);
    // If we perform an action after a series of undo, then delete the "undone" operations from history
    while (*operationIdx)
    {
        // Delete the front operation in the queue while decrementing the operation index until the index reaches 0
        --(*operationIdx);
        struct OperationParams *frontOP = operationHist[0];
        delete frontOP;
        operationHist.pop_front();
    }
    operationHist.push_front(operation);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Perform an operation based on its parameters, and add it to the undo deque.
/// This is for performing an operation within a Room.
/// </summary>
/// <param name="operation">
/// The operation to perform.
/// </param>
void ExecuteOperation(struct OperationParams *operation)
{
    int currentRoomNumber = singleton->GetCurrentRoom()->GetRoomID();
    ExecuteOperationImpl(operation, operationHistory[currentRoomNumber], operationIndex + currentRoomNumber);
}

/// <summary>
/// Perform an operation based on its parameters, and add it to the undo deque.
/// This is for performing a global operation.
/// </summary>
/// <param name="operation">
/// The operation to perform.
/// </param>
void ExecuteOperationGlobal(struct OperationParams *operation)
{
    ExecuteOperationImpl(operation, operationHistoryGlobal, &operationIndexGlobal);
}

/// <summary>
/// Undo a previously performed operation in the undo deque.
/// </summary>
/// <remarks>
/// This function does not remove the operation from the deque.
/// Instead, an index is used within the deque to track which operation should be undone next.
/// That way, an operation can be undone and redone multiple times.
/// </remarks>
/// </param>
/// <param name="operationHist">
/// The the history deque from which to undo an operation.
/// </param>
/// <param name="operationIdx">
/// The operation indexer to modify.
/// </param>
static void UndoOperationImpl(std::deque<struct OperationParams *> &operationHist, unsigned int *operationIdx)
{
    // We cannot undo past the end of the deque
    if (*operationIdx < operationHist.size())
    {
        BackTrackOperation(operationHist[(*operationIdx)++]);

        // If the entire operation history is undone for all rooms, then there are no unsaved changes
        for (unsigned int i = 0; i < sizeof(operationIndex) / sizeof(operationIndex[0]); ++i)
        {
            if (*operationIdx != operationHist.size())
            {
                return;
            }
        }
        // TODO uncomment this once all operations that change the level go through Operation.cpp
        // singleton->SetUnsavedChanges(false);
    }
}

/// <summary>
/// Undo a previously performed operation in the undo deque.
/// This is for undoing an operation within a Room.
/// </summary>
/// <remarks>
/// This function does not remove the operation from the deque.
/// Instead, an index is used within the deque to track which operation should be undone next.
/// That way, an operation can be undone and redone multiple times.
/// </remarks>
void UndoOperation()
{
    int currentRoomNumber = singleton->GetCurrentRoom()->GetRoomID();
    UndoOperationImpl(operationHistory[currentRoomNumber], operationIndex + currentRoomNumber);
}

/// <summary>
/// Undo a previously performed operation in the undo deque.
/// This is for undoing a global operation.
/// </summary>
/// <remarks>
/// This function does not remove the operation from the deque.
/// Instead, an index is used within the deque to track which operation should be undone next.
/// That way, an operation can be undone and redone multiple times.
/// </remarks>
void UndoOperationGlobal()
{
    UndoOperationImpl(operationHistoryGlobal, &operationIndexGlobal);
}

/// <summary>
/// Redo a previously undone operation from the undo deque.
/// </summary>
/// <remarks>
/// This function does not add the operation to the deque.
/// Instead, an index is used within the deque to track which operation should be redone next.
/// That way, an operation can be undone and redone multiple times.
/// </remarks>
/// <param name="operationHist">
/// The the history deque from which to undo an operation.
/// </param>
/// <param name="operationIdx">
/// The operation indexer to modify.
/// </param>
static void RedoOperationImpl(std::deque<struct OperationParams *> &operationHist, unsigned int *operationIdx)
{
    // We cannot redo past the front of the deque
    if (*operationIdx)
    {
        PerformOperation(operationHist[--(*operationIdx)]);

        // Performing a "redo" will make unsaved changes
        singleton->SetUnsavedChanges(true);
    }
}

/// <summary>
/// Redo a previously undone operation from the undo deque.
/// This is for redoing an operation within a Room.
/// </summary>
/// <remarks>
/// This function does not add the operation to the deque.
/// Instead, an index is used within the deque to track which operation should be redone next.
/// That way, an operation can be undone and redone multiple times.
/// </remarks>
void RedoOperation()
{
    int currentRoomNumber = singleton->GetCurrentRoom()->GetRoomID();
    RedoOperationImpl(operationHistory[currentRoomNumber], operationIndex + currentRoomNumber);
}

/// <summary>
/// Redo a previously undone operation from the undo deque.
/// This is for redoing a global operation.
/// </summary>
/// <remarks>
/// This function does not add the operation to the deque.
/// Instead, an index is used within the deque to track which operation should be redone next.
/// That way, an operation can be undone and redone multiple times.
/// </remarks>
void RedoOperationGlobal()
{
    RedoOperationImpl(operationHistoryGlobal, &operationIndexGlobal);
}

/// <summary>
/// Reset the undo deque.
/// </summary>
/// <remarks>
/// This is necessary to ensure that undo history does not persist between multiple levels.
/// </remarks>
void ResetUndoHistory()
{
    for (unsigned int i = 0; i < sizeof(operationHistory) / sizeof(operationHistory[0]); ++i)
    {
        // Deconstruct the dynamically allocated operation structs within the history queue
        for (unsigned int j = 0; j < operationHistory[i].size(); ++j)
        {
            delete operationHistory[i][j];
        }
        operationHistory[i].clear();
    }

    // Deconstruct the global history
    for (unsigned int j = 0; j < operationHistoryGlobal.size(); ++j)
    {
        delete operationHistoryGlobal[j];
    }
    operationHistoryGlobal.clear();

    // Re-initialize all the operation indexes to zero
    memset(operationIndex, 0, sizeof(operationIndex));
    operationIndexGlobal = 0;
}
