#ifndef OPERATION_H
#define OPERATION_H

#include "Dialog/RoomConfigDialog.h"
#include "Dialog/TilesetEditDialog.h"
#include "LevelComponents/Tile.h"
#include <memory>

// Enumerate the type of operations that can be performed and undone
enum class OperationType
{
    ChangeTileOperation,
    ChangeRoomConfigOperation,
    ObjectMoveOperation,
    ChangeTilesetOperation,
};

// The parameters specific to a tile change operation
class TileChangeParams
{
public:
    // Construct an instance of TileChangeParams, which represents a single changed tile.
    TileChangeParams(int X, int Y, int target, unsigned short nt, unsigned short ot) :
            tileX(X), tileY(Y), targetLayer(target), newTile(nt), oldTile(ot)
    {}

private:
    int tileX;
    int tileY;
    int targetLayer;
    unsigned short newTile;
    unsigned short oldTile;
};

// The parameters specific to an object move operation
class ObjectMoveParams
{
public:
    // Construct an instance of ObjectMoveParams, which represents a moved object
    ObjectMoveParams(int pX, int pY, int nX, int nY, int objectType, int objectID) :
            previousPositionX(pX), previousPositionY(pY), nextPositionX(nX), nextPositionY(nY), objectType_(objectType),
            objectID_(objectID)
    {}

private:
    int previousPositionX;
    int previousPositionY;
    int nextPositionX;
    int nextPositionY;
    int objectType_; // DOOR_TYPE or ENTITY_TYPE
    int objectID_;

    const static int DOOR_TYPE   = 1;
    const static int ENTITY_TYPE = 2;
};

// The parameters that pertain to a single operation which can be undone atomically
struct OperationParams
{
    OperationType objectType{};
    std::vector<TileChangeParams> tileChangeParams;
    ObjectMoveParams ObjectMoveParams(int pX, int pY, int nX, int nY, int objectType, int objectID){};
    DialogParams::RoomConfigParams lastRoomConfigParams{};
    DialogParams::RoomConfigParams newRoomConfigParams{};
    DialogParams::TilesetEditParams lastTilesetEditParams{};
    DialogParams::TilesetEditParams newTilesetEditParams{};
    bool tileChange{};
    bool roomConfigChange{};
    bool objectPositionChange{};
    bool TilesetChange{};

    // NOTE(alt): Commented because we don't actually need this anymore
    /*
    OperationParams() :
            lastRoomConfigParams(nullptr), newRoomConfigParams(nullptr), tileChange(false), roomConfigChange(false),
            objectPositionChange(false), TilesetChange(false)
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
        if (TilesetChange)
        {
            if (lastTilesetEditParams)
                delete lastTilesetEditParams;
            if (newTilesetEditParams)
            {
                delete newTilesetEditParams->newTileset;
                delete newTilesetEditParams;
            }
        }
    } 
    */
};

// Operation function prototypes
void ExecuteOperation(struct OperationParams *operation);
void PerformOperation(struct OperationParams *operation);
void BackTrackOperation(struct OperationParams *operation);
void UndoOperation();
void RedoOperation();
void ResetUndoHistory();

#endif // OPERATION_H
