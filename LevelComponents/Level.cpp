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
    Level::Level(int passage, int stage)
    {
        // Get the level header index
        int offset = WL4Constants::LevelHeaderIndexTable + passage * 24 + stage * 4;
        int levelHeaderIndex = ROMUtils::IntFromData(ROMUtils::CurrentFile, offset);

        // Load the level information
        int levelHeaderPointer = WL4Constants::LevelHeaderTable + levelHeaderIndex * 12;
        int levelIndex = ROMUtils::CurrentFile[levelHeaderPointer]; // 0x3000023

        memcpy(&this->LevelHeader, ROMUtils::CurrentFile + levelHeaderPointer, sizeof(struct __LevelHeader));

        // Load the door data
        std::vector<Door*> newDoors;
        std::vector<int> destinations;
        int doorStartAddress = ROMUtils::PointerFromData(ROMUtils::CurrentFile, WL4Constants::DoorTable + levelIndex * 4);
        struct __DoorEntry *doorPtr = (struct __DoorEntry*) (ROMUtils::CurrentFile + doorStartAddress);
        unsigned char *firstByte;
        while(*(firstByte = (unsigned char*) doorPtr))
        {
            enum DoorType type = static_cast<DoorType>(doorPtr->DoorTypeByte);
            Door *newDoor = new Door(doorPtr->RoomID, type, doorPtr->x1, doorPtr->x2, doorPtr->y1, doorPtr->y2);
            newDoor->SetSpriteMapID(doorPtr->SpriteMapID);
            newDoor->SetBGM(doorPtr->BGM_ID_LowByte | ((unsigned int) (doorPtr->BGM_ID_HighByte)) << 8);
            newDoor->SetDoorDisplacementROM(doorPtr->HorizontalDisplacement, doorPtr->VerticalDisplacement);
            newDoors.push_back(newDoor);
            destinations.push_back(doorPtr->LinkerDestination);
            ++doorPtr;
        }
        // Assign the destinations for the doors
        for(unsigned int i = 0; i < newDoors.size(); ++i)
        {
            newDoors[i]->SetDestinationDoor(newDoors[destinations[i]]);
        }
        this->doors = newDoors;

        // Load the room data
        int roomTableAddress = ROMUtils::PointerFromData(ROMUtils::CurrentFile, WL4Constants::RoomDataTable + levelIndex * 4);
        int roomCount = ROMUtils::CurrentFile[levelHeaderPointer + 1];
        for(int i = 0; i < roomCount; i++)
        {
            rooms.push_back(new Room(roomTableAddress + i * 0x2C, i, levelIndex));
        }

        // Load the level name
        int LevelNameAddress = ROMUtils::PointerFromData(ROMUtils::CurrentFile, WL4Constants::LevelNamePointerTable + 24 * passage + stage);
        ROMUtils::LevelNameFromData(ROMUtils::CurrentFile, LevelNameAddress, this->LevelName);

        // TODO

    }

    /// <summary>
    /// Set the countdown timer for a specific difficulty class.
    /// </summary>
    /// <param name="LevelDifficulty">An enumeration representing the level's difficulty</param>
    /// <param name="minutes">The number of minutes to set the timer for this difficulty level</param>
    /// <param name="seconds">The number of seconds to set the timer for this difficulty level</param>
    void Level::SetTimeCountdownCounter(enum __LevelDifficulty LevelDifficulty, unsigned int minutes, unsigned int seconds)
    {
        unsigned char a = (unsigned char) (minutes & 0xFF);
        unsigned char b = (unsigned char) ((seconds / 10) & 0xFF);
        unsigned char c = (unsigned char) ((seconds % 10) & 0xFF);
        if(LevelDifficulty == HardDifficulty)
        {
            this->LevelHeader.HardModeMinuteNum = a;
            this->LevelHeader.HardModeSecondTenPlaceNum = b;
            this->LevelHeader.HardModeSecondOnePlaceNum = c;
        }
        else if(LevelDifficulty == NormalDifficulty)
        {
            this->LevelHeader.NormalModeMinuteNum = a;
            this->LevelHeader.NormalModeSecondTenPlaceNum = b;
            this->LevelHeader.NormalModeSecondOnePlaceNum = c;
        }
        else if(LevelDifficulty == SHardDifficulty)
        {
            this->LevelHeader.SHardModeMinuteNum = a;
            this->LevelHeader.SHardModeSecondTenPlaceNum = b;
            this->LevelHeader.SHardModeSecondOnePlaceNum = c;
        }
    }
}
