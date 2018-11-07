#ifndef WL4EDITORWINDOW_H
#define WL4EDITORWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QButtonGroup>

#include "LevelComponents/Level.h"
#include "LevelComponents/Room.h"
#include "Dialog/ChooseLevelDialog.h"
#include "DockWidget/Tile16DockWidget.h"
#include "DockWidget/EditModeDockWidget.h"
#include "DockWidget/EntitySetDockWidget.h"
#include "Dialog/LevelConfigDialog.h"
#include "Dialog/RoomConfigDialog.h"
#include "Dialog/DoorConfigDialog.h"

namespace Ui {
    class WL4EditorWindow;
}

class WL4EditorWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::WL4EditorWindow *ui;
    QLabel *statusBarLabel;
    Tile16DockWidget *Tile16SelecterWidget;
    EditModeDockWidget *EditModeWidget;
    EntitySetDockWidget *EntitySetWidget;
    LevelComponents::Level *CurrentLevel = nullptr;
    int selectedRoom = 0;
    bool UnsavedChanges = false;
    bool firstROMLoaded = false;
    bool UnsavedChangesWarning();

protected:
    void resizeEvent(QResizeEvent *event);

public:
    explicit WL4EditorWindow(QWidget *parent = 0);
    ~WL4EditorWindow();
    void RenderScreenFull();
    void RenderScreenVisibilityChange();
    void RenderScreenElementsLayersUpdate(unsigned int DoorId, int EntityId);
    void RenderScreenTileChange(int tileX, int tileY, unsigned short tileID, int LayerID);
    void SetStatusBarText(char *str);
    void LoadRoomUIUpdate();
    Tile16DockWidget *GetTile16DockWidgetPtr() { return Tile16SelecterWidget; }
    EditModeDockWidget *GetEditModeWidgetPtr() { return EditModeWidget; }
    EntitySetDockWidget *GetEntitySetDockWidgetPtr() { return EntitySetWidget; }
    LevelComponents::Room *GetCurrentRoom() { return CurrentLevel->GetRooms()[selectedRoom]; }
    LevelComponents::Level *GetCurrentLevel() { return CurrentLevel; }
    void SetUnsavedChanges(bool newValue) { UnsavedChanges = newValue; }
    bool FirstROMIsLoaded() { return firstROMLoaded; }
    void OpenROM();
    void SetEditModeDockWidgetLayerEditability();
    bool *GetLayersVisibilityArray();
    void Graphicsview_UnselectDoorAndEntity();
    void RoomConfigReset(DialogParams::RoomConfigParams *currentroomconfig, DialogParams::RoomConfigParams *nextroomconfig);
    void ShowEntitySetDockWidget() { EntitySetWidget->setVisible(true); }
    void ShowTile16DockWidget() { Tile16SelecterWidget->setVisible(true); }
    void HideEntitySetDockWidget() { EntitySetWidget->setVisible(false); }
    void HideTile16DockWidget() { Tile16SelecterWidget->setVisible(false); }
    void ResetEntitySetDockWidget() { EntitySetWidget->ResetEntitySet(CurrentLevel->GetRooms()[selectedRoom]); }
    void DeleteEntity(int EntityIndex) { CurrentLevel->GetRooms()[selectedRoom]->DeleteEntity(EntityIndex); }

private slots:
    void on_actionOpen_ROM_triggered();
    void on_loadLevelButton_clicked();
    void on_roomDecreaseButton_clicked();
    void on_roomIncreaseButton_clicked();
    void on_actionLevel_Config_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionRoom_Config_triggered();
};

#endif // WL4EDITORWINDOW_H
