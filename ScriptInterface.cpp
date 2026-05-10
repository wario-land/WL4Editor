#include "ScriptInterface.h"

#include "Operation.h"
#include "ROMUtils.h"

#ifndef WINDOW_INSTANCE_SINGLETON
#define WINDOW_INSTANCE_SINGLETON
#include "WL4EditorWindow.h"
extern WL4EditorWindow *singleton;
#endif

#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ---------------------------Helper functions--------------------------------------
unsigned short *QStringToU16(QString input)
{
    int string_size = input.size();
    unsigned short *output = new unsigned short[string_size]; // larger than how much it needs, but won't be a big problem
    unsigned short *operation_ptr = output;
    memset((unsigned char *)output, 0, sizeof(unsigned short) * string_size);

    // deal with those \t, \r, \n and ',' and replace all of them by " " then split the QString
    // if the size of the result is still 0, then we read by characters, 4 letters for each unsigned short
    // assume the data is a string of hex numbers
    input.replace(QChar('\t'), QChar(' '), Qt::CaseSensitive);
    input.replace(QChar('\r'), QChar(' '), Qt::CaseSensitive);
    input.replace(QChar('\n'), QChar(' '), Qt::CaseSensitive);
    input.replace(QChar(','), QChar(' '), Qt::CaseSensitive);
    QStringList strList = input.split(QChar(' '), Qt::SkipEmptyParts);
    if (strList.size() > 1)
    {
        for (QString &data : strList)
        {
            *operation_ptr = data.toUInt(nullptr, 16) & 0xFFFF;
            operation_ptr++;
        }
    }
    else
    {
        int index = 0;
        while (index < input.size())
        {
            *operation_ptr = input.mid(index, 4).toUInt(nullptr, 16) & 0xFFFF;
            operation_ptr++;
            index += 4;
        }
    }
    return output;
}

// ------------------------------Public APIs----------------------------------------

ScriptInterface::ScriptInterface(QObject *parent) : QObject(parent)
{
    // installation
}

int ScriptInterface::GetCurRoomLayerWidth(int layerId)
{
    return static_cast<int>(singleton->GetCurrentRoom()->GetLayer(layerId)->GetLayerWidth());
}

int ScriptInterface::GetCurRoomLayerHeight(int layerId)
{
    return static_cast<int>(singleton->GetCurrentRoom()->GetLayer(layerId)->GetLayerHeight());
}

int ScriptInterface::GetCurRoomTile16(int layerID, int x, int y)
{
    LevelComponents::Layer *layer = singleton->GetCurrentRoom()->GetLayer(layerID);
    if(layer->GetMappingType() != LevelComponents::LayerMap16)
        return -1;
    int width = static_cast<int>(layer->GetLayerWidth());
    int height = static_cast<int>(layer->GetLayerHeight());
    if(x >= width || y >= height) {
        log("ScriptInterface::GetCurRoomTile16(): Tile position (0x" +
            QString::number(x, 16) + ", 0x" + QString::number(y, 16) +
            ") out of bounds on layer L (dimensions: 0x" + QString::number(width, 16) + ", 0x" + QString::number(height, 16) + ")");
        return -1;
    }
    return layer->GetTileData(x, y);
}

int ScriptInterface::GetCurRoomTile8(int layerID, int x, int y)
{
    LevelComponents::Layer *layer = singleton->GetCurrentRoom()->GetLayer(layerID);
    if(layer->GetMappingType() != LevelComponents::LayerTile8x8)
        return -1;
    int width = static_cast<int>(layer->GetLayerWidth());
    int height = static_cast<int>(layer->GetLayerHeight());
    if(x >= width || y >= height) {
        log("ScriptInterface::GetCurRoomTile8(): Tile position (0x" +
            QString::number(x, 16) + ", 0x" + QString::number(y, 16) +
            ") out of bounds on layer L (dimensions: 0x" + QString::number(width, 16) + ", 0x" + QString::number(height, 16) + ")");
        return -1;
    }
    return layer->GetTileData(x, y) & 0x3FF;
}

int ScriptInterface::GetRoomNum()
{
    return singleton->GetCurrentLevel()->GetRooms().size();
}

int ScriptInterface::GetCurRoomId()
{
    return singleton->GetCurrentRoomId();
}

int ScriptInterface::GetCurTilesetTile16EventId(unsigned short tile16Id)
{
    return singleton->GetCurrentRoom()->GetTileset()->GetEventTablePtr()[(tile16Id > 0x2FF) ? 0 : tile16Id];
}

int ScriptInterface::GetCurTilesetTile16TerrainType(unsigned short tile16Id)
{
    return singleton->GetCurrentRoom()->GetTileset()->GetTerrainTypeIDTablePtr()[(tile16Id > 0x2FF) ? 0 : tile16Id];
}

void ScriptInterface::_UnpackScreen(int address)
{
    unsigned short *LayerData = ROMUtils::UnPackScreen(address);
    QString tmpstr;
    for(int j = 0; j < 32; ++j) {
        for(int i = 0; i < 32; ++i) {
            tmpstr += " " + QString::number(LayerData[i + j * 32], 16).rightJustified(4, '0');
        }
        tmpstr += '\n';
    }
    log(tmpstr);
    delete[] LayerData;
    log("Done!");
}

void ScriptInterface::_PackScreen(QString inputData, bool skipzeros)
{
    unsigned short *data = QStringToU16(inputData);
    unsigned short *output = nullptr;
    int length = ROMUtils::PackScreen(data, output, skipzeros);
    log("Legnth of decompressed data: 0x" + QString::number(length, 16));
    log("Decompressed data:");
    QString tmpstr;
    for (int i = 0; i < length; ++i)
    {
        tmpstr += " " + QString::number(output[i], 16).rightJustified(4, '0');
    }
    log(tmpstr);
    delete[] output;
    log("Done!");
}

void ScriptInterface::_DecompressData(int mappingtype, int address)
{
    int tmpw = 0, tmph = 0;
    unsigned short *LayerData = nullptr;
    if((mappingtype & 0x20) == 0x20) {
        tmpw = (1 + (ROMUtils::ROMFileMetadata->ROMDataPtr[address] & 1)) << 5;
        tmph = (1 + ((ROMUtils::ROMFileMetadata->ROMDataPtr[address] >> 1) & 1)) << 5;
        LayerData = reinterpret_cast<unsigned short *>(ROMUtils::LayerRLEDecompress(address + 1, tmpw * tmph * 2));
        if (ROMUtils::ROMFileMetadata->ROMDataPtr[address] == 1)
        {
            unsigned short *rearranged = new unsigned short[tmpw * tmph * 2];
            for (int j = 0; j < 32; ++j)
            {
                for (int k = 0; k < 32; ++k)
                {
                    rearranged[(j << 6) + k] = LayerData[(j << 5) + k];
                    rearranged[(j << 6) + k + 32] = LayerData[(j << 5) + k + 1024];
                }
            }
            unsigned short *tmp = LayerData;
            LayerData = rearranged;
            delete[] tmp;
        }

    } else if((mappingtype & 0x10) == 0x10) {
        tmpw = ROMUtils::ROMFileMetadata->ROMDataPtr[address];
        tmph = ROMUtils::ROMFileMetadata->ROMDataPtr[address + 1];
        LayerData = reinterpret_cast<unsigned short *>(ROMUtils::LayerRLEDecompress(address + 2, tmpw * tmph * 2));
    } else {
        singleton->GetOutputWidgetPtr()->PrintString("Corruption error: Invalid layer mapping type: 0x" + QString::number(mappingtype, 16).toUpper());
        return;
    }
    QString tmpstr;
    for(int j = 0; j < tmph; ++j) {
        for(int i = 0; i < tmpw; ++i) {
            tmpstr += " " + QString::number(LayerData[i + j * tmpw], 16).rightJustified(4, '0');
        }
        tmpstr += '\n';
    }

    if(LayerData == nullptr) {
        singleton->GetOutputWidgetPtr()->PrintString("Corruption error: Decompression failure. Mapping type: 0x" +
            QString::number(mappingtype, 16).toUpper() + ". Address: 0x" + QString::number(address, 16).toUpper());
        return;
    }
    singleton->GetOutputWidgetPtr()->PrintString(tmpstr);
}

unsigned int ScriptInterface::_GetLayerDecomdataPointer(int layerId)
{
    switch(layerId)
    {
        case 0:
        {
            return singleton->GetCurrentRoom()->GetRoomHeader().Layer0Data;
            break;
        }
        case 1:
        {
            return singleton->GetCurrentRoom()->GetRoomHeader().Layer1Data;
            break;
        }
        case 2:
        {
            return singleton->GetCurrentRoom()->GetRoomHeader().Layer2Data;
            break;
        }
        case 3:
        {
            return singleton->GetCurrentRoom()->GetRoomHeader().Layer3Data;
            break;
        }
        default:
            return 0;
    }
}

