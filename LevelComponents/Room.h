#ifndef ROOM_H
#define ROOM_H

#include "Door.h"
#include "Entity.h"
#include "Layer.h"
#include <DockWidget/EditModeDockWidget.h>

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <algorithm> // find
#include <vector>

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
        unsigned char LayerPriorityColorBlendingFlag;
        unsigned char DATA_1B;
        unsigned int EntityTableHard;
        unsigned int EntityTableNormal;
        unsigned int EntityTableSHard;
        unsigned char LayerGFXEffect01;
        unsigned char LayerGFXEffect02;
        unsigned short BGMVolume;

        __RoomHeader() {}
        __RoomHeader(Room *room);
    };

    // This struct defines the attributes for a single camera control record in rooms with camera boxes.
    struct __CameraControlRecord
    {
        unsigned char TransboundaryControl; // just set it to be x02
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
        FixedY = 1,
        NoLimit = 2,
        HasControlAttrs = 3,
        Vertical_Seperated = 4,
    };

    // This struct defines the attributes for a single entity record in RoomHeader EntityTable
    struct EntityRoomAttribute
    {
        unsigned char YPos;
        unsigned char XPos;
        unsigned char EntityID;
    };

    // Enumeration of the ways in which we can re-render the main graphics view
    enum RenderUpdateType
    {
        FullRender = 0,
        TileChanges = 1,
        LayerEnable = 2,
        ElementsLayersUpdate = 3
    };

    struct Tileinfo
    {
        int tileX = 0;
        int tileY = 0;
        unsigned short tileID = 0;
    };

    // This struct defines the parameters necessary to perform a rendering update to the main graphics view
    struct RenderUpdateParams
    {
        enum RenderUpdateType type;
        int tileX = 0;
        int tileY = 0;
        unsigned short tileID = 0;
        QVector<Tileinfo> tilechangelist;
        unsigned int SelectedDoorID = ~0u;
        int SelectedEntityID = -1;
        struct Ui::EditModeParams mode = {};
        RenderUpdateParams(enum RenderUpdateType _type) : type(_type) {}
    };

    // The Room class defines a room within the game. Levels consist of multiple rooms interconnected by doors.
    class Room
    {
    private:
        // This is an internal struct that is used for sorting layers by render priority
        struct DLS
        {
            Layer *layer;
            int index;
        } * drawLayers[4];

        // Locals
        int EntityLayerZValue[4];
        enum __CameraControlType CameraControlType;
        unsigned int RoomID;
        unsigned int LevelID;
        unsigned int Width, Height;
        bool Layer0ColorBlending = false;
        int Layer0ColorBlendCoefficient_EVA = 16;
        int Layer0ColorBlendCoefficient_EVB = 0;
        std::vector<struct __CameraControlRecord *> CameraControlRecords;
        struct __RoomHeader RoomHeader;
        int CurrentEntitySetID = 0;
        EntitySet *currentEntitySet = nullptr;
        std::vector<struct EntityRoomAttribute> EntityList[3]; // HMode = 0, NMode = 1, SHMode = 2
        bool EntityListDirty[3];
        std::vector<Entity *> currentEntityListSource; // Initialize Entities here
        int currentDifficulty = 1;
        Layer *layers[4];
        Tileset *tileset;
        std::vector<Door *> doors; // These Doors are deleted in the Level deconstructor
        QGraphicsPixmapItem
            *RenderedLayers[13]; // L0 - 3, E(Entities boxes), D, C, A (may not exist), E0 - 3, hidden coins
        bool CameraBoundaryDirty = false;
        bool IsCopy = false;

        // Helper functions
        void FreeDrawLayers();
        void FreeCurrentEntityListSource();
        void ResetEntitySet(int entitysetId);

    public:
        // Object construction
        Room(int roomDataPtr, unsigned char _RoomID, unsigned int _LevelID);
        Room(Room *room);
        ~Room();

        // Getters
        size_t CountDoors() { return doors.size(); }
        unsigned char GetBGScrollParameter() { return RoomHeader.Layer3Scrolling; }
        unsigned char GetLayerGFXEffect01() { return RoomHeader.LayerGFXEffect01; }
        unsigned char GetLayerGFXEffect02() { return RoomHeader.LayerGFXEffect02; }
        unsigned short GetBgmvolume() { return RoomHeader.BGMVolume; }
        std::vector<struct __CameraControlRecord *> GetCameraControlRecords(bool create_new_instances = false)
        {
            if (!create_new_instances)
            {
                return CameraControlRecords;
            }
            std::vector<struct __CameraControlRecord *> newCameraControlRecords;
            for (unsigned int i = 0; i < CameraControlRecords.size(); ++i)
            {
                struct __CameraControlRecord *newCameraLimitator = new __CameraControlRecord();
                memcpy(newCameraLimitator, CameraControlRecords[i], sizeof(struct __CameraControlRecord));
                newCameraControlRecords.push_back(newCameraLimitator);
            }
            return newCameraControlRecords;
        }
        enum __CameraControlType GetCameraControlType() { return CameraControlType; }
        std::vector<Entity *> GetCurrentEntityListSource() { return currentEntityListSource; }
        int GetCurrentEntitySetID() { return CurrentEntitySetID; }
        LevelComponents::Door *GetDoor(int _localdoorID) { return doors[_localdoorID]; }
        std::vector<Door *> GetDoors() { return doors; }
        bool GetEntityListDirty(int difficulty) { return EntityListDirty[difficulty]; }
        std::vector<struct EntityRoomAttribute> GetEntityListData(int difficulty) { return EntityList[difficulty]; }
        int GetEVA() { return Layer0ColorBlendCoefficient_EVA; }
        int GetEVB() { return Layer0ColorBlendCoefficient_EVB; }
        unsigned int GetHeight() { return Height; }
        unsigned int GetWidth() { return Width; }
        unsigned int GetLayer0Width() { return layers[0]->GetLayerWidth(); }
        unsigned int GetLayer0Height() { return layers[0]->GetLayerHeight(); }
        Layer *GetLayer(int LayerID) { return layers[LayerID]; }
        int GetLayer0MappingParam() { return RoomHeader.Layer0MappingType; }
        int GetLayerDataPtr(unsigned int LayerNum);
        int GetLayerEffectsParam() { return RoomHeader.LayerPriorityColorBlendingFlag; }
        unsigned int GetLevelID() { return LevelID; }
        struct __RoomHeader GetRoomHeader() { return RoomHeader; }
        unsigned int GetRoomID() { return RoomID; }
        Tileset *GetTileset() { return tileset; }
        int GetTilesetID() { return RoomHeader.TilesetID; }
        int GetEntityX(int index);
        int GetEntityY(int index);
        unsigned char GetBGLayerScrollFlag() { return RoomHeader.Layer3Scrolling; }
        bool IsBGLayerEnabled() { return RoomHeader.Layer3MappingType; }
        bool IsCameraBoundaryDirty() { return CameraBoundaryDirty; }
        bool IsLayer0ColorBlendingEnabled() { return Layer0ColorBlending; }
        bool IsLayer2Enabled() { return RoomHeader.Layer2MappingType; }

        // Setters
        void AddDoor(Door *newdoor);
        bool AddEntity(int XPos, int YPos, int localEntityId, int difficulty = -1);
        void DeleteCameraLimitator(int index);
        void DeleteDoor(int globalDoorIndex);
        void DeleteEntity(int index);
        void DeleteEntity(int difficulty, int index);
        void ClearEntitylist(int difficulty);
        void SetLayerGFXEffect01(unsigned char flag) { RoomHeader.LayerGFXEffect01 = flag; }
        void SetLayerGFXEffect02(unsigned char flag) { RoomHeader.LayerGFXEffect02 = flag; }
        void SetBgmvolume(unsigned short bgmvolume) { RoomHeader.BGMVolume = bgmvolume; }
        void SetBGLayerEnabled(bool enability) { RoomHeader.Layer3MappingType = enability ? '\x20' : '\x00'; }
        void SetBGLayerScrollFlag(unsigned char flag);
        void SetCameraBoundaryDirty(bool dirty) { CameraBoundaryDirty = dirty; }
        void SetCameraControlType(__CameraControlType new_control_type)
        {
            CameraControlType = new_control_type;
            RoomHeader.CameraControlType = (unsigned char) new_control_type;
        }
        void SetCurrentEntitySet(int _currentEntitySetID)
        {
            CurrentEntitySetID = _currentEntitySetID;
            ResetEntitySet(_currentEntitySetID);
        }
        void SetDoorsVector(std::vector<Door *> _doors)
        {
            if (!IsCopy)
                return;
            if (doors.size())
            {
                for (auto iter = doors.begin(); iter != doors.end(); ++iter)
                {
                    delete *iter; // Delete doors
                }
            }
            doors = _doors;
        }
        void SetEntityListDirty(int difficulty, bool dirty) { EntityListDirty[difficulty] = dirty; }
        void SetEntityListPtr(int difficulty, unsigned int ptr) { (&RoomHeader.EntityTableHard)[difficulty] = ptr; }
        void SetHeight(int _height) { Height = (unsigned int) _height; }
        void SetWidth(int _width) { Width = (unsigned int) _width; }
        void SetLayer(int LayerID, Layer *newLayer) { layers[LayerID] = newLayer; }
        void SetLayer0ColorBlendingEnabled(bool enability) { Layer0ColorBlending = enability; }
        void SetLayer0MappingParam(int layer0MappingTypeParam)
        {
            RoomHeader.Layer0MappingType = layer0MappingTypeParam;
        }
        void SetLayer2Enabled(bool enability) { RoomHeader.Layer2MappingType = enability ? '\x10' : '\x00'; }
        void SetLayerDataPtr(int LayerNum, int dataPtr);
        void SetLayerPriorityAndAlphaAttributes(int layerPriorityAndAlphaAttr);
        void SetTileset(Tileset *newtileset, int tilesetID)
        {
            tileset = newtileset;
            RoomHeader.TilesetID = (unsigned char) tilesetID;
        }
        void ResetTileSet();
        void SetEntityPosition(int XPos, int YPos, int index);
        void ResetRoomHeader(__RoomHeader newheader);

        // Functions
        void AddCameraLimitator();
        int FindEntity(int XPos, int YPos);
        unsigned int GetLocalDoorID(int globalDoorId);
        void GetSaveChunks(QVector<ROMUtils::SaveData> &chunks, ROMUtils::SaveData *headerChunk,
                           ROMUtils::SaveData *cameraPointerTableChunk, unsigned int *cameraPointerTableIndex);
        QImage AlphaBlend(int eva, int evb, int scrH, int scrW, QImage imgA, QImage imgB);
        QGraphicsScene *RenderGraphicsScene(QGraphicsScene *scene, RenderUpdateParams *renderParams);
        void SetCameraLimitator(int index, __CameraControlRecord limitator_data);
        void SwapEntityLists(int first_list_id, int second_list_id);
        bool IsNewDoorPositionInsideRoom(int x1, int x2, int y1, int y2);
        bool IsNewEntityPositionInsideRoom(int x, int y);
        QPixmap GetLayerPixmap(int layerId, int x, int y, int w, int h);
    };
} // namespace LevelComponents

#endif // ROOM_H
