#include "Operation.h"
#include "WL4EditorWindow.h"

#include <deque>

extern WL4EditorWindow *singleton;

// Globals used by the undo system
static std::deque<struct OperationParams*> operationHistory[16];
static unsigned int operationIndex[16];

/// <summary>
/// Perform an operation based on its parameters.
/// </summary>
/// <remarks>
/// This function does not take into consideration the undo deque. It only performs an operation.
/// </remarks>
/// <param name="operation">
/// The operation to perform.
/// </param>
static void PerformOperation(struct OperationParams *operation)
{
    switch(operation->type)
    {
    case ChangeTileOperation:
        LevelComponents::Room *room = singleton->GetCurrentRoom();
        for(auto iter = operation->tileChangeParams.begin(); iter != operation->tileChangeParams.end(); ++iter)
        {
            struct TileChangeParams *tcp = *iter;
            LevelComponents::Layer *layer = room->GetLayer(tcp->targetLayer);
            int index = tcp->tileX + tcp->tileY * room->GetWidth();
            layer->GetLayerData()[index] = tcp->newTile;
            // Re-render the tile
            singleton->RenderScreenTileChange(tcp->tileX, tcp->tileY, tcp->newTile, tcp->targetLayer);
        }
        break;
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
static void BackTrackOperation(struct OperationParams *operation)
{
    switch(operation->type)
    {
    case ChangeTileOperation:
        LevelComponents::Room *room = singleton->GetCurrentRoom();
        for(auto iter = operation->tileChangeParams.begin(); iter != operation->tileChangeParams.end(); ++iter)
        {
            struct TileChangeParams *tcp = *iter;
            LevelComponents::Layer *layer = room->GetLayer(tcp->targetLayer);
            int index = tcp->tileX + tcp->tileY * room->GetWidth();
            layer->GetLayerData()[index] = tcp->oldTile;
            // Re-render the tile
            singleton->RenderScreenTileChange(tcp->tileX, tcp->tileY, tcp->oldTile, tcp->targetLayer);
        }
        break;
    }
}

/// <summary>
/// Perform an operation based on its parameters, and add it to the undo deque.
/// </summary>
/// <param name="operation">
/// The operation to perform.
/// </param>
void ExecuteOperation(struct OperationParams *operation)
{
    int currentRoomNumber = singleton->GetCurrentRoom()->GetRoomID();

    PerformOperation(operation);
    // If we perform an action after a series of undo, then delete the "undone" operations from history
    while(operationIndex[currentRoomNumber])
    {
        --operationIndex[currentRoomNumber];
        operationHistory[currentRoomNumber].pop_front();
    }
    operationHistory[currentRoomNumber].push_front(operation);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Undo a previously performed operation in the undo deque.
/// </summary>
/// <remarks>
/// This function does not remove the operation from the deque.
/// Instead, an index is used within the deque to track which operation should be undone next.
/// That way, an operation can be undone and redone multiple times.
/// </remarks>
void UndoOperation()
{
    int currentRoomNumber = singleton->GetCurrentRoom()->GetRoomID();

    // We cannot undo past the end of the deque
    if(operationIndex[currentRoomNumber] < operationHistory[currentRoomNumber].size())
    {
        BackTrackOperation(operationHistory[currentRoomNumber][operationIndex[currentRoomNumber]++]);
        // If the entire operation history is undone, then there are no unsaved changes
        if(operationIndex[currentRoomNumber] == operationHistory[currentRoomNumber].size())
        {
            singleton->SetUnsavedChanges(false);
        }
    }
}

/// <summary>
/// Redo a previously undone operation from the undo deque.
/// </summary>
/// <remarks>
/// This function does not add the operation to the deque.
/// Instead, an index is used within the deque to track which operation should be redone next.
/// That way, an operation can be undone and redone multiple times.
/// </remarks>
void RedoOperation()
{
    int currentRoomNumber = singleton->GetCurrentRoom()->GetRoomID();

    // We cannot redo past the front of the deque
    if(operationIndex[currentRoomNumber])
    {
        PerformOperation(operationHistory[currentRoomNumber][--operationIndex[currentRoomNumber]]);
        // Performing a "redo" will make unsaved changes
        singleton->SetUnsavedChanges(true);
    }
}

/// <summary>
/// Reset the undo deque.
/// </summary>
/// <remarks>
/// This is necessary to ensure that undo history does not persist between multiple levels.
/// </remarks>
void ResetUndoHistory()
{
    for(unsigned int i = 0; i < 16; ++i)
    {
        operationHistory[i].clear();
    }
    memset(operationIndex, 0, sizeof(operationIndex));
}
