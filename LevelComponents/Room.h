#ifndef ROOM_H
#define ROOM_H

#include "Layer.h"
#include "Tileset.h"
#include <DockWidget/EditModeDockWidget.h>

#include <vector>
#include <list>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

namespace LevelComponents
{
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
        FullRender  = 0,
        SingleTile  = 1,
        LayerEnable = 2
    };

    // This struct defines the parameters necessary to perform a rendering update to the main graphics view
    struct RenderUpdateParams
    {
        enum RenderUpdateType type;
        int tileX = 0;
        int tileY = 0;
        unsigned short tileID = 0;
        struct Ui::EditModeParams mode = {};
        RenderUpdateParams(enum RenderUpdateType _type) : type(_type) {}
    };

    // The Room class defines a room within the game. Levels consist of multiple rooms interconnected by doors.
    class Room
    {
    private:
        enum __CameraControlType CameraControlType;
        unsigned int Layer3Scrolling;
        unsigned int LayerEffects;
        unsigned int RoomID;
        int TilesetID;
        unsigned int Width, Height;
        bool Layer0Locked = false;
        bool Layer0ColorBlending = false;
        int Layer0ColorBlendCoefficient_EVA;
        int Layer0ColorBlendCoefficient_EVB;
        std::vector<struct __CameraControlRecord*> CameraControlRecords;
        struct __RoomHeader RoomHeader;
        std::list<struct EntityRoomAttribute> EntityList[3]; // HMode = 0, NMode = 1, SHMode = 2
        Layer *layers[4];
        Tileset *tileset;
        QGraphicsPixmapItem *RenderedLayers[8]; // L0 - 3, E, D, C, A (may not exist)

    public:
        Room(int roomDataPtr, unsigned char _RoomID, unsigned int _LevelID);
        int GetTilesetID() { return TilesetID; }
        Tileset *GetTileset() { return tileset; }
        Layer *GetLayer(int LayerID) { return layers[LayerID]; }
        QGraphicsScene *RenderGraphicsScene(QGraphicsScene *scene, struct RenderUpdateParams *renderParams);
        bool IsLayer0ColorBlendingEnable() { return Layer0ColorBlending; }
        int GetEVA() { return Layer0ColorBlendCoefficient_EVA; }
        int GetEVB() { return Layer0ColorBlendCoefficient_EVB; }
        int GetWidth() { return (int) Width; }
        int GetHeight() { return (int) Height; }
    };
}

#endif // ROOM_H
