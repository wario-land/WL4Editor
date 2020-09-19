#include "Level.h"
#include "ROMUtils.h"
#include "WL4Constants.h"
#include "WL4EditorWindow.h"
#include "DockWidget/CameraControlDockWidget.h"

#include <cassert>
#include <cstring>

extern WL4EditorWindow *singleton;

namespace LevelComponents
{
    /// <summary>
    /// Helper function to Generate LevelName available character QString.
    /// </summary>
    QString Level::GetAvailableLevelNameChars()
    {
        /*"0123456789ABCDEFGHIJKLMNOPQRSTUV"
        "WXYZabcdefghijklmnopqrstuvwxyz.&"
        "あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみ"
        "むめもやゆよらりるれろわをんぁぃぅぇぉゃゅょっがぎぐげござじずぜ"
        "ぞだぢづでどばびぶべぼぱぴぷぺぽアイウエオカキクケコサシスセソタ"
        "チツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲンァィ"
        "ゥェォャュョッガギグゲゴザジズゼゾダヂヅデドバビブベボパピプペポ"
        "ヴ'、。—~…!?()「」『』[]℃-"*/
        unsigned char tmpstr[484] = "\xe3\x81\x82\xe3\x81\x84\xe3\x81\x86\xe3\x81\x88\xe3\x81\x8a"
                                    "\xe3\x81\x8b\xe3\x81\x8d\xe3\x81\x8f\xe3\x81\x91\xe3\x81\x93"
                                    "\xe3\x81\x95\xe3\x81\x97\xe3\x81\x99\xe3\x81\x9b\xe3\x81\x9d"
                                    "\xe3\x81\x9f\xe3\x81\xa1\xe3\x81\xa4\xe3\x81\xa6\xe3\x81\xa8"
                                    "\xe3\x81\xaa\xe3\x81\xab\xe3\x81\xac\xe3\x81\xad\xe3\x81\xae"
                                    "\xe3\x81\xaf\xe3\x81\xb2\xe3\x81\xb5\xe3\x81\xb8\xe3\x81\xbb"
                                    "\xe3\x81\xbe\xe3\x81\xbf\xe3\x82\x80\xe3\x82\x81\xe3\x82\x82"
                                    "\xe3\x82\x84\xe3\x82\x86\xe3\x82\x88\xe3\x82\x89\xe3\x82\x8a"
                                    "\xe3\x82\x8b\xe3\x82\x8c\xe3\x82\x8d\xe3\x82\x8f\xe3\x82\x92"
                                    "\xe3\x82\x93\xe3\x81\x81\xe3\x81\x83\xe3\x81\x85\xe3\x81\x87"
                                    "\xe3\x81\x89\xe3\x82\x83\xe3\x82\x85\xe3\x82\x87\xe3\x81\xa3"
                                    "\xe3\x81\x8c\xe3\x81\x8e\xe3\x81\x90\xe3\x81\x92\xe3\x81\x94"
                                    "\xe3\x81\x96\xe3\x81\x98\xe3\x81\x9a\xe3\x81\x9c\xe3\x81\x9e"
                                    "\xe3\x81\xa0\xe3\x81\xa2\xe3\x81\xa5\xe3\x81\xa7\xe3\x81\xa9"
                                    "\xe3\x81\xb0\xe3\x81\xb3\xe3\x81\xb6\xe3\x81\xb9\xe3\x81\xbc"
                                    "\xe3\x81\xb1\xe3\x81\xb4\xe3\x81\xb7\xe3\x81\xba\xe3\x81\xbd"
                                    "\xe3\x82\xa2\xe3\x82\xa4\xe3\x82\xa6\xe3\x82\xa8\xe3\x82\xaa"
                                    "\xe3\x82\xab\xe3\x82\xad\xe3\x82\xaf\xe3\x82\xb1\xe3\x82\xb3"
                                    "\xe3\x82\xb5\xe3\x82\xb7\xe3\x82\xb9\xe3\x82\xbb\xe3\x82\xbd"
                                    "\xe3\x82\xbf\xe3\x83\x81\xe3\x83\x84\xe3\x83\x86\xe3\x83\x88"
                                    "\xe3\x83\x8a\xe3\x83\x8b\xe3\x83\x8c\xe3\x83\x8d\xe3\x83\x8e"
                                    "\xe3\x83\x8f\xe3\x83\x92\xe3\x83\x95\xe3\x83\x98\xe3\x83\x9b"
                                    "\xe3\x83\x9e\xe3\x83\x9f\xe3\x83\xa0\xe3\x83\xa1\xe3\x83\xa2"
                                    "\xe3\x83\xa4\xe3\x83\xa6\xe3\x83\xa8\xe3\x83\xa9\xe3\x83\xaa"
                                    "\xe3\x83\xab\xe3\x83\xac\xe3\x83\xad\xe3\x83\xaf\xe3\x83\xb2"
                                    "\xe3\x83\xb3\xe3\x82\xa1\xe3\x82\xa3\xe3\x82\xa5\xe3\x82\xa7"
                                    "\xe3\x82\xa9\xe3\x83\xa3\xe3\x83\xa5\xe3\x83\xa7\xe3\x83\x83"
                                    "\xe3\x82\xac\xe3\x82\xae\xe3\x82\xb0\xe3\x82\xb2\xe3\x82\xb4"
                                    "\xe3\x82\xb6\xe3\x82\xb8\xe3\x82\xba\xe3\x82\xbc\xe3\x82\xbe"
                                    "\xe3\x83\x80\xe3\x83\x82\xe3\x83\x85\xe3\x83\x87\xe3\x83\x89"
                                    "\xe3\x83\x90\xe3\x83\x93\xe3\x83\x96\xe3\x83\x99\xe3\x83\x9c"
                                    "\xe3\x83\x91\xe3\x83\x94\xe3\x83\x97\xe3\x83\x9a\xe3\x83\x9d"
                                    "\xe3\x83\xb4"; // Hiragana and Katakana
                                    //"\x27" //'
        unsigned char tmpstr2[10] = "\xe3\x80\x81\xe3\x80\x82\xe2\x80\x94"; // 、。—
                                    //"\x7e" //~
        unsigned char tmpstr3[4]  = "\xe2\x80\xa6"; //…
                                    //"\x21\x3f\x28\x29" //!?()
        unsigned char tmpstr4[13] = "\xe3\x80\x8c\xe3\x80\x8d\xe3\x80\x8e\xe3\x80\x8f"; //「」『』
                                    //"\x5b\x5d"//[]
        unsigned char tmpstr5[4]  = "\xe2\x84\x83"; //℃
                                    //"\x2d"; //-
        QString othercharacters = QString::fromUtf8((char *)tmpstr);
        othercharacters.prepend("0123456789ABCDEFGHIJKLMNOPQRSTUV"
                                "WXYZabcdefghijklmnopqrstuvwxyz.&");
        othercharacters.append("\x27");
        othercharacters.append(QString::fromUtf8((char *)tmpstr2));
        othercharacters.append("\x7e");
        othercharacters.append(QString::fromUtf8((char *)tmpstr3));
        othercharacters.append("\x21\x3f\x28\x29");
        othercharacters.append(QString::fromUtf8((char *)tmpstr4));
        othercharacters.append("\x5b\x5d");
        othercharacters.append(QString::fromUtf8((char *)tmpstr5));
        othercharacters.append("\x2d");
        return othercharacters;
    }


