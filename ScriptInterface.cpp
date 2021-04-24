#include "ScriptInterface.h"

#include "WL4EditorWindow.h"
extern WL4EditorWindow *singleton;

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
        log("ScriptInterface::GetCurRoomTile16(): Tile position (" +
            QString::number(x) + ", " + QString::number(y) +
            ") out of bounds on layer L (dimensions: " + QString::number(width) + ", " + QString::number(height) + ")");
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
        log("ScriptInterface::GetCurRoomTile8(): Tile position (" +
            QString::number(x) + ", " + QString::number(y) +
            ") out of bounds on layer L (dimensions: " + QString::number(width) + ", " + QString::number(height) + ")");
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

void ScriptInterface::_DecompressData(int mappingtype, int address)
{
    int tmpw = 0, tmph = 0;
    unsigned short *LayerData = nullptr;
    if((mappingtype & 0x20) == 0x20) {
        tmpw = (1 + (ROMUtils::ROMFileMetaData->ROMDataPtr[address] & 1)) << 5;
        tmph = (1 + ((ROMUtils::ROMFileMetaData->ROMDataPtr[address] >> 1) & 1)) << 5;
        LayerData = reinterpret_cast<unsigned short *>(ROMUtils::LayerRLEDecompress(address + 1, tmpw * tmph * 2));
        if (ROMUtils::ROMFileMetaData->ROMDataPtr[address] == 1)
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
        tmpw = ROMUtils::ROMFileMetaData->ROMDataPtr[address];
        tmph = ROMUtils::ROMFileMetaData->ROMDataPtr[address + 1];
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
        filePath = QFileDialog::getSaveFileName(singleton, tr("Save Layer data file"), "", tr("bin files (*.bin)"));
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
            witdh = room->GetWidth();
            height = room->GetHeight();
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
                                                    tr("Load Layer data bin file"), "",
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
        witdh = room->GetWidth();
        height = room->GetHeight();
    }
    if(datasize != 2 * witdh * height)
    {
        log("File size not match (expected width:" + QString::number(witdh) + ", height:" + QString::number(height) + ")!");
        return;
    }
    memcpy(room->GetLayer(layerid)->GetLayerData(), tmptile8x8data.data(), 2 * witdh * height);
    room->GetLayer(layerid)->SetDirty(true);
    singleton->SetUnsavedChanges(true);
    singleton->RenderScreenFull();
    log("Done!");
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
            result += QString::number(entity.YPos, 16).toUpper() + QChar(' ');
            result += QString::number(entity.XPos, 16).toUpper() + QChar(' ');
            result += QString::number(entity.EntityID, 16).toUpper() + QChar(' ');
        }
        return result;
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

void ScriptInterface::SetCurRoomTile16(int layerID, int TileID, int x, int y)
{
    if(layerID > 2 || layerID < 0) {
        log(QString("Illegal layer ID!\n"));
        return;
    }
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    if(room->GetLayer(layerID)->GetMappingType() != LevelComponents::LayerMap16)
        return;
    int width = static_cast<int>(room->GetWidth());
    int height = static_cast<int>(room->GetHeight());
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

    singleton->RoomConfigReset(_currentRoomConfigParams, _nextRoomConfigParams);
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

void ScriptInterface::WriteTxtFile(QString filepath, QString test)
{
    if(!filepath.compare(""))
        filepath = QFileDialog::getSaveFileName(singleton, tr("Save Entity list data file"), "", tr("bin files (*.bin)"));
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
