#ifndef DOOR_H
#define DOOR_H

#include <memory>

namespace LevelComponents
{
    // An enumeration of the types of doors in the game.
    // The integers correspond to the real values the games uses internally to represent a door.
    enum DoorType
    {
        Portal  = 1, // A special type just for the level's portal
        Instant = 2, // Automatically "enter" the door. For example, the edge of a room leading to another one
        Normal  = 3, // A door with normal "door"-like behavior. Press up to enter
        TYPE_04 = 4, // unknown
        TYPE_05 = 5  // unknown
    };

    class Door
    {
    private:
        // Instance variables
        enum LevelComponents::DoorType type;
        unsigned char RoomID;
        int X1, X2, Y1, Y2; //destination just be (X1, Y1)
        std::shared_ptr<Door*> DestinationDoor = nullptr;
        unsigned char X_Displacement, Y_Displacement;
        unsigned char SpriteMapID;
        unsigned int BGM_ID;

    public:
        // Constructors
        Door(unsigned char *dataPtr);  //TODO: unimplement yet, I don't think I can manage the DestinationDoor, I don't think it is a good thing to use another door Instance inside the first one (spp)
        Door(unsigned char _RoomID, enum LevelComponents::DoorType _DoorType, unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2);

        // Methods
        Door *GetDestinationDoor();
        void SetDestinationDoor(Door *otherDoor);
        void SetDoorDisplacement(signed int _X_Displacement, signed int _Y_Displacement);
        void SetSpriteMapID(unsigned char _SpriteMapID);
        void SetBGM(unsigned int _BGM_ID);
        void SetDoorPlace(unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2);
        void SetDoorType(enum LevelComponents::DoorType _DoorType);
        bool IsUnused();
        //TODO: GenerateDoorSavingData()  (ssp)
    };
}

#endif // DOOR_H
