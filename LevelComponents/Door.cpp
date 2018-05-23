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

    Door *Door::GetDestinationDoor()
    {
        return *(this->DestinationDoor);
    }

    void Door::SetDestinationDoor(Door *otherDoor)
    {
        this->DestinationDoor = new std::shared_ptr<Door*>(otherDoor);
    }

    void Door::SetDoorDisplacement(signed int _X_Displacement, signed int _Y_Displacement)
    {
        this->X_Displacement = (unsigned char)(255-16 * _X_Displacement);
        this->Y_Displacement = (unsigned char)(255-16 * _Y_Displacement);
    }

    void Door::SetSpriteMapID(unsigned char _SpriteMapID)
    {
        this->SpriteMapID = _SpriteMapID;
    }

    void Door::SetBGM(unsigned int _BGM_ID)
    {
        this->BGM_ID = _BGM_ID;
    }

    void Door::SetDoorPlace(unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2)
    {
        this->X1 = _X1;
        this->X2 = _X2;
        this->Y1 = _Y1;
        this->Y2 = _Y2;
    }

    void Door::SetDoorType(LevelComponents::DoorType _DoorType)
    {
        this->type = _DoorType;
    }

    bool Door::IsUnused()
    {
        if((this->type != Portal) && (this->DestinationDoor != (unsigned char)'\x00'))
            return true;
        return false;
    }
}
