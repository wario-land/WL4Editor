#include "Room.h"
#include "WL4Constants.h"
#include "ROMUtils.h"

namespace LevelComponents
{
    Room::Room(unsigned char *roomData, unsigned char _RoomID, unsigned int _LevelID)
    {
        this->RoomID = _RoomID;

        switch (roomData[24]) {
        case '\x01':
            this->CameraControlType = LevelComponents::FixedY;
            break;
        case '\x02':
            this->CameraControlType = LevelComponents::noLimit;
            break;
        case '\x02':
            this->CameraControlType = LevelComponents::HasControlAttrs;
        default: //don't know if there are some other types
            this->CameraControlType = LevelComponents::FixedY;
            break;}
        if(this->CameraControlType == LevelComponents::HasControlAttrs)
        {
            int pLevelCameraCoontrolPointerTable = ROMUtils::PointerFromData(ROMUtils::CurrentFile, WL4Constants::CameraControlPointerTable + _LevelID * 4);
            int i, RecordNum, j;
            __CameraControlRecord tmpRecord;
            for(i = 0; i < 16; I++)
            {
                int CurrentPointer = ROMUtils::PointerFromData(ROMUtils::CurrentFile, pLevelCameraCoontrolPointerTable + 4 * i);
                if(CurrentPointer == 0x3F9D58)
                    break;
                if((unsigned char) (ROMUtils::CurrentFile[CurrentPointer] & 0xFF) == _RoomID)
                {
                    RecordNum = (unsigned char) (ROMUtils::CurrentFile[CurrentPointer + 1] & 0xFF);
                    for(j = 0; j < RecordNum; j++)
                    {
                        tmpRecord.TransboundaryControl = (unsigned char) (ROMUtils::CurrentFile[CurrentPointer + 2 + j * 9] & 0xFF);
                        tmpRecord.x1 = (unsigned char) (ROMUtils::CurrentFile[CurrentPointer + 2 + j * 9 + 1] & 0xFF);
                        tmpRecord.x2 = (unsigned char) (ROMUtils::CurrentFile[CurrentPointer + 2 + j * 9 + 2] & 0xFF);
                        tmpRecord.y1 = (unsigned char) (ROMUtils::CurrentFile[CurrentPointer + 2 + j * 9 + 3] & 0xFF);
                        tmpRecord.y2 = (unsigned char) (ROMUtils::CurrentFile[CurrentPointer + 2 + j * 9 + 4] & 0xFF);
                        tmpRecord.x3 = (unsigned char) (ROMUtils::CurrentFile[CurrentPointer + 2 + j * 9 + 5] & 0xFF);
                        tmpRecord.y3 = (unsigned char) (ROMUtils::CurrentFile[CurrentPointer + 2 + j * 9 + 6] & 0xFF);
                        tmpRecord.OffsetofChangeVlaue = (unsigned char) (ROMUtils::CurrentFile[CurrentPointer + 2 + j * 9 + 7] & 0xFF);
                        tmpRecord.ChangedVlaue = (unsigned char) (ROMUtils::CurrentFile[CurrentPointer + 2 + j * 9 + 8] & 0xFF);
                        this->CameraControlRecords.push_back(tmpRecord);
                    }
                    break;
                }
            }
        }
        // TODO
    }
}
