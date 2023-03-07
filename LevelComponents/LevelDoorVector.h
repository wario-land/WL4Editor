#ifndef LEVELDOORVECTOR_H
#define LEVELDOORVECTOR_H

#include <QPoint>
#include <QVector>

namespace LevelComponents
{
    // An enumeration of the types of doors in the game.
    // The integers correspond to the real values the games uses internally to represent a door.
    enum _DoorType
    {
        _EndOfVector = 0,     // this is not a valid type, but the end of the door vector of a Level
        _Portal = 1,          // A special type just for the level's portal
        _InstantWarp = 2,     // Automatically warp from door to door. For example, the edge of a room leading to another one
        _NormalDoor = 3,      // A door with normal "door"-like behavior. Press up to enter
        _BossDoor = 4,        // Boss Door
        _ItemShopDoor = 5     // Item shop in the boss corrider
    };

    struct DoorEntry
    {
        unsigned char DoorTypeByte;
        unsigned char RoomID;
        unsigned char x1;
        unsigned char x2;
        unsigned char y1;
        unsigned char y2;
        unsigned char DestinationDoorGlobalID;
        signed char HorizontalDeltaWario;
        signed char VerticalDeltaWario;
        unsigned char EntitySetID;
        unsigned short BGM_ID;

        QPoint GetWarioOriginalPosition_x4()
        {
            int ypos, xpos;
            if (this->DoorTypeByte == _NormalDoor)
            {
                xpos = ((this->x1 + 1) << 6) - 1;
                ypos = (this->y2 + 1) << 6;
                // The ypos is related to current Wario animations, we only generate case for standing Wario
            }
            else
            {
                xpos = ((this->x1 + 1) << 6) + 4 * (this->HorizontalDeltaWario + 8);
                ypos = ((this->y2 + 1) << 6) + 4 * this->VerticalDeltaWario - 1;
            }
            QPoint WarioLeftTopPosition;
            WarioLeftTopPosition.setX(xpos);
            WarioLeftTopPosition.setY(ypos);
            return WarioLeftTopPosition;
        }
    };

    class LevelDoorVector
    {
    public:
        LevelDoorVector() {}
        LevelDoorVector(unsigned int doorDataStartAddr);
        LevelDoorVector(LevelDoorVector &levelDoorVec);
        LevelDoorVector(QString &str);

        QString toString(bool endWithFullZeroEntry = false);
        unsigned char *CreateDataArray(bool endWithAllZeroEntry = false);

        // Getters
        struct DoorEntry GetDoor(unsigned char doorGlobalId);
        struct DoorEntry GetDoor(unsigned char roomID, unsigned char doorLocalId);
        struct DoorEntry GetDestinationDoor(unsigned char roomID, unsigned char doorLocalId);
        QString GetDoorName(unsigned char doorGlobalId)
        {
            return "Room 0x" + QString::number((int) doorvec[doorGlobalId].RoomID, 16) + " Door " + QString::number(doorGlobalId, 10);
        }
        QVector<struct DoorEntry> GetDoorsByRoomID(unsigned char roomID);
        QVector<struct DoorEntry> GetDoorVecDeepCopy();
        unsigned char GetLocalIDByGlobalID(unsigned char doorGlobalId);
        unsigned char GetGlobalIDByLocalID(unsigned char roomID, unsigned char doorLocalId);
        int size() { return doorvec.size(); }
        bool IsDirty() { return Dirty; }

        // operations
        int AddDoor(unsigned char roomID, unsigned char entitySetID = 1, unsigned char doorType = 2); // return the added door global id
        bool DeleteDoor(unsigned char doorGlobalId);

        // Setters
        void SetBGM(unsigned char doorGlobalId, unsigned short _BGM_ID)
        {
            this->doorvec[doorGlobalId].BGM_ID = _BGM_ID;
            this->Dirty = true;
        }
        void SetDestinationDoor(unsigned char doorGlobalId, unsigned char destinationDoorGlobalId)
        {   // Note: Set DestinationDoor Disabled by setting DestinationDoor point to the vortex Door
            this->doorvec[doorGlobalId].DestinationDoorGlobalID = destinationDoorGlobalId;
            this->Dirty = true;
        }
        void SetDoorType(unsigned char doorGlobalId, unsigned char _DoorType)
        {
            this->doorvec[doorGlobalId].DoorTypeByte = _DoorType;
            this->Dirty = true;
        }
        void SetEntitySetID(unsigned char doorGlobalId, unsigned char _EntitySetID)
        {
            this->doorvec[doorGlobalId].EntitySetID = _EntitySetID;
            this->Dirty = true;
        }
        void SetWarioDelta(unsigned char doorGlobalId, signed char _DeltaX, signed char _DeltaY)
        {
            this->doorvec[doorGlobalId].HorizontalDeltaWario = (signed char) _DeltaX;
            this->doorvec[doorGlobalId].VerticalDeltaWario = (signed char) _DeltaY;
            this->Dirty = true;
        }
        void SetDoorPlace(unsigned char doorGlobalId, unsigned char _X1, unsigned char _X2, unsigned char _Y1, unsigned char _Y2)
        {
            this->doorvec[doorGlobalId].x1 = _X1;
            this->doorvec[doorGlobalId].x2 = _X2;
            this->doorvec[doorGlobalId].y1 = _Y1;
            this->doorvec[doorGlobalId].y2 = _Y2;
            this->Dirty = true;
        }

    private:
        QVector<DoorEntry> doorvec; // we should not save the last all-zero entry into the vector when loading from anywhere
        bool Dirty = false;
    };
}

#endif // LEVELDOORVECTOR_H
