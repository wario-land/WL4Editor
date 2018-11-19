#include "Level.h"
#include "ROMUtils.h"
#include "WL4Constants.h"

#include <cstring>
#include <cassert>

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
    ///      0x05 (3): (padding, always 0)
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
    Level::Level(enum __passage _passage, enum __stage _stage) : passage(_passage), stage(_stage)
    {
        // Get the level header index
        int offset = WL4Constants::LevelHeaderIndexTable + passage * 24 + stage * 4;
        int levelHeaderIndex = ROMUtils::IntFromData(offset);

        // Load the level information
        int levelHeaderPointer = WL4Constants::LevelHeaderTable + levelHeaderIndex * 12;
        int levelIndex = ROMUtils::CurrentFile[levelHeaderPointer]; // 0x3000023

        memcpy(&LevelHeader, ROMUtils::CurrentFile + levelHeaderPointer, sizeof(struct __LevelHeader));

        // Load the door data
        std::vector<int> destinations;
        int doorStartAddress = ROMUtils::PointerFromData(WL4Constants::DoorTable + levelIndex * 4);
        struct __DoorEntry *doorPtr = (struct __DoorEntry*) (ROMUtils::CurrentFile + doorStartAddress);
        unsigned char *firstByte;
        int currentDoornum = 0;
        while(*(firstByte = (unsigned char*) doorPtr))
        {
            Door *newDoor = new Door(*doorPtr, doorPtr->RoomID, currentDoornum);
            newDoor->SetEntitySetID(doorPtr->EntitySetID);
            newDoor->SetBGM(doorPtr->BGM_ID);
            newDoor->SetDelta(doorPtr->HorizontalDelta, doorPtr->VerticalDelta);
            doors.push_back(newDoor);
            destinations.push_back(doorPtr->LinkerDestination);
            ++doorPtr;
            ++currentDoornum;
        }
        // Assign the destinations for the doors
        for(unsigned int i = 0; i < doors.size(); ++i)
        {
            doors[i]->SetDestinationDoor(doors[destinations[i]]);
        }
        // Set the first Door be Vortex Door
        doors[0]->SetAsVortex();

        // Load the room data
        int roomTableAddress = ROMUtils::PointerFromData(WL4Constants::RoomDataTable + levelIndex * 4);
        int roomCount = ROMUtils::CurrentFile[levelHeaderPointer + 1];
        for(int i = 0; i < roomCount; i++)
        {
            rooms.push_back(new Room(roomTableAddress + i * 0x2C, i, levelIndex));
        }

        // Distribute door data to every room
        RedistributeDoor();

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
    /// <returns>
    /// Total time in seconds
    /// </returns>
    int Level::GetTimeCountdownCounter(__LevelDifficulty LevelDifficulty)
    {
        int a, b, c;
        a = b = c = 0;
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

    void Level::RedistributeDoor()
    {
        // Distribute door data to every room
        for(unsigned int i = 0; i < doors.size(); ++i)
        {
            rooms[doors[i]->GetRoomID()]->PushBack_Door(doors[i]);
        }
    }

    std::vector<Door *> Level::GetRoomDoors(unsigned int roomId)
    {
        std::vector<Door *> roomDoors;
        // Distribute door data
        for(unsigned int i = 0; i < doors.size(); ++i)
        {
            if(doors[i]->GetRoomID() == (int) roomId)
            roomDoors.push_back(doors[i]);
        }
        return roomDoors;
    }

    /// <summary>
    /// Delete a Door from the Door list and destroy the intance.
    /// </summary>
    /// <param name="globalDoorIndex">
    /// The global Door id given by current Level.
    /// </param>
    void Level::DeleteDoor(int globalDoorIndex)
    {
        delete(*(doors.begin() + globalDoorIndex));
        doors.erase(doors.begin() + globalDoorIndex);
    }

    /// <summary>
    /// Add a new Door to the Door list and distribute it to the Room.
    /// </summary>
    /// <param name="newdoor">
    /// new Door instance.
    /// </param>
    void Level::AddDoor(Door *newdoor)
    {
        doors.push_back(newdoor);
        rooms[doors[doors.size() - 1]->GetRoomID()]->PushBack_Door(doors[doors.size() - 1]);
    }

    /// <summary>
    /// Helper function to populate LevelName with the name string from the ROM.
    /// </summary>
    /// <param name="address">
    /// Starting address of the level name string.
    /// </param>
    void Level::LoadLevelName(int address)
    {
        for(int i = 0; i < 26; i++)
        {
            unsigned char chr = ROMUtils::CurrentFile[address + i];
            if(chr <= 0x09)
            {
                LevelName.append(1, chr + 48);
            }
            else if(chr >= 0x0A && chr <= 0x23)
            {
                LevelName.append(1, chr + 55);
            }
            else if(chr >= 0x24 && chr <= 0x3D)
            {
                LevelName.append(1, chr + 61);
            }
            else
            {
                LevelName.append(1, (unsigned char) 32);
            }
        }
    }

    void Level::GetSaveChunks(QVector<ROMUtils::SaveData> &chunks)
    {
        // Calculate some values needed to initialize the save data
        int offset = WL4Constants::LevelHeaderIndexTable + passage * 24 + stage * 4;
        int levelHeaderIndex = ROMUtils::IntFromData(offset);
        int levelHeaderPointer = WL4Constants::LevelHeaderTable + levelHeaderIndex * 12;
        int levelIndex = ROMUtils::CurrentFile[levelHeaderPointer];
        unsigned int roomTablePtr = WL4Constants::RoomDataTable + levelIndex * 4;
        unsigned int roomHeaderChunkSize = rooms.size() * sizeof(struct __RoomHeader);
        unsigned int doorTablePtr = WL4Constants::DoorTable + levelIndex * 4;
        unsigned int doorChunkSize = doors.size() * sizeof(struct __DoorEntry);
        unsigned int LevelNamePtr = WL4Constants::LevelNamePointerTable + passage * 24 + stage * 4;

        // Create the contiguous room header chunk
        struct ROMUtils::SaveData roomHeaders =
        {
            roomTablePtr,
            roomHeaderChunkSize,
            (unsigned char*) malloc(roomHeaderChunkSize),
            ROMUtils::SaveDataIndex++,
            false,
            0
        };

        // Populate chunks with room data
        for(unsigned int i = 0; i < rooms.size(); ++i)
        {
            struct __RoomHeader rh = rooms[i]->GetRoomHeader();
            memcpy(roomHeaders.data + i * sizeof(struct __RoomHeader), &rh, sizeof(struct __RoomHeader));
            rooms[i]->GetSaveChunks(chunks, &roomHeaders);
        }

        // Create door list chunk
        struct ROMUtils::SaveData doorChunk =
        {
            doorTablePtr,
            doorChunkSize,
            (unsigned char*) malloc(doorChunkSize),
            ROMUtils::SaveDataIndex++,
            true,
            0
        };

        // Populate door chunk data
        std::map<Door*, int> indexMapping;
        for(unsigned int i = 0; i < doors.size(); ++i)
        {
            indexMapping[doors[i]] = i;
        }
        for(unsigned int i = 0; i < doors.size(); ++i)
        {
            struct __DoorEntry entryStruct = doors[i]->GetEntryStruct();
            entryStruct.LinkerDestination = indexMapping[doors[i]->GetDestinationDoor()];
            memcpy(doorChunk.data + i * sizeof(struct __DoorEntry), &entryStruct, sizeof(struct __DoorEntry));
        }

        // Create the level name chunk
        struct ROMUtils::SaveData levelNameChunk =
        {
            LevelNamePtr,
            26,
            (unsigned char*) malloc(26),
            ROMUtils::SaveDataIndex++,
            false,
            0
        };
        memset(levelNameChunk.data, 0xFF, 26);
        unsigned char *str = levelNameChunk.data + (26 - LevelName.size()) / 2;
        foreach(char c, LevelName)
        {
            if(c <= 57) { c -= 48; }
            else if(c >= 65 && c <= 90) { c -= 55; }
            else { c -= 61; }
            *str++ = c;
        }

        chunks.append(roomHeaders);
        chunks.append(doorChunk);
        chunks.append(levelNameChunk);
    }
}
