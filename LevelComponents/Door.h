#ifndef DOOR_H
#define DOOR_H

#include <QPoint>
#include <QString>
#include <memory>

namespace LevelComponents
{
    // An enumeration of the types of doors in the game.
    // The integers correspond to the real values the games uses internally to represent a door.
    enum DoorType
    {
        Portal = 1,     // A special type just for the level's portal
        Instant = 2,    // Automatically "enter" the door. For example, the edge of a room leading to another one
        NormalDoor = 3, // A door with normal "door"-like behavior. Press up to enter
        TYPE_04 = 4,    // unknown
        TYPE_05 = 5     // unknown
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
        signed char HorizontalDelta;
        signed char VerticalDelta;
        unsigned char EntitySetID;
        unsigned short BGM_ID;
    };

    class Door
    {
    private:
        // Instance variables
        unsigned char RoomID;
        Door *DestinationDoor = nullptr;
        bool is_vortex = false;
        bool newDoor = false;
        int DoorID; // Global Door Id in one Room
        __DoorEntry DoorEntry;

    public:
        // Constructors
        Door(__DoorEntry _DoorEntry, unsigned char _RoomID, int doorId) :
                RoomID(_RoomID), DoorID(doorId), DoorEntry(_DoorEntry)
        {}
        Door(Door &door);

        // Getters
        unsigned short GetBGM_ID() { return DoorEntry.BGM_ID; }
        int GetDeltaX() { return DoorEntry.HorizontalDelta; }
        int GetDeltaY() { return DoorEntry.VerticalDelta; }
        Door *GetDestinationDoor() { return DestinationDoor; }
        QString GetDoorName()
        {
            return "Room " + QString::number((int) RoomID, 16) + " Door " + QString::number(DoorID, 10);
        }
        int GetDoorTypeNum() { return DoorEntry.DoorTypeByte; }
        int GetEntitySetID() { return DoorEntry.EntitySetID; }
        struct __DoorEntry GetEntryStruct() { return DoorEntry; }
        int GetGlobalDoorID() { return DoorID; }
        int GetRoomID() { return RoomID; }
        QPoint GetWarioOriginalPosition_x4();
        int GetX1() { return DoorEntry.x1; }
        int GetX2() { return DoorEntry.x2; }
        int GetY1() { return DoorEntry.y1; }
        int GetY2() { return DoorEntry.y2; }

        // Setters
        void SetAsVortex() { is_vortex = true; }
        void SetBGM(unsigned short _BGM_ID) { DoorEntry.BGM_ID = _BGM_ID; }
        void SetDestinationDoor(Door *otherDoor)
        {
            DestinationDoor = otherDoor;
        } // Note: Set DestinationDoor Disabled by setting DestinationDoor point to the vortex Door
        void SetDoorType(enum DoorType _DoorType) { DoorEntry.DoorTypeByte = (unsigned char) _DoorType; }
        void SetEntitySetID(unsigned char _EntitySetID) { DoorEntry.EntitySetID = _EntitySetID; }
        void SetGlobalDoorID(int doorId) { DoorID = doorId; }
        void SetLinkerDestination(int dest_RoomId) { DoorEntry.LinkerDestination = (unsigned char) dest_RoomId; }
        void SetDelta(signed char _DeltaX, signed char _DeltaY)
        {
            DoorEntry.HorizontalDelta = (signed char) _DeltaX;
            DoorEntry.VerticalDelta = (signed char) _DeltaY;
        }
        void SetDoorPlace(unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2)
        {
            DoorEntry.x1 = _X1;
            DoorEntry.x2 = _X2;
            DoorEntry.y1 = _Y1;
            DoorEntry.y2 = _Y2;
        }

        // Misc functions
        bool IsUnused();
        void GlobalDoorIdDec() { --DoorID; }
        bool IsVortex() { return is_vortex; }
        bool IsDestinationDoorDisabled() { return DestinationDoor->IsVortex(); }
    };
} // namespace LevelComponents

#endif // DOOR_H
