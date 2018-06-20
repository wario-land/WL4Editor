#ifndef ROOM_H
#define ROOM_H

#include "Layer.h"
#include "Tileset.h"

#include <vector>
#include <QGraphicsScene>

namespace LevelComponents
{
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

    enum __CameraControlType
    {
        FixedY          = 1,
        NoLimit         = 2,
        HasControlAttrs = 3
    };

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
        std::vector<struct __CameraControlRecord*> CameraControlRecords;
        struct __RoomHeader RoomHeader;
        Layer *layers[4];
        Tileset *tileset;
    public:
        Room(int roomDataPtr, unsigned char _RoomID, unsigned int _LevelID);
        int GetTilesetID() {return TilesetID;}
        Tileset *GetTileset() { return tileset; }
        Layer *GetLayer(int LayerID) { return layers[LayerID]; }
        QGraphicsScene *GetGraphicsScene();
    };
}

#endif // ROOM_H
