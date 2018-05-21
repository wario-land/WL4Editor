#ifndef DOOR_H
#define DOOR_H


class Door
{
public:
    Door(unsigned char _RoomID, unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2);
    enum LevelComponents::DoorType type;
private:
    unsigned char RoomID;
    int X1, X2, Y1, Y2; //destination just be (X1, Y1)
    unsigned char DestinationDoorID; // index into currently loaded Door table
    unsigned char X_Displacement, Y_Displacement;
    unsigned char SpriteMapID;
    unsigned int BGM_ID;
public:
    void SetDoorDestination(unsigned char _DestinationDoorID);
    void SetDoorDisplacement(signed int _X_Displacement, signed int _Y_Displacement);
    void SetSpriteMapID(unsigned char _SpriteMapID);
    void SetBGM(unsigned int BGM_ID);
    void SetDoorPlace(unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2);
    void IsUnused();
    //the following function is provide for resetting the DestinationDoorID value when we delete a door
    void SpriteMapIDDec();
};

#endif // DOOR_H
