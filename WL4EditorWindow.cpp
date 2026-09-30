#include "WL4EditorWindow.h"

#include "SettingsUtils.h"
#include "Themes.h"
#include "ROMUtils.h"
#include "FileIOUtils.h"
#include "Operation.h"
#include "ChunkUtils.h"

#include "Dialog/SpritesEditorDialog.h"
#include "Dialog/PatchManagerDialog.h"
#include "Dialog/GraphicManagerDialog.h"
#include "Dialog/ChunkManagerDialog.h"
#include "Dialog/AnimatedTileGroupEditorDialog.h"
#include "Dialog/WallPaintEditorDialog.h"
#include "ui_WL4EditorWindow.h"

#include <cstdio>
#include <deque>

#include <QCloseEvent>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QTextEdit>
#include <QSizePolicy>
#include <algorithm>

// Variables used by WL4EditorWindow
bool editModeWidgetInitialized = false;

// Global variables
struct DialogParams::PassageAndLevelIndex selectedLevel = { 0, 0 };
WL4EditorWindow *singleton;

/// <summary>
/// Construct the instance of the WL4EditorWindow.
/// </summary>
/// <remarks>
/// The graphics view is hardcoded to scale at 2x size.
/// </remarks>
/// <param name="parent">
/// The parent QWidget.
/// </param>
WL4EditorWindow::WL4EditorWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::WL4EditorWindow)
{
    // Render Themes
    int themeId = SettingsUtils::GetKey(SettingsUtils::IniKeys::EditorThemeId).toInt();
    QApplication::setStyle("fusion");
    QApplication::setPalette(namedColorSchemePalette(static_cast<ThemeColorType>(themeId)));

    ui->setupUi(this);
    singleton = this;

    // MainWindow UI Initialization
    ui->graphicsView->scale(graphicViewScalerate, graphicViewScalerate);
    statusBarLabel = new QLabel(tr("Open a ROM file"));
    statusBarLabel_MousePosition = new QLabel();
    statusBarLabel_rectselectMode = new QLabel(tr("Rect Select: Off"));
    statusBarLabel_Scalerate = new QLabel(tr("scale rate: ") + QString::number(graphicViewScalerate) + "00%");
    statusBarLabel->setMargin(3);
    statusBarLabel_MousePosition->setMargin(3);
    statusBarLabel_rectselectMode->setMargin(3);
    statusBarLabel_Scalerate->setMargin(3);
    ui->statusBar->addWidget(statusBarLabel);
    ui->statusBar->addWidget(statusBarLabel_rectselectMode);
    ui->statusBar->addWidget(statusBarLabel_Scalerate);
    ui->statusBar->addWidget(statusBarLabel_MousePosition);
    switch (themeId) {
    case 0:
    { ui->actionLight->setChecked(true); break; }
    case 1:
    { ui->actionDark->setChecked(true); break; }
    }
    ui->actionRolling_Save->setChecked(SettingsUtils::GetKey(SettingsUtils::IniKeys::RollingSaveLimit).toInt());

    // Create DockWidgets
    EditModeWidget = new EditModeDockWidget();
    Tile16SelecterWidget = new Tile16DockWidget();
    EntitySetWidget = new EntitySetDockWidget();
    CameraControlWidget = new CameraControlDockWidget();
    OutputWidget = new OutputDockWidget();

    // Add Recent ROM QAction according to the INI file
    InitRecentFileMenuEntries();
    InitRecentFileMenuEntries(true);

    // Memory Initialization
    memset(ROMUtils::animatedTileGroups, 0, sizeof(ROMUtils::animatedTileGroups) / sizeof(ROMUtils::animatedTileGroups[0]));
    memset(ROMUtils::singletonTilesets, 0, sizeof(ROMUtils::singletonTilesets) / sizeof(ROMUtils::singletonTilesets[0]));
    memset(ROMUtils::entitiessets, 0, sizeof(ROMUtils::entitiessets) / sizeof(ROMUtils::entitiessets[0]));
    memset(ROMUtils::entities, 0, sizeof(ROMUtils::entities) / sizeof(ROMUtils::entities[0]));
}

/// <summary>
/// Deconstruct the WL4EditorWindow and clean up its instance objects on the heap.
/// </summary>
WL4EditorWindow::~WL4EditorWindow()
{
    // Clean up heap instance objects
    delete ui;
    delete Tile16SelecterWidget;
    delete EditModeWidget;
    delete OutputWidget;
    delete EntitySetWidget;
    delete CameraControlWidget;
    delete statusBarLabel;
    delete statusBarLabel_MousePosition;
    delete statusBarLabel_rectselectMode;
    delete statusBarLabel_Scalerate;

    // Decomstruct all Tileset singletons
    for(int i = 0; i < (sizeof(ROMUtils::animatedTileGroups) / sizeof(ROMUtils::animatedTileGroups[0])); i++)
    {
        delete ROMUtils::animatedTileGroups[i];
        ROMUtils::animatedTileGroups[i] = nullptr;
    }
    for(int i = 0; i < (sizeof(ROMUtils::singletonTilesets) / sizeof(ROMUtils::singletonTilesets[0])); i++)
    {
        delete ROMUtils::singletonTilesets[i];
        ROMUtils::singletonTilesets[i] = nullptr;
    }
    for(int i = 0; i < (sizeof(ROMUtils::entitiessets) / sizeof(ROMUtils::entitiessets[0])); i++)
    {
        delete ROMUtils::entitiessets[i];
        ROMUtils::entitiessets[i] = nullptr;
    }
    for(int i = 0; i < (sizeof(ROMUtils::entities) / sizeof(ROMUtils::entities[0])); i++)
    {
        delete ROMUtils::entities[i];
        ROMUtils::entities[i] = nullptr;
    }
    ResetUndoHistory();
    if (CurrentLevel)
    {
        delete CurrentLevel;
    }

    if (ROMUtils::ROMFileMetadata->ROMDataPtr)
    {
        delete[] ROMUtils::ROMFileMetadata->ROMDataPtr;
    }
}

/// <summary>
/// Set the text of the status bar.
/// </summary>
/// <remarks>
/// The QLabel for the status bar is on the heap, and the old one is deleted when changing the text.
/// </remarks>
/// <param name="str">
/// The string contents to put in the status bar.
/// </param>
void WL4EditorWindow::SetStatusBarText(char *str)
{
    QLabel *old = (QLabel *) ui->statusBar->children()[0];
    QLabel *newStr = new QLabel(str);
    ui->statusBar->removeWidget(old);
    ui->statusBar->addWidget(newStr);
    delete old;
}

/// <summary>
/// Perform the UI operations which must be done after loading a room.
/// </summary>
/// <remarks>
/// Set the text for the selected room.
/// Set the text for the selected level in the status bar.
/// Fully re-render the screen.
/// </remarks>
void WL4EditorWindow::LoadRoomUIUpdate()
{
    // Set the text for which room is currently loaded, near the top of the editor window
    char tmpStr[30];
    unsigned int currentroomid = ui->spinBox_RoomID->value();

    // Set the text for which level is loaded, near the bottom of the editor window
    sprintf(tmpStr, "Level ID: %d-%d", selectedLevel._PassageIndex, selectedLevel._LevelIndex);
    statusBarLabel->setText(tmpStr);
    ui->roomDecreaseButton->setEnabled(currentroomid);
    ui->roomIncreaseButton->setEnabled(CurrentLevel->GetRooms().size() > currentroomid + 1);

    // Set the max and min for room id spinbox
    ui->spinBox_RoomID->setMinimum(0);
    ui->spinBox_RoomID->setMaximum(CurrentLevel->GetRooms().size() - 1);

    // Render the screen
    RenderScreenFull();
    SetEditModeDockWidgetLayerEditability();
}

int WL4EditorWindow::GetCurrentRoomId()
{
    return ui->spinBox_RoomID->value();
}

/// <summary>
/// Present the user with an "open file" dialog, and perform necessary level loading actions if a ROM in successfully
/// loaded.
/// </summary>
/// <remarks>
/// Load the ROM file into CurrentFile.
/// Set the title of the main window.
/// Load level 0-0.
/// Render room 0 of the level.
/// On first successful ROM load, add and update UI that requires a ROM to have been loaded.
/// </remarks>
void WL4EditorWindow::OpenROM()
{
    // Check for unsaved operations
    if (!UnsavedChangesPrompt(tr("There are unsaved changes. Discard changes and load ROM anyway?")))
        return;

    // Select a ROM file to open
    QString openROMFileInitPath = SettingsUtils::GetKey(SettingsUtils::IniKeys::OpenRomInitPath);
    QString qFilePath =
        QFileDialog::getOpenFileName(this, tr("Open ROM file"), openROMFileInitPath, tr("GBA ROM files (*.gba)"));
    if (!qFilePath.compare(""))
    {
        return;
    }

    LoadROMDataFromFile(qFilePath);
}

/// <summary>
/// Load ROM data from a file into the editor's data structures
/// </summary>
/// <remarks>
/// This is a helper function to prevent duplication of code from multiple ways a ROM file can be loaded
/// </remarks>
/// <param name="filePath">
/// The path of the ROM file
/// </param>
void WL4EditorWindow::LoadROMDataFromFile(QString qFilePath)
{
    // Load the ROM file
    std::string filePath = qFilePath.toStdString();
    if (QString errorMessage = FileIOUtils::LoadROMFile(qFilePath); !errorMessage.isEmpty())
    {
        QMessageBox::critical(nullptr, QString(tr("Load Error")), QString(errorMessage));
        return;
    }
    dialogInitialPath = QFileInfo(qFilePath).dir().path();
    SettingsUtils::SetKey(SettingsUtils::IniKeys::OpenRomInitPath, dialogInitialPath);

    // Headless chunk health scan before constructing singletons
    // (prevents crashes from corrupted ROM data during singleton construction)
    {
        int orphanCount = 0, dupRefCount = 0, seriousCount = 0;
        ChunkUtils::QuickChunkHealthScan(
            ROMUtils::ROMFileMetadata->ROMDataPtr,
            ROMUtils::ROMFileMetadata->Length,
            orphanCount, dupRefCount, seriousCount);

        if (orphanCount > 0 || dupRefCount > 0)
        {
            OutputWidget->PrintString(
                QString("ROM loaded: %1 orphaned chunk(s), %2 duplicate reference(s) detected.")
                    .arg(orphanCount).arg(dupRefCount));
        }

        if (seriousCount > 0)
        {
            QString msg = QString(
                "This ROM contains %1 serious chunk issue(s) "
                "(corrupted headers, overlaps, or misaligned pointers) "
                "that may crash the editor.\n\n"
                "Open Chunk Manager to inspect and repair?")
                .arg(seriousCount);
            if (QMessageBox::Yes == QMessageBox::question(
                    this, tr("ROM Corruption Detected"), msg,
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes))
            {
                // Invoke CM before any singleton construction to avoid crashes
                ChunkManagerDialog cmDialog(ChunkManagerMode::LoadGuard, this);
                cmDialog.exec();

                if (cmDialog.WasSaved())
                {
                    // Re-scan after CM save
                    int o2 = 0, d2 = 0, s2 = 0;
                    ChunkUtils::QuickChunkHealthScan(
                        ROMUtils::ROMFileMetadata->ROMDataPtr,
                        ROMUtils::ROMFileMetadata->Length,
                        o2, d2, s2);
                    if (s2 > 0)
                    {
                        QMessageBox::critical(this, tr("Cannot Load ROM"),
                            tr("Serious chunk issues remain after repair (%1 issues). "
                               "Loading aborted to prevent a crash.").arg(s2));
                        return; // Block loading
                    }
                    if (o2 > 0)
                        OutputWidget->PrintString(
                            QString("%1 orphaned chunk(s) remain after repair.").arg(o2));
                    if (d2 > 0)
                        OutputWidget->PrintString(
                            QString("%1 duplicate reference(s) remain after repair.").arg(d2));
                }
                else
                {
                    // User cancelled — abort load
                    return;
                }
            }
            // else: user chose No — proceed at their own risk
        }
    }

    // Clean-up
    if (CurrentLevel)
    {
        delete CurrentLevel;
        // Decomstruct all LevelComponents singletons
        for(int i = 0; i < (sizeof(ROMUtils::animatedTileGroups) / sizeof(ROMUtils::animatedTileGroups[0])); i++)
        {
            delete ROMUtils::animatedTileGroups[i];
            ROMUtils::animatedTileGroups[i] = nullptr;
        }
        for(int i = 0; i < (sizeof(ROMUtils::singletonTilesets) / sizeof(ROMUtils::singletonTilesets[0])); i++)
        {
            delete ROMUtils::singletonTilesets[i];
            ROMUtils::singletonTilesets[i] = nullptr;
        }
        for(int i = 0; i < (sizeof(ROMUtils::entitiessets) / sizeof(ROMUtils::entitiessets[0])); i++)
        {
            delete ROMUtils::entitiessets[i];
            ROMUtils::entitiessets[i] = nullptr;
        }
        for(int i = 0; i < (sizeof(ROMUtils::entities) / sizeof(ROMUtils::entities[0])); i++)
        {
            delete ROMUtils::entities[i];
            ROMUtils::entities[i] = nullptr;
        }
        ResetUndoHistory();
        ResetGlobalElementOperationIndexes();
        OutputWidget->ClearTextEdit();
    }

    // Load the Project settings
    SettingsUtils::LoadProjectSettings();

    // Set the program title
    std::string fileName = filePath.substr(filePath.rfind('/') + 1);
    setWindowTitle(fileName.c_str());

    // Load all LevelComponents singletons
    for(int i = 0; i < (sizeof(ROMUtils::animatedTileGroups) / sizeof(ROMUtils::animatedTileGroups[0])); i++)
    {
        int animatedTileGroupHeaderAddr = WL4Constants::AnimatedTileHeaderTable + i * 8;
        ROMUtils::animatedTileGroups[i] = new LevelComponents::AnimatedTile8x8Group(animatedTileGroupHeaderAddr, i);
    }
    for(int i = 0; i < (sizeof(ROMUtils::singletonTilesets) / sizeof(ROMUtils::singletonTilesets[0])); i++)
    {
        int tilesetPtr = WL4Constants::TilesetDataTable + i * 36;
        ROMUtils::singletonTilesets[i] = new LevelComponents::Tileset(tilesetPtr, i);
    }
    for (unsigned int i = 0; i < sizeof(ROMUtils::entities) / sizeof(ROMUtils::entities[0]); ++i)
    {
        // TODO: the palette param should be loaded differently for different passages for gem palette
        ROMUtils::entities[i] = new LevelComponents::Entity(i, WL4Constants::UniversalSpritesPalette);
    }
    for (unsigned int i = 0; i < sizeof(ROMUtils::entitiessets) / sizeof(ROMUtils::entitiessets[0]); ++i)
    {
        ROMUtils::entitiessets[i] = new LevelComponents::EntitySet(i);
    }
    UnsavedChanges = false;
    UIStartUp();
}

