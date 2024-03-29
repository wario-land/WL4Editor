#include "LevelDoorVector.h"

#include <QRegularExpression>

#include "ROMUtils.h"

LevelComponents::LevelDoorVector::LevelDoorVector(unsigned int doorDataStartAddr)
{
    unsigned char *doorPtr = ROMUtils::ROMFileMetadata->ROMDataPtr + doorDataStartAddr;
    int currentDoorGlobalId = 0;

    while (*doorPtr) // when first byte of the door entry is not 0
    {
        // there is something wrong with the Door vector data
        // the Level cannot be loaded in this case
        assert(currentDoorGlobalId <= 0xFF);

        struct DoorEntry tmpDoor;
        memcpy(&tmpDoor, doorPtr, sizeof(DoorEntry));
        this->doorvec.push_back(tmpDoor);
        doorPtr += sizeof(DoorEntry);
        ++currentDoorGlobalId;
    }
}

LevelComponents::LevelDoorVector::LevelDoorVector(LevelDoorVector &levelDoorVec)
{
    for (auto &door : levelDoorVec.doorvec)
    {
        struct DoorEntry tmpDoor;
        memcpy(&tmpDoor, &door, sizeof(DoorEntry));
        this->doorvec.push_back(tmpDoor);
    }
    this->Dirty = true;
}

LevelComponents::LevelDoorVector::LevelDoorVector(QString &str)
{
    QRegularExpression tmp_regex(",|\\s+"); // split using " " and "," at the same time
    QStringList u8_datalist = str.split(tmp_regex, Qt::SkipEmptyParts);
    int entry_num = u8_datalist.size() / sizeof(DoorEntry);
    if ((u8_datalist.size() % sizeof(DoorEntry)) || (entry_num == 1 && (u8_datalist[0].toInt() == _EndOfVector)))
    {
        // keep the this->doorvec empty, we should check if the vec is empty when load from QString
        return;
    }
    for (int i = 0; i < entry_num; i++)
    {
        struct DoorEntry tmpDoor;
        for (int j = 0; j < sizeof(DoorEntry); j++)
        {
            ((unsigned char *)(&tmpDoor))[j] = u8_datalist[j + sizeof(DoorEntry) * i].toInt() & 0xFF;
        }
        this->doorvec.push_back(tmpDoor);
    }
    this->Dirty = true;
}

QString LevelComponents::LevelDoorVector::toString(bool endWithFullZeroEntry)
{
    QString result;
    unsigned char data[sizeof(DoorEntry)];
    for (auto &door : this->doorvec)
    {
        memcpy(data, &door, sizeof(DoorEntry));
        for (int i = 0; i < sizeof(DoorEntry); i++)
        {
            result.append("0x" + QString::number(data[i], 16) + ", ");
        }
    }
    if (endWithFullZeroEntry)
    {
        for (int i = 0; i < sizeof(DoorEntry); i++)
        {
            result.append("0x00, ");
        }
    }
    result.chop(2);
    return result;
}

/// <summary>
/// Create an u8 array on the heap for undo and redo operation logic.
/// the delete logic will be maintained by the operation code.
/// </summary>
unsigned char *LevelComponents::LevelDoorVector::CreateDataArray(bool endWithAllZeroEntry)
{
    int doorcount = this->doorvec.size();
    if (doorcount < 1) return nullptr;
    int size_helper = endWithAllZeroEntry ? 1 : 0;
    unsigned char *data = new unsigned char[(doorcount + size_helper) * sizeof(DoorEntry)];

    for (int i = 0; i < doorcount; i++)
    {
        memcpy(data + i * sizeof(DoorEntry), &(this->doorvec[i]), sizeof(DoorEntry));
    }
    if (endWithAllZeroEntry)
    {
        memset(data + doorcount * sizeof(DoorEntry), 0, sizeof(DoorEntry));
    }
    return data;
}

/// <return>
/// The global ID of the new created Door.
/// </return>
int LevelComponents::LevelDoorVector::AddDoor(unsigned char roomID, unsigned char entitySetID, unsigned char doorType)
{
    struct DoorEntry tmpDoor;
    memset(&tmpDoor, 0, sizeof(DoorEntry));

    // set up
    tmpDoor.RoomID = roomID;
    tmpDoor.EntitySetID = entitySetID;
    tmpDoor.DoorTypeByte = doorType;

    this->doorvec.push_back(tmpDoor);
    this->Dirty = true;
    return (this->doorvec.size() - 1);
}

/// <summary>
/// Delete a Door according to its global id, also manipulate the doorvec to keep the connection indexes up to date.
/// </summary>
bool LevelComponents::LevelDoorVector::DeleteDoor(unsigned char doorGlobalId)
{
    int doorcount = this->doorvec.size();
    if (doorGlobalId > (doorcount - 1)) return false;
    if (doorGlobalId < 1) return false;
    if (GetDoorsByRoomID(this->doorvec[doorGlobalId].RoomID).size() < 2) return false; // don't delete the last Door from a Room

    for (auto &door: this->doorvec)
    {
        if (door.DestinationDoorGlobalID > doorGlobalId)
        {
            door.DestinationDoorGlobalID -= 1;
        }
    }
    this->doorvec.remove(doorGlobalId);
    this->Dirty = true;
    return true;
}

