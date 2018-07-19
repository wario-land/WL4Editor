#include "Level.h"
#include "ROMUtils.h"
#include "WL4Constants.h"

#include <cstring>

namespace LevelComponents
{
    /// <summary>
    /// This is a helper funtion that allows you to load a level based on a passage and stage number.
    /// This constructor will chain to the other constructor.
    /// </summary>
    /// <remarks>
    /// Passage numbers:
    ///      0x00: Entry Passage
    ///      0x01: Emerald Passage
    ///      0x02: Ruby Passage
    ///      0x03: Topaz Passage
    ///      0x04: Sapphire Passage
    ///      0x05: Golden Pyramid
    ///      0x06: Sound Room
    /// Level numbers:
    ///      0x00: Level 1
    ///      0x01: Level 2
    ///      0x02: Level 3
    ///      0x03: Level 4
    ///      0x04: Mini-game shop
    ///      0x05: Boss door
    /// The level header exists in a record of size 0x2B bytes.
    /// LH + 0x00 (1): Tileset ID
    ///      0x01 (1): Mapping type for layer 0 (0x00, 0x10, 0x20)
    ///      0x02 (1): Mapping type for layer 1 (always 0x10?)
    ///      0x03 (1): Mapping type for layer 2 (0x00, 0x10)
    ///      0x04 (1): Mapping type for layer 3 (0x00, 0x20)
    ///      0x05 (3): (always 0?)
    ///      0x08 (4): Pointer to compressed layer 0 tiles
    ///      0x0C (4): Pointer to compressed layer 1 tiles
    ///      0x10 (4): Pointer to compressed layer 2 tiles
    ///      0x14 (4): Pointer to compressed layer 3 tiles
    ///      0x18 (1): Camera scroll type:
    ///           0x01: When Wario moves offscreen vertically, the camera will follow in screen-sized increments
    ///           0x02: The camera will freely follow Wario
    ///           0x03: Camera scrolling will be controlled by special parameters
    ///      0x19 (1): BG (layer 3) scrolling:
    ///           0x01: No scrolling
    ///           0x03: Layer 3 invisible
    ///           0x07: Normal scrolling
    ///      0x1A (1): Layer special effects byte
    ///      0x1B (1): (always 0?)
    ///      0x1C (4): Pointer to hard mode sprite data
    ///      0x20 (4): Pointer to normal mode sprite data
    ///      0x24 (4): Pointer to super hard mode sprite data
    ///      0x28 (4): (unknown)
    /// </remarks>
    /// <param name="passage">
    /// The passage number.
    /// </param>
    /// <param name="stage">
    /// The stage number.
    /// </param>
    Level::Level(enum __passage passage, enum __stage stage)
    {
        // Get the level header index
        int offset = WL4Constants::LevelHeaderIndexTable + passage * 24 + stage * 4;
        int levelHeaderIndex = ROMUtils::IntFromData(offset);

        // Load the level information
        int levelHeaderPointer = WL4Constants::LevelHeaderTable + levelHeaderIndex * 12;
        int levelIndex = ROMUtils::CurrentFile[levelHeaderPointer]; // 0x3000023

        memcpy(&LevelHeader, ROMUtils::CurrentFile + levelHeaderPointer, sizeof(struct __LevelHeader));

        // Load the door data
        std::vector<Door*> doors;
        std::vector<int> destinations;
        int doorStartAddress = ROMUtils::PointerFromData(WL4Constants::DoorTable + levelIndex * 4);
        struct __DoorEntry *doorPtr = (struct __DoorEntry*) (ROMUtils::CurrentFile + doorStartAddress);
        unsigned char *firstByte;
        while(*(firstByte = (unsigned char*) doorPtr))
        {
            enum DoorType type = static_cast<DoorType>(doorPtr->DoorTypeByte);
            Door *newDoor = new Door(doorPtr->RoomID, type, doorPtr->x1, doorPtr->x2, doorPtr->y1, doorPtr->y2);
            newDoor->SetEntitySetID(doorPtr->EntitySetID);
            newDoor->SetBGM(doorPtr->BGM_ID_LowByte | ((unsigned int) (doorPtr->BGM_ID_HighByte)) << 8);
            newDoor->SetDoorDisplacement(doorPtr->HorizontalDisplacement, doorPtr->VerticalDisplacement);
            doors.push_back(newDoor);
            destinations.push_back(doorPtr->LinkerDestination);
            ++doorPtr;
        }
        // Assign the destinations for the doors
        for(unsigned int i = 0; i < doors.size(); ++i)
        {
            doors[i]->SetDestinationDoor(doors[destinations[i]]);
        }

        // Load the room data
        int roomTableAddress = ROMUtils::PointerFromData(WL4Constants::RoomDataTable + levelIndex * 4);
        int roomCount = ROMUtils::CurrentFile[levelHeaderPointer + 1];
        for(int i = 0; i < roomCount; i++)
        {
            rooms.push_back(new Room(roomTableAddress + i * 0x2C, i, levelIndex));
        }

        // Distribute door data to every room
        for(unsigned int i = 0; i < doors.size(); ++i)
        {
            rooms[doors[i]->GetRoomID()]->PushBack_Door(doors[i]);
        }

        // Load the level name
        int LevelNameAddress = ROMUtils::PointerFromData(WL4Constants::LevelNamePointerTable + passage * 24 + stage * 4);
        LoadLevelName(LevelNameAddress);

        // TODO

    }

