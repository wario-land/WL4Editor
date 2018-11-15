#ifndef ROOM_H
#define ROOM_H

#include "Layer.h"
#include "Entity.h"
#include "Door.h"
#include <DockWidget/EditModeDockWidget.h>

#include <vector>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

namespace LevelComponents
{
    class Room;

    // This struct defines the header attributes for a Room. It is arranged similar to its format in the ROM file.
    struct __RoomHeader
    {
        unsigned char TilesetID;
        unsigned char Layer0MappingType;
        unsigned char Layer1MappingType;
        unsigned char Layer2MappingType;
        unsigned char Layer3MappingType;
        unsigned char DATA_05[3];
        unsigned int Layer0Data;
        unsigned int Layer1Data;
        unsigned int Layer2Data;
        unsigned int Layer3Data;
        unsigned char CameraControlType;
        unsigned char Layer3Scrolling;
        unsigned char LayerEffects;
        unsigned char DATA_1B;
        unsigned int EntityTableHard;
        unsigned int EntityTableNormal;
        unsigned int EntityTableSHard;
        unsigned char DATA_28[4];

        __RoomHeader() {}
        __RoomHeader(Room *room);
    };

    // This struct defines the attributes for a single camera control record in rooms with camera boxes.
    struct __CameraControlRecord
    {
        unsigned char TransboundaryControl; //just set it to be x02
        unsigned char x1;
        unsigned char x2;
        unsigned char y1;
        unsigned char y2;
        unsigned char x3;
        unsigned char y3;
        unsigned char ChangeValueOffset;
        unsigned char ChangedValue;
    };

    // Enumeration of the types of camera control that a room may have
    enum __CameraControlType
    {
        FixedY          = 1,
        NoLimit         = 2,
        HasControlAttrs = 3
    };

    // This struct defines the attributes for a single entity record in RoomHeader EntityTable
    struct EntityRoomAttribute
    {
        int YPos;
        int XPos;
        int EntityID;
    };

    // Enumeration of the ways in which we can re-render the main graphics view
    enum RenderUpdateType
    {
        FullRender   = 0,
        SingleTile   = 1,
        LayerEnable  = 2,
        ElementsLayersUpdate = 3
    };

    // This struct defines the parameters necessary to perform a rendering update to the main graphics view
    struct RenderUpdateParams
    {
        enum RenderUpdateType type;
        int tileX = 0;
        int tileY = 0;
        unsigned short tileID = 0;
        unsigned int SelectedDoorID = (unsigned int) -1;
        int SelectedEntityID = -1;
        struct Ui::EditModeParams mode = {};
        RenderUpdateParams(enum RenderUpdateType _type) : type(_type) {}
    };

    // The Room class defines a room within the game. Levels consist of multiple rooms interconnected by doors.
    class Room
    {
    private:
        struct DLS
        {
            Layer *layer;
            int index;
        } *drawLayers[4];
        int EntityLayerZValue[4];
        enum __CameraControlType CameraControlType;
        unsigned int RoomID;
        unsigned int LevelID;
        int TilesetID;
        unsigned int Width, Height;
        bool Layer0ColorBlending = false;
        int Layer0ColorBlendCoefficient_EVA = 16;
        int Layer0ColorBlendCoefficient_EVB = 0;
        std::vector<struct __CameraControlRecord*> CameraControlRecords;
        struct __RoomHeader RoomHeader;
        int CurrentEntitySetID = 0;
        EntitySet *currentEntitySet = nullptr;
        std::vector<struct EntityRoomAttribute> EntityList[3]; // HMode = 0, NMode = 1, SHMode = 2
        std::vector<Entity*> currentEntityListSource; // Initialize Entities here
        int currentDifficulty = 1;
        Layer *layers[4];
        Tileset *tileset;
        std::vector<Door*> doors; // These Doors are deleted in the Level deconstructor
        QGraphicsPixmapItem *RenderedLayers[12]; // L0 - 3, E, D, C, A (may not exist)
        void FreeDrawLayers();
        void FreecurrentEntityListSource();
        void ResetEntitySet(int entitysetId);

