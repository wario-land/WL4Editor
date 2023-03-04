#include "LevelDoorVector.h"

#include <QRegularExpression>

#include "ROMUtils.h"

LevelComponents::LevelDoorVector::LevelDoorVector(unsigned int doorDataStartAddr)
{
    unsigned char *doorPtr = ROMUtils::ROMFileMetadata->ROMDataPtr + doorDataStartAddr;
    int currentDoorGlobalId = 0;

    // this build cannot work in your computer
    assert(sizeof(DoorEntry) == 0xC);

    while (*doorPtr) // when first byte of the door entry is not 0
    {
        // there is something wrong with the Door vector data
        // the Level cannot be loaded in this case
        assert(currentDoorGlobalId > 0xFF);

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
}

LevelComponents::LevelDoorVector::LevelDoorVector(QString &str)
{
    QRegularExpression tmp_regex(",|\\s+"); // split using " " and "," at the same time
    QStringList u8_datalist = str.split(tmp_regex, Qt::SkipEmptyParts);
    int entry_num = u8_datalist.size() / 0xC;
    if ((u8_datalist.size() % 0xC) || (entry_num == 1 && (u8_datalist[0].toInt() == _EndOfVector)))
    {
        // keep the this->doorvec empty, we should check if the vec is empty when load from QString
        return;
    }
    for (int i = 0; i < entry_num; i++)
    {
        struct DoorEntry tmpDoor;
        for (int j = 0; j < sizeof(DoorEntry); j++)
        {
            ((unsigned char *)(&tmpDoor))[j] = u8_datalist[j + 0xC * i].toInt() & 0xFF;
        }
        this->doorvec.push_back(tmpDoor);
    }
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
unsigned char *LevelComponents::LevelDoorVector::CreateOperationData()
{
    int doorcount = this->doorvec.size();
    if (doorcount < 1) return nullptr;
    unsigned char *data = new unsigned char[doorcount * sizeof(DoorEntry)];

    for (int i = 0; i < doorcount; i++)
    {
        memcpy(data + i * sizeof(DoorEntry), &(this->doorvec[i]), sizeof(DoorEntry));
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

    for (auto &door: this->doorvec)
    {
        if (door.DestinationDoorGlobalID > doorGlobalId)
        {
            door.DestinationDoorGlobalID -= 1;
        }
    }
    this->doorvec.remove(doorGlobalId);
    return true;
}

LevelComponents::DoorEntry LevelComponents::LevelDoorVector::GetDoor(unsigned char doorGlobalId)
{
    struct DoorEntry tmpDoor;
    memcpy(&tmpDoor, &(this->doorvec[doorGlobalId]), sizeof(DoorEntry));
    return tmpDoor;
}

/// <summary>
/// A helper function to render camera boxes.
/// </summary>
QPoint LevelComponents::LevelDoorVector::GetWarioOriginalPosition_x4(unsigned char doorGlobalId)
{
    int ypos, xpos;
    struct DoorEntry &currentDoor = this->doorvec[doorGlobalId];
    if (currentDoor.DoorTypeByte == _NormalDoor)
    {
        xpos = ((currentDoor.x1 + 1) << 6) - 1;
        ypos = (currentDoor.y2 + 1) << 6;
        // The ypos is related to current Wario animations, we only generate case for standing Wario
    }
    else
    {
        xpos = ((currentDoor.x1 + 1) << 6) + 4 * (currentDoor.HorizontalDeltaWario + 8);
        ypos = ((currentDoor.y2 + 1) << 6) + 4 * currentDoor.VerticalDeltaWario - 1;
    }
    QPoint WarioLeftTopPosition;
    WarioLeftTopPosition.setX(xpos);
    WarioLeftTopPosition.setY(ypos);
    return WarioLeftTopPosition;
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