    /// <summary>
    /// Deconstruct the Level and clean up its instance objects on the heap.
    /// </summary>
    Level::~Level()
    {
        for(auto iter = doors.begin(); iter != doors.end(); ++iter)
        {
            delete *iter; // Delete doors
        }
        for(auto iter = rooms.begin(); iter != rooms.end(); ++iter)
        {
            delete *iter; // Delete rooms
        }
    }

    /// <summary>
    /// Set the countdown timer for a specific difficulty class.
    /// </summary>
    /// <param name="LevelDifficulty">An enumeration representing the level's difficulty</param>
    /// <param name="minutes">The number of minutes to set the timer for this difficulty level</param>
    /// <param name="seconds">The number of seconds to set the timer for this difficulty level</param>
    void Level::SetTimeCountdownCounter(enum __LevelDifficulty LevelDifficulty, unsigned int seconds)
    {
        int a = seconds / 60;
        int b = (seconds - 60 * a) / 10;
        int c = seconds - 60 * a - 10 * b;
        if(LevelDifficulty == HardDifficulty)
        {
            LevelHeader.HardModeMinuteNum = (unsigned char) a;
            LevelHeader.HardModeSecondTenPlaceNum = (unsigned char) b;
            LevelHeader.HardModeSecondOnePlaceNum = (unsigned char) c;
        }
        else if(LevelDifficulty == NormalDifficulty)
        {
            LevelHeader.NormalModeMinuteNum = (unsigned char) a;
            LevelHeader.NormalModeSecondTenPlaceNum = (unsigned char) b;
            LevelHeader.NormalModeSecondOnePlaceNum = (unsigned char) c;
        }
        else if(LevelDifficulty == SHardDifficulty)
        {
            LevelHeader.SHardModeMinuteNum = (unsigned char) a;
            LevelHeader.SHardModeSecondTenPlaceNum = (unsigned char) b;
            LevelHeader.SHardModeSecondOnePlaceNum = (unsigned char) c;
        }
    }

    /// <summary>
    /// Get the countdown timer for a specific difficulty class.
    /// </summary>
    /// <remarks>
    /// Return value is total time in seconds
    /// </remarks>
    int Level::GetTimeCountdownCounter(__LevelDifficulty LevelDifficulty)
    {
        int a, b, c;
        if(LevelDifficulty == HardDifficulty)
        {
            a = (int) LevelHeader.HardModeMinuteNum;
            b = (int) LevelHeader.HardModeSecondTenPlaceNum;
            c = (int) LevelHeader.HardModeSecondOnePlaceNum;
        }
        else if(LevelDifficulty == NormalDifficulty)
        {
            a = (int) LevelHeader.NormalModeMinuteNum;
            b = (int) LevelHeader.NormalModeSecondTenPlaceNum;
            c = (int) LevelHeader.NormalModeSecondOnePlaceNum;
        }
        else if(LevelDifficulty == SHardDifficulty)
        {
            a = (int) LevelHeader.SHardModeMinuteNum;
            b = (int) LevelHeader.SHardModeSecondTenPlaceNum;
            c = (int) LevelHeader.SHardModeSecondOnePlaceNum;
        }
        return (a * 60 + b * 10 + c);
    }

    void Level::LoadLevelName(int address)
    {
        for(int i = 0; i < 26; i++)
        {
            unsigned char chr = ROMUtils::CurrentFile[address + i];
            if(chr <= 0x09)
            {
                LevelName.append(1, chr + (unsigned char) 48);
            }
            else if(chr >= 0x0A && chr <= 0x23)
            {
                LevelName.append(1, chr + (unsigned char) 55);
            }
            else if(chr >= 0x24 && chr <= 0x3D)
            {
                LevelName.append(1, chr + (unsigned char) 61);
            }
            else
            {
                LevelName.append(1, (unsigned char) 32);
            }
        }
    }
}