void ScriptInterface::_PrintRoomHeader()
{
    LevelComponents::__RoomHeader header = singleton->GetCurrentRoom()->GetRoomHeader();
    QString roomheaderstr;
    for(size_t i = 0; i < sizeof(LevelComponents::__RoomHeader); i++)
    {
        roomheaderstr.push_back(QString::number(((unsigned char *)&header)[i], 16).toUpper());
        if((i + 1) % 4 == 0) roomheaderstr.push_back("  ");
        roomheaderstr.push_back(" ");
    }
    log(roomheaderstr);
}

void ScriptInterface::_ExportLayerData(QString filePath, int layerid)
{
    log("Export Layer Data from current Room.");
    if(!filePath.compare(""))
        filePath = QFileDialog::getSaveFileName(singleton, tr("Save Layer data file"), singleton->GetdDialogInitialPath(), tr("bin files (*.bin)"));
    if (filePath.compare(""))
    {
        if(layerid == -1)
            layerid = prompt("Input the Layer Id you want to save data:", "0").toInt();
        LevelComponents::Room *room = singleton->GetCurrentRoom();
        int witdh = 0, height = 0;
        if(layerid < 0 || layerid > 2)
        {
            log("Illegal Layer id!");
            return;
        }
        if((room->GetLayer(layerid)->GetMappingType() & 0x30) != LevelComponents::LayerMap16)
        {
            log("Illegal Layer mapping type!");
            return;
        }
        if(layerid == 0)
        {
            witdh = room->GetLayer0Width();
            height = room->GetLayer0Height();
        } else {
            witdh = room->GetLayer1Width();
            height = room->GetLayer1Height();
        }
        QFile file(filePath);
        file.open(QIODevice::WriteOnly);
        if (file.isOpen())
        {
            file.write(reinterpret_cast<const char*>(room->GetLayer(layerid)->GetLayerData()), 2 * witdh * height);
        } else {
            log("Cannot save data file!");
            return;
        }
        file.close();
    } else {
        log("Invalid file path!");
        return;
    }
    log("Done!");
}

void ScriptInterface::_ImportLayerData(QString fileName, int layerid)
{
    log("Import Layer Data from current Room.");
    // Load gfx bin file
    if(!fileName.compare(""))
    {
        fileName = QFileDialog::getOpenFileName(singleton,
                                                tr("Load Layer data bin file"),
                                                singleton->GetdDialogInitialPath(),
                                                tr("bin files (*.bin)"));
    }
    if (!fileName.compare(""))
    {
        log("Invalid file path!");
        return;
    }

    // load data into QBytearray
    QByteArray tmptile8x8data;
    QFile layerdatabinfile(fileName);
    int datasize = 0;
    if(!layerdatabinfile.open(QIODevice::ReadOnly))
    {
        log("Cannot open file!");
        return;
    }
    tmptile8x8data = layerdatabinfile.readAll();
    datasize = layerdatabinfile.size();
    layerdatabinfile.close();
    if(!datasize)
    {
        log("No available data in the file!");
        return;
    }

    if(layerid == -1)
        layerid = prompt("Input the Layer Id you choose to replace data:", "0").toInt();
    if(layerid < 0 || layerid > 2)
    {
        log("Illegal Layer id!");
        return;
    }

    // Paste data
    int witdh = 0, height = 0;
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    if((room->GetLayer(layerid)->GetMappingType() & 0x30) != LevelComponents::LayerMap16)
    {
        log("Illegal Layer mapping type!");
        return;
    }
    if(layerid == 0)
    {
        witdh = room->GetLayer0Width();
        height = room->GetLayer0Height();
    } else {
        witdh = room->GetLayer1Width();
        height = room->GetLayer1Height();
    }
    if(datasize != 2 * witdh * height)
    {
        log("File size not match (expected width: 0x" + QString::number(witdh, 16) + ", height: 0x" + QString::number(height, 16) + ")!");
        return;
    }
    memcpy(room->GetLayer(layerid)->GetLayerData(), tmptile8x8data.data(), 2 * witdh * height);
    room->GetLayer(layerid)->SetDirty(true);
    singleton->SetUnsavedChanges(true);
    singleton->RenderScreenFull();
    log("Done!");
}

void ScriptInterface::_GetTilesetGFXInfo(int tilesetId)
{
    if (tilesetId > 0x5B || tilesetId < 0)
    {
        log (tr("Illegal tilesetId. (0 <= tilesetId <= 0x4B)"));
        return;
    }
    LevelComponents::Tileset *tileset = ROMUtils::singletonTilesets[tilesetId];
    log("tileset 0x" + QString::number(tilesetId, 16) + ":");
    int fgGFXptr = tileset->GetfgGFXptr();
    log(" FG tile data address (Hex): 0x" + QString::number(fgGFXptr, 16));
    int fgGFXlen = tileset->GetfgGFXlen();
    log(" FG tile data length (Hex, Byte): 0x" + QString::number(fgGFXlen, 16));
    int bgGFXptr = tileset->GetbgGFXptr();
    log(" BG tile data address (Hex): 0x" + QString::number(bgGFXptr, 16));
    int bgGFXlen = tileset->GetbgGFXlen();
    log(" BG tile data length (Hex, Byte): 0x" + QString::number(bgGFXlen, 16));
    log(" BG tile offset in VRAM (Hex, Tile8x8): 0x" + QString::number(0x3FF - (bgGFXlen / 32), 16));
    unsigned int palptr = ROMUtils::PointerFromData(tileset->getTilesetPtr() + 8);
    log(" palette data address (Hex, Byte): 0x" + QString::number(palptr, 16));
}

void ScriptInterface::_ExtractSpriteOAMPackage(int address)
{
    if (address % 4)
    {
        log("Illegal address, the address value need to be a multiple of 4 to load the data pack correctly.");
        return;
    }
    if (address >= WL4Constants::AvailableSpaceBeginningInROM )
    {
        log("Illegal address, you should read oam data package from the vanilla ROM data area.");
        return;
    }
    log(tr("OAM data pack extract start from ") + QString::number(address, 16) + " in C format:");
    unsigned int oamdatapackPtr = ROMUtils::PointerFromData(address);
    int count_per_frame = ROMUtils::IntFromData(address + 4);
    QString oamdatatable = "const unsigned int oam_data_table[] = { ";
    int offset = 0;
    while (oamdatapackPtr)
    {
        unsigned short *data = (unsigned short *) (ROMUtils::ROMFileMetadata->ROMDataPtr + oamdatapackPtr);
        unsigned short oamnum = data[0];
        QString oamdata = "const unsigned short oam_data_0x" + QString::number(offset / 8, 16) + "[] = { 0x" + QString::number(oamnum, 16) + ", 0x";
        for (int i = 0; i < oamnum; i++)
        {
            oamdata += QString::number(data[i * 3 + 1], 16) + ", 0x" +
                       QString::number(data[i * 3 + 2], 16) + ", 0x" +
                       QString::number(data[i * 3 + 3], 16) + ", 0x";
        }
        oamdata.chop(4); // delete the last 4 chars ", 0x"
        oamdata += " };";
        log(oamdata);
        oamdatatable += "oam_data_0x" +
                        QString::number(offset / 8, 16) + ", 0x" +
                        QString::number(count_per_frame, 16) + ", ";

        // next package addresses prepare
        offset += 8;
        oamdatapackPtr = ROMUtils::PointerFromData(address + offset);
        count_per_frame = ROMUtils::IntFromData(address + 4 + offset);
    }
    oamdatatable += "0, 0 };";
    log(oamdatatable);
}

void ScriptInterface::_ExtractSpriteOAMPackage(QString address)
{
    _ExtractSpriteOAMPackage(address.toUInt(nullptr, 16));
}

QString ScriptInterface::GetEntityListData(int entitylistid)
{
    if(entitylistid < 0 || entitylistid > 2)
        entitylistid = prompt(tr("Illegal entitylist id, input it manually/n"
                                 "Input the Entity list Id you want to save data: 0(Hard) 1(Normal) 2(S Hard)"),
                              "0").toInt();
    if(entitylistid < 0 || entitylistid > 2)
    {
        log("Illegal Entity list id!");
        return "";
    }
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    std::vector<struct LevelComponents::EntityRoomAttribute> tmpvec = room->GetEntityListData(entitylistid);
    int size = tmpvec.size() * sizeof(struct LevelComponents::EntityRoomAttribute);
    if(!size) return "";
    QString result;
    for(auto entity: tmpvec)
    {
        result += "0x" + QString::number(entity.YPos, 16).toUpper() + QChar(' ');
        result += "0x" + QString::number(entity.XPos, 16).toUpper() + QChar(' ');
        result += "0x" + QString::number(entity.EntityID, 16).toUpper() + QChar(' ');
    }
    return result;
}

QString ScriptInterface::GetEntityListSource()
{
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    auto tmpvec = room->GetCurrentEntityListSource();
    if(!tmpvec.size()) return "";
    QString result;
    for(auto entity: tmpvec)
    {
        result += "0x" + QString::number(entity->GetEntityGlobalID(), 16).toUpper() + QChar(' ');
    }
    return result;
}