/// <summary>
/// Print Mouse Pos in the status bar
/// </summary>
/// <param name="x">
/// Mouse x position in scaled pixels
/// </param>
/// <param name="y">
/// Mouse y position in scaled pixels
/// </param>
void WL4EditorWindow::PrintMousePos(int x, int y)
{
    int selectedLayer = EditModeWidget->GetEditModeParams().selectedLayer;
    LevelComponents::Layer *layer = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()]->GetLayer(selectedLayer);
    int tileSize;
    if(layer->GetMappingType() == LevelComponents::LayerMappingType::LayerDisabled)
    {
        statusBarLabel_MousePosition->setText(tr("Selected layer is disabled!"));
        return;
    }
    int is8x8 = layer->GetMappingType() == LevelComponents::LayerMappingType::LayerTile8x8;
    tileSize = is8x8 ? 8 : 16;
    int xBound = (x /= tileSize) < layer->GetLayerWidth();
    int yBound = (y /= tileSize) < layer->GetLayerHeight();
    QString offset_text = "";
    if(!is8x8)
    {
        offset_text = tr(" Positional offset (y * width + x): 0x%1").arg(layer->GetLayerWidth() * y + x, 4, 16, QChar('0'));
    }
    if(xBound && yBound)
    {
        statusBarLabel_MousePosition->setText(QString("Mouse position (Hex): Layer %0 (%1, %2, %3)").arg(
            QString::number(selectedLayer),
            QString(is8x8 ? "Tile8x8" : "Map16"),
            "0x" + QString::number(x, 16),
            "0x" + QString::number(y, 16)) + offset_text);
    }
    else
    {
        statusBarLabel_MousePosition->setText(tr("Mouse out of range!"));
    }
}

/// <summary>
/// Set graphicView scalerate.
/// </summary>
void WL4EditorWindow::SetGraphicViewScalerate(uint scalerate)
{
    ui->graphicsView->scale((qreal)scalerate / (qreal)graphicViewScalerate, (qreal)scalerate / (qreal)graphicViewScalerate);
    graphicViewScalerate = scalerate;
    statusBarLabel_MousePosition->setText(tr("Move your mouse to show position again!"));
    statusBarLabel_Scalerate->setText(tr("Scale rate: ") + QString::number(graphicViewScalerate) + "00%");
}

/// <summary>
/// Show the rect select state in the main graphic view.
/// </summary>
/// <param name="state">
/// The toggle state of rect select
/// </param>
void WL4EditorWindow::RefreshRectSelectHint(bool state)
{
    statusBarLabel_rectselectMode->setText(QString(tr("Rectangle Select: ")) + (state ? tr("On") : tr("Off")));
}

/// <summary>
/// Unset the rect select state in the main graphic view.
/// </summary>
/// <param name="state">
/// The toggle state of rect select
/// </param>
void WL4EditorWindow::SetRectSelectMode(bool state)
{
    ui->actionRect_Select_Mode->setChecked(state);
}

/// <summary>
/// Get the pointer of the main graphic view.
/// </summary>
QGraphicsView *WL4EditorWindow::Getgraphicview()
{
    return ui->graphicsView;
}

/// <summary>
/// Set enable for buttons to go to a different room.
/// </summary>
/// <param name="state">
/// The toggle state of rect select
/// </param>
void WL4EditorWindow::SetChangeCurrentRoomEnabled(bool state)
{
    if (state) {
        unsigned int currentroomid = ui->spinBox_RoomID->value();
        if (currentroomid)
            ui->roomDecreaseButton->setEnabled(state);
        if (currentroomid < (CurrentLevel->GetRooms().size() - 1))
            ui->roomIncreaseButton->setEnabled(state);
    } else {
        ui->roomDecreaseButton->setEnabled(state);
        ui->roomIncreaseButton->setEnabled(state);
    }
}

/// <summary>
/// Set current room.
/// </summary>
/// <param name="roomid">
/// The new room's id.
/// </param>
void WL4EditorWindow::SetCurrentRoomId(int roomid, bool call_from_spinbox_valuechange)
{
    // enable or disable those buttons
    if (!roomid)
        ui->roomDecreaseButton->setEnabled(false);
    else
        ui->roomDecreaseButton->setEnabled(true);

    int currentRoomMaxId = CurrentLevel->GetRooms().size() - 1;
    if (roomid == currentRoomMaxId)
        ui->roomIncreaseButton->setEnabled(false);
    else if (roomid < currentRoomMaxId)
        ui->roomIncreaseButton->setEnabled(true);

    // Deselect rect
    // SetRectSelectMode(ui->actionRect_Select_Mode->isChecked());
    // Deselect Door and Entity
    ui->graphicsView->DeselectDoorAndEntity(false);
    ui->graphicsView->ResetRectPixmaps();
    ui->graphicsView->ResetRect();

    // Load the room
    if (call_from_spinbox_valuechange == false) ui->spinBox_RoomID->setValue(roomid);
    LoadRoomUIUpdate();
    int tmpTilesetID = CurrentLevel->GetRooms()[roomid]->GetTilesetID();
    Tile16SelecterWidget->SetTileset(tmpTilesetID);
    ResetEntitySetDockWidget();
    ResetCameraControlDockWidget();
}

/// <summary>
/// Helper function to edit current Tileset by opening a Tileset Editor dialog
/// </summary>
void WL4EditorWindow::EditCurrentTileset(DialogParams::TilesetEditParams *_newTilesetEditParams)
{
    // Show the dialog
    TilesetEditDialog dialog(this, _newTilesetEditParams);
    if (dialog.exec() == QDialog::Accepted)
    {
        int currentTilesetId = _newTilesetEditParams->currentTilesetIndex;
        int tilesetPtr = WL4Constants::TilesetDataTable + currentTilesetId * 36;
        DialogParams::TilesetEditParams *_oldRoomTilesetEditParams = new DialogParams::TilesetEditParams();
        _oldRoomTilesetEditParams->currentTilesetIndex = currentTilesetId;
        _oldRoomTilesetEditParams->newTileset = ROMUtils::singletonTilesets[currentTilesetId];
        _newTilesetEditParams->newTileset->setTilesetPtr(tilesetPtr);
        _newTilesetEditParams->newTileset->SetChanged(true);

        // Execute Operation and add changes into the operation history
        OperationParams *operation = new OperationParams;

        operation->TilesetChange = true;
        operation->lastTilesetEditParams = _oldRoomTilesetEditParams;
        operation->newTilesetEditParams = _newTilesetEditParams;
        ExecuteOperation(operation); // Set UnsavedChanges bool inside
    }
    else
    {
        delete _newTilesetEditParams->newTileset;
        delete _newTilesetEditParams;
    }
}

/// <summary>
/// Update the UI after loading a ROM.
/// </summary>
void WL4EditorWindow::UIStartUp()
{
    // Only modify UI on the first time a ROM is loaded
    if (!firstROMLoaded)
    {
        firstROMLoaded = true;

        // Enable UI that requires a ROM file to be loaded
        ui->actionSave_ROM->setEnabled(true);
        if (!SettingsUtils::GetKey(SettingsUtils::IniKeys::RollingSaveLimit).toInt())
        {
            ui->actionSave_As->setEnabled(true);
        }
        ui->actionSave_Room_s_graphic->setEnabled(true);
        ui->menuImport_from_ROM->setEnabled(true);
        ui->actionUndo->setEnabled(true);
        ui->actionRedo->setEnabled(true);
        ui->actionLevel_Config->setEnabled(true);
        ui->actionRoom_Config->setEnabled(true);
        ui->actionEdit_Animated_Tile_Groups->setEnabled(true);
        ui->actionEdit_Tileset->setEnabled(true);
        ui->actionEdit_Credits->setEnabled(true);
        ui->menuAdd->setEnabled(true);
        ui->menuDuplicate->setEnabled(true);
        ui->menuEntity_lists_2->setEnabled(true);
        ui->menuSwap->setEnabled(true);
        ui->menuClear->setEnabled(true);
        ui->menu_clear_Layer->setEnabled(true);
        ui->menu_clear_Entity_list->setEnabled(true);
        ui->actionClear_all->setEnabled(true);
        ui->actionPatch_Manager->setEnabled(true);
        ui->actionGraphic_Manager->setEnabled(true);
        ui->actionChunk_Manager->setEnabled(true);
        ui->actionEdit_Entity_EntitySet->setEnabled(true);
        ui->actionRun_from_file->setEnabled(true);
        ui->menuRecent_Script->setEnabled(true);
        ui->loadLevelButton->setEnabled(true);
        ui->actionReload_project_settings->setEnabled(true);
        ui->actionEdit_Wall_Paints->setEnabled(true);
        ui->spinBox_RoomID->setEnabled(true);

        // Load Dock widget
        addDockWidget(Qt::RightDockWidgetArea, EditModeWidget);
        addDockWidget(Qt::RightDockWidgetArea, Tile16SelecterWidget);
        addDockWidget(Qt::RightDockWidgetArea, EntitySetWidget);
        addDockWidget(Qt::RightDockWidgetArea, CameraControlWidget);
        addDockWidget(Qt::BottomDockWidgetArea, OutputWidget);
        CameraControlWidget->setVisible(false);
        EntitySetWidget->setVisible(false);
    }

    // Modify Recent ROM menu
    ManageRecentFilesOrScripts(ROMUtils::ROMFileMetadata->FilePath);

    // Load the first level and render the screen, also set up the UI
    selectedLevel._PassageIndex = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(SettingsUtils::IniKeys::RecentROM_0_RecentPassage_id)).toInt();
    selectedLevel._LevelIndex = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(SettingsUtils::IniKeys::RecentROM_0_RecentLevel_id)).toInt();
    CurrentLevel = new LevelComponents::Level(static_cast<enum LevelComponents::__passage>(selectedLevel._PassageIndex),
                                              static_cast<enum LevelComponents::__stage>(selectedLevel._LevelIndex));
    ui->spinBox_RoomID->setValue(SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(SettingsUtils::IniKeys::RecentROM_0_RecentRoom_id)).toInt());

    unsigned int currentroomid = ui->spinBox_RoomID->value();
    int tmpTilesetID = CurrentLevel->GetRooms()[currentroomid]->GetTilesetID();
    auto currentroom = CurrentLevel->GetRooms()[currentroomid];

    EntitySetWidget->ResetEntitySet(currentroom);
    Tile16SelecterWidget->SetTileset(tmpTilesetID);
    CameraControlWidget->PopulateCameraControlInfo(currentroom);

    // UI update
    LoadRoomUIUpdate();
}

/// <summary>
/// Set whether the the UI elements for WL4Editor are enabled, based on layer properties.
/// </summary>
void WL4EditorWindow::SetEditModeDockWidgetLayerEditability()
{
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];
    bool layer0enable = currentroom->GetLayer(0)->IsEnabled();
    EditModeWidget->SetLayersCheckBoxEnabled(0, layer0enable);
    EditModeWidget->SetLayersCheckBoxEnabled(1, currentroom->GetLayer(1)->IsEnabled());
    EditModeWidget->SetLayersCheckBoxEnabled(2, currentroom->GetLayer(2)->IsEnabled());
    EditModeWidget->SetLayersCheckBoxEnabled(3, currentroom->GetLayer(3)->IsEnabled());
    EditModeWidget->SetLayersCheckBoxEnabled(7, currentroom->IsLayer0ColorBlendingEnabled());
}

/// <summary>
/// Deselect doors or entities that are currently selected.
/// </summary>
void WL4EditorWindow::Graphicsview_UnselectDoorAndEntity() { ui->graphicsView->DeselectDoorAndEntity(true); }

/// <summary>
/// Reset the Room with a new/old RoomConfigParams.
/// </summary>
/// <param name="currentroomconfig">
/// The current RoomConfigParams which can be made by the current Room.
/// </param>
/// <param name="nextroomconfig">
/// The next RoomConfigParams which you want to apply to the current Room.
/// </param>
void WL4EditorWindow::RoomConfigReset(DialogParams::RoomConfigParams *currentroomconfig,
                                      DialogParams::RoomConfigParams *nextroomconfig)
{
    // Apply the selected parameters to the target room identified by the operation params
    // reset the Tileset instance in Room class
    LevelComponents::Room *currentRoom = CurrentLevel->GetRooms()[currentroomconfig->roomID];
    if (nextroomconfig->CurrentTilesetIndex != currentroomconfig->CurrentTilesetIndex)
    {
        currentRoom->SetTileset(ROMUtils::singletonTilesets[nextroomconfig->CurrentTilesetIndex], nextroomconfig->CurrentTilesetIndex);
        Tile16SelecterWidget->SetTileset(nextroomconfig->CurrentTilesetIndex);
    }

    // update the Layer 0. 1. 2, 3 instances
    // Layer 0
    if (nextroomconfig->Layer0MappingTypeParam > 0xF && nextroomconfig->Layer0MappingTypeParam < 0x20)
    {
        currentRoom->GetLayer(0)->SetWidthHeightData(nextroomconfig->Layer0Width, nextroomconfig->Layer0Height, nextroomconfig->LayerData[0]);
    }
    else if (nextroomconfig->Layer0MappingTypeParam < 0x10)
    {
        currentRoom->GetLayer(0)->SetDisabled();
    }
    else // if (currentroomconfig->Layer0MappingTypeParam > 0x1F)
    {
        LevelComponents::Layer *currentLayer0 = currentRoom->GetLayer(0);
        delete currentLayer0;
        currentLayer0 = new LevelComponents::Layer(nextroomconfig->Layer0DataPtr, LevelComponents::LayerTile8x8);
        currentRoom->SetLayer(0, currentLayer0);
    }
    // Layer 1
    currentRoom->GetLayer(1)->SetWidthHeightData(nextroomconfig->RoomWidth, nextroomconfig->RoomHeight, nextroomconfig->LayerData[1]);
    // Layer 2
    if (nextroomconfig->Layer2MappingTypeParam > 0xF)
    {
        currentRoom->GetLayer(2)->SetWidthHeightData(nextroomconfig->RoomWidth, nextroomconfig->RoomHeight, nextroomconfig->LayerData[2]);
    } else // if (currentroomconfig->Layer2MappingTypeParam < 0x10)
    {
        currentRoom->GetLayer(2)->SetDisabled();
    }
    // Layer 3
    if (nextroomconfig->BackgroundLayerEnable)
    {
        LevelComponents::Layer *currentLayer3 = currentRoom->GetLayer(3);
        delete currentLayer3;
        currentLayer3 = new LevelComponents::Layer(nextroomconfig->BackgroundLayerDataPtr, LevelComponents::LayerTile8x8);
        currentRoom->SetLayer(3, currentLayer3);
    }
    else if (!nextroomconfig->BackgroundLayerEnable)
    {
        currentRoom->GetLayer(3)->SetDisabled();
    }

    // Trim out-of-range Doors, Entities, Camera limitators when Room size changed
    if (nextroomconfig->RoomWidth != currentroomconfig->RoomWidth ||
            nextroomconfig->RoomHeight != currentroomconfig->RoomHeight)
    {
        TrimElementsOutOfRoomBounds(currentRoom, nextroomconfig->RoomWidth, nextroomconfig->RoomHeight);
    }

    // reset all the Parameters in Room class, except new layer data pointers, generate them on saving
    currentRoom->SetLayer0MappingParam(nextroomconfig->Layer0MappingTypeParam);
    currentRoom->SetRenderEffectFlag(nextroomconfig->LayerPriorityAndAlphaAttr);
    currentRoom->SetLayer2MappingType(nextroomconfig->Layer2MappingTypeParam);
    currentRoom->SetBGLayerEnabled(nextroomconfig->BackgroundLayerEnable);
    currentRoom->SetBGLayerScrollFlag(nextroomconfig->BGLayerScrollFlag);
    currentRoom->SetLayerGFXEffect01(nextroomconfig->RasterType);
    currentRoom->SetLayerGFXEffect02(nextroomconfig->Water);
    currentRoom->SetBgmvolume(nextroomconfig->BGMVolume);

    // Restore entity set (normally derived from doors, but saved here for completeness)
    if (nextroomconfig->CurrentEntitySetID != currentroomconfig->CurrentEntitySetID)
        currentRoom->SetCurrentEntitySet(nextroomconfig->CurrentEntitySetID);

    // Mark the layers as dirty
    for (unsigned int i = 0; i < 3; ++i)
        currentRoom->GetLayer(i)->SetDirty(true);
}