    /// <summary>
    /// Helper function to create a level name string from a data address in the ROM.
    /// </summary>
    /// <param name="address">
    /// Starting address of the level name string.
    /// </param>
    /// <returns>
    /// The string, as a QString
    /// </returns>
    QString Level::ConvertDataToLevelName(int address)
    {
        QString ret = "";
        QString avalialbechars = GetAvailableLevelNameChars();
        for (int i = 0; i < 26; i++)
        {
            unsigned char chr = ROMUtils::CurrentFile[address + i];
            if (chr < avalialbechars.size())
            {
                ret += avalialbechars.at(chr);
            }
            else // space
            {
                ret += ' ';
            }
        }
        return ret;
    }

    /// <summary>
    /// Helper function to write a QString level name to a 26-byte data buffer for saving
    /// </summary>
    /// <param name="levelName">
    /// The name of the level.
    /// </param>
    /// <param name="buffer">
    /// The buffer to write the level name to.
    /// </param>
    void Level::ConvertLevelNameToData(QString levelName, unsigned char *buffer)
    {
        QString charstable = GetAvailableLevelNameChars();
        for (unsigned int i = 0; i < 26; ++i)
        {
            buffer[i] = '\xFF';
            for(int j = 0; j < charstable.size(); ++j)
            {
                if(charstable.at(j) == levelName.at(i))
                {
                    buffer[i] = (unsigned char) j;
                    break;
                }
            }
        }
    }

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
        LevelID = ROMUtils::CurrentFile[levelHeaderPointer]; // 0x3000023

