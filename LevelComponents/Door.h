#ifndef DOOR_H
#define DOOR_H

#include <memory>
#include <QPoint>
#include <QString>

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
        unsigned char HorizontalDelta;
        unsigned char VerticalDelta;
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
        Door *DestinationDoor = nullptr;
        bool is_vortex = false;
        signed char DeltaX, DeltaY;
        unsigned char EntitySetID;
        unsigned int BGM_ID;
        bool newDoor = false;
        int DoorID; // Global Door Id in one Room
        __DoorEntry DoorEntry;

    public:
        // Constructors
        Door(__DoorEntry _DoorEntry, unsigned char _RoomID, enum DoorType _DoorType, unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2, int doorId);

        // Methods
        Door *GetDestinationDoor() { return DestinationDoor; }
        int GetRoomID() {return (int) this->RoomID; }
        QString GetDoorname() { return "Room " + QString::number((int) RoomID, 16) + " Door " + QString::number(DoorID, 10); }
        void SetVortex() { is_vortex = true; }
        bool IsVortex() { return is_vortex; }
        bool IsDestinationDoorDisabled() { return DestinationDoor->IsVortex(); }
        void SetDestinationDoor(Door *otherDoor) { DestinationDoor = otherDoor; } //Note: Set DestinationDoor Disabled by setting DestinationDoor point to the vortex Door
        void SetDelta(unsigned char _DeltaX, unsigned char _DeltaY);
        int GetDeltaX() { return (int) DeltaX; }
        int GetDeltaY() { return (int) DeltaY; }
        void SetEntitySetID(unsigned char _EntitySetID) { EntitySetID = _EntitySetID; }
        int GetEntitySetID() { return (int) EntitySetID; }
        void SetBGM(unsigned int _BGM_ID) { BGM_ID = _BGM_ID; }
        void SetDoorPlace(unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2);
        void SetDoorType(enum DoorType _DoorType) { type = _DoorType; }
        int GetDoortypeNum() { return static_cast<int>(type); }
        bool IsUnused();
        int GetX1() { return this->X1; }
        int GetY1() { return this->Y1; }
        int GetX2() { return this->X2; }
        int GetY2() { return this->Y2; }
        int GetBGM_ID() { return (int) BGM_ID; }
        QPoint GetWarioOriginalPosition_x4();
        void SetGlobalDoorID(int doorId) { DoorID = doorId; }
        void GlobalDoorIdDec() { --DoorID; }
        int GetGlobalDoorID() { return DoorID; }
        //TODO: GenerateDoorSavingData()  (ssp)
    };
}

#endif // DOOR_H
