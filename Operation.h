#ifndef OPERATION_H
#define OPERATION_H

#include "Dialog/RoomConfigDialog.h"
#include "Dialog/TilesetEditDialog.h"
#include "Dialog/CreditsEditDialog.h"
#include "Dialog/SpritesEditorDialog.h"
#include "Dialog/AnimatedTileGroupEditorDialog.h"
#include "Dialog/LevelConfigDialog.h"
#include "LevelComponents/LevelDoorVector.h"

// Enumerate the type of operations that can be performed and undone
enum OperationType
{
    ChangeTileOperation,
    ChangeRoomConfigOperation,
    ObjectMoveOperation,
    ChangeTilesetOperation,
    ChangeSpritesAndSpritesetsOperation,
    ChangeAnimatedTileGroupOperation,
    DoorMoveChangeOperation,
    EntityAddOperation,
    EntityDeleteOperation,
    DoorVectorChangeOperation,
    LevelConfigChangeOperation,
    CameraControlChangeOperation,
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

// The parameters specific to an entity move operation
struct ObjectMoveParams
{
    int previousPositionX;
    int previousPositionY;
    int nextPositionX;
    int nextPositionY;
    int objectID;

    // Create an instance of ObjectMoveParams on the heap, which represents a moved obect
    static ObjectMoveParams *Create(int pX, int pY, int nX, int nY, int objectID)
    {
        struct ObjectMoveParams *om = new struct ObjectMoveParams;
        om->previousPositionX = pX;
        om->previousPositionY = pY;
        om->nextPositionX = nX;
        om->nextPositionY = nY;
        om->objectID = objectID;

        return om;
    }
};

// Parameters for a door position move (global operation)
struct DoorMoveParams
{
    int roomID;
    int objectID; // local door ID in the room
    int previousPositionX;
    int previousPositionY;
    int nextPositionX;
    int nextPositionY;

    static DoorMoveParams *Create(int rID, int objID, int pX, int pY, int nX, int nY)
    {
        DoorMoveParams *p = new DoorMoveParams;
        p->roomID = rID;
        p->objectID = objID;
        p->previousPositionX = pX;
        p->previousPositionY = pY;
        p->nextPositionX = nX;
        p->nextPositionY = nY;
        return p;
    }
};

// Parameters for entity add (room-specific)
// cannot merge EntityAddParams and EntityDeleteParams since creating new entity cannot decide its index in EntityList[difficulty] before creation
struct EntityAddParams
{
    int difficulty;
    unsigned char XPos;
    unsigned char YPos;
    unsigned char EntityTypeLocalID;

    static EntityAddParams *Create(int diff, unsigned char x, unsigned char y, unsigned char eid)
    {
        EntityAddParams *p = new EntityAddParams;
        p->difficulty = diff;
        p->XPos = x;
        p->YPos = y;
        p->EntityTypeLocalID = eid;
        return p;
    }
};

// Parameters for a full layer data change (swap, clear)
struct LayerChangeParams
{
    unsigned short *oldLayerData;
    unsigned short *newLayerData;
    int layerWidth;
    int layerHeight;

    static LayerChangeParams *Create(unsigned short *oldData, unsigned short *newData, int width, int height)
    {
        LayerChangeParams *p = new LayerChangeParams;
        p->layerWidth = width;
        p->layerHeight = height;
        int size = width * height;
        p->oldLayerData = new unsigned short[size];
        memcpy(p->oldLayerData, oldData, 2 * size);
        p->newLayerData = new unsigned short[size];
        memcpy(p->newLayerData, newData, 2 * size);
        return p;
    }
};

// Parameters for a full entity list data change (swap, clear, duplicate)
struct EntityListChangeParams
{
    std::vector<LevelComponents::EntityRoomAttribute> oldEntityList;
    std::vector<LevelComponents::EntityRoomAttribute> newEntityList;

    static EntityListChangeParams *Create(std::vector<LevelComponents::EntityRoomAttribute> &oldList,
                                          std::vector<LevelComponents::EntityRoomAttribute> &newList)
    {
        EntityListChangeParams *p = new EntityListChangeParams;
        p->oldEntityList = oldList;
        p->newEntityList = newList;
        return p;
    }
};

// Parameters for entity delete (room-specific)
struct EntityDeleteParams
{
    int difficulty;
    int originalIndex; // index in EntityList[difficulty] at time of deletion
    unsigned char XPos;
    unsigned char YPos;
    unsigned char EntityTypeLocalID;

    static EntityDeleteParams *Create(int diff, int idx, unsigned char x, unsigned char y, unsigned char eid)
    {
        EntityDeleteParams *p = new EntityDeleteParams;
        p->difficulty = diff;
        p->originalIndex = idx;
        p->XPos = x;
        p->YPos = y;
        p->EntityTypeLocalID = eid;
        return p;
    }
};

// Parameters for camera control box and property changes (idempotent per room)
struct CameraControlChangeParams
{
    int roomID;
    enum LevelComponents::__CameraControlType oldCameraControlType;
    enum LevelComponents::__CameraControlType newCameraControlType;
    std::vector<struct LevelComponents::__CameraControlRecord *> oldCameraControlRecords;
    std::vector<struct LevelComponents::__CameraControlRecord *> newCameraControlRecords;

    static CameraControlChangeParams *Create(
        int roomID,
        enum LevelComponents::__CameraControlType oldType,
        enum LevelComponents::__CameraControlType newType,
        std::vector<struct LevelComponents::__CameraControlRecord *> oldRecords,
        std::vector<struct LevelComponents::__CameraControlRecord *> newRecords)
    {
        CameraControlChangeParams *p = new CameraControlChangeParams;
        p->roomID = roomID;
        p->oldCameraControlType = oldType;
        p->newCameraControlType = newType;
        p->oldCameraControlRecords = oldRecords;
        p->newCameraControlRecords = newRecords;
        return p;
    }
};

// Parameters for a full door vector swap (global - handles add, delete, and config changes)
struct DoorVectorChangeParams
{
    LevelComponents::LevelDoorVector *oldDoorVec; // deep copy before change
    LevelComponents::LevelDoorVector *newDoorVec; // deep copy after change