/// <summary>
/// Trim doors, entities, and camera limitators that fall outside the new room boundaries.
/// </summary>
void WL4EditorWindow::TrimElementsOutOfRoomBounds(LevelComponents::Room *currentRoom, int newWidth, int newHeight)
{
    // -- Door --
    LevelComponents::LevelDoorVector &allDoor = CurrentLevel->GetDoorListRef();
    int doornum = allDoor.size();
    for (int i = doornum - 1; i > 0; i--)
    {
        auto curDoor = allDoor.GetDoor(i);
        if (curDoor.RoomID == currentRoom->GetRoomID())
        {
            if (curDoor.x1 > (newWidth - 1) || curDoor.y1 > (newHeight - 1))
            {
                if (curDoor.DoorTypeByte == LevelComponents::_Portal)
                {
                    allDoor.SetDoorPlace(i, 2, 2, 2, 2);
                }
                else if (allDoor.GetDoorsByRoomID(curDoor.RoomID).size() < 2)
                {
                    // Cannot delete the last door in a room — move it to a safe position
                    allDoor.SetDoorPlace(i, 2, 2, 2, 2);
                }
                else
                {
                    CurrentLevel->DeleteDoorByGlobalID(i);
                }
            }
        }
    }

    // -- Entity --
    for (uint i = 0; i < 3; i++)
    {
        std::vector<struct LevelComponents::EntityRoomAttribute> entitylist = currentRoom->GetEntityListData(i);
        size_t entitynum = entitylist.size();
        for (uint j = entitynum; j > 0; j--)
        {
            if ((entitylist[j - 1].XPos > (newWidth - 1)) || (entitylist[j - 1].YPos > (newHeight - 1)))
            {
                currentRoom->DeleteEntity(i, j - 1);
                currentRoom->SetEntityListDirty(i, true);
            }
        }
    }

    // -- Camera limitator --
    std::vector<struct LevelComponents::__CameraControlRecord *> limitatorlist =
        currentRoom->GetCameraControlRecords(false);
    size_t limitatornum = limitatorlist.size();
    if (limitatornum > 0)
    {
        size_t k = limitatornum - 1;
        uint *deleteLimitatorIdlist = new uint[limitatornum]();
        for (uint i = 0; i < limitatornum; i++)
        {
            int x2_prime =
                (limitatorlist[i]->ChangeValueOffset == 1) ? (limitatorlist[i]->ChangedValue) : (limitatorlist[i]->x2);
            int y2_prime =
                (limitatorlist[i]->ChangeValueOffset == 3) ? (limitatorlist[i]->ChangedValue) : (limitatorlist[i]->y2);
            if ((x2_prime >= newWidth) || (y2_prime >= newHeight))
            {
                deleteLimitatorIdlist[k--] = i + 1;
            }
        }

        // If all limitators would be deleted and the camera type requires them, preserve one
        bool hasControlAttrs = (currentRoom->GetCameraControlType() == LevelComponents::HasControlAttrs);
        if (hasControlAttrs)
        {
            size_t deleteCount = 0;
            for (uint i = 0; i < limitatornum; i++)
                if (deleteLimitatorIdlist[i] != 0) deleteCount++;
            if (deleteCount == limitatornum)
            {
                // Preserve the limitator with the smallest 1-based index (last non-zero entry)
                for (int i = (int)limitatornum - 1; i >= 0; i--)
                {
                    if (deleteLimitatorIdlist[i] != 0)
                    {
                        int idx = deleteLimitatorIdlist[i] - 1;
                        limitatorlist[idx]->TransboundaryControl = limitatorlist[idx]->x1 = limitatorlist[idx]->y1 = 2;
                        limitatorlist[idx]->x2 = 16;
                        limitatorlist[idx]->y2 = 11;
                        limitatorlist[idx]->ChangedValue = limitatorlist[idx]->ChangeValueOffset = 0xFF;
                        limitatorlist[idx]->x3 = limitatorlist[idx]->y3 = 0xFF;
                        deleteLimitatorIdlist[i] = 0;
                        break;
                    }
                }
            }
        }

        for (uint i = 0; i < limitatornum; i++)
        {
            if (deleteLimitatorIdlist[i] != 0)
            {
                currentRoom->DeleteCameraLimitator(deleteLimitatorIdlist[i] - 1);
            }
        }
        delete[] deleteLimitatorIdlist;
    }
}

/// <summary>
/// Delete a Door from the current Level.
/// </summary>
/// <param name="globalDoorIndex">
/// The global Door id given by current Level.
/// </param>
bool WL4EditorWindow::DeleteDoor(int globalDoorIndex)
{
    if (!CurrentLevel->DeleteDoorByGlobalID(globalDoorIndex))
    {
        OutputWidget->PrintString(tr("Cannot Delete the current Door!\nYou cannot delete a portal Door or the last Door in a Room!"));
        return false;
    }
    return true;
}

/// <summary>
/// Slot function to load a ROM.
/// </summary>
void WL4EditorWindow::openRecentROM()
{
    // Check for unsaved operations
    if(!UnsavedChangesPrompt(tr("There are unsaved changes. Discard changes and load ROM anyway?"))) return;

    QString filepath;
    QAction *action = qobject_cast<QAction *>(sender());
    if(action)
    {
        filepath = action->text();
    }

    // check if the file loaded from the recent file record still exist
    // manage the QAction list if it needs changes
    if (!OpenRecentFile(filepath)) return;

    LoadROMDataFromFile(filepath);
}

/// <summary>
/// Slot function to load and execute a js script.
/// </summary>
void WL4EditorWindow::openRecentScript()
{
    QString filepath;
    QAction *action = qobject_cast<QAction *>(sender());
    if(action)
    {
        filepath = action->text();
    }

    // check if the file loaded from the recent file record still exist
    // manage the QAction list if it needs changes
    if (!OpenRecentFile(filepath, true)) return;

    // Modify Recent Script menu
    ManageRecentFilesOrScripts(filepath, true);

    QFile file(filepath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
            QMessageBox::critical(this, tr("Error"), tr("Can't open file."));
            return;
    }
    QString code = QString::fromUtf8(file.readAll());
    OutputWidget->ExecuteJSScript(code);
}

/// <summary>
/// Call the OpenROM function when the action for it is triggered in the main window.
/// </summary>
void WL4EditorWindow::on_actionOpen_ROM_triggered()
{
    OpenROM();
}

/// <summary>
/// Perform a full render of the currently selected room.
/// </summary>
void WL4EditorWindow::RenderScreenFull()
{
    // Delete the old scene, if it exists
    QGraphicsScene *oldScene = ui->graphicsView->scene();
    if (oldScene)
    {
        delete oldScene;
    }
    ui->graphicsView->ClearRectPointer();

    // Perform a full render of the screen
    struct LevelComponents::RenderUpdateParams renderParams(LevelComponents::FullRender);
    renderParams.mode = EditModeWidget->GetEditModeParams();
    renderParams.SelectedDoorID = (unsigned int) ui->graphicsView->GetSelectedDoorID();
    LevelComponents::Room *curRoom = this->GetCurrentRoom();
    renderParams.localDoors = CurrentLevel->GetRoomDoorVec(curRoom->GetRoomID());
    QGraphicsScene *scene = curRoom->RenderGraphicsScene(ui->graphicsView->scene(), &renderParams);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
}

/// <summary>
/// Perform a re-render of the currently selected room, if only layer visibility has been toggled.
/// </summary>
void WL4EditorWindow::RenderScreenVisibilityChange()
{
    struct LevelComponents::RenderUpdateParams renderParams(LevelComponents::LayerEnable);
    renderParams.mode = EditModeWidget->GetEditModeParams();
    LevelComponents::Room *curRoom = this->GetCurrentRoom();
    renderParams.localDoors = CurrentLevel->GetRoomDoorVec(curRoom->GetRoomID());
    QGraphicsScene *scene = curRoom->RenderGraphicsScene(ui->graphicsView->scene(), &renderParams);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
}

/// <summary>
/// Perform a re-render of the Door/Camera limitation rectangle/Entity layer.
/// </summary>
void WL4EditorWindow::RenderScreenElementsLayersUpdate(unsigned int DoorId, int EntityId)
{
    struct LevelComponents::RenderUpdateParams renderParams(LevelComponents::ElementsLayersUpdate);
    renderParams.mode = EditModeWidget->GetEditModeParams();
    renderParams.SelectedDoorID = DoorId;
    renderParams.SelectedEntityID = EntityId;
    LevelComponents::Room *curRoom = this->GetCurrentRoom();
    renderParams.localDoors = CurrentLevel->GetRoomDoorVec(curRoom->GetRoomID());
    QGraphicsScene *scene = curRoom->RenderGraphicsScene(ui->graphicsView->scene(), &renderParams);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
}

/// <summary>
/// Perform a re-render of multiple tiles changes.
/// </summary>
void WL4EditorWindow::RenderScreenTilesChange(QVector<LevelComponents::Tileinfo> tilelist, int LayerID)
{
    struct LevelComponents::RenderUpdateParams renderParams(LevelComponents::TileChanges);
    renderParams.mode = EditModeWidget->GetEditModeParams();
    renderParams.mode.selectedLayer = LayerID;
    renderParams.tilechangelist = tilelist;
    LevelComponents::Room *curRoom = this->GetCurrentRoom();
    renderParams.localDoors = CurrentLevel->GetRoomDoorVec(curRoom->GetRoomID());
    curRoom->RenderGraphicsScene(ui->graphicsView->scene(), &renderParams);
}

/// <summary>
/// Override the close window functionality so that a save prompt is offered if there are unsaved changes.
/// </summary>
/// <param name="event">
/// Close window event information.
/// </param>
void WL4EditorWindow::closeEvent(QCloseEvent *event)
{
    if (UnsavedChanges)
    {
        // Show save prompt
        QMessageBox savePrompt;
        savePrompt.setWindowTitle(tr("Unsaved changes"));
        savePrompt.setText(tr("There are unsaved changes. Discard changes and quit anyway?"));
        QPushButton *quitButton = savePrompt.addButton(tr("Discard"), QMessageBox::DestructiveRole);
        QPushButton *cancelButton = savePrompt.addButton(tr("Cancel"), QMessageBox::NoRole);
        QPushButton *saveButton = savePrompt.addButton(tr("Save"), QMessageBox::ApplyRole);
        QPushButton *saveAsButton = savePrompt.addButton(tr("Save As"), QMessageBox::ApplyRole);
        savePrompt.setDefaultButton(cancelButton);
        savePrompt.exec();

        if (savePrompt.clickedButton() == quitButton)
        {
            event->accept();
            return;
        }
        else if (savePrompt.clickedButton() == saveButton)
        {
            // Do not exit if there was an issue saving the file
            if (!SaveCurrentFile())
            {
                event->ignore();
                return;
            }
        }
        else if (savePrompt.clickedButton() == saveAsButton)
        {
            // Do not exit if the file cannot be saved, or the user cancels the save prompt
            if (!SaveCurrentFileAs())
            {
                event->ignore();
                return;
            }
        }
        else
        {
            // If cancel is clicked, or X is clicked on the save prompt, then do nothing
            event->ignore();
            return;
        }
    }

    // No unsaved changes (quit)
    event->accept();
}

bool WL4EditorWindow::SaveCurrentFile()
{
    bool result = ROMUtils::SaveLevel(ROMUtils::ROMFileMetadata->FilePath);
    if (result)
    {
        int array_recent_room_start_id = SettingsUtils::IniKeys::RecentROM_0_RecentRoom_id;
        int array_recent_level_start_id = SettingsUtils::IniKeys::RecentROM_0_RecentLevel_id;
        int array_recent_passage_start_id = SettingsUtils::IniKeys::RecentROM_0_RecentPassage_id;
        SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_recent_level_start_id), QString::number(selectedLevel._LevelIndex));
        SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_recent_room_start_id), QString::number(ui->spinBox_RoomID->value()));
        SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_recent_passage_start_id), QString::number(selectedLevel._PassageIndex));
    }
    return result;
}

/// <summary>
/// Present the user with an "open level" dialog, in which a level can be selected to load.
/// </summary>
/// <remarks>
/// The newly loaded level will start by loading room 0 into the editor.
/// </remarks>
void WL4EditorWindow::on_loadLevelButton_clicked()
{
    // Check for unsaved operations
    if (!UnsavedChangesPrompt(tr("There are unsaved changes. Discard changes and load level anyway?")))
        return;

    // Deselect Door and Entity and deselect rect
    ui->graphicsView->DeselectDoorAndEntity(false);
    ui->graphicsView->ResetRectPixmaps();
    ui->graphicsView->ResetRect();

    // Load the selected level and render the screen
    ChooseLevelDialog tmpdialog(selectedLevel);
    if (tmpdialog.exec() == QDialog::Accepted)
    {
        selectedLevel = tmpdialog.GetResult();
        if (CurrentLevel)
            delete CurrentLevel;
        CurrentLevel =
            new LevelComponents::Level(static_cast<enum LevelComponents::__passage>(selectedLevel._PassageIndex),
                                       static_cast<enum LevelComponents::__stage>(selectedLevel._LevelIndex));
        ui->spinBox_RoomID->setValue(0);
        LoadRoomUIUpdate();
        int tmpTilesetID = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()]->GetTilesetID();
        Tile16SelecterWidget->SetTileset(tmpTilesetID);
        ResetEntitySetDockWidget();
        ResetCameraControlDockWidget();

        // Set program control changes
        UnsavedChanges = false;
        ResetUndoHistory();
        OutputWidget->ClearTextEdit();
    }
}

