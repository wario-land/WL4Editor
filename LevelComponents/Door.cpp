#include "Door.h"

namespace LevelComponents
{
    Door::Door(unsigned char _RoomID, LevelComponents::DoorType _DoorType, unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2)
    {
        this->RoomID = _RoomID;
        this->type = _DoorType;
        this->X1 = _X1;
        this->X2 = _X2;
        this->Y1 = _Y1;
        this->Y2 = _Y2;
    }

    void SetDoorDisplacement(char _X_Displacement, char _Y_Displacement)
    {
        X_Displacement = (unsigned char) _X_Displacement;
        Y_Displacement = (unsigned char) _Y_Displacement;
    }

    void Door::GetDoorDisplacement(unsigned char _X_Displacement, unsigned char _Y_Displacement)
    {
        this->X_Displacement = _X_Displacement;
        this->Y_Displacement = _Y_Displacement;
    }

    void Door::SetDoorPlace(unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2)
    {
        this->X1 = _X1;
        this->X2 = _X2;
        this->Y1 = _Y1;
        this->Y2 = _Y2;
    }

    bool Door::IsUnused()
    {
        if((this->type != Portal) && (this->DestinationDoor == nullptr))
            return true;
        return false;
    }

    /// <summary>
    /// Generate Wario's original position (unit: pixel) when appear from the door.
    /// </summary>
    /// <remarks>
    /// it return a QPoint(xpos, ypos).
    /// </remarks>
    QPoint Door::GetWarioOriginalPosition()
    {
        int ypos, xpos;
        if(this->type == NormalDoor)
        {
            ypos = (Y2 + 1) << 4;
            xpos = (X1 + 1) << 4;
            // The ypos is related to current Wario animations, we only generate case for standing Wario
        }
        else
        {
            xpos = X1 << 4 + (char) X_Displacement + 7;
            ypos = Y2 << 4 + (char) Y_Displacement;
        }
        QPoint WarioLeftTopPosition;
        WarioLeftTopPosition.setX(xpos);
        WarioLeftTopPosition.setY(ypos);
        return WarioLeftTopPosition;
    }
}
