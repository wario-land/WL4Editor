#ifndef ROOM_H
#define ROOM_H

#include "Layer.h"

namespace LevelComponents
{
    class Room
    {
    private:
        Layer *layers[4];
    public:
        Room(unsigned char *roomData);
    };
}

#endif // ROOM_H