/// <summary>
/// Provide the user with a choice whether or not to save the ROM if there are unsaved changes.
/// </summary>
/// <param name="str">
/// The message to display in the save prompt.
/// </param>
/// <returns>
/// True if the user chose to continue with the save prompt.
/// </returns>
bool WL4EditorWindow::UnsavedChangesPrompt(QString str)
{
    if (UnsavedChanges)
    {
        // Show save prompt
        QMessageBox savePrompt;
        savePrompt.setWindowTitle(tr("Unsaved changes"));
        savePrompt.setText(str);
        QPushButton *discardButton = savePrompt.addButton(tr("Discard"), QMessageBox::DestructiveRole);
        QPushButton *cancelButton = savePrompt.addButton(tr("Cancel"), QMessageBox::NoRole);
        QPushButton *saveButton = savePrompt.addButton(tr("Save"), QMessageBox::ApplyRole);
        savePrompt.setDefaultButton(cancelButton);
        savePrompt.exec();

        if (savePrompt.clickedButton() == saveButton)
        {
            // Do not load level if there was an issue saving the file
            if (!SaveCurrentFile())
            {
                OutputWidget->PrintString(tr("Save failure!"));
                return false;
            }
            OutputWidget->PrintString(tr("Saved successfully!"));
            return true;
        }
        else if (savePrompt.clickedButton() == discardButton)
        {
            return true;
        }
        return false;
    }
    else
        return true;
}

/// <summary>
/// Clear eventhing in the current room.
/// But at least one door will be kept.
/// </summary>
/// <param name="no_warning">
/// Optional param for showing warning of deleting doors.
/// </param>
/// <param name="roomId">
/// Optional param for selecting a room to clear, set -1 as a default value for current room.
/// </param>
void WL4EditorWindow::ClearEverythingInRoom(bool no_warning)
{
    bool IfDeleteAllDoors = false;
    // Show asking deleting Doors messagebox
    if(no_warning == false)
    {
        QMessageBox IfDeleteDoors;
        IfDeleteDoors.setWindowTitle(tr("WL4Editor"));
        IfDeleteDoors.setText(tr(
            "You just triggered the clear-all shortcut for current Room.\nDo you want to delete all the doors in this Room at the same time?\n"
            "(Only one door will be reserved to and keep data association for entityset settings.\n"
            "Camera limitators will also be reset.)"));
        QPushButton *CancelClearingButton = IfDeleteDoors.addButton(tr("Cancel Clearing"), QMessageBox::RejectRole);
        QPushButton *NoButton = IfDeleteDoors.addButton(tr("No"), QMessageBox::NoRole);
        QPushButton *YesButton = IfDeleteDoors.addButton(tr("Yes"), QMessageBox::ApplyRole);
        IfDeleteDoors.setDefaultButton(CancelClearingButton);
        IfDeleteDoors.exec();

        if (IfDeleteDoors.clickedButton() == YesButton)
        {
            IfDeleteAllDoors = true;
        }
        else if (IfDeleteDoors.clickedButton() != NoButton)
        {
            return;
        }
    }
    else
    {
        IfDeleteAllDoors = true;
    }

    int currentRoomID = ui->spinBox_RoomID->value();
    LevelComponents::Room *currentRoom = CurrentLevel->GetRooms()[currentRoomID];

    // Capture old states before clearing
    struct OperationParams *operation = new struct OperationParams;

    // -- Layers --
    int w0 = currentRoom->GetLayer0Width(), h0 = currentRoom->GetLayer0Height();
    unsigned short *oldLayer0 = nullptr;
    if (currentRoom->GetLayer(0)->GetMappingType() == LevelComponents::LayerMap16)
        oldLayer0 = currentRoom->GetLayer(0)->CreateLayerDataCopy();

    int w1 = currentRoom->GetLayer1Width(), h1 = currentRoom->GetLayer1Height();
    unsigned short *oldLayer1 = nullptr;
    if (currentRoom->GetLayer(1)->GetMappingType() == LevelComponents::LayerMap16)
        oldLayer1 = currentRoom->GetLayer(1)->CreateLayerDataCopy();

    int w2 = currentRoom->GetLayer(2)->GetLayerWidth(), h2 = currentRoom->GetLayer(2)->GetLayerHeight();
    unsigned short *oldLayer2 = nullptr;
    if (currentRoom->GetLayer(2)->GetMappingType() == LevelComponents::LayerMap16)
        oldLayer2 = currentRoom->GetLayer(2)->CreateLayerDataCopy();

    // -- Entity lists --
    std::vector<LevelComponents::EntityRoomAttribute> oldNormal = currentRoom->GetEntityListData(1);
    std::vector<LevelComponents::EntityRoomAttribute> oldHard = currentRoom->GetEntityListData(0);
    std::vector<LevelComponents::EntityRoomAttribute> oldSHard = currentRoom->GetEntityListData(2);

    // -- Doors (if deleting) --
    LevelComponents::LevelDoorVector *oldDoorVec = nullptr;
    if (IfDeleteAllDoors)
        oldDoorVec = new LevelComponents::LevelDoorVector(CurrentLevel->GetDoorList());

    // -- Camera control --
    auto oldCameraType = currentRoom->GetCameraControlType();
    std::vector<struct LevelComponents::__CameraControlRecord *> oldCameraRecords;
    bool hasCameraControl = (oldCameraType == LevelComponents::HasControlAttrs);
    if (hasCameraControl)
        oldCameraRecords = currentRoom->GetCameraControlRecords(true);

    // Clear Layers 0, 1, 2
    for (int i = 0; i < 3; ++i)
    {
        LevelComponents::Layer *layer = currentRoom->GetLayer(i);
        if (layer->GetMappingType() == LevelComponents::LayerMap16)
        {
            layer->ResetData();
        }
    }

    // Delete Entity lists and set dirty
    for (int i = 0; i < 3; ++i)
    {
        currentRoom->ClearEntitylist(i);
        currentRoom->SetEntityListDirty(i, true);
    }

    // Delete most of the Doors
    if (IfDeleteAllDoors)
    {
        LevelComponents::LevelDoorVector &allDoor = CurrentLevel->GetDoorListRef();
        int doornum = allDoor.size();
        int curRoomDoorIdIter = -1;
        for (int i = doornum - 1; i > 0; i--)
        {
            auto curDoor = allDoor.GetDoor(i);
            if (curDoor.RoomID == currentRoom->GetRoomID())
            {
                curRoomDoorIdIter++;
                if (curRoomDoorIdIter > 0)
                {
                    CurrentLevel->DeleteDoorByGlobalID(i);
                }
            }
        }
    }

    // Clear camera limitators — reset to a single default limitator
    if (hasCameraControl)
    {
        while (currentRoom->GetCameraControlRecords(false).size() > 0)
        {
            currentRoom->DeleteCameraLimitator((int)currentRoom->GetCameraControlRecords(false).size() - 1);
        }
        currentRoom->AddCameraLimitator();
    }

    // Capture new states after clearing
    unsigned short *newLayer0 = nullptr, *newLayer1 = nullptr, *newLayer2 = nullptr;
    if (oldLayer0) newLayer0 = currentRoom->GetLayer(0)->CreateLayerDataCopy();
    if (oldLayer1) newLayer1 = currentRoom->GetLayer(1)->CreateLayerDataCopy();
    if (oldLayer2) newLayer2 = currentRoom->GetLayer(2)->CreateLayerDataCopy();

    std::vector<LevelComponents::EntityRoomAttribute> newNormal = currentRoom->GetEntityListData(1);
    std::vector<LevelComponents::EntityRoomAttribute> newHard = currentRoom->GetEntityListData(0);
    std::vector<LevelComponents::EntityRoomAttribute> newSHard = currentRoom->GetEntityListData(2);

    // -- Camera control --
    auto newCameraType = currentRoom->GetCameraControlType();
    std::vector<struct LevelComponents::__CameraControlRecord *> newCameraRecords;
    if (hasCameraControl)
        newCameraRecords = currentRoom->GetCameraControlRecords(true);

    // Build operation params
    if (oldLayer0)
    {
        operation->layer0Change = true;
        operation->layer0ChangeParams = LayerChangeParams::Create(oldLayer0, newLayer0, w0, h0, currentRoom->GetRoomID());
        delete[] oldLayer0; delete[] newLayer0;
    }
    if (oldLayer1)
    {
        operation->layer1Change = true;
        operation->layer1ChangeParams = LayerChangeParams::Create(oldLayer1, newLayer1, w1, h1, currentRoom->GetRoomID());
        delete[] oldLayer1; delete[] newLayer1;
    }
    if (oldLayer2)
    {
        operation->layer2Change = true;
        operation->layer2ChangeParams = LayerChangeParams::Create(oldLayer2, newLayer2, w2, h2, currentRoom->GetRoomID());
        delete[] oldLayer2; delete[] newLayer2;
    }

    operation->entityNormalChange = true;
    operation->entityNormalChangeParams = EntityListChangeParams::Create(oldNormal, newNormal, currentRoom->GetRoomID());
    operation->entityHardChange = true;
    operation->entityHardChangeParams = EntityListChangeParams::Create(oldHard, newHard, currentRoom->GetRoomID());
    operation->entitySHardChange = true;
    operation->entitySHardChangeParams = EntityListChangeParams::Create(oldSHard, newSHard, currentRoom->GetRoomID());

    if (IfDeleteAllDoors)
    {
        LevelComponents::LevelDoorVector *newDoorVec =
            new LevelComponents::LevelDoorVector(CurrentLevel->GetDoorList());
        operation->doorVectorChange = true;
        operation->doorVectorChangeParams = DoorVectorChangeParams::Create(oldDoorVec, newDoorVec);
    }

    if (hasCameraControl)
    {
        operation->cameraControlChange = true;
        operation->cameraControlChangeParams = CameraControlChangeParams::Create(
            currentRoom->GetRoomID(),
            oldCameraType, newCameraType,
            oldCameraRecords, newCameraRecords);
    }

    ExecuteOperation(operation);

    // UI update
    ResetEntitySetDockWidget();
    RenderScreenFull();

    // Set change flag
    SetUnsavedChanges(true);
}

/// <summary>
/// Initialize the recent file menu entries.
/// </summary>
void WL4EditorWindow::InitRecentFileMenuEntries(const bool manageRecentScripts)
{
    // variable settings
    QMenu *filemenu = ui->menuRecent_ROM;
    int recentFileNum = recentROMnum;
    int array_max_size = SettingsUtils::RecentFileNum;
    int array_start_id = 1;
    if (manageRecentScripts)
    {
        filemenu = ui->menuRecent_Script;
        recentFileNum = recentScriptNum;
        array_start_id = 9;
    }
    QAction **actionlist_ptr = new QAction*[array_max_size];
    if (!manageRecentScripts) {
        for (int i = 0; i < array_max_size; i++) {
            actionlist_ptr[i] = RecentROMs[i];
        }
    } else {
        for (int i = 0; i < array_max_size; i++) {
            actionlist_ptr[i] = RecentScripts[i];
        }
    }

    // Add Recent ROM QAction according to the INI file
    for(uint i = 0; i < array_max_size; i++)
    {
        recentFileNum = i;
        QString filepath = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + array_start_id));
        if(!filepath.length())
        {
            if(i == 0)
            {
                actionlist_ptr[0] = new QAction("-/-", this);
                filemenu->addAction(actionlist_ptr[0]);
                if (!manageRecentScripts) {
                    connect(actionlist_ptr[0], SIGNAL(triggered()), this, SLOT(openRecentROM()));
                } else {
                    connect(actionlist_ptr[0], SIGNAL(triggered()), this, SLOT(openRecentScript()));
                }
            }
            break;
        }
        actionlist_ptr[i] = new QAction(filepath, this);
        filemenu->addAction(actionlist_ptr[i]);
        if (!manageRecentScripts) {
            connect(actionlist_ptr[i], SIGNAL(triggered()), this, SLOT(openRecentROM()));
        } else {
            connect(actionlist_ptr[i], SIGNAL(triggered()), this, SLOT(openRecentScript()));
        }
    }

    // write back the value to the global variables
    if (!manageRecentScripts) {
        recentROMnum = recentFileNum;
        for (int i = 0; i < array_max_size; i++)
        {
            RecentROMs[i] = actionlist_ptr[i];
        }
    } else {
        recentScriptNum = recentFileNum;
        for (int i = 0; i < array_max_size; i++)
        {
            RecentScripts[i] = actionlist_ptr[i];
        }
    }
}

/// <summary>
/// open a recent ROM or a recent script file from a menu entry.
/// this function check file existance. and don't add new file into the list.
/// </summary>
/// <return>
/// return false if the file cannot be loaded.
/// </return>
bool WL4EditorWindow::OpenRecentFile(QString filepath, const bool manageRecentScripts)
{
    // Check if it is a valid slot function call
    if(filepath == "-/-" || filepath == "") return false;

    int recentFileNum = recentROMnum;
    int array_max_size = SettingsUtils::RecentFileNum;
    int array_start_id = SettingsUtils::IniKeys::RecentROMPath_0;
    int array_recent_room_start_id = SettingsUtils::IniKeys::RecentROM_0_RecentRoom_id;
    int array_recent_level_start_id = SettingsUtils::IniKeys::RecentROM_0_RecentLevel_id;
    int array_recent_passage_start_id = SettingsUtils::IniKeys::RecentROM_0_RecentPassage_id;
    if (manageRecentScripts)
    {
        recentFileNum = recentScriptNum;
        array_start_id = SettingsUtils::IniKeys::RecentScriptPath_0;
    }
    QAction **actionlist_ptr = new QAction*[array_max_size];
    if (!manageRecentScripts) {
        for (int i = 0; i < array_max_size; i++) {
            actionlist_ptr[i] = RecentROMs[i];
        }
    } else {
        for (int i = 0; i < array_max_size; i++) {
            actionlist_ptr[i] = RecentScripts[i];
        }
    }

    // Check if the file exist, if not, modify the Recent ROM QAction list
    QFile file(filepath);
    bool result = true;
    if(!file.exists())
    {
        if(recentFileNum == 1)
        {
            SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_start_id), "");
            if (manageRecentScripts == false)
            {
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_recent_level_start_id), "");
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_recent_room_start_id), "");
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_recent_passage_start_id), "");
            }
            actionlist_ptr[0]->setText("-/-");
        }
        if(recentFileNum > 1)
        {
            int deletelinenum = -1;
            for(int i = 0; i < array_max_size; i++)
            {
                if(actionlist_ptr[i]->text() == filepath)
                {
                    deletelinenum = i;
                    break;
                }
            }
            for(int i = deletelinenum; i < (recentFileNum - 1); i++)
            {
                actionlist_ptr[i]->setText(actionlist_ptr[i + 1]->text());
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + array_start_id), actionlist_ptr[i + 1]->text());
                if (manageRecentScripts == false)
                {
                    QString recent_levelid = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_level_start_id + 1));
                    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_level_start_id), recent_levelid);
                    QString recent_roomid = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_room_start_id + 1));
                    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_room_start_id), recent_roomid);
                    QString recent_passageid = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_passage_start_id + 1));
                    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_passage_start_id), recent_passageid);
                }
            }
            SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(recentFileNum + array_start_id - 1), "");
            if (manageRecentScripts == false)
            {
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(recentFileNum + array_recent_level_start_id - 1), "");
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(recentFileNum + array_recent_room_start_id - 1), "");
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(recentFileNum + array_recent_passage_start_id - 1), "");
            }
            delete actionlist_ptr[recentFileNum - 1];
        }
        recentFileNum--;
        QMessageBox::critical(nullptr, QString(tr("Load Error")), QString(tr("This File no longer exists!")));
        result = false;
    }

    // write back the value to the global variables
    if (!manageRecentScripts) {
        recentROMnum = recentFileNum;
        for (int i = 0; i < array_max_size; i++)
        {
            RecentROMs[i] = actionlist_ptr[i];
        }
    } else {
        recentScriptNum = recentFileNum;
        for (int i = 0; i < array_max_size; i++)
        {
            RecentScripts[i] = actionlist_ptr[i];
        }
    }

    return result;
}

