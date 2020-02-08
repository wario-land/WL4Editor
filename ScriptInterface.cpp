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
    return room->GetLayer(layerID)->GetLayerData()[y * width + x];
}

void ScriptInterface::SetCurRoomTile16(int layerID, int TileID, int x, int y)
{
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    if(room->GetLayer(layerID)->GetMappingType() != LevelComponents::LayerMap16)
        return;
    int width = static_cast<int>(room->GetWidth());
    int height = static_cast<int>(room->GetHeight());
    if(x >= width || y >= height) {
        log(QString("position out of range!\n"));
        return;
    }
    room->GetLayer(layerID)->GetLayerData()[y * width + x] = (unsigned short) (TileID & 0xFFFF);
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
