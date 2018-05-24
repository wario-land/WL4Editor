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

    void Door::SetDoorDisplacement(int _X_Displacement, int _Y_Displacement)
    {
        if(_X_Displacement < 0)
            this->X_Displacement = (unsigned char)(255 + 16 * _X_Displacement);
        else
            this->X_Displacement = (unsigned char)(16 * _X_Displacement);
        if(_Y_Displacement < 0)
            this->Y_Displacement = (unsigned char)(255 + 16 * _Y_Displacement);
        else
            this->Y_Displacement = (unsigned char)(16 * _Y_Displacement);
    }

    void Door::SetDoorDisplacementROM(unsigned char _X_Displacement, unsigned char _Y_Displacement)
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
}