/// <summary>
/// a manager to deal with the recent ROM and recent script files' actions in the menu.
/// this function won't check file existance. and will add new file into the list.
/// </summary>
void WL4EditorWindow::ManageRecentFilesOrScripts(QString newFilepath, const bool manageRecentScripts)
{
    int foundInRecentFile = -1; // start by 0
    int recentFileNum = recentROMnum;
    int array_max_size = SettingsUtils::RecentFileNum;
    int array_start_id = SettingsUtils::IniKeys::RecentROMPath_0;
    int array_recent_room_start_id = SettingsUtils::IniKeys::RecentROM_0_RecentRoom_id;
    int array_recent_level_start_id = SettingsUtils::IniKeys::RecentROM_0_RecentLevel_id;
    int array_recent_passage_start_id = SettingsUtils::IniKeys::RecentROM_0_RecentPassage_id;
    QMenu *filemenu = ui->menuRecent_ROM;
    if (manageRecentScripts)
    {
        filemenu = ui->menuRecent_Script;
        recentFileNum = recentScriptNum;
        array_start_id = SettingsUtils::IniKeys::RecentScriptPath_0;
    }
    if(recentFileNum > 0)
    {
        for(uint i = 0; i < recentFileNum; i++)
        {
            QString filepath = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + array_start_id));
            if(filepath == newFilepath)
            {
                foundInRecentFile = i;
                break;
            }
        }
    }
    QAction **actionlist_ptr = new QAction*[array_max_size];
    if (!manageRecentScripts) {
        for (int i = 0; i < array_max_size; i++) {
            actionlist_ptr[i] = RecentROMs[i];
        }
    } else {
        for (int i = 0; i < array_max_size; i++) {
            actionlist_ptr[i] = RecentScripts[i];
        }
    }

    // new file never be loaded into the WL4Editor before
    if(foundInRecentFile == -1)
    {
        // we have other recent file record in the list
        if(recentFileNum > 0)
        {
            // add the new file QAction to the menu, append a new entry to the bottom of the list, using the last name from the list
            if(recentFileNum < array_max_size)
            {
                actionlist_ptr[recentFileNum] = new QAction(actionlist_ptr[recentFileNum - 1]->text(), this);
                filemenu->addAction(actionlist_ptr[recentFileNum]);
                if (!manageRecentScripts) {
                    connect(actionlist_ptr[recentFileNum], SIGNAL(triggered()), this, SLOT(openRecentROM()));
                } else {
                    connect(actionlist_ptr[recentFileNum], SIGNAL(triggered()), this, SLOT(openRecentScript()));
                }
            }

            // get recent file record from the ini file and set the other QActions text, from oldest to newest
            // move 3 -> 4, 2 -> 3, 1 -> 2, 0 -> 1
            for(uint i = (std::min(recentFileNum, array_max_size - 1)); i > 0 ; i--)
            {
                QString filepath = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + array_start_id - 1));
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + array_start_id), filepath);
                if (manageRecentScripts == false)
                {
                    QString recent_levelid = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_level_start_id - 1));
                    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_level_start_id), recent_levelid);
                    QString recent_roomid = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_room_start_id - 1));
                    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_room_start_id), recent_roomid);
                    QString recent_passageid = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_passage_start_id - 1));
                    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_passage_start_id), recent_passageid);
                }
                actionlist_ptr[i]->setText(filepath);
            }
            // reset recent passage, level and room id to 0 for new rom
            if (manageRecentScripts == false)
            {
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_recent_level_start_id), "");
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_recent_room_start_id), "");
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_recent_passage_start_id), "");
            }
        }
        recentFileNum++;
    }
    else
    {
        // it is some file which has record in the recent file list
        // do nothing if it is already the first one in the list
        // modify the list if it is not the fist one in the list
        if(foundInRecentFile > 0)
        {
            // backup the ROM loading info from the current one
            QString current_levelid = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(foundInRecentFile + array_recent_level_start_id));
            QString current_roomid = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(foundInRecentFile + array_recent_room_start_id));
            QString current_passageid = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(foundInRecentFile + array_recent_passage_start_id));
            for(int i = foundInRecentFile; i > 0; i--)
            {
                // get recent file record from the ini file and set the other QActions text, from oldest to newest
                // do a part of (3 -> 4, 2 -> 3, 1 -> 2, 0 -> 1) move, from where the file got found in the recent file list
                QString filepath = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + array_start_id - 1));
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + array_start_id), filepath);
                if (manageRecentScripts == false)
                {
                    QString recent_levelid = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_level_start_id - 1));
                    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_level_start_id), recent_levelid);
                    QString recent_roomid = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_room_start_id - 1));
                    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_room_start_id), recent_roomid);
                    QString recent_passageid = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_passage_start_id - 1));
                    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + array_recent_passage_start_id), recent_passageid);
                }
                actionlist_ptr[i]->setText(filepath);
            }
            // then the ROM loading info of the current one will be put on the top
            // do (foundInRecentFile -> 0) move
            if (manageRecentScripts == false)
            {
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_recent_level_start_id), current_levelid);
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_recent_room_start_id), current_roomid);
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_recent_passage_start_id), current_passageid);
            }
        }
    }
    // now we update the current file to be the first one in the list, in both ini and runtime WL4Editor variables
    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(array_start_id), newFilepath);
    actionlist_ptr[0]->setText(newFilepath);

    // write back the value to the global variables
    if (!manageRecentScripts) {
        recentROMnum = recentFileNum;
        for (int i = 0; i < array_max_size; i++)
        {
            RecentROMs[i] = actionlist_ptr[i];
        }
    } else {
        recentScriptNum = recentFileNum;
        for (int i = 0; i < array_max_size; i++)
        {
            RecentScripts[i] = actionlist_ptr[i];
        }
    }
}

/// <summary>
/// Decrease the index of the currently loaded room.
/// </summary>
/// <remarks>
/// If the new room is index 0, then this button will become disabled.
/// The increase room index button will be enabled.
/// </remarks>
void WL4EditorWindow::on_roomDecreaseButton_clicked()
{
    unsigned int currentroomid = ui->spinBox_RoomID->value();
    SetCurrentRoomId(currentroomid - 1);
}

/// <summary>
/// Increase the index of the currently loaded room.
/// </summary>
/// <remarks>
/// If the new room is the last available, then this button will become disabled.
/// The decrease room index button will be enabled.
/// </remarks>
void WL4EditorWindow::on_roomIncreaseButton_clicked()
{
    unsigned int currentroomid = ui->spinBox_RoomID->value();
    SetCurrentRoomId(currentroomid + 1);
}

/// <summary>
/// Present the user with a level config dialog for changing the level title and frog timer values.
/// </summary>
/// <remarks>
/// Only set the new parameters from the dialog if OK is pressed.
/// </remarks>
void WL4EditorWindow::on_actionLevel_Config_triggered()
{
    // Capture old state before the dialog
    DialogParams::LevelConfigParams *lastParams = new DialogParams::LevelConfigParams();
    lastParams->oldLevelName = CurrentLevel->GetLevelName();
    lastParams->oldLevelNameJ = CurrentLevel->GetLevelName(1);
    lastParams->oldHModeTimer = CurrentLevel->GetTimeCountdownCounter(LevelComponents::HardDifficulty);
    lastParams->oldNModeTimer = CurrentLevel->GetTimeCountdownCounter(LevelComponents::NormalDifficulty);
    lastParams->oldSHModeTimer = CurrentLevel->GetTimeCountdownCounter(LevelComponents::SHardDifficulty);

    // Show a level config dialog to the user
    LevelConfigDialog dialog;
    dialog.InitTextBoxes(CurrentLevel->GetLevelName(),
                         CurrentLevel->GetLevelName(1),
                         lastParams->oldHModeTimer,
                         lastParams->oldNModeTimer,
                         lastParams->oldSHModeTimer);

    // If OK is pressed, then set the level attributes
    auto acc = dialog.exec();
    if (acc == QDialog::Accepted)
    {
        // Capture new state
        DialogParams::LevelConfigParams *newParams = new DialogParams::LevelConfigParams();
        newParams->newLevelName = dialog.GetPaddedLevelName();
        newParams->newLevelNameJ = dialog.GetPaddedLevelName(1);
        newParams->newHModeTimer = dialog.GetHModeTimer();
        newParams->newNModeTimer = dialog.GetNModeTimer();
        newParams->newSHModeTimer = dialog.GetSHModeTimer();

        // Apply changes
        CurrentLevel->SetLevelName(newParams->newLevelName);
        CurrentLevel->SetLevelName(newParams->newLevelNameJ, 1);
        CurrentLevel->SetTimeCountdownCounter(LevelComponents::HardDifficulty, (unsigned int) newParams->newHModeTimer);
        CurrentLevel->SetTimeCountdownCounter(LevelComponents::NormalDifficulty, (unsigned int) newParams->newNModeTimer);
        CurrentLevel->SetTimeCountdownCounter(LevelComponents::SHardDifficulty, (unsigned int) newParams->newSHModeTimer);

        // Create and execute operation
        OperationParams *operation = new OperationParams;
        operation->levelConfigChange = true;
        operation->lastLevelConfigParams = lastParams;
        operation->newLevelConfigParams = newParams;
        ExecuteOperation(operation);
    }
    else
    {
        delete lastParams;
    }
}

/// <summary>
/// Resize the main window, and set the size of the edit mode dock widget to its minimum size.
/// </summary>
/// <remarks>
/// Without this override, the edit mode widget and tile 16 selector widget will each take up half the space
/// on the side of the main window if they are both docked.
/// </remarks>
/// <param name="event">
/// Resize event information which is sent to the parent implementation of this function.
/// </param>
void WL4EditorWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (firstROMLoaded)
    {
        resizeDocks({ EditModeWidget }, { 1 }, Qt::Vertical);
    }
}

/// <summary>
/// Undo the previous operation, if an action has been performed.
/// </summary>
void WL4EditorWindow::on_actionUndo_triggered()
{
    UndoOperation();
}

/// <summary>
/// Redo a previously undone operation.
/// </summary>
void WL4EditorWindow::on_actionRedo_triggered()
{
    RedoOperation();
}

/// <summary>
/// Show the user a dialog for configuring the current room. If the user clicks OK, apply selected parameters to the
/// room.
/// </summary>
void WL4EditorWindow::on_actionRoom_Config_triggered()
{
    // Set up parameters for the currently selected room, for the purpose of initializing the dialog's selections
    DialogParams::RoomConfigParams *_currentRoomConfigParams =
        new DialogParams::RoomConfigParams(CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()]);

    // Show the dialog
    RoomConfigDialog dialog(this, _currentRoomConfigParams);
    if (dialog.exec() == QDialog::Accepted)
    {
        DialogParams::RoomConfigParams *newRoomConfigParams = dialog.GetConfigParams(_currentRoomConfigParams);

        OperationParams *operation = new OperationParams;
        operation->roomConfigChange = true;
        operation->lastRoomConfigParams = new DialogParams::RoomConfigParams(*_currentRoomConfigParams);
        operation->newRoomConfigParams = newRoomConfigParams;

        // Capture door, entity, and camera states when room dimensions change,
        // since out-of-bounds elements will be trimmed
        if (newRoomConfigParams->RoomWidth != _currentRoomConfigParams->RoomWidth ||
            newRoomConfigParams->RoomHeight != _currentRoomConfigParams->RoomHeight)
        {
            LevelComponents::Room *currentRoom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];

            // Capture pre-trim states
            LevelComponents::LevelDoorVector *oldDoorVec =
                new LevelComponents::LevelDoorVector(CurrentLevel->GetDoorList());

            std::vector<LevelComponents::EntityRoomAttribute> oldNormal = currentRoom->GetEntityListData(1);
            std::vector<LevelComponents::EntityRoomAttribute> oldHard = currentRoom->GetEntityListData(0);
            std::vector<LevelComponents::EntityRoomAttribute> oldSHard = currentRoom->GetEntityListData(2);

            auto oldCameraType = currentRoom->GetCameraControlType();
            auto oldCameraRecords = currentRoom->GetCameraControlRecords(true);

            // Trim out-of-bounds elements
            TrimElementsOutOfRoomBounds(currentRoom,
                                        newRoomConfigParams->RoomWidth,
                                        newRoomConfigParams->RoomHeight);

            // Capture post-trim states
            LevelComponents::LevelDoorVector *newDoorVec =
                new LevelComponents::LevelDoorVector(CurrentLevel->GetDoorList());

            std::vector<LevelComponents::EntityRoomAttribute> newNormal = currentRoom->GetEntityListData(1);
            std::vector<LevelComponents::EntityRoomAttribute> newHard = currentRoom->GetEntityListData(0);
            std::vector<LevelComponents::EntityRoomAttribute> newSHard = currentRoom->GetEntityListData(2);

            auto newCameraType = currentRoom->GetCameraControlType();
            auto newCameraRecords = currentRoom->GetCameraControlRecords(true);

            // Add door change to operation
            operation->doorVectorChange = true;
            operation->doorVectorChangeParams = DoorVectorChangeParams::Create(oldDoorVec, newDoorVec);

            // Add entity list changes to operation
            operation->entityNormalChange = true;
            operation->entityNormalChangeParams = EntityListChangeParams::Create(oldNormal, newNormal, currentRoom->GetRoomID());
            operation->entityHardChange = true;
            operation->entityHardChangeParams = EntityListChangeParams::Create(oldHard, newHard, currentRoom->GetRoomID());
            operation->entitySHardChange = true;
            operation->entitySHardChangeParams = EntityListChangeParams::Create(oldSHard, newSHard, currentRoom->GetRoomID());

            // Add camera control change to operation
            operation->cameraControlChange = true;
            operation->cameraControlChangeParams = CameraControlChangeParams::Create(
                currentRoom->GetRoomID(),
                oldCameraType, newCameraType,
                oldCameraRecords, newCameraRecords);
        }

        ExecuteOperation(operation); // Set UnsavedChanges bool inside
    }
}

/// <summary>
/// Edit current tileset.
/// </summary>
void WL4EditorWindow::on_actionEdit_Tileset_triggered()
{
    // Set up parameters for the currently selected room, for the purpose of initializing the dialog's selections
    DialogParams::TilesetEditParams *_newRoomTilesetEditParams =
        new DialogParams::TilesetEditParams(CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()]);

    // call helper function to open dialog and apply changes
    EditCurrentTileset(_newRoomTilesetEditParams);
}

/// <summary>
/// Add a new Door to the current room.
/// </summary>
void WL4EditorWindow::on_actionNew_Door_triggered()
{
    unsigned int currentroomid = ui->spinBox_RoomID->value();

    LevelComponents::LevelDoorVector *oldDoorVec =
        new LevelComponents::LevelDoorVector(CurrentLevel->GetDoorList());
    CurrentLevel->AddDoor(currentroomid, (unsigned char) CurrentLevel->GetRooms()[currentroomid]->GetCurrentEntitySetID());
    LevelComponents::LevelDoorVector *newDoorVec =
        new LevelComponents::LevelDoorVector(CurrentLevel->GetDoorList());

    struct OperationParams *params = new struct OperationParams();
    params->doorVectorChange = true;
    params->doorVectorChangeParams = DoorVectorChangeParams::Create(oldDoorVec, newDoorVec);
    ExecuteOperation(params);

    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
}

