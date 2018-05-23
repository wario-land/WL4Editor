#ifndef DOOR_H
#define DOOR_H

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

    class Door
    {
    public:
        Door(unsigned char _RoomID, enum LevelComponents::DoorType _DoorType, unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2);
    private:
        enum LevelComponents::DoorType type;
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
        void SetBGM(unsigned int _BGM_ID);
        void SetDoorPlace(unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2);
        void SetDoorType(enum LevelComponents::DoorType _DoorType);
        bool IsUnused();
        //the following function is provide for resetting the DestinationDoorID value when we delete a door
        void DestinationDoorIDDec();
    };
}
#endif // DOOR_H
