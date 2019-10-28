#ifndef OPERATION_H
#define OPERATION_H

#include "LevelComponents/Tile.h"
#include "Dialog/RoomConfigDialog.h"

// Enumerate the type of operations that can be performed and undone
enum OperationType
{
    ChangeTileOperation,
    ChangeRoomConfigOperation,
    ObjectMoveOperation
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

// The parameters specific to an object move operation
struct ObjectMoveParams
{
    int previousPositionX;
    int previousPositionY;
    int nextPositionX;
    int nextPositionY;
    int type; // DOOR_TYPE or ENTITY_TYPE
    int objectID;

    const static int DOOR_TYPE=1;
    const static int ENTITY_TYPE=2;

    // Create an instance of ObjectMoveParams on the heap, which represents a moved obect
    static ObjectMoveParams *Create(int pX, int pY, int nX, int nY, int type, int objectID)
    {
        struct ObjectMoveParams *om = new struct ObjectMoveParams;
        om->previousPositionX = pX;
        om->previousPositionY = pY;
        om->nextPositionX = nX;
        om->nextPositionY = nY;
        om->type = type;
        om->objectID = objectID;

        return om;
    }
};

// The parameters that pertain to a single operation which can be undone atomically
struct OperationParams;
struct OperationParams
{
    // Fields
    enum OperationType type;
    std::vector<struct TileChangeParams *> tileChangeParams;
    ObjectMoveParams *objectMoveParams;
    DialogParams::RoomConfigParams *lastRoomConfigParams;
    DialogParams::RoomConfigParams *newRoomConfigParams;
    bool tileChange;
    bool roomConfigChange;
    bool objectPositionChange;

    OperationParams() :
            lastRoomConfigParams(nullptr), newRoomConfigParams(nullptr), tileChange(false), roomConfigChange(false), objectPositionChange(false)
    {}

    // Clean up the struct when it is deconstructed
    ~OperationParams()
    {
        if (tileChange)
        {
            for(unsigned int i = 0; i < tileChangeParams.size(); ++i)
            {
                struct TileChangeParams *p = tileChangeParams[i];
                delete p;
            }
        }
        if (roomConfigChange)
        {
            if(lastRoomConfigParams) delete lastRoomConfigParams;
            if(newRoomConfigParams) delete newRoomConfigParams;
        }
    }
};

// Operation function prototypes
void ExecuteOperation(struct OperationParams *operation);
void PerformOperation(struct OperationParams *operation);
void BackTrackOperation(struct OperationParams *operation);
void UndoOperation();
void RedoOperation();
void ResetUndoHistory();


#endif // OPERATION_H
