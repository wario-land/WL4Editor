#include "Door.h"

namespace LevelComponents
{
    enum DoorType
    {
        Portal  = 1,
        Instant = 2,
        Normal  = 3,
        TYPE_04 = 4,
        TYPE_05 = 5
    };

    /// TODO
    Door::Door(unsigned char *doorData) :
        type(doorData[0]),
        RoomID(doorData[1]),
        X1(doorData[2]),
        X2(doorData[3]),
        Y1(doorData[4]),
        Y2(doorData[5]),
        DestinationDoor(doorData[6]),
        X_Displacement(doorData[7]),
        Y_Displacement(doorData[8]),
        SpriteMapID(doorData[9])
    {
        BGM_ID = doorData[10] | ((int) doorData[11] << 8);
    }
}