    static DoorVectorChangeParams *Create(LevelComponents::LevelDoorVector *oldVec,
                                          LevelComponents::LevelDoorVector *newVec)
    {
        DoorVectorChangeParams *p = new DoorVectorChangeParams;
        p->oldDoorVec = oldVec;
        p->newDoorVec = newVec;
        return p;
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
    DialogParams::LevelConfigParams *lastLevelConfigParams = nullptr;
    DialogParams::LevelConfigParams *newLevelConfigParams = nullptr;
    DoorMoveParams *doorMoveParams = nullptr;
    EntityAddParams *entityAddParams = nullptr;
    EntityDeleteParams *entityDeleteParams = nullptr;
    DoorVectorChangeParams *doorVectorChangeParams = nullptr;
    CameraControlChangeParams *cameraControlChangeParams = nullptr;
    LayerChangeParams *layer0ChangeParams = nullptr;
    LayerChangeParams *layer1ChangeParams = nullptr;
    LayerChangeParams *layer2ChangeParams = nullptr;
    EntityListChangeParams *entityNormalChangeParams = nullptr;
    EntityListChangeParams *entityHardChangeParams = nullptr;
    EntityListChangeParams *entitySHardChangeParams = nullptr;
    bool tileChange = false;
    bool roomConfigChange = false;
    bool objectPositionChange = false;
    bool TilesetChange = false;
    bool CreditChange = false;
    bool SpritesSpritesetChange = false;
    bool AnimatedTileGroupChange = false;
    bool doorMoveChange = false;
    bool entityAdd = false;
    bool entityDelete = false;
    bool doorVectorChange = false;
    bool levelConfigChange = false;
    bool cameraControlChange = false;
    bool layer0Change = false;
    bool layer1Change = false;
    bool layer2Change = false;
    bool entityNormalChange = false;
    bool entityHardChange = false;
    bool entitySHardChange = false;

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
        if (objectPositionChange)
        {
            if (objectMoveParams)
            {
                delete objectMoveParams;
                objectMoveParams = nullptr;
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
        if (CreditChange)
        {
            if (lastCreditsEditParams)
            {
                delete lastCreditsEditParams;
                lastCreditsEditParams = nullptr;
            }
            if (newCreditsEditParams)
            {
                delete newCreditsEditParams;
                newCreditsEditParams = nullptr;
            }
        }
        if (doorMoveChange)
        {
            if (doorMoveParams)
            {
                delete doorMoveParams;
                doorMoveParams = nullptr;
            }
        }
        if (entityAdd)
        {
            if (entityAddParams)
            {
                delete entityAddParams;
                entityAddParams = nullptr;
            }
        }
        if (entityDelete)
        {
            if (entityDeleteParams)
            {
                delete entityDeleteParams;
                entityDeleteParams = nullptr;
            }
        }
        if (doorVectorChange)
        {
            if (doorVectorChangeParams)
            {
                delete doorVectorChangeParams->oldDoorVec;
                delete doorVectorChangeParams->newDoorVec;
                delete doorVectorChangeParams;
                doorVectorChangeParams = nullptr;
            }
        }
        if (cameraControlChange)
        {
            if (cameraControlChangeParams)
            {
                for (auto *rec : cameraControlChangeParams->oldCameraControlRecords)
                    delete rec;
                for (auto *rec : cameraControlChangeParams->newCameraControlRecords)
                    delete rec;
                delete cameraControlChangeParams;
                cameraControlChangeParams = nullptr;
            }
        }
        if (levelConfigChange)
        {
            if (lastLevelConfigParams)
            {
                delete lastLevelConfigParams;
                lastLevelConfigParams = nullptr;
            }
            if (newLevelConfigParams)
            {
                delete newLevelConfigParams;
                newLevelConfigParams = nullptr;
            }
        }
        if (layer0Change)
        {
            if (layer0ChangeParams)
            {
                delete[] layer0ChangeParams->oldLayerData;
                delete[] layer0ChangeParams->newLayerData;
                delete layer0ChangeParams;
                layer0ChangeParams = nullptr;
            }
        }
        if (layer1Change)
        {
            if (layer1ChangeParams)
            {
                delete[] layer1ChangeParams->oldLayerData;
                delete[] layer1ChangeParams->newLayerData;
                delete layer1ChangeParams;
                layer1ChangeParams = nullptr;
            }
        }
        if (layer2Change)
        {
            if (layer2ChangeParams)
            {
                delete[] layer2ChangeParams->oldLayerData;
                delete[] layer2ChangeParams->newLayerData;
                delete layer2ChangeParams;
                layer2ChangeParams = nullptr;
            }
        }
        if (entityNormalChange)
        {
            if (entityNormalChangeParams)
            {
                delete entityNormalChangeParams;
                entityNormalChangeParams = nullptr;
            }
        }
        if (entityHardChange)
        {
            if (entityHardChangeParams)
            {
                delete entityHardChangeParams;
                entityHardChangeParams = nullptr;
            }
        }
        if (entitySHardChange)
        {
            if (entitySHardChangeParams)
            {
                delete entitySHardChangeParams;
                entitySHardChangeParams = nullptr;
            }
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
void ResetChangedBoolsThroughHistory();
void ResetGlobalElementOperationIndexes();


#endif // OPERATION_H
