#ifndef LEVEL_H
#define LEVEL_H

#include "LevelDoorVector.h"
#include "Room.h"
#include <string>
#include <vector>

namespace LevelComponents
{
    // This enumeration defines the difficulty mode you are playing in.
    // Different difficulties have the same map data, but different sprites
    enum __LevelDifficulty
    {
        NormalDifficulty = 0,
        HardDifficulty = 1,
        SHardDifficulty = 2
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
        std::vector<Room *> rooms;
        QString LevelName;
        QString LevelNameJ;
        LevelDoorVector doorlist;
        __LevelHeader LevelHeader;
        enum __passage passage;
        enum __stage stage;
        unsigned int LevelID;

        bool DataHasCameraLimitators();

    public:
        static QString GetAvailableLevelNameChars();
        static QString ConvertDataToLevelName(int address);
        static void ConvertLevelNameToData(QString levelName, unsigned char *buffer);

        Level(enum __passage passage, enum __stage stage);
        ~Level();

        void SetTimeCountdownCounter(enum __LevelDifficulty LevelDifficulty, unsigned int seconds);
        int GetTimeCountdownCounter(enum __LevelDifficulty LevelDifficulty);
        std::vector<Room *> GetRooms() { return rooms; }
        void AddRoom(Room *newroom) { rooms.push_back(newroom); }
        QString GetLevelName(int levelnameid = 0) { return levelnameid ? LevelNameJ : LevelName; }
        unsigned int GetLevelID() { return LevelID; }
        void SetLevelName(QString newlevelname, int levelnameid = 0) { (levelnameid ? LevelNameJ : LevelName) = newlevelname; }
        void InitLevelEntitySet();

        // Door stuff
        LevelDoorVector GetDoorList() {return LevelDoorVector(doorlist); } // rerurn a copy of doorlist
        LevelDoorVector &GetDoorListRef() {return doorlist; } // for fast editing or no-editing data extraction
        void SetDoorVec(LevelDoorVector newdoorlist) { doorlist = LevelDoorVector(newdoorlist); }
        QVector<struct DoorEntry> GetRoomDoorVec(unsigned int roomId) {return doorlist.GetDoorsByRoomID(roomId); }
        bool DeleteDoorByGlobalID(int globalDoorIndex) { return doorlist.DeleteDoor(globalDoorIndex); }
        void AddDoor(unsigned char roomId, unsigned char entitySetId = 1, unsigned char doorTypeId = 2)
        { doorlist.AddDoor(roomId, entitySetId, doorTypeId); }

        bool GetSaveChunks(QVector<struct ROMUtils::SaveData> &chunks);
        struct __LevelHeader *GetLevelHeader() { return &LevelHeader; }
        enum __passage GetPassage() { return passage; }
        enum __stage GetStage() { return stage; }
    };
} // namespace LevelComponents

#endif // LEVEL_H