QString ScriptInterface::GetCurRoomAllDoorsRangeData()
{
    auto doordata = singleton->GetCurrentLevel()->GetDoorListRef().GetDoorsByRoomID(singleton->GetCurrentRoom()->GetRoomID());
    QString result = "";
    for (auto &door: doordata)
    {
        int x1 = door.x1;
        int x2 = door.x2;
        int y1 = door.y1;
        int y2 = door.y2;
        result += QString::number(x1, 10) + "," + QString::number(x2, 10) + "," + QString::number(y1, 10) + "," + QString::number(y2, 10) + ";";
    }
    result.chop(1); // get rid of the last ";"
    return result;
}

void ScriptInterface::PrintEntityDefaultOAMData(int globalEntityId)
{
    auto oamdata = LevelComponents::Entity::GetDefaultOAMData(globalEntityId);
    QString result = "no default oam data for this Entity.";
    if (oamdata.length() > 0)
    {
        result = "naked OAM data (no obj number in the first u16): ";
        for (int i = 0; i < (oamdata.length() / 3); i++)
        {
            result += "0x" + QString::number(oamdata[i * 3], 16) + ", 0x" + \
                    QString::number(oamdata[i * 3 + 1], 16) + ", 0x" + \
                    QString::number(oamdata[i * 3 + 2], 16) + ", ";
        }
        result.chop(2);
    }
    log(result);
}

void ScriptInterface::SetEntityListData(QString entitylistdata, int entitylistid)
{
    QStringList EntitylistStrData = entitylistdata.split(QChar(' '), Qt::SkipEmptyParts);
    if(!EntitylistStrData.size())
    {
        log("No available data in the String!");
        return;
    }
    if(EntitylistStrData.size() % 3)
    {
        log("Illegal string size! the size of the string must be a multiple of 3");
        return;
    }

    if(entitylistid < 0 || entitylistid > 2)
        entitylistid = prompt(tr("Illegal entitylist id, input it manually/n"
                                 "Input the Entity list Id you want to save data: 0(Hard) 1(Normal) 2(S Hard)"),
                              "0").toInt();
    if(entitylistid < 0 || entitylistid > 2)
    {
        log("Illegal Entity list id!");
        return;
    }
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    room->ClearEntitylist(entitylistid);
    for(int i = 0; i < (EntitylistStrData.size() / 3); ++i)
    {
        room->AddEntity(EntitylistStrData[3 * i + 1].toUInt(nullptr, 16),
                        EntitylistStrData[3 * i].toUInt(nullptr, 16),
                        EntitylistStrData[3 * i + 2].toUInt(nullptr, 16),
                        entitylistid);
    }
    room->SetEntityListDirty(entitylistid, true);
    singleton->SetUnsavedChanges(true);
    singleton->RenderScreenFull();
}

void ScriptInterface::SetCurrentRoomId(int roomid)
{
    singleton->SetCurrentRoomId(roomid);
}

int ScriptInterface::AddNewRoom()
{
    LevelComponents::Level *level = singleton->GetCurrentLevel();
    int newRoomId = level->GetRooms().size();
    if (newRoomId >= 16)
    {
        log("Cannot add another Room to the current Level (max 16).");
        return -1;
    }
    LevelComponents::Room *currentRoom = singleton->GetCurrentRoom();
    int entitySetId = currentRoom->GetCurrentEntitySetID();
    int tilesetId = currentRoom->GetTilesetID();
    level->AddRoom(new LevelComponents::Room(newRoomId, level->GetLevelID(),
                                              tilesetId, entitySetId));
    level->AddDoor(newRoomId, entitySetId);
    level->GetLevelHeader()->NumOfMap++;
    singleton->SetUnsavedChanges(true);
    LevelComponents::Room *newRoom = level->GetRooms()[newRoomId];
    newRoom->SetEntityListDirty(0, true);
    newRoom->SetEntityListDirty(1, true);
    newRoom->SetEntityListDirty(2, true);
    log("Added new room with ID " + QString::number(newRoomId));
    return newRoomId;
}

void ScriptInterface::SetCurRoomTile16(int layerID, int TileID, int x, int y)
{
    if(layerID > 2 || layerID < 0) {
        log(QString("Illegal layer ID!\n"));
        return;
    }
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    if(room->GetLayer(layerID)->GetMappingType() != LevelComponents::LayerMap16)
        return;
    int width = static_cast<int>(room->GetLayer1Width());
    int height = static_cast<int>(room->GetLayer1Height());
    if(x >= width || y >= height) {
        log(QString("Position out of range!\n"));
        return;
    }
    room->GetLayer(layerID)->SetTileData(TileID & 0xFFFF, x, y);
    room->GetLayer(layerID)->SetDirty(true);
    singleton->SetUnsavedChanges(true);
}

void ScriptInterface::SetRoomSize(int roomwidth, int roomheight, int layer0width, int layer0height)
{
    if(roomwidth < 19 || layer0width < 19 || roomheight < 14 || layer0height < 14)
    {
        log("Room size and Layer size too small, Must be bigger than (18, 13).");
        return;
    }
    // Set up parameters for the currently selected room, for the purpose of initializing the dialog's selections
    DialogParams::RoomConfigParams *_currentRoomConfigParams =
        new DialogParams::RoomConfigParams(singleton->GetCurrentRoom());
    DialogParams::RoomConfigParams *_nextRoomConfigParams =
        new DialogParams::RoomConfigParams(singleton->GetCurrentRoom());

    _nextRoomConfigParams->RoomWidth = roomwidth;
    _nextRoomConfigParams->RoomHeight = roomheight;
    _nextRoomConfigParams->Layer0Width = layer0width;
    _nextRoomConfigParams->Layer0Height = layer0height;

    // Reset Layers, iterate 0, 1, 2, logic from DialogParams::RoomConfigParams *RoomConfigDialog::GetConfigParams(...)
    if((_nextRoomConfigParams->Layer0MappingTypeParam & 0x30) == LevelComponents::LayerMap16) {
        _nextRoomConfigParams->LayerData[0] = RoomConfigDialog::ChangeLayerDimensions(layer0width, layer0height,
                                                                                      _currentRoomConfigParams->Layer0Width,
                                                                                      _currentRoomConfigParams->Layer0Height,
                                                                                      _currentRoomConfigParams->LayerData[0]);
    } else {
        _nextRoomConfigParams->LayerData[0] = nullptr;
    }
    _nextRoomConfigParams->LayerData[1] = RoomConfigDialog::ChangeLayerDimensions(roomwidth, roomheight,
                                                                                  _currentRoomConfigParams->RoomWidth,
                                                                                  _currentRoomConfigParams->RoomHeight,
                                                                                  _currentRoomConfigParams->LayerData[1]);
    if((_nextRoomConfigParams->Layer2MappingTypeParam & 0x30) == LevelComponents::LayerMap16) {
        _nextRoomConfigParams->LayerData[2] = RoomConfigDialog::ChangeLayerDimensions(roomwidth, roomheight,
                                                                                      _currentRoomConfigParams->RoomWidth,
                                                                                      _currentRoomConfigParams->RoomHeight,
                                                                                      _currentRoomConfigParams->LayerData[2]);
    } else {
        _nextRoomConfigParams->LayerData[2] = nullptr;
    }

    // Add changes into the operation history
    OperationParams *operation = new OperationParams;
    operation->roomConfigChange = true;
    operation->lastRoomConfigParams = _currentRoomConfigParams;
    operation->newRoomConfigParams = _nextRoomConfigParams;

    // Capture door, entity, and camera states when room dimensions change
    if (_nextRoomConfigParams->RoomWidth != _currentRoomConfigParams->RoomWidth ||
        _nextRoomConfigParams->RoomHeight != _currentRoomConfigParams->RoomHeight)
    {
        LevelComponents::Room *currentRoom = singleton->GetCurrentLevel()->GetRooms()[singleton->GetCurrentRoomId()];

        LevelComponents::LevelDoorVector *oldDoorVec =
            new LevelComponents::LevelDoorVector(singleton->GetCurrentLevel()->GetDoorList());

        std::vector<LevelComponents::EntityRoomAttribute> oldNormal = currentRoom->GetEntityListData(1);
        std::vector<LevelComponents::EntityRoomAttribute> oldHard = currentRoom->GetEntityListData(0);
        std::vector<LevelComponents::EntityRoomAttribute> oldSHard = currentRoom->GetEntityListData(2);

        auto oldCameraType = currentRoom->GetCameraControlType();
        auto oldCameraRecords = currentRoom->GetCameraControlRecords(true);

        singleton->TrimElementsOutOfRoomBounds(currentRoom,
                                               _nextRoomConfigParams->RoomWidth,
                                               _nextRoomConfigParams->RoomHeight);

        LevelComponents::LevelDoorVector *newDoorVec =
            new LevelComponents::LevelDoorVector(singleton->GetCurrentLevel()->GetDoorList());

        std::vector<LevelComponents::EntityRoomAttribute> newNormal = currentRoom->GetEntityListData(1);
        std::vector<LevelComponents::EntityRoomAttribute> newHard = currentRoom->GetEntityListData(0);
        std::vector<LevelComponents::EntityRoomAttribute> newSHard = currentRoom->GetEntityListData(2);

        auto newCameraType = currentRoom->GetCameraControlType();
        auto newCameraRecords = currentRoom->GetCameraControlRecords(true);

        operation->doorVectorChange = true;
        operation->doorVectorChangeParams = DoorVectorChangeParams::Create(oldDoorVec, newDoorVec);

        operation->entityNormalChange = true;
        operation->entityNormalChangeParams = EntityListChangeParams::Create(oldNormal, newNormal, currentRoom->GetRoomID());
        operation->entityHardChange = true;
        operation->entityHardChangeParams = EntityListChangeParams::Create(oldHard, newHard, currentRoom->GetRoomID());
        operation->entitySHardChange = true;
        operation->entitySHardChangeParams = EntityListChangeParams::Create(oldSHard, newSHard, currentRoom->GetRoomID());

        operation->cameraControlChange = true;
        operation->cameraControlChangeParams = CameraControlChangeParams::Create(
            currentRoom->GetRoomID(),
            oldCameraType, newCameraType,
            oldCameraRecords, newCameraRecords);
    }

    ExecuteOperation(operation); // Set UnsavedChanges bool inside
}

