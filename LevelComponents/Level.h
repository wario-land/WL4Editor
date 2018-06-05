#ifndef LEVEL_H
#define LEVEL_H

#include "Room.h"
#include "Door.h"
#include <string>
#include <vector>

namespace LevelComponents
{
    // This enumeration defines the difficulty mode you are playing in.
    // Different difficulties have the same map data, but different sprites
    enum __LevelDifficulty
    {
        NormalDifficulty = 0,
        HardDifficulty   = 1,
        SHardDifficulty  = 2
    };

    enum __passage
    {
        EntryPassage = 0,
        EmeraldPassage = 1,
        RubyPassage = 2,
        TopazPassage = 3,
        SapphirePassage = 4,
        GoldenPassage = 5
    };

    enum __stage
    {
        FirstLevel = 0,
        SecondLevel = 1,
        ThirdLevel = 2,
        FourthLevel = 3,
        BossLevel = 4
    };

    // This structure is set up the same way the level header is organized in the ROM.
    struct __LevelHeader
    {
        unsigned char HeaderPointerIndex; // Multiply 4 make a shift from some base pointers
        unsigned char NumOfMap;           // Start from 1 so it's okay to initialize it by 0
        unsigned char Unknown0A;          // Always 0x0A ?
        unsigned char HardModeMinuteNum;
        unsigned char HardModeSecondTenPlaceNum;
        unsigned char HardModeSecondOnePlaceNum;
        unsigned char NormalModeMinuteNum;
        unsigned char NormalModeSecondTenPlaceNum;
        unsigned char NormalModeSecondOnePlaceNum;
        unsigned char SHardModeMinuteNum;
        unsigned char SHardModeSecondTenPlaceNum;
        unsigned char SHardModeSecondOnePlaceNum;
    };

    class Level
    {
    private:
        std::vector<Room*> rooms;
        std::string LevelName;
        std::vector<Door*> doors;
        __LevelHeader LevelHeader;
    public:
        Level(enum __passage passage, enum __stage stage); //TODO: need another construction function ? one for load a level and the other for create a level (ssp)
        void SetTimeCountdownCounter(enum __LevelDifficulty LevelDifficulty, unsigned int minutes, unsigned int seconds);
        std::vector<Door*> GetDoors() { return doors; }
        std::vector<Room*> GetRooms() { return rooms; }
    };
}

#endif // LEVEL_H
