#include "Door.h"

LevelComponents::Door::Door(unsigned char _RoomID, LevelComponents::DoorType _DoorType, unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2)
{
    this->RoomID = _RoomID;
    this->type = _DoorType;
    this->X1 = _X1;
    this->X2 = _X2;
    this->Y1 = _Y1;
    this->Y2 = _Y2;
}

void LevelComponents::Door::SetDoorDestination(unsigned char _DestinationDoorID)
{
    this->DestinationDoorID = _DestinationDoorID;
}

void LevelComponents::Door::SetDoorDisplacement(signed int _X_Displacement, signed int _Y_Displacement)
{
    this->X_Displacement = (unsigned char)(255-16*_X_Displacement);
    this->Y_Displacement = (unsigned char)(255-16*_Y_Displacement);
}

void LevelComponents::Door::SetSpriteMapID(unsigned char _SpriteMapID)
{
    this->SpriteMapID = _SpriteMapID;
}

void LevelComponents::Door::SetBGM(unsigned int _BGM_ID)
{
    this->BGM_ID = _BGM_ID;
}

void LevelComponents::Door::SetDoorPlace(unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2)
{
    this->X1 = _X1;
    this->X2 = _X2;
    this->Y1 = _Y1;
    this->Y2 = _Y2;
}

bool LevelComponents::Door::IsUnused()
{
    if((this->type != Portal) && (this->DestinationDoorID == (unsigned char)'\x00'))
        return true;
    return false;
}

void LevelComponents::Door::SpriteMapIDDec()
{
    this->DestinationDoorID = this->DestinationDoorID - 1;
}