/// <summary>
/// Call the function which saves the currently loaded level.
/// </summary>
void WL4EditorWindow::on_actionSave_ROM_triggered()
{
    if (SaveCurrentFile())
    {
        OutputWidget->PrintString(tr("Saved successfully!"));
    }
    else
    {
        OutputWidget->PrintString(tr("Save failure!"));
    }
}

/// <summary>
/// Select a file, and save the modified ROM to the file.
/// </summary>
void WL4EditorWindow::on_actionSave_As_triggered()
{
    if (SaveCurrentFileAs())
    {
        OutputWidget->PrintString(tr("Saved successfully!"));
    }
    else
    {
        OutputWidget->PrintString(tr("Save failure!"));
    }
}

/// <summary>
/// Select a file, and save the modified ROM to the file.
/// </summary>
/// <returns>
/// True if the file was saved. False if the user declined, or was unable to save the file.
/// </returns>
bool WL4EditorWindow::SaveCurrentFileAs()
{
    QString qFilePath =
        QFileDialog::getSaveFileName(this, tr("Save ROM file as"), dialogInitialPath, tr("GBA ROM files (*.gba)"));
    if (qFilePath.compare(""))
    {
        if (ROMUtils::SaveLevel(qFilePath))
        {
            // If successful in saving the file, set the window title to reflect the new file
            ROMUtils::ROMFileMetadata->FilePath = qFilePath;
            dialogInitialPath = QFileInfo(qFilePath).dir().path();
            std::string filePath = qFilePath.toStdString();
            std::string fileName = filePath.substr(filePath.rfind('/') + 1);
            setWindowTitle(fileName.c_str());
            return true;
        }
    }
    return false;
}

/// <summary>
/// Show information about the editor.
/// </summary>
void WL4EditorWindow::on_actionAbout_triggered()
{
    // Show the about dialog
    QMessageBox infoPrompt;
    infoPrompt.setWindowTitle(tr("About"));
    infoPrompt.setText(QString("WL4Editor human contributors in alphabetical order are:\n"
                               "    chanchancl\n"
                               "    Goldensunboy\n"
                               "    IamRifki\n"
                               "    Kleyment\n"
                               "    shinespeciall\n"
                               "    xiazhanjian\n\n"
                               "AI contributor(s):\n"
                               "    deepseek-v4\n\n"
                               "Special Thanks:\n"
                               "    becored\n"
                               "    Blanchon\n"
                               /*"    Dax89 (QHexView)\n"*/ // have not been used in release builds yet
                               "    Hiro_sofT\n"
                               "    interdpth\n"
                               "    MemeMayhem (icon)\n"
                               "    Spleeeeen\n"
                               "    xTibor\n\n"
                               "Version: ") +
                       WL4EDITOR_VERSION);
    QPushButton *changelogButton = infoPrompt.addButton(tr("Ok"), QMessageBox::NoRole);
    infoPrompt.exec();
    /*
    if(infoPrompt.clickedButton() == changelogButton)
    {
        // Get the changelog
        const QString URI("https://raw.githubusercontent.com/Goldensunboy/WL4Editor/master/LICENSE");
        QUrl URL = QUrl::fromEncoded(URI.toLocal8Bit());
        QNetworkRequest request(URL);
        QNetworkAccessManager manager;
        QNetworkReply *reply = manager.get(request);
        QString errorText = reply->errorString();
        QByteArray data = reply->readAll();
        QString changelogText = QString::fromUtf8(data.data(), data.size());
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString statusString = QVariant(statusCode).toString();

        // If the changelog button is clicked, show the changelog
        QDialog changelogDialog(this);
        changelogDialog.setWindowTitle("Changelog");
        QHBoxLayout *layout = new QHBoxLayout();
        QTextEdit *textArea = new QTextEdit();
        textArea->setReadOnly(true);
        layout->addWidget(textArea);
        textArea->setText(statusString);
        changelogDialog.setLayout(layout);
        changelogDialog.exec();
        delete textArea;
        delete layout;
    }
    */
    changelogButton = changelogButton;
}

static const char *layerSwapFailureMsg = "Swapping Layers failed!";

