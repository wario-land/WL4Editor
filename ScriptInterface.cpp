#include "ScriptInterface.h"

#include "WL4EditorWindow.h"
extern WL4EditorWindow *singleton;

ScriptInterface::ScriptInterface(QObject *parent) : QObject(parent)
{
    // installation
}

int ScriptInterface::GetCurRoomWidth()
{
    return static_cast<int>(singleton->GetCurrentRoom()->GetWidth());
}

int ScriptInterface::GetCurRoomHeight()
{
    return static_cast<int>(singleton->GetCurrentRoom()->GetHeight());
}

int ScriptInterface::GetCurRoomTile16(int layerID, int x, int y)
{
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    if(room->GetLayer(layerID)->GetMappingType() != LevelComponents::LayerMap16)
        return -1;
    int width = static_cast<int>(room->GetWidth());
    int height = static_cast<int>(room->GetHeight());
    if(x >= width || y >= height) {
        log(QString("position out of range!\n"));
        return -1;
    }
    return room->GetLayer(layerID)->GetTileData(x, y);
}

void ScriptInterface::Test_DecompressData(int mappingtype, int address)
{
    int tmpw = 0, tmph = 0;
    unsigned short *LayerData = nullptr;
    if((mappingtype & 0x20) == 0x20) {
        tmpw = (1 + (ROMUtils::CurrentFile[address] & 1)) << 5;
        tmph = (1 + ((ROMUtils::CurrentFile[address] >> 1) & 1)) << 5;
        LayerData = reinterpret_cast<unsigned short *>(ROMUtils::LayerRLEDecompress(address + 1, tmpw * tmph * 2));
        if (ROMUtils::CurrentFile[address] == 1)
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
        tmpw = ROMUtils::CurrentFile[address];
        tmph = ROMUtils::CurrentFile[address + 1];
        LayerData = reinterpret_cast<unsigned short *>(ROMUtils::LayerRLEDecompress(address + 2, tmpw * tmph * 2));
    } else {
        singleton->GetOutputWidgetPtr()->PrintString("Mapping Type unavailable.");
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
        singleton->GetOutputWidgetPtr()->PrintString("Decompress error.");
        return;
    }
    singleton->GetOutputWidgetPtr()->PrintString(tmpstr);
}

unsigned int ScriptInterface::Test_GetLayerDecomdataPointer(int layerId)
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

void ScriptInterface::SetCurRoomTile16(int layerID, int TileID, int x, int y)
{
    if(layerID > 2 || layerID < 0) {
        log(QString("Illegal layer Id !\n"));
        return;
    }
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    if(room->GetLayer(layerID)->GetMappingType() != LevelComponents::LayerMap16)
        return;
    int width = static_cast<int>(room->GetWidth());
    int height = static_cast<int>(room->GetHeight());
    if(x >= width || y >= height) {
        log(QString("position out of range!\n"));
        return;
    }
    room->GetLayer(layerID)->SetTileData(TileID & 0xFFFF, x, y);
    room->GetLayer(layerID)->SetDirty(true);
    singleton->SetUnsavedChanges(true);
}

void ScriptInterface::alert(QString message)
{
    QMessageBox::critical(nullptr, QString("Error"), message);
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