void ScriptInterface::alert(QString message)
{
    QMessageBox::critical(singleton, QString("Error"), message);
}

void ScriptInterface::clear()
{
    singleton->GetOutputWidgetPtr()->ClearTextEdit();
}

void ScriptInterface::log(QString message)
{
    singleton->GetOutputWidgetPtr()->PrintString(message);
}

QString ScriptInterface::prompt(QString message, QString defaultInput)
{
    bool ok;
    QString text = QInputDialog::getText(nullptr, tr("InputBox"),
                                         message, QLineEdit::Normal,
                                         defaultInput, &ok);
    if (ok && !text.isEmpty())
        return text;
    else
        return QString("");
}

void ScriptInterface::UpdateRoomGFXFull()
{
    singleton->RenderScreenFull();
}

void ScriptInterface::DoEvents()
{
    QApplication::processEvents();
}

void ScriptInterface::WriteTxtFile(QString filepath, QString test)
{
    if(!filepath.compare(""))
        filepath = QFileDialog::getSaveFileName(singleton, tr("Save Entity list data file"), singleton->GetdDialogInitialPath(), tr("bin files (*.bin)"));
    if(!filepath.compare(""))
    {
        log("Invalid file path!");
        return;
    }
    QFile file(filepath);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // Stream text to the file
        QTextStream out(&file);
        out << test;

        file.close();
        log("Writing finished");
    }
    else
    {
        log("Write file failed !");
    }
}

void ScriptInterface::WriteJsonFile(QString filepath, QString test)
{
    if(!filepath.compare(""))
        filepath = QFileDialog::getSaveFileName(singleton, tr("Save JSON file"), singleton->GetdDialogInitialPath(), tr("JSON files (*.json)"));
    if(!filepath.compare(""))
    {
        log("Invalid file path!");
        return;
    }
    QFile file(filepath);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        out << test;
        file.close();
        log("Writing finished");
    }
    else
    {
        log("Write file failed !");
    }
}

QString ScriptInterface::ReadTxtFile(QString filepath)
{
    QFile f(filepath);
    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        log("Read file failed !");
        return QString();
    }
    QTextStream in(&f);
    return in.readAll();
}

QString ScriptInterface::ReadJsonFileDialog()
{
    QString filepath = QFileDialog::getOpenFileName(singleton, tr("Open JSON file"),
        singleton->GetdDialogInitialPath(), tr("JSON files (*.json)"));
    if (filepath.isEmpty()) return QString();
    QFile f(filepath);
    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        log("Read file failed !");
        return QString();
    }
    return QTextStream(&f).readAll();
}

void ScriptInterface::ShowSaveDataAnalysis()
{
    log(ROMUtils::SaveDataAnalysis());
}

// ------------------------------ Export/Import APIs ----------------------------------------

// ---- Room Export Getters ----

int ScriptInterface::ExportGetLayerDataPtr(int layerId)
{
    return (int) singleton->GetCurrentRoom()->GetLayerDataPtr(layerId);
}

int ScriptInterface::ExportGetTilesetId()
{
    return singleton->GetCurrentRoom()->GetTilesetID();
}

int ScriptInterface::ExportGetEntitySetId()
{
    return singleton->GetCurrentRoom()->GetCurrentEntitySetID();
}

int ScriptInterface::ExportGetCameraControlType()
{
    return (int) singleton->GetCurrentRoom()->GetCameraControlType();
}

QString ScriptInterface::ExportGetCameraControlRecords()
{
    return singleton->GetCurrentRoom()->GetCameraControlRecordsString();
}

QString ScriptInterface::ExportGetDoorsFullData()
{
    auto doordata = singleton->GetCurrentLevel()->GetDoorListRef().GetDoorsByRoomID(
        singleton->GetCurrentRoom()->GetRoomID());
    QString result;
    for (auto &door : doordata)
    {
        result += QString::number(door.DoorTypeByte) + "," +
                  QString::number(door.RoomID) + "," +
                  QString::number(door.x1) + "," +
                  QString::number(door.x2) + "," +
                  QString::number(door.y1) + "," +
                  QString::number(door.y2) + "," +
                  QString::number(door.DestinationDoorGlobalID) + "," +
                  QString::number(door.HorizontalDeltaWario) + "," +
                  QString::number(door.VerticalDeltaWario) + "," +
                  QString::number(door.EntitySetID) + "," +
                  QString::number(door.BGM_ID) + ";";
    }
    if (!result.isEmpty())
        result.chop(1);
    return result;
}

QString ScriptInterface::ExportGetRoomHeaderHex()
{
    return singleton->GetCurrentRoom()->GetRoomHeaderHex();
}

// ---- Level Export Getters ----

QString ScriptInterface::ExportGetLevelName()
{
    return singleton->GetCurrentLevel()->GetLevelName();
}

QString ScriptInterface::ExportGetLevelNameJ()
{
    return singleton->GetCurrentLevel()->GetLevelName(1);
}

int ScriptInterface::ExportGetLevelTimerSeconds(int difficulty)
{
    return singleton->GetCurrentLevel()->GetTimeCountdownCounter(
        static_cast<LevelComponents::__LevelDifficulty>(difficulty));
}

int ScriptInterface::ExportGetLevelPassage()
{
    return (int) singleton->GetCurrentLevel()->GetPassage();
}

int ScriptInterface::ExportGetLevelStage()
{
    return (int) singleton->GetCurrentLevel()->GetStage();
}

// ---- Changed Globals Detection ----

QString ScriptInterface::ExportGetChangedTilesetIds()
{
    QString result;
    for (int i = 0; i < 92; ++i)
    {
        if (ROMUtils::singletonTilesets[i] && ROMUtils::singletonTilesets[i]->IsNewTileset())
        {
            if (!result.isEmpty()) result += ",";
            result += QString::number(i);
        }
    }
    return result;
}

QString ScriptInterface::ExportGetChangedEntityIds()
{
    QString result;
    for (int i = 0; i < 129; ++i)
    {
        if (ROMUtils::entities[i] && ROMUtils::entities[i]->IsNewEntity())
        {
            if (!result.isEmpty()) result += ",";
            result += QString::number(i);
        }
    }
    return result;
}

QString ScriptInterface::ExportGetChangedEntitySetIds()
{
    QString result;
    for (int i = 0; i < 90; ++i)
    {
        if (ROMUtils::entitiessets[i] && ROMUtils::entitiessets[i]->IsNewEntitySet())
        {
            if (!result.isEmpty()) result += ",";
            result += QString::number(i);
        }
    }
    return result;
}

QString ScriptInterface::ExportGetChangedAnimatedTileGroupIds()
{
    QString result;
    for (int i = 0; i < 270; ++i)
    {
        if (ROMUtils::animatedTileGroups[i] && ROMUtils::animatedTileGroups[i]->IsNewAnimatedTile8x8Group())
        {
            if (!result.isEmpty()) result += ",";
            result += QString::number(i);
        }
    }
    return result;
}

// ---- Tileset Export Getters ----

int ScriptInterface::ExportGetTilesetFGGFXPtr(int id)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return 0;
    return ROMUtils::singletonTilesets[id]->GetfgGFXptr();
}

int ScriptInterface::ExportGetTilesetFGGFXLen(int id)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return 0;
    return ROMUtils::singletonTilesets[id]->GetfgGFXlen();
}

int ScriptInterface::ExportGetTilesetBGGFXPtr(int id)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return 0;
    return ROMUtils::singletonTilesets[id]->GetbgGFXptr();
}

int ScriptInterface::ExportGetTilesetBGGFXLen(int id)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return 0;
    return ROMUtils::singletonTilesets[id]->GetbgGFXlen();
}

int ScriptInterface::ExportGetTilesetMap16Ptr(int id)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return 0;
    return ROMUtils::singletonTilesets[id]->GetMap16Ptr();
}

int ScriptInterface::ExportGetTilesetTile8x8Count(int id)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return 0;
    return ROMUtils::singletonTilesets[id]->GetTile8x8Count();
}

QString ScriptInterface::ExportGetTilesetTile8x8DataHex(int id, int index)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return "";
    return ROMUtils::singletonTilesets[id]->GetTile8x8DataHex(index);
}