/// <summary>
/// Swap the Layerdata for Layer_0 and Layer_1.
/// </summary>
void WL4EditorWindow::on_action_swap_Layer_0_Layer_1_triggered()
{
    // TODO: support swap a disabled Layer with a normal Layer
    // swap Layerdata if possible
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];
    if (!(currentroom->GetLayer(0)->IsEnabled()))
    {
        OutputWidget->PrintString(tr(layerSwapFailureMsg));
        return;
    }
    if (currentroom->GetLayer(0)->GetMappingType() != LevelComponents::LayerMap16)
    {
        OutputWidget->PrintString(tr(layerSwapFailureMsg));
        return;
    }

    // Capture old layer data before swap
    int w0 = currentroom->GetLayer0Width();
    int h0 = currentroom->GetLayer0Height();
    unsigned short *oldData0 = currentroom->GetLayer(0)->CreateLayerDataCopy();
    int w1 = currentroom->GetLayer1Width();
    int h1 = currentroom->GetLayer1Height();
    unsigned short *oldData1 = currentroom->GetLayer(1)->CreateLayerDataCopy();

    // Copy data between layer buffers instead of swapping pointers,
    // so each layer keeps a buffer sized for its own dimensions
    int size0 = w0 * h0;
    int size1 = w1 * h1;
    unsigned short *layer0Data = currentroom->GetLayer(0)->GetLayerData();
    unsigned short *layer1Data = currentroom->GetLayer(1)->GetLayerData();

    memcpy(layer0Data, oldData1, 2 * std::min(size0, size1));
    if (size0 > size1)
        memset(layer0Data + size1, 0, 2 * (size0 - size1));

    memcpy(layer1Data, oldData0, 2 * std::min(size1, size0));
    if (size1 > size0)
        memset(layer1Data + size0, 0, 2 * (size1 - size0));

    // Capture new layer data after swap and record history
    unsigned short *newData0 = currentroom->GetLayer(0)->CreateLayerDataCopy();
    unsigned short *newData1 = currentroom->GetLayer(1)->CreateLayerDataCopy();
    struct OperationParams *operation = new struct OperationParams;

    operation->layer0Change = true;
    operation->layer1Change = true;
    operation->layer0ChangeParams = LayerChangeParams::Create(oldData0, newData0, w0, h0, currentroom->GetRoomID());
    operation->layer1ChangeParams = LayerChangeParams::Create(oldData1, newData1, w1, h1, currentroom->GetRoomID());
    delete[] oldData0; delete[] oldData1; delete[] newData0; delete[] newData1;
    ExecuteOperation(operation);

    // UI update
    RenderScreenFull();

    // Set Dirty and change flag
    currentroom->GetLayer(0)->SetDirty(true);
    currentroom->GetLayer(1)->SetDirty(true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Swap the Layerdata for Layer_1 and Layer_2.
/// </summary>
void WL4EditorWindow::on_action_swap_Layer_1_Layer_2_triggered()
{
    // TODO: support swap a disabled Layer with a normal Layer
    // swap Layerdata if possible
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];
    if (!(currentroom->GetLayer(2)->IsEnabled()))
    {
        OutputWidget->PrintString(tr(layerSwapFailureMsg));
        return;
    }

    // Capture old layer data before swap
    int w1 = currentroom->GetLayer1Width();
    int h1 = currentroom->GetLayer1Height();
    unsigned short *oldData1 = currentroom->GetLayer(1)->CreateLayerDataCopy();
    int w2 = currentroom->GetLayer(2)->GetLayerWidth();
    int h2 = currentroom->GetLayer(2)->GetLayerHeight();
    unsigned short *oldData2 = currentroom->GetLayer(2)->CreateLayerDataCopy();

    // Copy data between layer buffers instead of swapping pointers,
    // so each layer keeps a buffer sized for its own dimensions
    int size1 = w1 * h1;
    int size2 = w2 * h2;
    unsigned short *layer1Data = currentroom->GetLayer(1)->GetLayerData();
    unsigned short *layer2Data = currentroom->GetLayer(2)->GetLayerData();

    memcpy(layer1Data, oldData2, 2 * std::min(size1, size2));
    if (size1 > size2)
        memset(layer1Data + size2, 0, 2 * (size1 - size2));

    memcpy(layer2Data, oldData1, 2 * std::min(size2, size1));
    if (size2 > size1)
        memset(layer2Data + size1, 0, 2 * (size2 - size1));

    // Capture new layer data after swap and record history
    unsigned short *newData1 = currentroom->GetLayer(1)->CreateLayerDataCopy();
    unsigned short *newData2 = currentroom->GetLayer(2)->CreateLayerDataCopy();
    struct OperationParams *operation = new struct OperationParams;

    operation->layer1Change = true;
    operation->layer2Change = true;
    operation->layer1ChangeParams = LayerChangeParams::Create(oldData1, newData1, w1, h1, currentroom->GetRoomID());
    operation->layer2ChangeParams = LayerChangeParams::Create(oldData2, newData2, w2, h2, currentroom->GetRoomID());
    delete[] oldData1; delete[] oldData2; delete[] newData1; delete[] newData2;
    ExecuteOperation(operation);

    // UI update
    RenderScreenFull();

    // Set Dirty and change flag
    currentroom->GetLayer(1)->SetDirty(true);
    currentroom->GetLayer(2)->SetDirty(true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Swap the Layerdata for Layer_0 and Layer_2.
/// </summary>
void WL4EditorWindow::on_action_swap_Layer_0_Layer_2_triggered()
{
    // TODO: support swap a disabled Layer with a normal Layer
    // swap Layerdata if possible
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];
    if (!(currentroom->GetLayer(0)->IsEnabled()) ||
        !(currentroom->GetLayer(2)->IsEnabled()))
    {
        OutputWidget->PrintString(tr(layerSwapFailureMsg));
        return;
    }
    if (currentroom->GetLayer(0)->GetMappingType() != LevelComponents::LayerMap16)
    {
        OutputWidget->PrintString(tr(layerSwapFailureMsg));
        return;
    }

    // Capture old layer data before swap
    int w0 = currentroom->GetLayer0Width();
    int h0 = currentroom->GetLayer0Height();
    unsigned short *oldData0 = currentroom->GetLayer(0)->CreateLayerDataCopy();
    int w2 = currentroom->GetLayer(2)->GetLayerWidth();
    int h2 = currentroom->GetLayer(2)->GetLayerHeight();
    unsigned short *oldData2 = currentroom->GetLayer(2)->CreateLayerDataCopy();

    // Copy data between layer buffers instead of swapping pointers,
    // so each layer keeps a buffer sized for its own dimensions
    int size0 = w0 * h0;
    int size2 = w2 * h2;
    unsigned short *layer0Data = currentroom->GetLayer(0)->GetLayerData();
    unsigned short *layer2Data = currentroom->GetLayer(2)->GetLayerData();

    memcpy(layer0Data, oldData2, 2 * std::min(size0, size2));
    if (size0 > size2)
        memset(layer0Data + size2, 0, 2 * (size0 - size2));

    memcpy(layer2Data, oldData0, 2 * std::min(size2, size0));
    if (size2 > size0)
        memset(layer2Data + size0, 0, 2 * (size2 - size0));

    // Capture new layer data after swap and record history
    unsigned short *newData0 = currentroom->GetLayer(0)->CreateLayerDataCopy();
    unsigned short *newData2 = currentroom->GetLayer(2)->CreateLayerDataCopy();
    struct OperationParams *operation = new struct OperationParams;

    operation->layer0Change = true;
    operation->layer2Change = true;
    operation->layer0ChangeParams = LayerChangeParams::Create(oldData0, newData0, w0, h0, currentroom->GetRoomID());
    operation->layer2ChangeParams = LayerChangeParams::Create(oldData2, newData2, w2, h2, currentroom->GetRoomID());
    delete[] oldData0; delete[] oldData2; delete[] newData0; delete[] newData2;
    ExecuteOperation(operation);

    // UI update
    RenderScreenFull();

    // Set Dirty and change flag
    currentroom->GetLayer(0)->SetDirty(true);
    currentroom->GetLayer(2)->SetDirty(true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Swap Normal and Hard Entity lists.
/// </summary>
void WL4EditorWindow::on_action_swap_Normal_Hard_triggered()
{
    // swap Entity lists
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];

    // Capture old entity list data before swap
    std::vector<LevelComponents::EntityRoomAttribute> oldNormalList = currentroom->GetEntityListData(1);
    std::vector<LevelComponents::EntityRoomAttribute> oldHardList = currentroom->GetEntityListData(0);

    currentroom->SwapEntityLists(0, 1);

    // Capture new entity list data after swap and record history
    std::vector<LevelComponents::EntityRoomAttribute> newNormalList = currentroom->GetEntityListData(1);
    std::vector<LevelComponents::EntityRoomAttribute> newHardList = currentroom->GetEntityListData(0);
    struct OperationParams *operation = new struct OperationParams;

    operation->entityNormalChange = true;
    operation->entityHardChange = true;
    operation->entityNormalChangeParams = EntityListChangeParams::Create(oldNormalList, newNormalList, currentroom->GetRoomID());
    operation->entityHardChangeParams = EntityListChangeParams::Create(oldHardList, newHardList, currentroom->GetRoomID());
    ExecuteOperation(operation);

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    currentroom->SetEntityListDirty(0, true);
    currentroom->SetEntityListDirty(1, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Swap Hard and S-Hard Entity lists.
/// </summary>
void WL4EditorWindow::on_action_swap_Hard_S_Hard_triggered()
{
    // swap Entity lists
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];

    // Capture old entity list data before swap
    std::vector<LevelComponents::EntityRoomAttribute> oldHardList = currentroom->GetEntityListData(0);
    std::vector<LevelComponents::EntityRoomAttribute> oldSHardList = currentroom->GetEntityListData(2);

    currentroom->SwapEntityLists(0, 2);

    // Capture new entity list data after swap and record history
    std::vector<LevelComponents::EntityRoomAttribute> newHardList = currentroom->GetEntityListData(0);
    std::vector<LevelComponents::EntityRoomAttribute> newSHardList = currentroom->GetEntityListData(2);
    struct OperationParams *operation = new struct OperationParams;

    operation->entityHardChange = true;
    operation->entitySHardChange = true;
    operation->entityHardChangeParams = EntityListChangeParams::Create(oldHardList, newHardList, currentroom->GetRoomID());
    operation->entitySHardChangeParams = EntityListChangeParams::Create(oldSHardList, newSHardList, currentroom->GetRoomID());
    ExecuteOperation(operation);

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    currentroom->SetEntityListDirty(0, true);
    currentroom->SetEntityListDirty(2, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Swap Normal and S-Hard Entity lists.
/// </summary>
void WL4EditorWindow::on_action_swap_Normal_S_Hard_triggered()
{
    // swap Entity lists
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];

    // Capture old entity list data before swap
    std::vector<LevelComponents::EntityRoomAttribute> oldNormalList = currentroom->GetEntityListData(1);
    std::vector<LevelComponents::EntityRoomAttribute> oldSHardList = currentroom->GetEntityListData(2);

    currentroom->SwapEntityLists(1, 2);

    // Capture new entity list data after swap and record history
    std::vector<LevelComponents::EntityRoomAttribute> newNormalList = currentroom->GetEntityListData(1);
    std::vector<LevelComponents::EntityRoomAttribute> newSHardList = currentroom->GetEntityListData(2);
    struct OperationParams *operation = new struct OperationParams;

    operation->entityNormalChange = true;
    operation->entitySHardChange = true;
    operation->entityNormalChangeParams = EntityListChangeParams::Create(oldNormalList, newNormalList, currentroom->GetRoomID());
    operation->entitySHardChangeParams = EntityListChangeParams::Create(oldSHardList, newSHardList, currentroom->GetRoomID());
    ExecuteOperation(operation);

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    currentroom->SetEntityListDirty(1, true);
    currentroom->SetEntityListDirty(2, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Clear Layer 0 for the current Room if condition permit.
/// </summary>
void WL4EditorWindow::on_action_clear_Layer_0_triggered()
{
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];
    LevelComponents::Layer *layer0 = currentroom->GetLayer(0);
    if (layer0->GetMappingType() == LevelComponents::LayerMap16)
    {
        // Capture old layer data before clear
        int w = currentroom->GetLayer0Width();
        int h = currentroom->GetLayer0Height();
        unsigned short *oldData = layer0->CreateLayerDataCopy();

        layer0->ResetData();

        // Capture new layer data after clear and record history
        unsigned short *newData = layer0->CreateLayerDataCopy();
        struct OperationParams *operation = new struct OperationParams;
    
        operation->layer0Change = true;
        operation->layer0ChangeParams = LayerChangeParams::Create(oldData, newData, w, h, currentroom->GetRoomID());
        delete[] oldData; delete[] newData;
        ExecuteOperation(operation);
    }

    // UI update
    RenderScreenFull();

    // Set change flag
    SetUnsavedChanges(true);
}

/// <summary>
/// Clear Layer 1 for the current Room if condition permit.
/// </summary>
void WL4EditorWindow::on_action_clear_Layer_1_triggered()
{
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];
    LevelComponents::Layer *layer1 = currentroom->GetLayer(1);
    if (layer1->GetMappingType() == LevelComponents::LayerMap16)
    {
        // Capture old layer data before clear
        int w = currentroom->GetLayer1Width();
        int h = currentroom->GetLayer1Height();
        unsigned short *oldData = layer1->CreateLayerDataCopy();

        layer1->ResetData();

        // Capture new layer data after clear and record history
        unsigned short *newData = layer1->CreateLayerDataCopy();
        struct OperationParams *operation = new struct OperationParams;
    
        operation->layer1Change = true;
        operation->layer1ChangeParams = LayerChangeParams::Create(oldData, newData, w, h, currentroom->GetRoomID());
        delete[] oldData; delete[] newData;
        ExecuteOperation(operation);
    }

    // UI update
    RenderScreenFull();

    // Set change flag
    SetUnsavedChanges(true);
}

/// <summary>
/// Clear Layer 2 for the current Room if condition permit.
/// </summary>
void WL4EditorWindow::on_action_clear_Layer_2_triggered()
{
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];
    LevelComponents::Layer *layer2 = currentroom->GetLayer(2);
    if (layer2->GetMappingType() == LevelComponents::LayerMap16)
    {
        // Capture old layer data before clear
        int w = currentroom->GetLayer(2)->GetLayerWidth();
        int h = currentroom->GetLayer(2)->GetLayerHeight();
        unsigned short *oldData = layer2->CreateLayerDataCopy();

        layer2->ResetData();

        // Capture new layer data after clear and record history
        unsigned short *newData = layer2->CreateLayerDataCopy();
        struct OperationParams *operation = new struct OperationParams;
    
        operation->layer2Change = true;
        operation->layer2ChangeParams = LayerChangeParams::Create(oldData, newData, w, h, currentroom->GetRoomID());
        delete[] oldData; delete[] newData;
        ExecuteOperation(operation);
    }

    // UI update
    RenderScreenFull();

    // Set change flag
    SetUnsavedChanges(true);
}

/// <summary>
/// Clear Entity list 1 for the current Room.
/// </summary>
void WL4EditorWindow::on_action_clear_Normal_triggered()
{
    // Delete Entity list
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];

    // Capture old entity list data before clear
    std::vector<LevelComponents::EntityRoomAttribute> oldList = currentroom->GetEntityListData(1);

    currentroom->ClearEntitylist(1);

    // Capture new entity list data after clear and record history
    std::vector<LevelComponents::EntityRoomAttribute> newList = currentroom->GetEntityListData(1);
    struct OperationParams *operation = new struct OperationParams;

    operation->entityNormalChange = true;
    operation->entityNormalChangeParams = EntityListChangeParams::Create(oldList, newList, currentroom->GetRoomID());
    ExecuteOperation(operation);

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    currentroom->SetEntityListDirty(1, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Clear Entity list 0 for the current Room.
/// </summary>
void WL4EditorWindow::on_action_clear_Hard_triggered()
{
    // Delete Entity list
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];

    // Capture old entity list data before clear
    std::vector<LevelComponents::EntityRoomAttribute> oldList = currentroom->GetEntityListData(0);

    currentroom->ClearEntitylist(0);

    // Capture new entity list data after clear and record history
    std::vector<LevelComponents::EntityRoomAttribute> newList = currentroom->GetEntityListData(0);
    struct OperationParams *operation = new struct OperationParams;

    operation->entityHardChange = true;
    operation->entityHardChangeParams = EntityListChangeParams::Create(oldList, newList, currentroom->GetRoomID());
    ExecuteOperation(operation);

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    currentroom->SetEntityListDirty(0, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Clear Entity list 2 for the current Room.
/// </summary>
void WL4EditorWindow::on_action_clear_S_Hard_triggered()
{
    // Delete Entity list
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];

    // Capture old entity list data before clear
    std::vector<LevelComponents::EntityRoomAttribute> oldList = currentroom->GetEntityListData(2);

    currentroom->ClearEntitylist(2);

    // Capture new entity list data after clear and record history
    std::vector<LevelComponents::EntityRoomAttribute> newList = currentroom->GetEntityListData(2);
    struct OperationParams *operation = new struct OperationParams;

    operation->entitySHardChange = true;
    operation->entitySHardChangeParams = EntityListChangeParams::Create(oldList, newList, currentroom->GetRoomID());
    ExecuteOperation(operation);

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    currentroom->SetEntityListDirty(2, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Save graphic for the current Room.
/// </summary>
void WL4EditorWindow::on_actionSave_Room_s_graphic_triggered()
{
    QString qFilePath = QFileDialog::getSaveFileName(this, tr("Save current Room graphic to new file"),
                                                     dialogInitialPath, tr("PNG files (*.png)"));
    if (qFilePath.compare(""))
    {
        int CR_width, CR_height;
        auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];
        CR_width = currentroom->GetLayer1Width();
        CR_height = currentroom->GetLayer1Height();
        QGraphicsScene *tmpscene = ui->graphicsView->scene();
        QPixmap currentRoompixmap(CR_width * 16, CR_height * 16);
        QPainter tmppainter(&currentRoompixmap);
        tmpscene->render(&tmppainter);
        // The graphicscene has not been scaled, so don't need to scale it
        currentRoompixmap.save(qFilePath, "PNG", 100);
    }
}

/// <summary>
/// Open the patch manager.
/// </summary>
void WL4EditorWindow::on_actionPatch_Manager_triggered()
{
    PatchManagerDialog dialog(this);
    dialog.exec();
}

/// <summary>
/// Reset Theme to Light Theme.
/// </summary>
void WL4EditorWindow::on_actionLight_triggered()
{
    QApplication::setPalette(namedColorSchemePalette(Light));
    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(6), QString::number(ThemeColorType::Light));
    ui->actionDark->setChecked(false);
}

/// <summary>
/// Reset Theme to Dark Theme.
/// </summary>
void WL4EditorWindow::on_actionDark_triggered()
{
    QApplication::setPalette(namedColorSchemePalette(Dark));
    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(6), QString::number(ThemeColorType::Dark));
    ui->actionLight->setChecked(false);
}

/// <summary>
/// Open a script file and run it.
/// </summary>
void WL4EditorWindow::on_actionRun_from_file_triggered()
{
    // Select a Script file to open and run
    QString qFilePath =
        QFileDialog::getOpenFileName(this, tr("Open Script file"), dialogInitialPath, tr("Script files (*.js)"));
    if (!qFilePath.compare("")) {
        return;
    }
    QFile file(qFilePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
            QMessageBox::critical(this, tr("Error"), tr("Can't open file."));
            return;
    }

    // Modify Recent ROM menu
    ManageRecentFilesOrScripts(qFilePath, true);

    QString code = QString::fromUtf8(file.readAll());
    OutputWidget->ExecuteJSScript(code);
}

/// <summary>
/// Open the Output dock widget.
/// </summary>
void WL4EditorWindow::on_actionOutput_window_triggered()
{
    if(OutputWidget == nullptr) {
        OutputWidget = new OutputDockWidget(this);
        addDockWidget(Qt::BottomDockWidgetArea, OutputWidget);
    } else if(OutputWidget != nullptr) {
        addDockWidget(Qt::BottomDockWidgetArea, OutputWidget);
        OutputWidget->setVisible(true);
    }
}

/// <summary>
/// Clear everything in the current Room.
/// </summary>
void WL4EditorWindow::on_actionClear_all_triggered()
{
    ClearEverythingInRoom();
}

/// <summary>
/// Zoom in the graphic render for the current Room.
/// </summary>
void WL4EditorWindow::on_actionZoom_in_triggered()
{
    uint rate = GetGraphicViewScalerate();
    rate += 1;
    SetGraphicViewScalerate(rate);
}

/// <summary>
/// Zoom out the graphic render for the current Room.
/// </summary>
void WL4EditorWindow::on_actionZoom_out_triggered()
{
    uint rate = GetGraphicViewScalerate();
    if(rate > 1) rate -= 1;
    SetGraphicViewScalerate(rate);
}

/// <summary>
/// Toggle the rect select mode.
/// </summary>
/// <param name="arg1">
/// bool state of the rect select mode.
/// </param>
void WL4EditorWindow::on_actionRect_Select_Mode_toggled(bool arg1)
{
    ui->graphicsView->SetRectSelectMode(arg1);
}

/// <summary>
/// Add a new Room to the current Level.
/// </summary>
void WL4EditorWindow::on_actionNew_Room_triggered()
{
    // Create new Room based on current Room
    int newRoomId = CurrentLevel->GetRooms().size();
    if (newRoomId == 16)
    {
        OutputWidget->PrintString(tr("Cannot add another Room to the current Level!"));
        return;
    }

    int entitysetId = GetCurrentRoom()->GetCurrentEntitySetID();
    CurrentLevel->AddRoom(new LevelComponents::Room(newRoomId,
                                                    CurrentLevel->GetLevelID(),
                                                    GetCurrentRoom()->GetTilesetID(),
                                                    entitysetId));
    LevelComponents::Room *newRoom = CurrentLevel->GetRooms()[newRoomId];

    // Add one Door to the new Room so spriteset settings can work
    CurrentLevel->AddDoor(newRoomId, entitysetId);

    // Reset LevelHeader param
    CurrentLevel->GetLevelHeader()->NumOfMap++;

    // set dirty
    SetUnsavedChanges(true);
    newRoom->SetEntityListDirty(0, true);
    newRoom->SetEntityListDirty(1, true);
    newRoom->SetEntityListDirty(2, true);

    // UI updates
    SetCurrentRoomId(newRoomId);
    OutputWidget->PrintString(QString(tr("Created a new blank room")) +
                              " (# 0x" + QString::number(newRoomId, 16) + ") " +
                              tr("successfully using tileset and entityset from the current room."));
}

/// <summary>
/// Display the edit credits edit dialog.
/// It contains 13 tabs to edit corresponding credits
/// </summary>
void WL4EditorWindow::on_actionEdit_Credits_triggered()
{
    // Capture old credit data before the dialog opens
    DialogParams::CreditsEditParams *lastParams = new DialogParams::CreditsEditParams();
    memcpy(lastParams->oldCreditData,
           &ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::CreditsTiles],
           NUMBEROFCREDITSSCREEN * 1280);

    // Show the dialog
    DialogParams::CreditsEditParams *unusedParam = new DialogParams::CreditsEditParams();
    CreditsEditDialog dialog(this, unusedParam);
    if (dialog.exec() == QDialog::Accepted)
    {
        // Capture new credit data (dialog already wrote to ROM)
        DialogParams::CreditsEditParams *newParams = new DialogParams::CreditsEditParams();
        memcpy(newParams->newCreditData,
               &ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::CreditsTiles],
               NUMBEROFCREDITSSCREEN * 1280);

        // Create and execute operation
        OperationParams *operation = new OperationParams;
        operation->CreditChange = true;
        operation->lastCreditsEditParams = lastParams;
        operation->newCreditsEditParams = newParams;
        ExecuteOperation(operation);
    }
    else
    {
        delete lastParams;
    }
    delete unusedParam;
}

/// <summary>
/// Trigger to open a dialog to edit Entities and Entitysets.
/// </summary>
void WL4EditorWindow::on_actionEdit_Entity_EntitySet_triggered()
{
    // Set up parameters for the new entities and entitysets, for the purpose of initializing the dialog's selections
    DialogParams::EntitiesAndEntitySetsEditParams *_currentEntitiesAndEntitysetsEditParams =
        new DialogParams::EntitiesAndEntitySetsEditParams();

    // Show the dialog
    SpritesEditorDialog dialog(this, _currentEntitiesAndEntitysetsEditParams);
    if (dialog.exec() == QDialog::Accepted)
    {
        // Generate operation history data
        // Set changed bools in multiple places
        DialogParams::EntitiesAndEntitySetsEditParams *_oldEntitiesAndEntitysetsEditParams =
            new DialogParams::EntitiesAndEntitySetsEditParams();
        for (LevelComponents::Entity *entityIter: _currentEntitiesAndEntitysetsEditParams->entities)
        {
            _oldEntitiesAndEntitysetsEditParams->entities.push_back(ROMUtils::entities[entityIter->GetEntityGlobalID()]);
            entityIter->SetChanged(true);
        }
        for (LevelComponents::EntitySet *entitySetIter: _currentEntitiesAndEntitysetsEditParams->entitySets)
        {
            _oldEntitiesAndEntitysetsEditParams->entitySets.push_back(ROMUtils::entitiessets[entitySetIter->GetEntitySetId()]);
            entitySetIter->SetChanged(true);
        }

        // Execute Operation
        OperationParams *operation = new OperationParams;

        operation->SpritesSpritesetChange = true;
        operation->lastSpritesAndSetParam = _oldEntitiesAndEntitysetsEditParams;
        operation->newSpritesAndSetParam = _currentEntitiesAndEntitysetsEditParams;
        ExecuteOperation(operation); // Set UnsavedChanges bool inside
    }
    else
    {
        // We don't call the operation deconstructor, so we need to manually delete them if cancel the changes
        for (LevelComponents::Entity *entityIter: _currentEntitiesAndEntitysetsEditParams->entities)
            delete entityIter;
        for (LevelComponents::EntitySet *entitySetIter: _currentEntitiesAndEntitysetsEditParams->entitySets)
            delete entitySetIter;
        delete _currentEntitiesAndEntitysetsEditParams;
    }
}

/// <summary>
/// Copy current difficulty Entity list into Normal Entity list
/// </summary>
void WL4EditorWindow::on_action_duplicate_Normal_triggered()
{
    int selectedDifficulty=GetEditModeWidgetPtr()->GetEditModeParams().selectedDifficulty;

    // copy current difficulty Entity list into Normal Entity list
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];

    // Capture old entity list data before copy
    std::vector<LevelComponents::EntityRoomAttribute> oldList = currentroom->GetEntityListData(1);

    currentroom->CopyEntityLists(selectedDifficulty, 1);

    // Capture new entity list data after copy and record history
    std::vector<LevelComponents::EntityRoomAttribute> newList = currentroom->GetEntityListData(1);
    struct OperationParams *operation = new struct OperationParams;

    operation->entityNormalChange = true;
    operation->entityNormalChangeParams = EntityListChangeParams::Create(oldList, newList, currentroom->GetRoomID());
    ExecuteOperation(operation);

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    currentroom->SetEntityListDirty(1, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Copy current difficulty Entity list into Hard Entity list
/// </summary>
void WL4EditorWindow::on_action_duplicate_Hard_triggered()
{
    int selectedDifficulty=GetEditModeWidgetPtr()->GetEditModeParams().selectedDifficulty;

    // copy current difficulty Entity list into Hard Entity list
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];

    // Capture old entity list data before copy
    std::vector<LevelComponents::EntityRoomAttribute> oldList = currentroom->GetEntityListData(0);

    currentroom->CopyEntityLists(selectedDifficulty, 0);

    // Capture new entity list data after copy and record history
    std::vector<LevelComponents::EntityRoomAttribute> newList = currentroom->GetEntityListData(0);
    struct OperationParams *operation = new struct OperationParams;

    operation->entityHardChange = true;
    operation->entityHardChangeParams = EntityListChangeParams::Create(oldList, newList, currentroom->GetRoomID());
    ExecuteOperation(operation);

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    currentroom->SetEntityListDirty(0, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Copy current difficulty Entity list into Super Hard Entity list
/// </summary>
void WL4EditorWindow::on_action_duplicate_S_Hard_triggered()
{
    int selectedDifficulty=GetEditModeWidgetPtr()->GetEditModeParams().selectedDifficulty;

    // copy current difficulty Entity list into Super Hard Entity list
    auto currentroom = CurrentLevel->GetRooms()[ui->spinBox_RoomID->value()];

    // Capture old entity list data before copy
    std::vector<LevelComponents::EntityRoomAttribute> oldList = currentroom->GetEntityListData(2);

    currentroom->CopyEntityLists(selectedDifficulty, 2);

    // Capture new entity list data after copy and record history
    std::vector<LevelComponents::EntityRoomAttribute> newList = currentroom->GetEntityListData(2);
    struct OperationParams *operation = new struct OperationParams;

    operation->entitySHardChange = true;
    operation->entitySHardChangeParams = EntityListChangeParams::Create(oldList, newList, currentroom->GetRoomID());
    ExecuteOperation(operation);

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    currentroom->SetEntityListDirty(2, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Import a Tileset to the current ROM from other ROM
/// </summary>
void WL4EditorWindow::on_actionImport_Tileset_from_ROM_triggered()
{
    if (!firstROMLoaded)
        return;

    // Open a rom and get a Tileset instance from it
    QString qFilePath =
        QFileDialog::getOpenFileName(this, tr("Open ROM file"), dialogInitialPath, tr("GBA ROM files (*.gba)"));
    if (!qFilePath.compare(""))
    {
        return;
    }

    // switch ROM MetaData
    ROMUtils::ROMFileMetadata = &ROMUtils::TempROMMetadata;

    if (QString errorMessage = FileIOUtils::LoadROMFile(qFilePath); !errorMessage.isEmpty())
    {
        QMessageBox::critical(nullptr, QString(tr("Load Error")), QString(errorMessage));
        return;
    }

    bool okay = false;
    QString text = QInputDialog::getText(nullptr, tr("InputBox"),
                                         tr("Input a Tileset Id using HEX number\n"
                                            "The Editor will use this to load a Tileset from the temp-opened ROM\n"
                                            "and replace the corresponding Tileset in the current ROM if accepted."),
                                         QLineEdit::Normal,
                                         "1", &okay);
    // gcc does not allow any new variable to appear after using goto in a function, so have to do this -- ssp
    if (!okay)
    {
        ROMUtils::CleanUpTmpCurrentFileMetaData();
        ROMUtils::ROMFileMetadata = &ROMUtils::CurrentROMMetadata;
        return;
    }

    int tilesetId = text.toInt(nullptr, 16);
    if (tilesetId < 0 || tilesetId > 91)
    {
        QMessageBox::critical(this, tr("Error"), tr("Illegal Tileset Id!\n"
                                                    "The index should between 0 to 0x5B."));
        return;
    }

    // Set up parameters for the dialog's selections
    DialogParams::TilesetEditParams *_newTilesetEditParams = new DialogParams::TilesetEditParams();
    _newTilesetEditParams->currentTilesetIndex = tilesetId;
    int tilesetPtr = WL4Constants::TilesetDataTable + tilesetId * 36;
    _newTilesetEditParams->newTileset = new LevelComponents::Tileset(tilesetPtr, tilesetId, true);

    // call helper function to open dialog and apply changes
    EditCurrentTileset(_newTilesetEditParams);

    // RAM cleanup
    ROMUtils::CleanUpTmpCurrentFileMetaData();

    // switch back ROM MetaData
    ROMUtils::ROMFileMetadata = &ROMUtils::CurrentROMMetadata;
}

/// <summary>
/// UI Logic of Rolling Save
/// </summary>
void WL4EditorWindow::on_actionRolling_Save_triggered()
{
    OutputWidget->PrintString("Warning: Don't use Rolling Save feature for 2 projects under the same folder!");
    bool okay = false;
    int value = QInputDialog::getInt(this, QApplication::applicationName(),
                                     tr("input a number to configure rolling save:\n"
                                     "-1 to save infinite temp rom file,\n"
                                     "0 to disable this feature,\n"
                                     "positive int number to set the number of file the editor will keep."),
                                     SettingsUtils::GetKey(SettingsUtils::IniKeys::RollingSaveLimit).toInt(),
                                     -1, 0x7FFF'FFFF /*2147483647*/, 1, &okay);
    if (okay)
    {
        SettingsUtils::SetKey(SettingsUtils::IniKeys::RollingSaveLimit, QString::number(value));
        if (!value) // value == 0
        {
            ui->actionSave_As->setEnabled(true);
        }
        else
        {
            ui->actionSave_As->setEnabled(false);
        }
        ui->actionRolling_Save->setChecked(value);
    }
}

/// <summary>
/// Open the Graphic manager.
/// </summary>
void WL4EditorWindow::on_actionGraphic_Manager_triggered()
{
    // perform File save before using graphic manager in case some silly cases happen to cause ROM data corruption
    // Check for unsaved operations
    if (UnsavedChanges) // add an extra check so it will be faster to debug the dialog
    {
        if (!UnsavedChangesPrompt(tr("There are unsaved changes.\n"
                                     "WL4Editor wants to save the Level and reload the ROM before using Graphic Manager to avoid potential ROM data corruption.\n"
                                     "You can also discard all the unsaved changes. in this case, WL4Editor will directly reload the ROM.")))
            return;
        LoadROMDataFromFile(ROMUtils::ROMFileMetadata->FilePath);
    }

    // open graphic manager dialog
    GraphicManagerDialog dialog(this);
    dialog.exec();
}

/// <summary>
/// Reload project settings from json file.
/// </summary>
void WL4EditorWindow::on_actionReload_project_settings_triggered()
{
    // Load the Project settings
    SettingsUtils::LoadProjectSettings();

    // Render the screen
    RenderScreenFull();
}


void WL4EditorWindow::on_actionEdit_Animated_Tile_Groups_triggered()
{
    DialogParams::AnimatedTileGroupsEditParams *_currentAnimatedTileGroupsEditParams =
        new DialogParams::AnimatedTileGroupsEditParams();

    unsigned int currentroomid = ui->spinBox_RoomID->value();
    AnimatedTileGroupEditorDialog tmpdialog(this, CurrentLevel->GetRooms()[currentroomid]->GetTileset(), _currentAnimatedTileGroupsEditParams);
    if (tmpdialog.exec() == QDialog::Accepted)
    {
        // Generate operation history data
        // Set changed bools in to all the new data
        DialogParams::AnimatedTileGroupsEditParams *_oldAnimatedTileGroupsEditParams =
            new DialogParams::AnimatedTileGroupsEditParams();
        for (auto *&animatedTileGroupIter: _currentAnimatedTileGroupsEditParams->animatedTileGroups)
        {
            _oldAnimatedTileGroupsEditParams->animatedTileGroups.push_back(ROMUtils::animatedTileGroups[animatedTileGroupIter->GetGlobalID()]);
            animatedTileGroupIter->SetChanged(true);
        }

        // Execute Operation
        OperationParams *operation = new OperationParams;

        operation->AnimatedTileGroupChange = true;
        operation->lastAnimatedTileEditParam = _oldAnimatedTileGroupsEditParams;
        operation->newAnimatedTileEditParam = _currentAnimatedTileGroupsEditParams;
        ExecuteOperation(operation); // Set UnsavedChanges bool inside
    }
    else
    {
        // We don't call the operation deconstructor, so we need to manually delete them if cancel the changes
        for (auto *&animatedTileGroupIter: _currentAnimatedTileGroupsEditParams->animatedTileGroups)
        { delete animatedTileGroupIter; }
        delete _currentAnimatedTileGroupsEditParams;
    }
}

void WL4EditorWindow::on_actionEdit_Wall_Paints_triggered()
{
    // Capture old ROM data at all locations the dialog may modify
    unsigned char oldGFX[1024 * 5 * 6];
    unsigned char oldPassageColor[32 * 5 * 6];
    unsigned char oldPassageGray[32 * 5 * 6];
    memcpy(oldGFX, &ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintGFXAddr], sizeof(oldGFX));
    memcpy(oldPassageColor, &ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageColor], sizeof(oldPassageColor));
    memcpy(oldPassageGray, &ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageGray], sizeof(oldPassageGray));

    // Collect scattered data addresses and capture old data
    struct ScatteredCapture {
        unsigned int addr;
        unsigned int size;
        unsigned char *oldData;
    };
    std::vector<ScatteredCapture> captures;

    for (int passage = 0; passage < 6; passage++)
    {
        unsigned int mmapAddr = ROMUtils::PointerFromData(
            WL4Constants::WallPaintPalSixInOneMMapColorPtrTable + 4 * passage);

        for (int level = 0; level < 4; level++)
        {
            unsigned int gradAddr = ROMUtils::PointerFromData(
                WL4Constants::WallPaintPalStartLevelPointerTable + passage * 16 + level * 4);
            if (!gradAddr) continue;

            // Gradient palette data (256 bytes)
            ScatteredCapture gradCap;
            gradCap.addr = gradAddr;
            gradCap.size = 256;
            gradCap.oldData = new unsigned char[256];
            memcpy(gradCap.oldData, &ROMUtils::ROMFileMetadata->ROMDataPtr[gradAddr], 256);
            captures.push_back(gradCap);

            // MMAP palette data (32 bytes per level)
            ScatteredCapture mmapCap;
            mmapCap.addr = mmapAddr + 32 * (0xA + level);
            mmapCap.size = 32;
            mmapCap.oldData = new unsigned char[32];
            memcpy(mmapCap.oldData, &ROMUtils::ROMFileMetadata->ROMDataPtr[mmapCap.addr], 32);
            captures.push_back(mmapCap);
        }

        // Boss level MMAP palette (always written)
        ScatteredCapture bossCap;
        bossCap.addr = mmapAddr + 32 * (0xA + 4);
        bossCap.size = 32;
        bossCap.oldData = new unsigned char[32];
        memcpy(bossCap.oldData, &ROMUtils::ROMFileMetadata->ROMDataPtr[bossCap.addr], 32);
        captures.push_back(bossCap);
    }

    WallPaintEditorDialog dialog;
    auto acc = dialog.exec();
    if (acc == QDialog::Accepted)
    {
        dialog.AcceptChanges();

        // Capture new ROM data
        unsigned char newGFX[1024 * 5 * 6];
        unsigned char newPassageColor[32 * 5 * 6];
        unsigned char newPassageGray[32 * 5 * 6];
        memcpy(newGFX, &ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintGFXAddr], sizeof(newGFX));
        memcpy(newPassageColor, &ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageColor], sizeof(newPassageColor));
        memcpy(newPassageGray, &ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageGray], sizeof(newPassageGray));

        // Build operation params
        WallPaintChangeParams *wp = WallPaintChangeParams::Create(
            oldGFX, newGFX, oldPassageColor, newPassageColor, oldPassageGray, newPassageGray);

        for (auto &cap : captures)
        {
            wp->AddScatteredBlock(cap.addr, cap.size, cap.oldData,
                                  &ROMUtils::ROMFileMetadata->ROMDataPtr[cap.addr]);
            delete[] cap.oldData;
        }

        struct OperationParams *operation = new struct OperationParams;
        operation->WallPaintChange = true;
        operation->wallPaintChangeParams = wp;
        ExecuteOperation(operation);
    }
    else
    {
        for (auto &cap : captures)
            delete[] cap.oldData;
    }
}

