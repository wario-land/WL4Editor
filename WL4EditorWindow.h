#ifndef WL4EDITORWINDOW_H
#define WL4EDITORWINDOW_H

#include <QButtonGroup>
#include <QLabel>
#include <QMainWindow>

#include "SettingsUtils.h"

#include "Dialog/ChooseLevelDialog.h"
#include "Dialog/DoorConfigDialog.h"
#include "Dialog/LevelConfigDialog.h"
#include "Dialog/RoomConfigDialog.h"
#include "Dialog/TilesetEditDialog.h"
#include "DockWidget/CameraControlDockWidget.h"
#include "DockWidget/EditModeDockWidget.h"
#include "DockWidget/EntitySetDockWidget.h"
#include "DockWidget/Tile16DockWidget.h"
#include "LevelComponents/Level.h"
#include "LevelComponents/Room.h"

namespace Ui
{
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
    CameraControlDockWidget *CameraControlWidget;
    LevelComponents::Level *CurrentLevel = nullptr;
    QAction *RecentROMs[5];
    uint recentROMnum = 0;

    unsigned int selectedRoom = 0;
    uint graphicViewScalerate = 2;
    bool UnsavedChanges = false; // state check bool only be used when user try loading another ROM, another Level or
                                 // close the editor without saving changes
    bool firstROMLoaded = false;
    void closeEvent(QCloseEvent *event);
    bool notify(QObject *receiver, QEvent *event);
    static bool SaveCurrentFile() { return ROMUtils::SaveLevel(ROMUtils::ROMFilePath); }
    bool SaveCurrentFileAs();
    bool UnsavedChangesPrompt(QString str);
    void CurrentRoomClearEverything();

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
    void UIStartUp(int currentTilesetID);
    void SetEditModeDockWidgetLayerEditability();
    bool *GetLayersVisibilityArray() { return EditModeWidget->GetLayersVisibilityArray(); }
    void Graphicsview_UnselectDoorAndEntity();
    void RoomConfigReset(DialogParams::RoomConfigParams *currentroomconfig,
                         DialogParams::RoomConfigParams *nextroomconfig);
    void ShowEntitySetDockWidget() { EntitySetWidget->setVisible(true); }
    void ShowTile16DockWidget() { Tile16SelecterWidget->setVisible(true); }
    void ShowCameraControlDockWidget() { CameraControlWidget->setVisible(true); }
    void HideCameraControlDockWidget() { CameraControlWidget->setVisible(false); }
    void HideEntitySetDockWidget() { EntitySetWidget->setVisible(false); }
    void HideTile16DockWidget() { Tile16SelecterWidget->setVisible(false); }
    void ResetEntitySetDockWidget() { EntitySetWidget->ResetEntitySet(CurrentLevel->GetRooms()[selectedRoom]); }
    void ResetCameraControlDockWidget()
    {
        CameraControlWidget->SetCameraControlInfo(CurrentLevel->GetRooms()[selectedRoom]);
    }
    void DeleteEntity(int EntityIndex) { CurrentLevel->GetRooms()[selectedRoom]->DeleteEntity(EntityIndex); }
    void DeleteDoor(int globalDoorIndex);
    void SetEditModeWidgetDifficultyRadioBox(int rd) { EditModeWidget->SetDifficultyRadioBox(rd); }
    void LoadROMDataFromFile(QString qFilePath);

    // Events
    void keyPressEvent(QKeyEvent *event);

private slots:
    // called slots
    void openRecentROM();

    // Auto-generated
    void on_actionOpen_ROM_triggered();
    void on_loadLevelButton_clicked();
    void on_roomDecreaseButton_clicked();
    void on_roomIncreaseButton_clicked();
    void on_actionLevel_Config_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionRoom_Config_triggered();
    void on_actionNew_Door_triggered();
    void on_actionSave_ROM_triggered();
    void on_actionAbout_triggered();
    void on_actionSave_As_triggered();
    void on_action_swap_Layer_0_Layer_1_triggered();
    void on_action_swap_Layer_1_Layer_2_triggered();
    void on_action_swap_Layer_0_Layer_2_triggered();
    void on_action_swap_Normal_Hard_triggered();
    void on_action_swap_Hard_S_Hard_triggered();
    void on_action_swap_Normal_S_Hard_triggered();
    void on_action_clear_Layer_0_triggered();
    void on_action_clear_Layer_1_triggered();
    void on_action_clear_Layer_2_triggered();
    void on_action_clear_Normal_triggered();
    void on_action_clear_Hard_triggered();
    void on_action_clear_S_Hard_triggered();
    void on_actionSave_Room_s_graphic_triggered();
    void on_actionManager_triggered();
    void on_actionEdit_Tileset_triggered();
    void on_actionLight_triggered();
    void on_actionDark_triggered();
};

#endif // WL4EDITORWINDOW_H