int ScriptInterface::ExportGetTilesetMap16Count(int id)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return 0;
    return ROMUtils::singletonTilesets[id]->GetMap16Count();
}

QString ScriptInterface::ExportGetTilesetMap16DataHex(int id, int index)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return "";
    return ROMUtils::singletonTilesets[id]->GetMap16DataHex(index);
}

QString ScriptInterface::ExportGetTilesetPalettesHex(int id)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return "";
    return ROMUtils::singletonTilesets[id]->GetAllPalettesHex();
}

QString ScriptInterface::ExportGetTilesetEventTableHex(int id)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return "";
    return ROMUtils::singletonTilesets[id]->GetEventTableDataHex();
}

QString ScriptInterface::ExportGetTilesetTerrainTableHex(int id)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return "";
    return ROMUtils::singletonTilesets[id]->GetTerrainTableDataHex();
}

QString ScriptInterface::ExportGetTilesetAnimatedSwitchTableHex(int id)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return "";
    return ROMUtils::singletonTilesets[id]->GetAnimatedSwitchTableHex();
}

QString ScriptInterface::ExportGetTilesetAnimatedTileDataHex(int id, int switchState)
{
    if (id < 0 || id >= 92 || !ROMUtils::singletonTilesets[id]) return "";
    return ROMUtils::singletonTilesets[id]->GetAnimatedTileDataHex(switchState);
}

// ---- Entity Export Getters ----

int ScriptInterface::ExportGetEntityPaletteCount(int id)
{
    if (id < 0 || id >= 129 || !ROMUtils::entities[id]) return 0;
    return ROMUtils::entities[id]->GetPalNum();
}

QString ScriptInterface::ExportGetEntityPaletteDataHex(int id, int paletteId)
{
    if (id < 0 || id >= 129 || !ROMUtils::entities[id]) return "";
    return ROMUtils::entities[id]->GetPaletteDataHex(paletteId);
}

int ScriptInterface::ExportGetEntityTile8x8Count(int id)
{
    if (id < 0 || id >= 129 || !ROMUtils::entities[id]) return 0;
    return ROMUtils::entities[id]->GetTilesNum();
}

QString ScriptInterface::ExportGetEntityTile8x8DataHex(int id, int index)
{
    if (id < 0 || id >= 129 || !ROMUtils::entities[id]) return "";
    return ROMUtils::entities[id]->GetTile8x8DataHex(index);
}

// ---- EntitySet Export Getters ----

int ScriptInterface::ExportGetEntitySetInfoTableSize(int id)
{
    if (id < 0 || id >= 90 || !ROMUtils::entitiessets[id]) return 0;
    return ROMUtils::entitiessets[id]->GetEntityInfoTableSize();
}

QString ScriptInterface::ExportGetEntitySetInfoEntry(int id, int index)
{
    if (id < 0 || id >= 90 || !ROMUtils::entitiessets[id]) return "";
    return ROMUtils::entitiessets[id]->GetEntityInfoEntry(index);
}

// ---- AnimatedTile8x8Group Export Getters ----

int ScriptInterface::ExportGetAnimatedTileGroupAnimType(int id)
{
    if (id < 0 || id >= 270 || !ROMUtils::animatedTileGroups[id]) return 0;
    return ROMUtils::animatedTileGroups[id]->GetAnimationType();
}

int ScriptInterface::ExportGetAnimatedTileGroupCountPerFrame(int id)
{
    if (id < 0 || id >= 270 || !ROMUtils::animatedTileGroups[id]) return 0;
    return ROMUtils::animatedTileGroups[id]->GetCountPerFrame();
}

int ScriptInterface::ExportGetAnimatedTileGroupTotalFrameCount(int id)
{
    if (id < 0 || id >= 270 || !ROMUtils::animatedTileGroups[id]) return 0;
    return ROMUtils::animatedTileGroups[id]->GetTotalFrameCount();
}

int ScriptInterface::ExportGetAnimatedTileGroupTileCount(int id)
{
    if (id < 0 || id >= 270 || !ROMUtils::animatedTileGroups[id]) return 0;
    return ROMUtils::animatedTileGroups[id]->GetTile8x8Count();
}

QString ScriptInterface::ExportGetAnimatedTileGroupTileDataHex(int id, int index)
{
    if (id < 0 || id >= 270 || !ROMUtils::animatedTileGroups[id]) return "";
    return ROMUtils::animatedTileGroups[id]->GetTileDataHex(index);
}

// ---- WallPaint/Credits Export ----

// ---- Helper: enumerate wall paint scattered blocks from ROM pointer tables ----
struct WallPaintScatteredBlockEntry
{
    unsigned int romAddr;
    unsigned int size;
};

static std::vector<WallPaintScatteredBlockEntry> EnumerateWallPaintScatteredBlocks()
{
    std::vector<WallPaintScatteredBlockEntry> blocks;
    for (int passage = 0; passage < 6; passage++)
    {
        unsigned int mmapAddr = ROMUtils::PointerFromData(
            WL4Constants::WallPaintPalSixInOneMMapColorPtrTable + 4 * passage);

        for (int level = 0; level < 4; level++)
        {
            unsigned int gradAddr = ROMUtils::PointerFromData(
                WL4Constants::WallPaintPalStartLevelPointerTable + passage * 16 + level * 4);
            if (!gradAddr) continue;

            // Gradient palette data (256 bytes = 8 palettes x 32 bytes)
            blocks.push_back({gradAddr, 256});

            // MMAP palette for each regular level (32 bytes)
            blocks.push_back({mmapAddr + 32 * (0xA + level), 32});
        }

        // Boss level MMAP palette (32 bytes)
        blocks.push_back({mmapAddr + 32 * (0xA + 4), 32});
    }
    return blocks;
}

QString ScriptInterface::ExportGetWallPaintGFXHex()
{
    QByteArray bytes(
        reinterpret_cast<const char *>(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintGFXAddr]),
        1024 * 5 * 6);
    return QString::fromLatin1(bytes.toHex());
}

QString ScriptInterface::ExportGetWallPaintPassageColorHex()
{
    QByteArray bytes(
        reinterpret_cast<const char *>(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageColor]),
        32 * 5 * 6);
    return QString::fromLatin1(bytes.toHex());
}

QString ScriptInterface::ExportGetWallPaintPassageGrayHex()
{
    QByteArray bytes(
        reinterpret_cast<const char *>(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageGray]),
        32 * 5 * 6);
    return QString::fromLatin1(bytes.toHex());
}

int ScriptInterface::ExportGetWallPaintScatteredBlockCount()
{
    auto blocks = EnumerateWallPaintScatteredBlocks();
    return static_cast<int>(blocks.size());
}

int ScriptInterface::ExportGetWallPaintScatteredBlockAddr(int index)
{
    auto blocks = EnumerateWallPaintScatteredBlocks();
    if (index < 0 || index >= static_cast<int>(blocks.size())) return 0;
    return static_cast<int>(blocks[index].romAddr);
}

int ScriptInterface::ExportGetWallPaintScatteredBlockSize(int index)
{
    auto blocks = EnumerateWallPaintScatteredBlocks();
    if (index < 0 || index >= static_cast<int>(blocks.size())) return 0;
    return static_cast<int>(blocks[index].size);
}

QString ScriptInterface::ExportGetWallPaintScatteredBlockDataHex(int index)
{
    auto blocks = EnumerateWallPaintScatteredBlocks();
    if (index < 0 || index >= static_cast<int>(blocks.size())) return QString();
    QByteArray bytes(
        reinterpret_cast<const char *>(&ROMUtils::ROMFileMetadata->ROMDataPtr[blocks[index].romAddr]),
        blocks[index].size);
    return QString::fromLatin1(bytes.toHex());
}

QString ScriptInterface::ExportGetCreditsDataHex()
{
    QByteArray bytes(
        reinterpret_cast<const char *>(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::CreditsTiles]),
        NUMBEROFCREDITSSCREEN * 1280);
    return QString::fromLatin1(bytes.toHex());
}

// ---- Import APIs ----

// ---- Room Import APIs ----