/// <summary>
/// Go to a new Room while set the buttons' enability.
/// </summary>
void WL4EditorWindow::on_spinBox_RoomID_valueChanged(int arg1)
{
    if (firstROMLoaded)
    { // avoid render the Room twice when click the button to go to neighbor Room can call this function twice
        if (sender() != nullptr) SetCurrentRoomId(arg1, true);
    }
}

void WL4EditorWindow::on_actionChunk_Manager_triggered()
{
    // Chunk Manager requires a clean ROM state — block if there are unsaved changes
    if (UnsavedChanges)
    {
        QMessageBox::information(this, tr("Unsaved Changes"),
            tr("Chunk Manager requires a clean ROM state.\n"
               "Please save or discard your changes before opening Chunk Manager."));
        return;
    }
    if (!ROMUtils::ROMFileMetadata->ROMDataPtr)
    {
        QMessageBox::information(this, tr("No ROM Loaded"),
            tr("Please open a ROM file first."));
        return;
    }

    ChunkManagerDialog dialog(ChunkManagerMode::Standalone, this);
    if (dialog.exec() == QDialog::Accepted && dialog.HasUnsavedChanges())
    {
        LoadROMDataFromFile(ROMUtils::ROMFileMetadata->FilePath);
        OutputWidget->PrintString("ROM reloaded after Chunk Manager changes.");
    }
}

