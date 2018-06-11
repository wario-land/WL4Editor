#include "Room.h"
#include "WL4Constants.h"
#include "ROMUtils.h"

namespace LevelComponents
{
    /// <summary>
    /// Construct a new Room object.
    /// </summary>
    /// <param name="roomDataPtr">Pointer to the start of the room data header.</param>
    /// <param name="_RoomID">Zero-based ID for the room in the level.</param>
    /// <param name="_LevelID">0x03000023 level index value.</param>
    Room::Room(int roomDataPtr, unsigned char _RoomID, unsigned int _LevelID)
    {
        this->RoomID = _RoomID;

        // Copy the room header information
        memcpy(&this->RoomHeader, ROMUtils::CurrentFile + roomDataPtr, sizeof(struct __RoomHeader));

        // Set up tileset
        int tilesetIndex = ROMUtils::CurrentFile[roomDataPtr];
        int tilesetPtr = WL4Constants::TilesetDataTable + tilesetIndex * 36;
        tileset = new Tileset(tilesetPtr);

        // Set up the layer data
        int dimensionPointer = ROMUtils::PointerFromData(roomDataPtr + 12);
        Width = ROMUtils::CurrentFile[dimensionPointer];
        Height = ROMUtils::CurrentFile[dimensionPointer + 1];
        for(int i = 0; i < 4; ++i)
        {
            enum LayerMappingType mappingType = static_cast<enum LayerMappingType>(ROMUtils::CurrentFile[roomDataPtr + i + 1]);
            int layerPtr = ROMUtils::PointerFromData(roomDataPtr + i * 4 + 8);
            layers[i] = new Layer(layerPtr, mappingType, tileset);
        }

        // Set up camera control data
        // TODO are there more types than 1, 2 and 3?
        if((CameraControlType = static_cast<enum __CameraControlType>(ROMUtils::CurrentFile[roomDataPtr + 24])) == HasControlAttrs)
        {
            int pLevelCameraControlPointerTable = ROMUtils::PointerFromData(WL4Constants::CameraControlPointerTable + _LevelID * 4);
            for(int i = 0; i < 16; i++)
            {
                int CurrentPointer = ROMUtils::PointerFromData(pLevelCameraControlPointerTable + i * 4);
                if(CurrentPointer == WL4Constants::CameraRecordSentinel)
                    break;
                if(ROMUtils::CurrentFile[CurrentPointer] == _RoomID)
                {
                    int RecordNum = ROMUtils::CurrentFile[CurrentPointer + 1];
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