bool ScriptInterface::ImportRoomConfig(int roomWidth, int roomHeight, int layer0Width, int layer0Height,
                                       QString roomHeaderHex)
{
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    int roomId = singleton->GetCurrentRoomId();
    LevelComponents::Level *level = singleton->GetCurrentLevel();

    int curRoomWidth = (int) room->GetLayer1Width();
    int curRoomHeight = (int) room->GetLayer1Height();
    int curLayer0Width = (int) room->GetLayer0Width();
    int curLayer0Height = (int) room->GetLayer0Height();

    bool dimsChanged = (roomWidth != curRoomWidth || roomHeight != curRoomHeight ||
                        layer0Width != curLayer0Width || layer0Height != curLayer0Height);

    DialogParams::RoomConfigParams *currentParams = new DialogParams::RoomConfigParams(room);
    DialogParams::RoomConfigParams *newParams = new DialogParams::RoomConfigParams(room);
    newParams->RoomWidth = roomWidth;
    newParams->RoomHeight = roomHeight;
    newParams->Layer0Width = layer0Width;
    newParams->Layer0Height = layer0Height;

    // Parse roomHeaderHex into __RoomHeader struct
    struct LevelComponents::__RoomHeader header;
    memset(&header, 0, sizeof(header));
    unsigned char *raw = (unsigned char *) &header;
    for (int i = 0; i < (int) sizeof(header) && i * 2 < roomHeaderHex.length(); ++i)
        raw[i] = (unsigned char) roomHeaderHex.mid(i * 2, 2).toInt(nullptr, 16);

    // Apply header fields
    newParams->CurrentTilesetIndex = header.TilesetID;
    newParams->Layer0MappingTypeParam = header.Layer0MappingType; // preserves extra bits
    newParams->Layer2MappingTypeParam = header.Layer2MappingType; // preserves extra bits
    newParams->BackgroundLayerEnable = (header.Layer3MappingType != 0);
    newParams->BGMVolume = header.BGMVolume;
    newParams->LayerPriorityAndAlphaAttr = header.RenderEffect;
    newParams->RasterType = header.LayerGFXEffect01;
    newParams->Water = header.LayerGFXEffect02;
    newParams->BGLayerScrollFlag = header.Layer3Scrolling;

    // Handle Layer 3 DataPtr preservation
    if (header.Layer3MappingType != 0)
    {
        if (header.Layer3Data < WL4Constants::AvailableSpaceBeginningInROM)
        {
            newParams->BackgroundLayerDataPtr = (int) header.Layer3Data;
        }
    }

    // Layer data reshaping (matching SetRoomSize pattern)
    if ((newParams->Layer0MappingTypeParam & 0x30) == LevelComponents::LayerMap16)
    {
        newParams->LayerData[0] = RoomConfigDialog::ChangeLayerDimensions(layer0Width, layer0Height,
                                                                          currentParams->Layer0Width,
                                                                          currentParams->Layer0Height,
                                                                          currentParams->LayerData[0]);
    }
    else
    {
        newParams->LayerData[0] = nullptr;
    }
    newParams->LayerData[1] = RoomConfigDialog::ChangeLayerDimensions(roomWidth, roomHeight,
                                                                      currentParams->RoomWidth,
                                                                      currentParams->RoomHeight,
                                                                      currentParams->LayerData[1]);
    if ((newParams->Layer2MappingTypeParam & 0x30) == LevelComponents::LayerMap16)
    {
        newParams->LayerData[2] = RoomConfigDialog::ChangeLayerDimensions(roomWidth, roomHeight,
                                                                          currentParams->RoomWidth,
                                                                          currentParams->RoomHeight,
                                                                          currentParams->LayerData[2]);
    }
    else
    {
        newParams->LayerData[2] = nullptr;
    }

    OperationParams *op = new OperationParams;
    op->roomConfigChange = true;
    op->lastRoomConfigParams = currentParams;
    op->newRoomConfigParams = newParams;

    // Handle trim of doors/entities/camera if room dimensions change
    if (dimsChanged && (roomWidth != curRoomWidth || roomHeight != curRoomHeight))
    {
        LevelComponents::LevelDoorVector *oldDoorVec =
            new LevelComponents::LevelDoorVector(level->GetDoorList());
        std::vector<LevelComponents::EntityRoomAttribute> oldNormal = room->GetEntityListData(1);
        std::vector<LevelComponents::EntityRoomAttribute> oldHard = room->GetEntityListData(0);
        std::vector<LevelComponents::EntityRoomAttribute> oldSHard = room->GetEntityListData(2);
        auto oldCameraType = room->GetCameraControlType();
        auto oldCameraRecords = room->GetCameraControlRecords(true);

        singleton->TrimElementsOutOfRoomBounds(room, roomWidth, roomHeight);

        LevelComponents::LevelDoorVector *newDoorVec =
            new LevelComponents::LevelDoorVector(level->GetDoorList());
        std::vector<LevelComponents::EntityRoomAttribute> newNormal = room->GetEntityListData(1);
        std::vector<LevelComponents::EntityRoomAttribute> newHard = room->GetEntityListData(0);
        std::vector<LevelComponents::EntityRoomAttribute> newSHard = room->GetEntityListData(2);
        auto newCameraType = room->GetCameraControlType();
        auto newCameraRecords = room->GetCameraControlRecords(true);

        op->doorVectorChange = true;
        op->doorVectorChangeParams = DoorVectorChangeParams::Create(oldDoorVec, newDoorVec);
        op->entityNormalChange = true;
        op->entityNormalChangeParams = EntityListChangeParams::Create(oldNormal, newNormal, roomId);
        op->entityHardChange = true;
        op->entityHardChangeParams = EntityListChangeParams::Create(oldHard, newHard, roomId);
        op->entitySHardChange = true;
        op->entitySHardChangeParams = EntityListChangeParams::Create(oldSHard, newSHard, roomId);
        op->cameraControlChange = true;
        op->cameraControlChangeParams = CameraControlChangeParams::Create(roomId, oldCameraType, newCameraType,
                                                                          oldCameraRecords, newCameraRecords);
    }

    ExecuteOperation(op);
    log("ImportRoomConfig: Room config applied.");
    return true;
}

bool ScriptInterface::ImportLayerTiles(int layerId, int width, int height, QString tileDataHex)
{
    if (layerId < 0 || layerId > 2)
    {
        log("ImportLayerTiles: Invalid layer ID " + QString::number(layerId));
        return false;
    }

    LevelComponents::Room *room = singleton->GetCurrentRoom();
    int roomId = singleton->GetCurrentRoomId();
    LevelComponents::Layer *layer = room->GetLayer(layerId);
    unsigned short *oldData = layer->GetLayerData();
    int expectedSize = width * height;

    // Parse hex string into unsigned short array
    // The hex string is concatenated 4-char hex values (e.g. "001A00B2...")
    unsigned short *newData = new unsigned short[expectedSize];
    int hexLen = tileDataHex.size();
    for (int i = 0; i < expectedSize; ++i)
    {
        if (i * 4 + 4 <= hexLen)
        {
            newData[i] = (unsigned short) tileDataHex.mid(i * 4, 4).toUInt(nullptr, 16);
        }
        else
        {
            newData[i] = 0;
        }
    }

    OperationParams *op = new OperationParams;
    LayerChangeParams *layerChange = LayerChangeParams::Create(oldData, newData, width, height, roomId);
    delete[] newData;

    switch (layerId)
    {
    case 0:
        op->layer0Change = true;
        op->layer0ChangeParams = layerChange;
        break;
    case 1:
        op->layer1Change = true;
        op->layer1ChangeParams = layerChange;
        break;
    case 2:
        op->layer2Change = true;
        op->layer2ChangeParams = layerChange;
        break;
    }

    ExecuteOperation(op);
    log("ImportLayerTiles: Layer " + QString::number(layerId) + " tiles imported.");
    return true;
}

bool ScriptInterface::ImportDoorsDisableDest(QString doorsData)
{
    LevelComponents::Level *level = singleton->GetCurrentLevel();
    int roomId = singleton->GetCurrentRoomId();

    LevelComponents::LevelDoorVector *oldDoorVec =
        new LevelComponents::LevelDoorVector(level->GetDoorList());
    LevelComponents::LevelDoorVector *newDoorVec =
        new LevelComponents::LevelDoorVector(level->GetDoorList());

    // Delete all existing doors for this room
    QVector<struct LevelComponents::DoorEntry> existingDoors =
        newDoorVec->GetDoorsByRoomID((unsigned char) roomId);
    for (int i = existingDoors.size() - 1; i >= 0; --i)
    {
        unsigned char globalId = newDoorVec->GetGlobalIDByLocalID((unsigned char) roomId, (unsigned char) i);
        newDoorVec->DeleteDoor(globalId);
    }

    // Parse semicolon-separated door entries: type,roomID,x1,x2,y1,y2,destID,dx,dy,entitySetID,bgm
    QStringList doorEntries = doorsData.split(QChar(';'), Qt::SkipEmptyParts);
    for (const QString &entry : doorEntries)
    {
        QStringList fields = entry.split(QChar(','), Qt::SkipEmptyParts);
        if (fields.size() < 11) continue;

        int type = fields[0].toInt();
        int doorRoomId = fields[1].toInt();
        int x1 = fields[2].toInt();
        int x2 = fields[3].toInt();
        int y1 = fields[4].toInt();
        int y2 = fields[5].toInt();
        // fields[6] = destID (ignored)
        int deltaX = fields[7].toInt();
        int deltaY = fields[8].toInt();
        int entitySetId = fields[9].toInt();
        int bgm = fields[10].toInt();

        // Room 0 portal (global ID 0) cannot be deleted; update it in-place instead
        if (roomId == 0 && type == 1)
        {
            newDoorVec->SetDoorPlace(0, (unsigned char) x1, (unsigned char) x2,
                                     (unsigned char) y1, (unsigned char) y2);
            newDoorVec->SetDestinationDoor(0, 0);
            newDoorVec->SetWarioDelta(0, (signed char) deltaX, (signed char) deltaY);
            newDoorVec->SetBGM(0, (unsigned short) bgm);
            newDoorVec->SetEntitySetID(0, (unsigned char) entitySetId);
            continue;
        }
        unsigned char newId = (unsigned char) newDoorVec->AddDoor(
            (unsigned char) doorRoomId, (unsigned char) entitySetId, (unsigned char) type);
        newDoorVec->SetDoorPlace(newId, (unsigned char) x1, (unsigned char) x2,
                                 (unsigned char) y1, (unsigned char) y2);
        newDoorVec->SetDestinationDoor(newId, 0);
        newDoorVec->SetWarioDelta(newId, (signed char) deltaX, (signed char) deltaY);
        newDoorVec->SetBGM(newId, (unsigned short) bgm);
    }

    OperationParams *op = new OperationParams;
    op->doorVectorChange = true;
    op->doorVectorChangeParams = DoorVectorChangeParams::Create(oldDoorVec, newDoorVec);
    ExecuteOperation(op);

    log("ImportDoorsDisableDest: Doors imported with destinations disabled.");
    return true;
}

