#include "Door.h"

namespace LevelComponents
{
    Door::Door(__DoorEntry _DoorEntry, unsigned char _RoomID, LevelComponents::DoorType _DoorType, unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2, int doorId) :
        DoorEntry(_DoorEntry),
        type(_DoorType),
        RoomID(_RoomID),
        X1(_X1),
        X2(_X2),
        Y1(_Y1),
        Y2(_Y2),
        DoorID(doorId)
    {
        // nothing here (yet)
    }

    void Door::SetDelta(unsigned char _DeltaX, unsigned char _DeltaY)
    {
        DeltaX = (signed char) _DeltaX;
        DeltaY = (signed char) _DeltaY;
    }

    void Door::SetDoorPlace(unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2)
    {
        X1 = _X1;
        X2 = _X2;
        Y1 = _Y1;
        Y2 = _Y2;
    }

    /// <summary>
    /// Determine if a door is unused.
    /// </summary>
    /// <return>
    /// it return a QPoint(xpos, ypos).
    /// </return>
    bool Door::IsUnused()
    {
        return type != Portal && DestinationDoor == nullptr;
    }

    /// <summary>
    /// Generate Wario's original position (unit: pixel) when appear from the door.
    /// </summary>
    /// <return>
    /// true if the door is unused.
    /// </return>
    QPoint Door::GetWarioOriginalPosition_x4()
    {
        int ypos, xpos;
        if(this->type == NormalDoor)
        {
            ypos = (Y2 + 1) << 6;
            xpos = ((X1 + 1) << 6) - 1;
            // The ypos is related to current Wario animations, we only generate case for standing Wario
        }
        else
        {
            xpos = ((X1 + 1) << 6) + 4 * (DeltaX + 8);
            ypos = ((Y2 + 1) << 6) + 4 * DeltaY - 1;
        }
        QPoint WarioLeftTopPosition;
        WarioLeftTopPosition.setX(xpos);
        WarioLeftTopPosition.setY(ypos);
        return WarioLeftTopPosition;
    }
}
