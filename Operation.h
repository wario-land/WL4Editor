#ifndef OPERATION_H
#define OPERATION_H

#include "Dialog/RoomConfigDialog.h"
#include "Dialog/TilesetEditDialog.h"
#include "Dialog/CreditsEditDialog.h"
#include "Dialog/SpritesEditorDialog.h"
#include "Dialog/AnimatedTileGroupEditorDialog.h"
#include "LevelComponents/Tile.h"

// Enumerate the type of operations that can be performed and undone
enum OperationType
{
    ChangeTileOperation,
    ChangeRoomConfigOperation,
    ObjectMoveOperation,
    ChangeTilesetOperation,
    ChangeSpritesAndSpritesetsOperation,
    ChangeAnimatedTileGroupOperation,
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

    const static int DOOR_TYPE = 1;
    const static int ENTITY_TYPE = 2;

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
    enum OperationType type; // TODO: this seems not needed or those following bools are not needed -- ssp
    std::vector<struct TileChangeParams *> tileChangeParams;
    ObjectMoveParams *objectMoveParams = nullptr;
    DialogParams::RoomConfigParams *lastRoomConfigParams = nullptr;
    DialogParams::RoomConfigParams *newRoomConfigParams = nullptr;
    DialogParams::TilesetEditParams *lastTilesetEditParams = nullptr;
    DialogParams::TilesetEditParams *newTilesetEditParams = nullptr;
    DialogParams::CreditsEditParams *lastCreditsEditParams = nullptr;
    DialogParams::CreditsEditParams *newCreditsEditParams = nullptr;
    DialogParams::EntitiesAndEntitySetsEditParams *lastSpritesAndSetParam = nullptr;
    DialogParams::EntitiesAndEntitySetsEditParams *newSpritesAndSetParam = nullptr;
    DialogParams::AnimatedTileGroupsEditParams *lastAnimatedTileEditParam = nullptr;
    DialogParams::AnimatedTileGroupsEditParams *newAnimatedTileEditParam = nullptr;
    bool tileChange = false;
    bool roomConfigChange = false;
    bool objectPositionChange = false;
    bool TilesetChange = false;
    bool CreditChange = false;
    bool SpritesSpritesetChange = false;
    bool AnimatedTileGroupChange = false;

    OperationParams() {}

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
            {
                delete lastRoomConfigParams;
                lastRoomConfigParams = nullptr;
            }
            if (newRoomConfigParams)
            {
                delete newRoomConfigParams;
                newRoomConfigParams = nullptr;
            }
        }
        if (TilesetChange)
        {
            if (lastTilesetEditParams)
            {
                delete lastTilesetEditParams;
                lastTilesetEditParams = nullptr;
            }
            if (newTilesetEditParams)
            {
                delete newTilesetEditParams->newTileset;
                newTilesetEditParams->newTileset = nullptr;
                delete newTilesetEditParams;
                newTilesetEditParams = nullptr;
            }
        }
        if (SpritesSpritesetChange)
        {
            if (lastSpritesAndSetParam)
            {
                delete lastSpritesAndSetParam;
                lastSpritesAndSetParam = nullptr;
            }
            if (newSpritesAndSetParam)
            {
                for (LevelComponents::Entity *&entityIter: newSpritesAndSetParam->entities)
                { delete entityIter; entityIter = nullptr; }
                for (LevelComponents::EntitySet *&entitySetIter: newSpritesAndSetParam->entitySets)
                { delete entitySetIter; entitySetIter = nullptr; }
                delete newSpritesAndSetParam;
                newSpritesAndSetParam = nullptr;
            }
        }
        if (AnimatedTileGroupChange)
        {
            if (lastAnimatedTileEditParam)
            {
                delete lastAnimatedTileEditParam;
                lastAnimatedTileEditParam = nullptr;
            }
            if (newAnimatedTileEditParam)
            {
                for (LevelComponents::AnimatedTile8x8Group *&animtedTileGroupIter: newAnimatedTileEditParam->animatedTileGroups)
                { delete animtedTileGroupIter; animtedTileGroupIter = nullptr; }
                newAnimatedTileEditParam = nullptr;
            }
        }
    }
};

// Operation function prototypes
void ExecuteOperation(struct OperationParams *operation);
void ExecuteOperationGlobal(struct OperationParams *operation);
void PerformOperation(struct OperationParams *operation);
void BackTrackOperation(struct OperationParams *operation);
void UndoOperation();
void UndoOperationGlobal();
void RedoOperation();
void RedoOperationGlobal();
void ResetUndoHistory();
void DeleteUndoHistoryGlobal();
void ResetChangedBoolsThroughHistory();
void ResetGlobalElementOperationIndexes();


#endif // OPERATION_H