        memcpy(&LevelHeader, ROMUtils::CurrentFile + levelHeaderPointer, sizeof(struct __LevelHeader));

        // Load the door data
        std::vector<int> destinations;
        int doorStartAddress = ROMUtils::PointerFromData(WL4Constants::DoorTable + LevelID * 4);
        struct __DoorEntry *doorPtr = (struct __DoorEntry *) (ROMUtils::CurrentFile + doorStartAddress);
        unsigned char *firstByte;
        int currentDoornum = 0;
        while (*(firstByte = (unsigned char *) doorPtr))
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
        for (unsigned int i = 0; i < doors.size(); ++i)
        {
            doors[i]->SetDestinationDoor(doors[destinations[i]]);
        }
        // Set the first Door be Vortex Door
        doors[0]->SetAsVortex();

        // Load the room data
        int roomTableAddress = ROMUtils::PointerFromData(WL4Constants::RoomDataTable + LevelID * 4);
        int roomCount = ROMUtils::CurrentFile[levelHeaderPointer + 1];
        for (int i = 0; i < roomCount; i++)
        {
            rooms.push_back(new Room(roomTableAddress + i * 0x2C, i, LevelID));
        }

        // Distribute door data to every room
        RedistributeDoor();

        // Load the level names
        int LevelNameAddress =
            ROMUtils::PointerFromData(WL4Constants::LevelNamePointerTable + passage * 24 + stage * 4);
        LevelName = ConvertDataToLevelName(LevelNameAddress);
        LevelNameAddress =
            ROMUtils::PointerFromData(WL4Constants::LevelNameJPointerTable + passage * 24 + stage * 4);
        LevelNameJ = ConvertDataToLevelName(LevelNameAddress);

