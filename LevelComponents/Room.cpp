#include "Room.h"
#include "WL4Constants.h"
#include "ROMUtils.h"

namespace LevelComponents
{
    Room::Room(unsigned char *roomData, unsigned char _RoomID, unsigned int _LevelID)
    {
        this->RoomID = _RoomID;

        // Set up camera control data
        // TODO are there more types than 1, 2 and 3?
        if((CameraControlType = static_cast<__CameraControlType>(roomData[24])) == HasControlAttrs)
        {
            int pLevelCameraCoontrolPointerTable = ROMUtils::PointerFromData(ROMUtils::CurrentFile, WL4Constants::CameraControlPointerTable + _LevelID * 4);
            for(int i = 0; i < 16; i++)
            {
                int CurrentPointer = ROMUtils::PointerFromData(ROMUtils::CurrentFile, pLevelCameraCoontrolPointerTable + i * 4);
                if(CurrentPointer == 0x3F9D58) // TODO what is this?
                    break;
                if(ROMUtils::CurrentFile[CurrentPointer] == _RoomID)
                {
                    int RecordNum = ROMUtils::CurrentFile[CurrentPointer + 1] & 0xFF;
                    struct __CameraControlRecord *recordPtr = (struct __CameraControlRecord*) (ROMUtils::CurrentFile + CurrentPointer + 2);
                    while(RecordNum--)
                    {
                        CameraControlRecords.push_back(recordPtr++);
                    }
                    break;
                }
            }
        }

        // TODO
    }
}
