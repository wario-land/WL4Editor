#include "Operation.h"
#include "WL4EditorWindow.h"

#include <deque>

extern WL4EditorWindow *singleton;

// Globals used by the undo system
static std::deque<struct OperationParams *> operationHistory[16];
static unsigned int operationIndex[16]; // For room-specific changes
static std::deque<struct OperationParams *> operationHistoryGlobal;
static unsigned int operationIndexGlobal; // For level-wide changes

static unsigned int CurrentTilesetOperationId = 0;
static unsigned int CurrentSpritestuffOperationId = 0;

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
        singleton->ResetCameraControlDockWidget();
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
                    int difficulty = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedDifficulty;
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

        if (singleton->GetCurrentRoom()->GetTilesetID() == tilesetId)
        {
            singleton->GetTile16DockWidgetPtr()->SetTileset(tilesetId);
            singleton->RenderScreenFull();
        }
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

        singleton->ResetEntitySetDockWidget();
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

        // hint to show undo operation
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo tile changes."));
    }
    if (operation->roomConfigChange)
    {
        // new to last
        singleton->RoomConfigReset(operation->newRoomConfigParams, operation->lastRoomConfigParams);
        singleton->RenderScreenFull();
        singleton->SetEditModeDockWidgetLayerEditability();
        singleton->SetEditModeWidgetDifficultyRadioBox(1);
        singleton->ResetEntitySetDockWidget();

        // hint to show undo operation
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Room Config changes."));
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
                    int difficulty = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedDifficulty;
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
        CurrentTilesetOperationId = operationIndexGlobal;

        // hint to show undo operation
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Tileset changes."));
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
        CurrentSpritestuffOperationId = operationIndexGlobal;

        // hint to show undo operation
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Sprites and Spritesets changes."));
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
    else
    {
        // hint to show undo operation
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("No more operation to undo."));
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
    else
    {
        // hint to show redo operation
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("No more operation to redo."));
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

    // Re-initialize all the operation indexes to zero
    memset(operationIndex, 0, sizeof(operationIndex));
}

/// <summary>
/// Clean up the global undo deque.
/// </summary>
/// <remarks>
/// Only call this when deconstruct the editor, don't use it elsewhere.
/// </remarks>
void DeleteUndoHistoryGlobal()
{
    // It is different from deleting OperationParams, so cannot use the deconstructor
    // Deconstruct the global history
    for (unsigned int j = 0; j < operationHistoryGlobal.size(); ++j) // from old to new
    {
        if (operationHistoryGlobal[j]->TilesetChange)
        {
            if (CurrentTilesetOperationId > 0)
            {
                delete operationHistoryGlobal[j]; // call default deconstructor
                CurrentTilesetOperationId--;
            }
            else
            {
                if (operationHistoryGlobal[j]->newTilesetEditParams)
                {
                    delete operationHistoryGlobal[j]->newTilesetEditParams;
                }
                if (operationHistoryGlobal[j]->lastTilesetEditParams)
                {
                    delete operationHistoryGlobal[j]->lastTilesetEditParams->newTileset;
                    operationHistoryGlobal[j]->lastTilesetEditParams->newTileset = nullptr;
                    delete operationHistoryGlobal[j]->lastTilesetEditParams;
                }
            }
        }
        else if (operationHistoryGlobal[j]->SpritesSpritesetChange)
        {
            if (CurrentSpritestuffOperationId > 0)
            {
                delete operationHistoryGlobal[j]; // call default deconstructor
                CurrentSpritestuffOperationId--;
            }
            else
            {
                if (operationHistoryGlobal[j]->newSpritesAndSetParam)
                {
                    delete operationHistoryGlobal[j]->newSpritesAndSetParam;
                }
                if (operationHistoryGlobal[j]->lastSpritesAndSetParam)
                {
                    qDeleteAll(operationHistoryGlobal[j]->lastSpritesAndSetParam->entities);
                    qDeleteAll(operationHistoryGlobal[j]->lastSpritesAndSetParam->entitySets);
                    delete operationHistoryGlobal[j]->lastSpritesAndSetParam;
                }
            }
        }
    }
    operationHistoryGlobal.clear();
    operationIndexGlobal = 0;
}

/// <summary>
/// Reset all the changes bools can be found through the whole history
/// </summary>
/// <remarks>
/// Sometimes the history saved the vanilla elements' data and we make some changes to it and save changes
/// now the current changed bool of the element is false and the first corresponding element in the history is false too
/// all the others corresponding elements' changed bools are true
/// usual undo and redo won't cause problems, but on the second or the third save
/// when the current element are marked unchanged, the save process cannot detect it.
/// So we have to always make it that only the current element changed bool to be false, and all the others be true.
/// call this function every time when finishing saving level
/// </remarks>
void ResetChangedBoolsThroughHistory()
{
    for (unsigned int j = 0; j < operationHistoryGlobal.size(); ++j) // from old to new
    {
        // no need to exclude the current elements, they will be set changed bool to false in the SaveLevel function
        // so call this function before that part
        if (operationHistoryGlobal[j]->TilesetChange)
        {
            operationHistoryGlobal[j]->newTilesetEditParams->newTileset->SetChanged(true);
            operationHistoryGlobal[j]->lastTilesetEditParams->newTileset->SetChanged(true);
        }
        else if (operationHistoryGlobal[j]->SpritesSpritesetChange)
        {
            if (operationHistoryGlobal[j]->newSpritesAndSetParam)
            {
                for (LevelComponents::Entity *entityIter: operationHistoryGlobal[j]->newSpritesAndSetParam->entities)
                {
                    entityIter->SetChanged(true);
                }
                for (LevelComponents::EntitySet *entitySetIter: operationHistoryGlobal[j]->newSpritesAndSetParam->entitySets)
                {
                    entitySetIter->SetChanged(true);
                }
            }
            if (operationHistoryGlobal[j]->lastSpritesAndSetParam)
            {
                for (LevelComponents::Entity *entityIter: operationHistoryGlobal[j]->lastSpritesAndSetParam->entities)
                {
                    entityIter->SetChanged(true);
                }
                for (LevelComponents::EntitySet *entitySetIter: operationHistoryGlobal[j]->lastSpritesAndSetParam->entitySets)
                {
                    entitySetIter->SetChanged(true);
                }
            }
        }
    }
}

/// <summary>
/// Reset all the global elements operation indexes
/// </summary>
/// <remarks>
/// the DeleteUndoHistoryGlobal() function won't set global element indexes back to 0,
/// by using this function, we can reset some global element operation indexes back to 0 when we need to load a new ROM
/// </remarks>
void ResetGlobalElementOperationIndexes()
{
    CurrentTilesetOperationId = 0;
    CurrentSpritestuffOperationId = 0;
}