        // TODO
    }

    /// <summary>
    /// Deconstruct the Level and clean up its instance objects on the heap.
    /// </summary>
    Level::~Level()
    {
        for (auto iter = doors.begin(); iter != doors.end(); ++iter)
        {
            delete *iter; // Delete doors
        }
        for (auto iter = rooms.begin(); iter != rooms.end(); ++iter)
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
        if (LevelDifficulty == HardDifficulty)
        {
            LevelHeader.HardModeMinuteNum = (unsigned char) a;
            LevelHeader.HardModeSecondTenPlaceNum = (unsigned char) b;
            LevelHeader.HardModeSecondOnePlaceNum = (unsigned char) c;
        }
        else if (LevelDifficulty == NormalDifficulty)
        {
            LevelHeader.NormalModeMinuteNum = (unsigned char) a;
            LevelHeader.NormalModeSecondTenPlaceNum = (unsigned char) b;
            LevelHeader.NormalModeSecondOnePlaceNum = (unsigned char) c;
        }
        else if (LevelDifficulty == SHardDifficulty)
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
        if (LevelDifficulty == HardDifficulty)
        {
            a = (int) LevelHeader.HardModeMinuteNum;
            b = (int) LevelHeader.HardModeSecondTenPlaceNum;
            c = (int) LevelHeader.HardModeSecondOnePlaceNum;
        }
        else if (LevelDifficulty == NormalDifficulty)
        {
            a = (int) LevelHeader.NormalModeMinuteNum;
            b = (int) LevelHeader.NormalModeSecondTenPlaceNum;
            c = (int) LevelHeader.NormalModeSecondOnePlaceNum;
        }
        else if (LevelDifficulty == SHardDifficulty)
        {
            a = (int) LevelHeader.SHardModeMinuteNum;
            b = (int) LevelHeader.SHardModeSecondTenPlaceNum;
            c = (int) LevelHeader.SHardModeSecondOnePlaceNum;
        }
        return (a * 60 + b * 10 + c);
    }

    /// <summary>
    /// Distribute door data to every room.
    /// </summary>
    void Level::RedistributeDoor()
    {
        // Distribute door data to every room
        for (unsigned int i = 0; i < doors.size(); ++i)
        {
            rooms[doors[i]->GetRoomID()]->AddDoor(doors[i]);
        }

        // Check if every Room have at least one Door, if not, set the entityset id to skip some problems
        // the code is only for avoiding crash
        for (unsigned int i = 0; i < rooms.size(); ++i)
        {
            if (rooms[i]->CountDoors() == 0)
            {
                rooms[i]->SetCurrentEntitySet(37);
            }
        }
    }

    /// <summary>
    /// Get the Doors data copy.
    /// </summary>
    /// <param name="roomId">
    /// Room id of the Room the temp-Room copied from.
    /// </param>
    /// <remark>
    /// only used to give data to the Room copy.
    /// </remark>
    std::vector<Door *> Level::GetRoomDoors(unsigned int roomId)
    {
        std::vector<Door *> roomDoors;
        // Distribute door data
        for (unsigned int i = 0; i < doors.size(); ++i)
        {
            if (doors[i]->GetRoomID() == (int) roomId)
            {
                Door *newDoor = new Door(*doors[i]);
                roomDoors.push_back(newDoor);
            }
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
        delete (*(doors.begin() + globalDoorIndex));
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
        rooms[newdoor->GetRoomID()]->AddDoor(newdoor);
    }

    /// <summary>
    /// Populate a vector with save data chunks for a level.
    /// </summary>
    /// <param name="chunks">
    /// The vector to populate.
    /// </param>
    bool Level::GetSaveChunks(QVector<ROMUtils::SaveData> &chunks)
    {
        // Calculate some values needed to initialize the save data
        int offset = WL4Constants::LevelHeaderIndexTable + passage * 24 + stage * 4;
        int levelHeaderIndex = ROMUtils::IntFromData(offset);
        int levelHeaderPointer = WL4Constants::LevelHeaderTable + levelHeaderIndex * 12;
        int levelIndex = ROMUtils::CurrentFile[levelHeaderPointer];
        unsigned int roomTablePtr = WL4Constants::RoomDataTable + levelIndex * 4;
        unsigned int roomHeaderChunkSize = rooms.size() * sizeof(struct __RoomHeader);
        unsigned int doorTablePtr = WL4Constants::DoorTable + levelIndex * 4;
        unsigned int doorChunkSize = (doors.size() + 1) * sizeof(struct __DoorEntry);
        unsigned int LevelNamePtr = WL4Constants::LevelNamePointerTable + passage * 24 + stage * 4;
        unsigned int LevelNameJPtr = WL4Constants::LevelNameJPointerTable + passage * 24 + stage * 4;
        const unsigned int GBAptrSentinel = 0x8000000 | WL4Constants::CameraRecordSentinel;

        // Create the contiguous room header chunk
        struct ROMUtils::SaveData roomHeaders = { roomTablePtr,
                                                  roomHeaderChunkSize,
                                                  (unsigned char *) malloc(roomHeaderChunkSize),
                                                  ROMUtils::SaveDataIndex++,
                                                  true,
                                                  0,
                                                  ROMUtils::PointerFromData(roomTablePtr),
                                                  ROMUtils::SaveDataChunkType::RoomHeaderChunkType };

        // If level had camera limitators before saving, invalidate old chunks
        if(DataHasCameraLimitators())
        {
            // Camera boundary table chunk
            unsigned int cameraPointerTablePtr = WL4Constants::CameraControlPointerTable + LevelID * 4;
            unsigned int cameraBoundaryEntryAddress = ROMUtils::PointerFromData(cameraPointerTablePtr);
            struct ROMUtils::SaveData cameraPointerTableInvalidation = {
                0,
                0,
                nullptr,
                ROMUtils::SaveDataIndex++,
                false,
                0,
                cameraBoundaryEntryAddress,
                ROMUtils::SaveDataChunkType::InvalidationChunk
            };
            chunks.append(cameraPointerTableInvalidation);

            // Camera boundary chunks
            unsigned int iterations = 0;
            while(*(int*)(ROMUtils::CurrentFile + cameraBoundaryEntryAddress) != GBAptrSentinel)
            {
                if(++iterations > CameraControlDockWidget::MAX_CAMERA_LIMITATORS)
                {
                    singleton->GetOutputWidgetPtr()->PrintString(QString("Save error: Infinitely looping through camera boundary entries. Address: 0x") +
                        QString::number(ROMUtils::PointerFromData(cameraPointerTablePtr), 16).toUpper());
                    return false;
                }
                unsigned int cameraBoundaryListEntryPtr = ROMUtils::PointerFromData(cameraBoundaryEntryAddress);
                struct ROMUtils::SaveData invalidationEntry = {
                    0,
                    0,
                    nullptr,
                    ROMUtils::SaveDataIndex++,
                    false,
                    0,
                    cameraBoundaryListEntryPtr,
                    ROMUtils::SaveDataChunkType::InvalidationChunk
                };
                chunks.append(invalidationEntry);
                cameraBoundaryEntryAddress += 4;
            }
        }

        // If modified level has camera limitators, create new chunks
        struct ROMUtils::SaveData *cameraPointerTable = nullptr;
        if (rooms.end() != std::find_if(rooms.begin(), rooms.end(), [](Room *R) {
                return R->GetCameraControlType() == __CameraControlType::HasControlAttrs;
            }))
        {
            // Create the camera boundary pointer table save chunk
            unsigned int cameraPointerTablePtr = WL4Constants::CameraControlPointerTable + LevelID * 4;
            int boundaryEntries =
                std::count_if(rooms.begin(), rooms.end(), [](Room *R) { return R->GetCameraControlRecords().size(); });
            unsigned int cameraPointerTableSize = (boundaryEntries + 1) * 4;
            cameraPointerTable = (struct ROMUtils::SaveData *) malloc(sizeof(struct ROMUtils::SaveData));
            cameraPointerTable->ptr_addr = cameraPointerTablePtr;
            cameraPointerTable->size = cameraPointerTableSize;
            cameraPointerTable->data = (unsigned char *) malloc(cameraPointerTableSize);
            cameraPointerTable->index = ROMUtils::SaveDataIndex++;
            cameraPointerTable->alignment = true;
            cameraPointerTable->dest_index = 0;
            cameraPointerTable->old_chunk_addr = 0;
            cameraPointerTable->ChunkType = ROMUtils::CameraPointerTableType;
            *(int *) (cameraPointerTable->data + boundaryEntries * 4) = GBAptrSentinel;
        }

        // Populate chunks with room data
        unsigned int cameraPointerTableIndex = 0;
        for (unsigned int i = 0; i < rooms.size(); ++i)
        {
            struct __RoomHeader rh = rooms[i]->GetRoomHeader();
            memcpy(roomHeaders.data + i * sizeof(struct __RoomHeader), &rh, sizeof(struct __RoomHeader));
            rooms[i]->GetSaveChunks(chunks, &roomHeaders, cameraPointerTable, &cameraPointerTableIndex);
        }

        // Create door list chunk
        struct ROMUtils::SaveData doorChunk = { doorTablePtr,
                                                doorChunkSize,
                                                (unsigned char *) malloc(doorChunkSize),
                                                ROMUtils::SaveDataIndex++,
                                                true,
                                                0,
                                                ROMUtils::PointerFromData(doorTablePtr),
                                                ROMUtils::SaveDataChunkType::DoorChunkType };

        // Populate door chunk data
        std::map<Door *, int> indexMapping;
        for (unsigned int i = 0; i < doors.size(); ++i)
        {
            indexMapping[doors[i]] = i;
        }
        for (unsigned int i = 0; i < doors.size(); ++i)
        {
            struct __DoorEntry entryStruct = doors[i]->GetEntryStruct();
            entryStruct.LinkerDestination = indexMapping[doors[i]->GetDestinationDoor()];
            memcpy(doorChunk.data + i * sizeof(struct __DoorEntry), &entryStruct, sizeof(struct __DoorEntry));
        }
        memset(doorChunk.data + doors.size() * sizeof(struct __DoorEntry), 0, sizeof(struct __DoorEntry));

        // Create the level names chunk
        // En
        struct ROMUtils::SaveData levelNameChunk = { LevelNamePtr,
                                                     26,
                                                     (unsigned char *) malloc(26),
                                                     ROMUtils::SaveDataIndex++,
                                                     false,
                                                     0,
                                                     ROMUtils::PointerFromData(LevelNamePtr),
                                                     ROMUtils::SaveDataChunkType::LevelNameChunkType };
        QString levelName = QString(LevelName);
        if(levelName.length() != 26)
        {
            singleton->GetOutputWidgetPtr()->PrintString(QString("Internal error: English Level name has invalid length (") + levelName.length() + "): \"" + levelName + "\"");
            levelName.truncate(26);
        }
        ConvertLevelNameToData(levelName, levelNameChunk.data);
        // Jp
        struct ROMUtils::SaveData levelNameJChunk = { LevelNameJPtr,
                                                     26,
                                                     (unsigned char *) malloc(26),
                                                     ROMUtils::SaveDataIndex++,
                                                     false,
                                                     0,
                                                     ROMUtils::PointerFromData(LevelNameJPtr),
                                                     ROMUtils::SaveDataChunkType::LevelNameChunkType };
        QString levelNameJ = QString(LevelNameJ);
        if(levelNameJ.length() != 26)
        {
            singleton->GetOutputWidgetPtr()->PrintString(QString("Internal error: Japanese Level name has invalid length (") + levelNameJ.length() + "): \"" + levelName + "\"");
            levelNameJ.truncate(26);
        }
        ConvertLevelNameToData(levelNameJ, levelNameChunk.data);

        // Append all the save chunks which have been created
        chunks.append(roomHeaders);
        chunks.append(doorChunk);
        chunks.append(levelNameChunk);
        chunks.append(levelNameJChunk);
        if (cameraPointerTable)
        {
            chunks.append(*cameraPointerTable);
            free(cameraPointerTable);
        }
        return true;
    }

    /// <summary>
    /// This function does a low-level dig through the data to find if the current level
    /// has at least one camera limitator record
    /// </summary>
    /// <returns>
    /// true if one of the rooms uses camera limitators
    /// </returns>
    bool Level::DataHasCameraLimitators()
    {
        int offset = WL4Constants::LevelHeaderIndexTable + passage * 24 + stage * 4;
        int levelHeaderIndex = ROMUtils::IntFromData(offset);
        int levelHeaderPointer = WL4Constants::LevelHeaderTable + levelHeaderIndex * 12;
        struct __LevelHeader *levelHeader = (struct __LevelHeader*)(ROMUtils::CurrentFile + levelHeaderPointer);
        int roomTableAddress = ROMUtils::PointerFromData(WL4Constants::RoomDataTable + levelHeader->HeaderPointerIndex * 4);
        for(int i = 0; i < levelHeader->NumOfMap; ++i)
        {
            int roomDataPtr = roomTableAddress + i * 0x2C;
            struct __RoomHeader *roomHeader = (struct __RoomHeader*)(ROMUtils::CurrentFile + roomDataPtr);
            if(roomHeader->CameraControlType == __CameraControlType::HasControlAttrs)
            {
                return true;
            }
        }
        return false;
    }

} // namespace LevelComponents