/// <summary>
/// change the data corresponding to the swap of 2 Rooms.
/// need to update the first Door in the whole vector if the Room 0 get swapped.
/// </summary>
bool LevelComponents::LevelDoorVector::SwapRooms(unsigned char first_room_id, unsigned char second_room_id)
{
    int roomIdMax = 0;
    for (auto &door: this->doorvec)
    {
        if (door.RoomID > roomIdMax)
        {
            roomIdMax = door.RoomID;
        }
    }
    if (first_room_id > roomIdMax || second_room_id > roomIdMax) return false;

    int first_door_id_in_old_R1 = -1;
    int first_door_id_in_old_R2 = -1;
    for (int i = 0; i < this->doorvec.size(); i++)
    {
        if (this->doorvec[i].RoomID == first_room_id)
        {
            if (first_door_id_in_old_R1 == -1)
            {
                first_door_id_in_old_R1 = i;
            }
            this->doorvec[i].RoomID = second_room_id;
        }
        else if (this->doorvec[i].RoomID == second_room_id)
        {
            if (first_door_id_in_old_R2 == -1)
            {
                first_door_id_in_old_R2 = i;
            }
            this->doorvec[i].RoomID = first_room_id;
        }
    }

    // we can always swap the first Doors of the 2 Rooms here
    // then there will always be a Door in the new Room 0 can be used as portal Door
    if (first_door_id_in_old_R1 != -1 && first_door_id_in_old_R2 != -1)
    {
        struct DoorEntry tmpDoor;
        memcpy(&tmpDoor, &(this->doorvec[first_door_id_in_old_R1]), sizeof(DoorEntry));
        memcpy(&(this->doorvec[first_door_id_in_old_R1]), &(this->doorvec[first_door_id_in_old_R2]), sizeof(DoorEntry));
        memcpy(&(this->doorvec[first_door_id_in_old_R2]), &tmpDoor, sizeof(DoorEntry));
        for (auto &door: this->doorvec)
        {
            // if the door's destination points to the first portal door and set disabled, we should keep the value unchanged
            if (door.DestinationDoorGlobalID == first_door_id_in_old_R1 && door.DestinationDoorGlobalID)
            {
                door.DestinationDoorGlobalID = first_door_id_in_old_R2;
            }
            else if (door.DestinationDoorGlobalID == first_door_id_in_old_R2 && door.DestinationDoorGlobalID)
            {
                door.DestinationDoorGlobalID = first_door_id_in_old_R1;
            }
        }

        // the first door in the doorvec should always be the portal door and does not have a destination
        this->doorvec[0].DestinationDoorGlobalID = 0;
    }
    this->Dirty = true;
    return true;
}

LevelComponents::DoorEntry LevelComponents::LevelDoorVector::GetDoor(unsigned char doorGlobalId)
{
    struct DoorEntry tmpDoor;
    memcpy(&tmpDoor, &(this->doorvec[doorGlobalId]), sizeof(DoorEntry));
    return tmpDoor;
}

LevelComponents::DoorEntry LevelComponents::LevelDoorVector::GetDoor(unsigned char roomID, unsigned char doorLocalId)
{
    struct DoorEntry tmpDoor;
    memset(&tmpDoor, 0, sizeof(DoorEntry));
    int doorcount = this->doorvec.size();
    if (doorcount < 1) return tmpDoor;

    int localDoorIdIter = -1;
    for (auto &door: this->doorvec)
    {
        if (door.RoomID == roomID)
        {
            localDoorIdIter++;
            if (localDoorIdIter == doorLocalId)
            {
                memcpy(&tmpDoor, &(door), sizeof(DoorEntry));
                return tmpDoor;
            }
        }
    }
    return tmpDoor;
}

LevelComponents::DoorEntry LevelComponents::LevelDoorVector::GetDestinationDoor(unsigned char roomID, unsigned char doorLocalId)
{
    struct DoorEntry tmpDoor;
    memset(&tmpDoor, 0, sizeof(DoorEntry));
    int doorcount = this->doorvec.size();
    if (doorcount < 1) return tmpDoor;

    int localDoorIdIter = -1;
    for (int i = 0; i < doorcount; i++)
    {
        if (this->doorvec[i].RoomID == roomID)
        {
            localDoorIdIter++;
            if (localDoorIdIter == doorLocalId)
            {
                memcpy(&tmpDoor, &(this->doorvec[i]), sizeof(DoorEntry));
                return tmpDoor;
            }
        }
    }
    return tmpDoor;
}

/// <summary>
/// Get a vector of Doors from a specified Room.
/// </summary>
QVector<LevelComponents::DoorEntry> LevelComponents::LevelDoorVector::GetDoorsByRoomID(unsigned char roomID)
{
    QVector<LevelComponents::DoorEntry> result;
    for (auto &door: this->doorvec)
    {
        if (door.RoomID == roomID)
        {
            struct DoorEntry tmpDoor;
            memcpy(&tmpDoor, &door, sizeof(DoorEntry));
            result.push_back(tmpDoor);
        }
    }
    return result;
}

unsigned char LevelComponents::LevelDoorVector::GetLocalIDByGlobalID(unsigned char doorGlobalId)
{
    int roomID = this->doorvec[doorGlobalId].RoomID;
    int localID_result = -1;
    for (int i = 0; i < this->doorvec.size(); i++)
    {
        if (this->doorvec[i].RoomID == roomID)
        {
            localID_result++;
        }
        if (doorGlobalId == i)
        {
            break;
        }
    }
    return localID_result;
}

unsigned char LevelComponents::LevelDoorVector::GetGlobalIDByLocalID(unsigned char roomID, unsigned char doorLocalId)
{
    int localID_counter = -1;
    for (int i = 0; i < this->doorvec.size(); i++)
    {
        if (this->doorvec[i].RoomID == roomID)
        {
            localID_counter++;
        }
        if (localID_counter == doorLocalId)
        {
            return i;
        }
    }
    return 0;
}
