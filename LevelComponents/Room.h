#ifndef ROOM_H
#define ROOM_H

#include "Layer.h"

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

    class Room
    {
    private:
    unsigned int CameraControlType;
    unsigned int Layer3Scrolling;
    unsigned int LayerEffects;
    unsigned int TilesetID;
    unsigned int Width, Height;
        Layer *layers[4];
    public:
        Room(unsigned char *roomData);

    };
}

#endif // ROOM_H
