#include "Door.h"

namespace LevelComponents
{
    Door::Door(unsigned char _RoomID, LevelComponents::DoorType _DoorType, unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2) :
        type(_DoorType),
        RoomID(_RoomID),
        X1(_X1),
        X2(_X2),
        Y1(_Y1),
        Y2(_Y2)
    {
        // nothing here (yet)
    }

    void Door::SetDoorDisplacement(unsigned char _X_Displacement, unsigned char _Y_Displacement)
    {
        X_Displacement = _X_Displacement;
        Y_Displacement = _Y_Displacement;
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
            xpos = ((X1 + 1) << 6) + 4 * ((char) X_Displacement + 8);
            ypos = ((Y2 + 1) << 6) + 4 * ((char) Y_Displacement) - 1;
        }
        QPoint WarioLeftTopPosition;
        WarioLeftTopPosition.setX(xpos);
        WarioLeftTopPosition.setY(ypos);
        return WarioLeftTopPosition;
    }
}