bool ScriptInterface::ImportDoors(QString doorsData)
{
    LevelComponents::Level *level = singleton->GetCurrentLevel();
    int roomId = singleton->GetCurrentRoomId();

    LevelComponents::LevelDoorVector *oldDoorVec =
        new LevelComponents::LevelDoorVector(level->GetDoorList());
    LevelComponents::LevelDoorVector *newDoorVec =
        new LevelComponents::LevelDoorVector(level->GetDoorList());

    // Delete all existing doors for this room
    QVector<struct LevelComponents::DoorEntry> existingDoors =
        newDoorVec->GetDoorsByRoomID((unsigned char) roomId);
    for (int i = existingDoors.size() - 1; i >= 0; --i)
    {
        unsigned char globalId = newDoorVec->GetGlobalIDByLocalID((unsigned char) roomId, (unsigned char) i);
        newDoorVec->DeleteDoor(globalId);
    }

    // Parse semicolon-separated door entries: type,roomID,x1,x2,y1,y2,destID,dx,dy,entitySetID,bgm
    QStringList doorEntries = doorsData.split(QChar(';'), Qt::SkipEmptyParts);
    for (const QString &entry : doorEntries)
    {
        QStringList fields = entry.split(QChar(','), Qt::SkipEmptyParts);
        if (fields.size() < 11) continue;

        int type = fields[0].toInt();
        int doorRoomId = fields[1].toInt();
        int x1 = fields[2].toInt();
        int x2 = fields[3].toInt();
        int y1 = fields[4].toInt();
        int y2 = fields[5].toInt();
        int destID = fields[6].toInt();
        int deltaX = fields[7].toInt();
        int deltaY = fields[8].toInt();
        int entitySetId = fields[9].toInt();
        int bgm = fields[10].toInt();

        // Room 0 portal (global ID 0) cannot be deleted; update it in-place instead
        if (roomId == 0 && type == 1)
        {
            newDoorVec->SetDoorPlace(0, (unsigned char) x1, (unsigned char) x2,
                                     (unsigned char) y1, (unsigned char) y2);
            newDoorVec->SetDestinationDoor(0, (unsigned char) destID);
            newDoorVec->SetWarioDelta(0, (signed char) deltaX, (signed char) deltaY);
            newDoorVec->SetBGM(0, (unsigned short) bgm);
            newDoorVec->SetEntitySetID(0, (unsigned char) entitySetId);
            continue;
        }
        unsigned char newId = (unsigned char) newDoorVec->AddDoor(
            (unsigned char) doorRoomId, (unsigned char) entitySetId, (unsigned char) type);
        newDoorVec->SetDoorPlace(newId, (unsigned char) x1, (unsigned char) x2,
                                 (unsigned char) y1, (unsigned char) y2);
        newDoorVec->SetDestinationDoor(newId, (unsigned char) destID);
        newDoorVec->SetWarioDelta(newId, (signed char) deltaX, (signed char) deltaY);
        newDoorVec->SetBGM(newId, (unsigned short) bgm);
    }

    OperationParams *op = new OperationParams;
    op->doorVectorChange = true;
    op->doorVectorChangeParams = DoorVectorChangeParams::Create(oldDoorVec, newDoorVec);
    ExecuteOperation(op);

    log("ImportDoors: Doors imported with destinations preserved.");
    return true;
}

bool ScriptInterface::ImportEntityList(int difficulty, QString entityDataHex)
{
    if (difficulty < 0 || difficulty > 2)
    {
        log("ImportEntityList: Invalid difficulty " + QString::number(difficulty));
        return false;
    }

    LevelComponents::Room *room = singleton->GetCurrentRoom();
    int roomId = singleton->GetCurrentRoomId();

    std::vector<LevelComponents::EntityRoomAttribute> oldList = room->GetEntityListData(difficulty);
    std::vector<LevelComponents::EntityRoomAttribute> newList;

    // Parse hex triples: "YPos XPos EntityID ..." space-separated, hex values
    QStringList tokens = entityDataHex.split(QChar(' '), Qt::SkipEmptyParts);
    for (int i = 0; i + 2 < tokens.size(); i += 3)
    {
        LevelComponents::EntityRoomAttribute attr;
        attr.YPos = (unsigned char) tokens[i].toUInt(nullptr, 16);
        attr.XPos = (unsigned char) tokens[i + 1].toUInt(nullptr, 16);
        attr.EntityID = (unsigned char) tokens[i + 2].toUInt(nullptr, 16);
        newList.push_back(attr);
    }

    OperationParams *op = new OperationParams;
    EntityListChangeParams *entityChange = EntityListChangeParams::Create(oldList, newList, roomId);
    switch (difficulty)
    {
    case 0:
        op->entityHardChange = true;
        op->entityHardChangeParams = entityChange;
        break;
    case 1:
        op->entityNormalChange = true;
        op->entityNormalChangeParams = entityChange;
        break;
    case 2:
        op->entitySHardChange = true;
        op->entitySHardChangeParams = entityChange;
        break;
    }

    ExecuteOperation(op);
    log("ImportEntityList: Difficulty " + QString::number(difficulty) + " imported.");
    return true;
}

bool ScriptInterface::ImportCameraControl(int camType, QString recordsData)
{
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    int roomId = singleton->GetCurrentRoomId();

    auto oldType = room->GetCameraControlType();
    auto oldRecords = room->GetCameraControlRecords(true);

    std::vector<struct LevelComponents::__CameraControlRecord *> newRecordsVec;

    // Parse semicolon-separated records: trans,x1,x2,y1,y2,x3,y3,offset,value
    if (!recordsData.isEmpty())
    {
        QStringList recordEntries = recordsData.split(QChar(';'), Qt::SkipEmptyParts);
        for (const QString &entry : recordEntries)
        {
            QStringList fields = entry.split(QChar(','), Qt::SkipEmptyParts);
            if (fields.size() < 9) continue;

            struct LevelComponents::__CameraControlRecord *rec =
                new LevelComponents::__CameraControlRecord();
            rec->TransboundaryControl = (unsigned char) fields[0].toInt();
            rec->x1 = (unsigned char) fields[1].toInt();
            rec->x2 = (unsigned char) fields[2].toInt();
            rec->y1 = (unsigned char) fields[3].toInt();
            rec->y2 = (unsigned char) fields[4].toInt();
            rec->x3 = (unsigned char) fields[5].toInt();
            rec->y3 = (unsigned char) fields[6].toInt();
            rec->ChangeValueOffset = (unsigned char) fields[7].toInt();
            rec->ChangedValue = (unsigned char) fields[8].toInt();
            newRecordsVec.push_back(rec);
        }
    }

    OperationParams *op = new OperationParams;
    op->cameraControlChange = true;
    op->cameraControlChangeParams = CameraControlChangeParams::Create(
        roomId, oldType, static_cast<LevelComponents::__CameraControlType>(camType),
        oldRecords, newRecordsVec);
    ExecuteOperation(op);
    log("ImportCameraControl: Camera control imported.");
    return true;
}

QString ScriptInterface::GetCurRoomDoorGlobalIds()
{
    LevelComponents::Level *level = singleton->GetCurrentLevel();
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    QVector<struct LevelComponents::DoorEntry> doors =
        level->GetDoorListRef().GetDoorsByRoomID((unsigned char) room->GetRoomID());
    QString result;
    for (int i = 0; i < doors.size(); ++i)
    {
        unsigned char globalId = level->GetDoorListRef().GetGlobalIDByLocalID(
            (unsigned char) room->GetRoomID(), (unsigned char) i);
        if (i > 0) result += ",";
        result += QString::number(globalId);
    }
    return result;
}

bool ScriptInterface::DeleteDoorByGlobalId(int globalId)
{
    return singleton->GetCurrentLevel()->DeleteDoorByGlobalID(globalId);
}

void ScriptInterface::ResetRoomEntitySet(int roomId)
{
    LevelComponents::Room *room = singleton->GetCurrentLevel()->GetRooms()[roomId];
    auto doors = singleton->GetCurrentLevel()->GetDoorListRef().GetDoorsByRoomID(
        (unsigned char) roomId);
    if (!doors.isEmpty())
        room->SetCurrentEntitySet(doors[0].EntitySetID);
}

void ScriptInterface::PostImportRefresh()
{
    singleton->RenderScreenFull();
    singleton->ResetEntitySetDockWidget();
    singleton->ResetCameraControlDockWidget();
}

