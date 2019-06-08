#ifndef OPERATION_H
#define OPERATION_H

#include "Dialog/RoomConfigDialog.h"
#include "LevelComponents/Tile.h"

// Enumerate the type of operations that can be performed and undone
enum OperationType
{
    ChangeTileOperation,
    ChangeRoomConfigOperation
};

// The parameters specific to a tile change operation
struct TileChangeParams
{
    // Fields
    int tileX;
    int tileY;
    int targetLayer;
    unsigned short newTile;
    unsigned short oldTile;

    // Create an instance of TileChangeParams on the heap, which represents a single changed tile
    static TileChangeParams *Create(int X, int Y, int target, unsigned short nt, unsigned short ot)
    {
        struct TileChangeParams *p = new struct TileChangeParams;
        p->tileX = X;
        p->tileY = Y;
        p->targetLayer = target;
        p->newTile = nt;
        p->oldTile = ot;
        return p;
    }
};

// The parameters that pertain to a single operation which can be undone atomically
struct OperationParams;
struct OperationParams
{
    // Fields
    enum OperationType type;
    std::vector<struct TileChangeParams *> tileChangeParams;
    DialogParams::RoomConfigParams *lastRoomConfigParams;
    DialogParams::RoomConfigParams *newRoomConfigParams;
    bool tileChange;
    bool roomConfigChange;

    OperationParams() :
            lastRoomConfigParams(nullptr), newRoomConfigParams(nullptr), tileChange(false), roomConfigChange(false)
    {}

    // Clean up the struct when it is deconstructed
    ~OperationParams()
    {
        if (tileChange)
        {
            for (unsigned int i = 0; i < tileChangeParams.size(); ++i)
            {
                struct TileChangeParams *p = tileChangeParams[i];
                delete p;
            }
        }
        if (roomConfigChange)
        {
            if (lastRoomConfigParams)
                delete lastRoomConfigParams;
            if (newRoomConfigParams)
                delete newRoomConfigParams;
        }
    }
};

// Operation function prototypes
void ExecuteOperation(struct OperationParams *operation);
void UndoOperation();
void RedoOperation();
void ResetUndoHistory();

#endif // OPERATION_H
