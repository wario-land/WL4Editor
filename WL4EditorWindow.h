#ifndef WL4EDITORWINDOW_H
#define WL4EDITORWINDOW_H

#include <QButtonGroup>
#include <QLabel>
#include <QMainWindow>

#include "Dialog/ChooseLevelDialog.h"
#include "Dialog/DoorConfigDialog.h"
#include "Dialog/LevelConfigDialog.h"
#include "Dialog/RoomConfigDialog.h"
#include "Dialog/TilesetEditDialog.h"
#include "Dialog/CreditsEditDialog.h"
#include "DockWidget/CameraControlDockWidget.h"
#include "DockWidget/EditModeDockWidget.h"
#include "DockWidget/EntitySetDockWidget.h"
#include "DockWidget/Tile16DockWidget.h"
#include "DockWidget/OutputDockWidget.h"
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
    QLabel *statusBarLabel_MousePosition;
    QLabel *statusBarLabel_Scalerate;
    QLabel *statusBarLabel_rectselectMode;
    Tile16DockWidget *Tile16SelecterWidget;
    EditModeDockWidget *EditModeWidget;
    EntitySetDockWidget *EntitySetWidget;
    CameraControlDockWidget *CameraControlWidget;
    OutputDockWidget *OutputWidget = nullptr;
    LevelComponents::Level *CurrentLevel = nullptr;
    QAction *RecentROMs[5];
    uint recentROMnum = 0;
    QAction *RecentScripts[5];
    uint recentScriptNum = 0;

    unsigned int selectedRoom = 0;
    uint graphicViewScalerate = 2;
    bool UnsavedChanges = false; // state check bool only be used when user try loading another ROM, another Level or
                                 // close the editor without saving changes
    bool firstROMLoaded = false;
    QString dialogInitialPath = QString("");

    void closeEvent(QCloseEvent *event);
    bool notify(QObject *receiver, QEvent *event);
    static bool SaveCurrentFile() { return ROMUtils::SaveLevel(ROMUtils::ROMFileMetadata->FilePath); }
    bool SaveCurrentFileAs();
    bool UnsavedChangesPrompt(QString str);
    void ClearEverythingInRoom(bool no_warning = false);

    // recent file manager functions
    void InitRecentFileMenuEntries(const bool manageRecentScripts = false);
    bool OpenRecentFile(QString newFilepath, const bool manageRecentScripts = false);
    void ManageRecentFilesOrScripts(QString newFilepath, const bool manageRecentScripts = false);

protected:
    void resizeEvent(QResizeEvent *event);

public:
    explicit WL4EditorWindow(QWidget *parent = 0);
    ~WL4EditorWindow();
    void RenderScreenFull();
    void RenderScreenVisibilityChange();
    void RenderScreenElementsLayersUpdate(unsigned int DoorId, int EntityId);
    void RenderScreenTilesChange(QVector<LevelComponents::Tileinfo> tilelist, int LayerID);
    void SetStatusBarText(char *str);
    void LoadRoomUIUpdate();
    Tile16DockWidget *GetTile16DockWidgetPtr() { return Tile16SelecterWidget; }
    EditModeDockWidget *GetEditModeWidgetPtr() { return EditModeWidget; }
    EntitySetDockWidget *GetEntitySetDockWidgetPtr() { return EntitySetWidget; }
    OutputDockWidget *GetOutputWidgetPtr() { return OutputWidget; }
    void InvalidOutputWidgetPtr() { OutputWidget = nullptr; }
    LevelComponents::Room *GetCurrentRoom() { return CurrentLevel->GetRooms()[selectedRoom]; }
    int GetCurrentRoomId() { return selectedRoom; }
    LevelComponents::Level *GetCurrentLevel() { return CurrentLevel; }
    void SetUnsavedChanges(bool newValue) { UnsavedChanges = newValue; }
    bool FirstROMIsLoaded() { return firstROMLoaded; }
    void OpenROM();
    void UIStartUp(int currentTilesetID);
    void SetEditModeDockWidgetLayerEditability();
    QVector<bool> GetLayersVisibilityArray() { return EditModeWidget->GetLayersVisibilityArray(); }
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
        CameraControlWidget->PopulateCameraControlInfo(CurrentLevel->GetRooms()[selectedRoom]);
    }
    void DeleteEntity(int EntityIndex) { CurrentLevel->GetRooms()[selectedRoom]->DeleteEntity(EntityIndex); }
    bool DeleteDoor(int globalDoorIndex);
    void SetEditModeWidgetDifficultyRadioBox(int rd) { EditModeWidget->SetDifficultyRadioBox(rd); }
    void LoadROMDataFromFile(QString qFilePath);
    void PrintMousePos(int x, int y);
    uint GetGraphicViewScalerate() { return graphicViewScalerate; }
    void SetGraphicViewScalerate(uint scalerate);
    void RefreshRectSelectHint(bool state);
    void SetRectSelectMode(bool state);
    QGraphicsView *Getgraphicview();
    void SetChangeCurrentRoomEnabled(bool state);
    void SetCurrentRoomId(int roomid);
    void EditCurrentTileset(DialogParams::TilesetEditParams *_newTilesetEditParams);
    QString GetdDialogInitialPath() { return dialogInitialPath; }
    void SetDialogInitialPath(QString newpath) { dialogInitialPath = newpath; }

private slots:
    // called slots
    void openRecentROM();
    void openRecentScript();

    // Auto-generated
    void on_actionOpen_ROM_triggered();
    void on_actionSave_ROM_triggered();
    void on_actionSave_As_triggered();
    void on_actionSave_Room_s_graphic_triggered();
    void on_loadLevelButton_clicked();
    void on_roomDecreaseButton_clicked();
    void on_roomIncreaseButton_clicked();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionUndo_global_triggered();
    void on_actionRedo_global_triggered();
    void on_actionLevel_Config_triggered();
    void on_actionRoom_Config_triggered();
    void on_actionEdit_Tileset_triggered();
    void on_actionEdit_Credits_triggered();
    void on_actionNew_Door_triggered();
    void on_actionNew_Room_triggered();
    void on_action_duplicate_Normal_triggered();
    void on_action_duplicate_Hard_triggered();
    void on_action_duplicate_S_Hard_triggered();
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
    void on_actionClear_all_triggered();
    void on_actionRect_Select_Mode_toggled(bool arg1);
    void on_actionPatch_Manager_triggered();
    void on_actionRun_from_file_triggered();
    void on_actionLight_triggered();
    void on_actionDark_triggered();
    void on_actionZoom_in_triggered();
    void on_actionZoom_out_triggered();
    void on_actionEdit_Entity_EntitySet_triggered();
    void on_actionOutput_window_triggered();
    void on_actionAbout_triggered();
    void on_actionImport_Tileset_from_ROM_triggered();
    void on_actionRolling_Save_triggered();
    void on_actionGraphic_Manager_triggered();
    void on_actionReload_project_settings_triggered();
    void on_actionEdit_Animated_Tile_Groups_triggered();
    void on_actionEdit_Wall_Paints_triggered();
};

#endif // WL4EDITORWINDOW_H
