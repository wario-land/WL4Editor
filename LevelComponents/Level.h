#ifndef LEVEL_H
#define LEVEL_H

#include "Room.h"
#include "Door.h"
#include <string>
#include <vector>

namespace LevelComponents
{
    enum __LevelDifficulty{Normal = 0, Hard = 1, SHard = 2};
    struct __LevelHeader
    {
        unsigned char HeaderPointerIndex;   //multiply 4 make a shift from some base pointers
        unsigned char NumOfMap;    //star from 1 so it's okey to initial it by 0
        unsigned char Unknown0A;   //always 0x0A
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
        std::vector<Room> rooms;
        std::string LevelName;
        std::vector<Door*> doors;
        __LevelHeader LevelHeader;
    public:
        Level(int passage, int stage); //TODO: need another construction function ? one for load a level and the other for create a level (ssp)
        void SetTimeCountdownCounter(enum __LevelDifficulty LevelDifficulty, unsigned char _MinuteNum, unsigned char _SecondTenPlaceNum, unsigned char _OnePlaceNum);// TODO void or bool ? (ssp)
    };
}

#endif // LEVEL_H
