#ifndef DOOR_H
#define DOOR_H

#include <memory>
#include <QPoint>

namespace LevelComponents
{
    // An enumeration of the types of doors in the game.
    // The integers correspond to the real values the games uses internally to represent a door.
    enum DoorType
    {
        Portal     = 1, // A special type just for the level's portal
        Instant    = 2, // Automatically "enter" the door. For example, the edge of a room leading to another one
        NormalDoor = 3, // A door with normal "door"-like behavior. Press up to enter
        TYPE_04    = 4, // unknown
        TYPE_05    = 5  // unknown
    };

    struct __DoorEntry
    {
        unsigned char DoorTypeByte;
        unsigned char RoomID;
        unsigned char x1;
        unsigned char x2;
        unsigned char y1;
        unsigned char y2;
        unsigned char LinkerDestination;
        unsigned char HorizontalDisplacement;
        unsigned char VerticalDisplacement;
        unsigned char EntitySetID;
        unsigned char BGM_ID_LowByte;
        unsigned char BGM_ID_HighByte;
    };

    class Door
    {
    private:
        // Instance variables
        enum DoorType type;
        unsigned char RoomID;
        int X1, X2, Y1, Y2; //destination just be (X1, Y1)
        std::shared_ptr<Door*> DestinationDoor = nullptr;
        unsigned char X_Displacement, Y_Displacement;
        unsigned char EntitySetID;
        unsigned int BGM_ID;

    public:
        // Constructors
        Door(unsigned char _RoomID, enum DoorType _DoorType, unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2);

        // Methods
        Door *GetDestinationDoor() { return *DestinationDoor; }
        int GetRoomID() {return (int) this->RoomID; }
        void SetDestinationDoor(Door *otherDoor) { DestinationDoor = std::make_shared<Door*>(otherDoor); }
        void SetDoorDisplacement(char _X_Displacement, char _Y_Displacement);
        void GetDoorDisplacement(unsigned char _X_Displacement, unsigned char _Y_Displacement);
        void SetEntitySetID(unsigned char _EntitySetID) { EntitySetID = _EntitySetID; }
        int GetEntitySetID() {return (int) EntitySetID; }
        void SetBGM(unsigned int _BGM_ID) { BGM_ID = _BGM_ID; }
        void SetDoorPlace(unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2);
        void SetDoorType(enum DoorType _DoorType) { type = _DoorType; }
        bool IsUnused();
        int GetX1() {return this->X1; }
        int GetY1() {return this->Y1; }
        int GetX2() {return this->X2; }
        int GetY2() {return this->Y2; }
        QPoint GetWarioOriginalPosition();
        //TODO: GenerateDoorSavingData()  (ssp)
    };
}

#endif // DOOR_H