// ---- Global Import Stubs ----

bool ScriptInterface::ImportGlobalTilesets(QString jsonString)
{
    log("ImportGlobalTilesets: received " + QString::number(jsonString.size()) + " bytes of JSON data.");
    return true;
}

bool ScriptInterface::ImportGlobalEntities(QString jsonString)
{
    log("ImportGlobalEntities: received " + QString::number(jsonString.size()) + " bytes of JSON data.");
    return true;
}

bool ScriptInterface::ImportGlobalEntitySets(QString jsonString)
{
    log("ImportGlobalEntitySets: received " + QString::number(jsonString.size()) + " bytes of JSON data.");
    return true;
}

bool ScriptInterface::ImportGlobalAnimatedTileGroups(QString jsonString)
{
    log("ImportGlobalAnimatedTileGroups: received " + QString::number(jsonString.size()) + " bytes of JSON data.");
    return true;
}

bool ScriptInterface::ImportGlobalWallPaint(QString jsonString)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (doc.isNull() || !doc.isObject())
    {
        log("ImportGlobalWallPaint: Invalid JSON.");
        return false;
    }
    QJsonObject obj = doc.object();

    QString gfxHex = obj["gfxHex"].toString();
    QString passageColorHex = obj["passageColorHex"].toString();
    QString passageGrayHex = obj["passageGrayHex"].toString();

    if (gfxHex.isEmpty() || passageColorHex.isEmpty() || passageGrayHex.isEmpty())
    {
        log("ImportGlobalWallPaint: Missing required hex fields (gfxHex, passageColorHex, passageGrayHex).");
        return false;
    }

    QByteArray newGFX = QByteArray::fromHex(gfxHex.toLatin1());
    QByteArray newPassageColor = QByteArray::fromHex(passageColorHex.toLatin1());
    QByteArray newPassageGray = QByteArray::fromHex(passageGrayHex.toLatin1());

    const int gfxSize = 1024 * 5 * 6;
    const int palSize = 32 * 5 * 6;

    if (newGFX.size() != gfxSize || newPassageColor.size() != palSize || newPassageGray.size() != palSize)
    {
        log("ImportGlobalWallPaint: Data size mismatch. Expected GFX=" +
            QString::number(gfxSize) + ", Color=" + QString::number(palSize) +
            ", Gray=" + QString::number(palSize) + " bytes.");
        return false;
    }

    // Capture old ROM data for fixed blocks
    unsigned char oldGFX[1024 * 5 * 6];
    unsigned char oldPassageColor[32 * 5 * 6];
    unsigned char oldPassageGray[32 * 5 * 6];
    memcpy(oldGFX, &ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintGFXAddr], gfxSize);
    memcpy(oldPassageColor, &ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageColor], palSize);
    memcpy(oldPassageGray, &ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageGray], palSize);

    // Apply new fixed-block data to ROM
    memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintGFXAddr], newGFX.constData(), gfxSize);
    memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageColor], newPassageColor.constData(), palSize);
    memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageGray], newPassageGray.constData(), palSize);

    // Create WallPaintChangeParams for fixed blocks
    WallPaintChangeParams *wp = WallPaintChangeParams::Create(
        oldGFX, reinterpret_cast<unsigned char *>(newGFX.data()),
        oldPassageColor, reinterpret_cast<unsigned char *>(newPassageColor.data()),
        oldPassageGray, reinterpret_cast<unsigned char *>(newPassageGray.data()));

    // Handle scattered blocks
    QJsonArray scatteredArray = obj["scatteredBlocks"].toArray();
    for (const QJsonValue &val : scatteredArray)
    {
        QJsonObject blockObj = val.toObject();
        unsigned int addr = static_cast<unsigned int>(blockObj["addr"].toInt());
        unsigned int size = static_cast<unsigned int>(blockObj["size"].toInt());
        QString dataHex = blockObj["dataHex"].toString();
        QByteArray newBlockData = QByteArray::fromHex(dataHex.toLatin1());

        // Capture old data
        unsigned char *oldBlockData = new unsigned char[size];
        memcpy(oldBlockData, &ROMUtils::ROMFileMetadata->ROMDataPtr[addr], size);

        // Apply new data to ROM
        int copySize = qMin(newBlockData.size(), static_cast<int>(size));
        memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[addr], newBlockData.constData(), static_cast<size_t>(copySize));

        // Add to change params (ScatteredBlock constructor copies internally)
        wp->AddScatteredBlock(addr, size, oldBlockData,
                              &ROMUtils::ROMFileMetadata->ROMDataPtr[addr]);
        delete[] oldBlockData;
    }

    // Execute operation
    struct OperationParams *operation = new struct OperationParams;
    operation->WallPaintChange = true;
    operation->wallPaintChangeParams = wp;
    ExecuteOperation(operation);

    log("ImportGlobalWallPaint: Wall paint data imported successfully.");
    return true;
}

bool ScriptInterface::ImportGlobalCredits(QString jsonString)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (doc.isNull() || !doc.isObject())
    {
        log("ImportGlobalCredits: Invalid JSON.");
        return false;
    }
    QJsonObject obj = doc.object();
    QString hexStr = obj["creditsHex"].toString();
    if (hexStr.isEmpty())
    {
        log("ImportGlobalCredits: Missing 'creditsHex' field.");
        return false;
    }

    QByteArray newData = QByteArray::fromHex(hexStr.toLatin1());
    const int expectedSize = NUMBEROFCREDITSSCREEN * 1280;
    if (newData.size() != expectedSize)
    {
        log("ImportGlobalCredits: Data size mismatch. Expected " +
            QString::number(expectedSize) + " bytes, got " +
            QString::number(newData.size()) + " bytes.");
        return false;
    }

    // Capture old credit data
    DialogParams::CreditsEditParams *lastParams = new DialogParams::CreditsEditParams();
    memcpy(lastParams->oldCreditData,
           &ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::CreditsTiles],
           expectedSize);

    // Apply new data to ROM
    memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::CreditsTiles],
           newData.constData(), static_cast<size_t>(expectedSize));

    // Capture new credit data
    DialogParams::CreditsEditParams *newParams = new DialogParams::CreditsEditParams();
    memcpy(newParams->newCreditData,
           &ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::CreditsTiles],
           expectedSize);

    // Create and execute operation
    OperationParams *operation = new OperationParams;
    operation->CreditChange = true;
    operation->lastCreditsEditParams = lastParams;
    operation->newCreditsEditParams = newParams;
    ExecuteOperation(operation);

    log("ImportGlobalCredits: Credits imported successfully.");
    return true;
}

bool ScriptInterface::ImportLevelConfig(QString jsonString)
{
    log("ImportLevelConfig: received " + QString::number(jsonString.size()) + " bytes of JSON data.");
    return true;
}

// ---------------------- current Room's hint layer render stuff --------------------------

void HintLayer::GetAutoGeneratedHintLayer()
{
    tmpHintLayerPixmap = singleton->GetCurrentRoom()->GetHintLayerPixmap();
}

void HintLayer::GetblankHintLayer()
{
    QPixmap tmppximap = singleton->GetCurrentRoom()->GetHintLayerPixmap();
    tmpHintLayerPixmap = QPixmap(tmppximap.width(), tmppximap.height());
    tmpHintLayerPixmap.fill(Qt::transparent);
}

void HintLayer::drawRect(int x, int y, int width, int height, int line_width, int red, int green, int blue, int alpha)
{
    if (tmpHintLayerPixmap.isNull()) return;
    QPainter tmpPainter(&tmpHintLayerPixmap);
    QPen tmpPen = QPen(QBrush(QColor(red, green, blue, alpha)), line_width);
    tmpPen.setJoinStyle(Qt::MiterJoin);
    tmpPainter.setPen(tmpPen);
    tmpPainter.drawRect(x, y, width, height);
}

void HintLayer::drawLine(int x1, int y1, int x2, int y2, int line_width, int red, int green, int blue, int alpha)
{
    if (tmpHintLayerPixmap.isNull()) return;
    QPainter tmpPainter(&tmpHintLayerPixmap);
    QPen tmpPen = QPen(QBrush(QColor(red, green, blue, alpha)), line_width);
    tmpPen.setJoinStyle(Qt::MiterJoin);
    tmpPainter.setPen(tmpPen);
    tmpPainter.drawLine(x1, y1, x2, y2);
}

void HintLayer::drawText(int x, int y, QString show_text, int red, int green, int blue, int alpha)
{
    if (tmpHintLayerPixmap.isNull()) return;
    QPainter tmpPainter(&tmpHintLayerPixmap);
    QPen tmpPen = QPen(QBrush(QColor(red, green, blue, alpha)), 2);
    tmpPen.setJoinStyle(Qt::MiterJoin);
    tmpPainter.setPen(tmpPen);
    tmpPainter.drawText(x, y, show_text);
}

void HintLayer::SubmitHintLayer()
{
    if (tmpHintLayerPixmap.isNull()) return;
    singleton->GetCurrentRoom()->SetHintLayerPixmap(tmpHintLayerPixmap);
}