    public:
        Room(int roomDataPtr, unsigned char _RoomID, unsigned int _LevelID);
        Room(Room *room);
        ~Room();
        int GetTilesetID() { return TilesetID; }
        Tileset *GetTileset() { return tileset; }
        unsigned int GetRoomID() { return RoomID; }
        unsigned int GetLevelID() { return LevelID; }
        struct __RoomHeader GetRoomHeader() { return RoomHeader; }
        void SetTileset(Tileset *newtileset, int tilesetID) { tileset = newtileset; TilesetID = tilesetID; RoomHeader.TilesetID = (unsigned int)tilesetID; }
        void PushBack_Door(Door* newdoor);
        Layer *GetLayer(int LayerID) { return layers[LayerID]; }
        void SetLayer(int LayerID, Layer *newLayer) { layers[LayerID] = newLayer; }
        QGraphicsScene *RenderGraphicsScene(QGraphicsScene *scene, struct RenderUpdateParams *renderParams);
        bool IsLayer0ColorBlendingEnable() { return Layer0ColorBlending; }
        void SetLayer0ColorBlendingEnabled(bool enability) { Layer0ColorBlending = enability; }
        int GetEVA() { return Layer0ColorBlendCoefficient_EVA; }
        int GetEVB() { return Layer0ColorBlendCoefficient_EVB; }
        void SetLayerPriorityAndAlphaAttributes(int layerPriorityAndAlphaAttr);
        int GetWidth() { return (int) Width; }
        void SetWidth(int _width) { Width = (unsigned int)_width; }
        int GetHeight() { return (int) Height; }
        void SetHeight(int _height) { Height = (unsigned int)_height; }
        int GetLayer0MappingParam() { return (int) RoomHeader.Layer0MappingType; }
        void SetLayer0MappingParam(int layer0MappingTypeParam) { RoomHeader.Layer0MappingType = layer0MappingTypeParam; }
        int GetLayerDataPtr(int LayerNum);
        void SetLayerDataPtr(int LayerNum, int dataPtr);
        bool IsLayer2Enabled() { return RoomHeader.Layer2MappingType; }
        void SetLayer2Enabled(bool enability) { RoomHeader.Layer2MappingType = enability ? (unsigned char) 0x10 : (unsigned char) 0; }
        bool IsBGLayerEnabled() { return RoomHeader.Layer3MappingType; }
        void SetBGLayerEnabled(bool enability) { RoomHeader.Layer3MappingType = enability ? (unsigned char) 0x20 : (unsigned char) 0; }
        bool IsBGLayerAutoScrollEnabled() { return RoomHeader.Layer3Scrolling == (unsigned char) 7; }
        void SetBGLayerAutoScrollEnabled(bool enability);
        int GetLayerEffectsParam() { return (int) RoomHeader.LayerEffects; }
        LevelComponents::Door *GetDoor(int _doorID) { return doors[_doorID]; }
        int CountDoors() { return (int)doors.size(); }
        void SetDoors(std::vector<Door*> _doors) { doors = _doors; }
        int GetLocalDoorID(int globalDoorId);
        int GetCurrentEntitySetID() { return CurrentEntitySetID; }
        void SetCurrentEntitySetID(int _currentEntitySetID) { CurrentEntitySetID = _currentEntitySetID; }
        void Save(QVector<struct ROMUtils::SaveData> chunks, ROMUtils::SaveData *headerChunk);
        enum __CameraControlType GetCameraControlType() { return CameraControlType; }
        unsigned char GetBGScrollParameter() { return RoomHeader.Layer3Scrolling; }
        std::vector<Entity*> GetCurrentEntityListSource() { return currentEntityListSource; }
        int FindEntity(int XPos, int YPos);
        bool AddEntity(int XPos, int YPos, int localEntityId);
        void DeleteEntity(int index);
        void DeleteDoor(int globalDoorIndex);
    };
}

#endif // ROOM_H
