#include "WL4EditorWindow.h"
#include "Operation.h"
#include "Dialog/PatchManagerDialog.h"
#include "ROMUtils.h"
#include "ui_WL4EditorWindow.h"
#include "Themes.h"

#include <cstdio>
#include <deque>

#include <QCloseEvent>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QTextEdit>
#include <QSizePolicy>

bool LoadROMFile(QString); // Prototype for main.cpp function

// Variables used by WL4EditorWindow
bool editModeWidgetInitialized = false;

// Global variables
struct DialogParams::PassageAndLevelIndex selectedLevel = { 0, 0 };
WL4EditorWindow *singleton;
QString dialogInitialPath = QString("");

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
    int themeId = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(6)).toInt();
    QApplication::setStyle(new PhantomStyle);
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

    // Create DockWidgets
    EditModeWidget = new EditModeDockWidget();
    Tile16SelecterWidget = new Tile16DockWidget();
    EntitySetWidget = new EntitySetDockWidget();
    CameraControlWidget = new CameraControlDockWidget();
    OutputWidget = new OutputDockWidget();

    // Add Recent ROM QAction according to the INI file
    QMenu *filemenu = ui->menuRecent_ROM;
    for(uint i = 0; i < sizeof(RecentROMs) / sizeof(RecentROMs[0]); i++)
    {
        recentROMnum = i;
        QString filepath = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + 1));
        if(!filepath.length())
        {
            if(i == 0)
            {
                RecentROMs[0] = new QAction("-/-", this);
                filemenu->addAction(RecentROMs[0]);
                connect(RecentROMs[0], SIGNAL(triggered()), this, SLOT(openRecentROM()));
            }
            break;
        }
        RecentROMs[i] = new QAction(filepath, this);
        filemenu->addAction(RecentROMs[i]);
        connect(RecentROMs[i], SIGNAL(triggered()), this, SLOT(openRecentROM()));
    }

    // Memory Initialization
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
    for(int i = 0; i < (sizeof(ROMUtils::singletonTilesets) / sizeof(ROMUtils::singletonTilesets[0])); i++)
    {
        delete ROMUtils::singletonTilesets[i];
    }
    for(int i = 0; i < (sizeof(ROMUtils::entitiessets) / sizeof(ROMUtils::entitiessets[0])); i++)
    {
        delete ROMUtils::entitiessets[i];
    }
    for(int i = 0; i < (sizeof(ROMUtils::entitiessets) / sizeof(ROMUtils::entities[0])); i++)
    {
        delete ROMUtils::entities[i];
    }

    if (CurrentLevel)
    {
        delete CurrentLevel;
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
    sprintf(tmpStr, "Room %d", selectedRoom);
    ui->selectedRoomLabel->setText(tmpStr);

    // Set the text for which level is loaded, near the bottom of the editor window
    sprintf(tmpStr, "Level ID: %d-%d", selectedLevel._PassageIndex, selectedLevel._LevelIndex);
    statusBarLabel->setText(tmpStr);
    ui->roomDecreaseButton->setEnabled(selectedRoom);
    ui->roomIncreaseButton->setEnabled(CurrentLevel->GetRooms().size() > selectedRoom + 1);

    // Render the screen
    RenderScreenFull();
    SetEditModeDockWidgetLayerEditability();
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
    QString qFilePath =
        QFileDialog::getOpenFileName(this, tr("Open ROM file"), dialogInitialPath, tr("GBA ROM files (*.gba)"));
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
    if (!LoadROMFile(qFilePath))
    {
        QMessageBox::critical(nullptr, QString(tr("Load Error")), QString(tr("You may have loaded an invalid ROM!")));
        return;
    }

    // Clean-up
    if (CurrentLevel)
    {
        delete CurrentLevel;
        // Decomstruct all LevelComponents singletons
        for(int i = 0; i < (sizeof(ROMUtils::singletonTilesets) / sizeof(ROMUtils::singletonTilesets[0])); i++)
        {
            delete ROMUtils::singletonTilesets[i];
        }
        for(int i = 0; i < (sizeof(ROMUtils::entitiessets) / sizeof(ROMUtils::entitiessets[0])); i++)
        {
            delete ROMUtils::entitiessets[i];
        }
        for(int i = 0; i < (sizeof(ROMUtils::entitiessets) / sizeof(ROMUtils::entities[0])); i++)
        {
            delete ROMUtils::entities[i];
        }
    }

    // Set the program title
    std::string fileName = filePath.substr(filePath.rfind('/') + 1);
    setWindowTitle(fileName.c_str());

    // Load all LevelComponents singletons
    for(int i = 0; i < (sizeof(ROMUtils::singletonTilesets) / sizeof(ROMUtils::singletonTilesets[0])); i++)
    {
        int tilesetPtr = WL4Constants::TilesetDataTable + i * 36;
        ROMUtils::singletonTilesets[i] = new LevelComponents::Tileset(tilesetPtr, i);
    }
    for (unsigned int i = 0; i < sizeof(ROMUtils::entitiessets) / sizeof(ROMUtils::entitiessets[0]); ++i)
    {
        ROMUtils::entitiessets[i] = new LevelComponents::EntitySet(i, WL4Constants::UniversalSpritesPalette);
    }
    for (unsigned int i = 0; i < sizeof(ROMUtils::entities) / sizeof(ROMUtils::entities[0]); ++i)
    {
        struct LevelComponents::EntitySetAndEntitylocalId tmpEntitysetAndEntitylocalId =
            LevelComponents::EntitySet::EntitySetFromEntityID(i);
        ROMUtils::entities[i] = new LevelComponents::Entity(tmpEntitysetAndEntitylocalId.entitylocalId, i,
                                                  ROMUtils::entitiessets[tmpEntitysetAndEntitylocalId.entitysetId]);
    }

    // Load the first level and render the screen
    selectedLevel._PassageIndex = selectedLevel._LevelIndex = 0;
    CurrentLevel = new LevelComponents::Level(static_cast<enum LevelComponents::__passage>(selectedLevel._PassageIndex),
                                              static_cast<enum LevelComponents::__stage>(selectedLevel._LevelIndex));
    selectedRoom = 0;
    int tmpTilesetID = CurrentLevel->GetRooms()[selectedRoom]->GetTilesetID();
    UnsavedChanges = false;

    UIStartUp(tmpTilesetID);
}

/// <summary>
/// Print Mouse Pos in the status bar
/// </summary>
/// <param name="x">
/// current tile x position
/// </param>
/// <param name="y">
/// current tile y position
/// </param>
void WL4EditorWindow::PrintMousePos(uint x, uint y)
{
    bool mouseOutofTileArea = false;
    bool mouseOutofTileArea2 = false;
    if(CurrentLevel->GetRooms()[selectedRoom]->GetLayer0MappingParam() & 0x10) {
        mouseOutofTileArea = (x >= CurrentLevel->GetRooms()[selectedRoom]->GetLayer0Width()) || (y >= CurrentLevel->GetRooms()[selectedRoom]->GetLayer0Height());
    }
    mouseOutofTileArea2 = (x >= CurrentLevel->GetRooms()[selectedRoom]->GetWidth()) || (y >= CurrentLevel->GetRooms()[selectedRoom]->GetHeight());
    if(mouseOutofTileArea && mouseOutofTileArea2)
        statusBarLabel_MousePosition->setText(tr("Out of range!"));
    else
        statusBarLabel_MousePosition->setText("(" + QString::number(x) + ", " + QString::number(y) + ")");
}

/// <summary>
/// Set graphicView scalerate.
/// </summary>
void WL4EditorWindow::SetGraphicViewScalerate(uint scalerate)
{
    ui->graphicsView->scale((qreal)scalerate / (qreal)graphicViewScalerate, (qreal)scalerate / (qreal)graphicViewScalerate);
    graphicViewScalerate = scalerate;
    // TODO: find a method to get mouse pos instantly
//    int mouse_x = ui->graphicsView->mapFromGlobal(QCursor::pos()).x();
//    int mouse_y = ui->graphicsView->mapFromGlobal(QCursor::pos()).y();
//    PrintMousePos(mouse_x, mouse_y);
    statusBarLabel_MousePosition->setText(tr("Move your mouse to show position again!"));
    statusBarLabel_Scalerate->setText(tr("scale rate: ") + QString::number(graphicViewScalerate) + "00%");
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
        if (selectedRoom)
            ui->roomDecreaseButton->setEnabled(state);
        if (selectedRoom < (CurrentLevel->GetRooms().size() - 1))
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
void WL4EditorWindow::SetCurrentRoomId(int roomid)
{
    if (static_cast<int>(selectedRoom) == roomid)
        return;
    if(roomid < 0 || roomid >= static_cast<int>(CurrentLevel->GetRooms().size()))
        return;

    // Deselect rect
    // SetRectSelectMode(ui->actionRect_Select_Mode->isChecked());
    // Deselect Door and Entity
    ui->graphicsView->DeselectDoorAndEntity(true);

    // Load the previous room
    selectedRoom = roomid;
    LoadRoomUIUpdate();
    int tmpTilesetID = CurrentLevel->GetRooms()[selectedRoom]->GetTilesetID();
    Tile16SelecterWidget->SetTileset(tmpTilesetID);
    ResetEntitySetDockWidget();
    ResetCameraControlDockWidget();
}

/// <summary>
/// Update the UI after loading a ROM.
/// </summary>
void WL4EditorWindow::UIStartUp(int currentTilesetID)
{
    // Only modify UI on the first time a ROM is loaded
    if (!firstROMLoaded)
    {
        firstROMLoaded = true;

        // Enable UI that requires a ROM file to be loaded
        ui->loadLevelButton->setEnabled(true);
        ui->actionLevel_Config->setEnabled(true);
        ui->actionRoom_Config->setEnabled(true);
        ui->actionSave_ROM->setEnabled(true);
        ui->actionSave_As->setEnabled(true);
        ui->actionSave_Room_s_graphic->setEnabled(true);
        ui->actionEdit_Tileset->setEnabled(true);
        ui->menuAdd->setEnabled(true);
        ui->menuSwap->setEnabled(true);
        ui->menuClear->setEnabled(true);
        ui->menu_clear_Layer->setEnabled(true);
        ui->menu_clear_Entity_list->setEnabled(true);
        ui->actionClear_all->setEnabled(true);
        ui->actionRedo->setEnabled(true);
        ui->actionRedo_global->setEnabled(true);
        ui->actionUndo->setEnabled(true);
        ui->actionUndo_global->setEnabled(true);
        ui->actionRun_from_file->setEnabled(true);
        ui->actionManager->setEnabled(true);

        // Load Dock widget
        addDockWidget(Qt::RightDockWidgetArea, EditModeWidget);
        addDockWidget(Qt::RightDockWidgetArea, Tile16SelecterWidget);
        addDockWidget(Qt::RightDockWidgetArea, EntitySetWidget);
        addDockWidget(Qt::RightDockWidgetArea, CameraControlWidget);
        addDockWidget(Qt::BottomDockWidgetArea, OutputWidget);
        CameraControlWidget->setVisible(false);
        EntitySetWidget->setVisible(false);
    }

    // Modify UI every time when a ROM is loaded
    EntitySetWidget->ResetEntitySet(CurrentLevel->GetRooms()[selectedRoom]);
    Tile16SelecterWidget->SetTileset(currentTilesetID);
    CameraControlWidget->PopulateCameraControlInfo(CurrentLevel->GetRooms()[selectedRoom]);

    // Modify Recent ROM menu
    int findedInRecentFile = -1; // start by 0
    if(recentROMnum > 0)
    {
        for(uint i = 0; i < recentROMnum; i++)
        {
            QString filepath = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i + 1));
            if(filepath == ROMUtils::ROMFilePath)
            {
                findedInRecentFile = i;
                break;
            }
        }
    }
    QMenu *filemenu = ui->menuRecent_ROM;
    if(findedInRecentFile == -1)
    {
        if(recentROMnum > 0)
        {
            if(recentROMnum < (sizeof(RecentROMs) / sizeof(RecentROMs[0])))
            {
                RecentROMs[recentROMnum] = new QAction(RecentROMs[recentROMnum - 1]->text(), this);
                filemenu->addAction(RecentROMs[recentROMnum]);
                connect(RecentROMs[recentROMnum], SIGNAL(triggered()), this, SLOT(openRecentROM()));
            }
            for(uint i = ((recentROMnum < (sizeof(RecentROMs) / sizeof(RecentROMs[0]) - 1) ? recentROMnum : (sizeof(RecentROMs) / sizeof(RecentROMs[0]) - 1))); i > 0 ; i--)
            {
                QString filepath = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i));
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + 1), filepath);
                RecentROMs[i]->setText(filepath);
            }
        }
        SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(1), ROMUtils::ROMFilePath);
        RecentROMs[0]->setText(ROMUtils::ROMFilePath);
        recentROMnum++;
    }
    else
    {
        if(findedInRecentFile > 0)
        {
            for(int i = findedInRecentFile; i > -1; i--)
            {
                QString filepath = SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(i));
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + 1), filepath);
                RecentROMs[i]->setText(filepath);
            }
            SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(1), ROMUtils::ROMFilePath);
            RecentROMs[0]->setText(ROMUtils::ROMFilePath);
        }
    }

    // UI update
    LoadRoomUIUpdate();
}

/// <summary>
/// Set whether the the UI elements for WL4Editor are enabled, based on layer properties.
/// </summary>
void WL4EditorWindow::SetEditModeDockWidgetLayerEditability()
{
    bool layer0enable = CurrentLevel->GetRooms()[selectedRoom]->GetLayer(0)->IsEnabled();
    EditModeWidget->SetLayersCheckBoxEnabled(0, layer0enable);
    EditModeWidget->SetLayersCheckBoxEnabled(1, CurrentLevel->GetRooms()[selectedRoom]->GetLayer(1)->IsEnabled());
    EditModeWidget->SetLayersCheckBoxEnabled(2, CurrentLevel->GetRooms()[selectedRoom]->GetLayer(2)->IsEnabled());
    EditModeWidget->SetLayersCheckBoxEnabled(3, CurrentLevel->GetRooms()[selectedRoom]->GetLayer(3)->IsEnabled());
    EditModeWidget->SetLayersCheckBoxEnabled(7, CurrentLevel->GetRooms()[selectedRoom]->IsLayer0ColorBlendingEnabled());
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
    // Apply the selected parameters to the current room
    // reset the Tileset instance in Room class
    LevelComponents::Room *currentRoom = CurrentLevel->GetRooms()[selectedRoom];
    if (nextroomconfig->CurrentTilesetIndex != currentroomconfig->CurrentTilesetIndex)
    {
        currentRoom->SetTileset(ROMUtils::singletonTilesets[nextroomconfig->CurrentTilesetIndex], nextroomconfig->CurrentTilesetIndex);
        Tile16SelecterWidget->SetTileset(nextroomconfig->CurrentTilesetIndex);
    }

    // refresh the Layer 2, 0, 3 instances
    if (nextroomconfig->Layer2Enable && !currentroomconfig->Layer2Enable)
    {
        currentRoom->GetLayer(2)->CreateNewLayer_type0x10(nextroomconfig->RoomWidth, nextroomconfig->RoomHeight);
    }
    else if (currentroomconfig->Layer2Enable && !nextroomconfig->Layer2Enable)
    {
        currentRoom->GetLayer(2)->SetDisabled();
    }

    // Need to change layer 0 data when Layer 0 size changed
    if (nextroomconfig->Layer0Width != currentroomconfig->Layer0Width ||
            nextroomconfig->Layer0Height != currentroomconfig->Layer0Height)
    {
        if ((currentroomconfig->Layer0MappingTypeParam & 0x30) == LevelComponents::LayerMap16)
        {
            // save previous Layer data
            size_t datasize1 = 0;
            datasize1 = 2 * currentroomconfig->Layer0Width * currentroomconfig->Layer0Height;
            unsigned short *tmpLayerdata1 = new unsigned short[datasize1];
            memcpy(tmpLayerdata1, currentRoom->GetLayer(0)->GetLayerData(), datasize1);
            currentroomconfig->LayerData[0] = tmpLayerdata1;

            // reset Layer size
            size_t datasize2 = 0;
            currentRoom->GetLayer(0)->ChangeDimensions(nextroomconfig->Layer0Width, nextroomconfig->Layer0Height);
            datasize2 = 2 * nextroomconfig->Layer0Width * nextroomconfig->Layer0Height;

            // save result Layer data
            unsigned short *tmpLayerdata2 = new unsigned short[datasize2];
            memcpy(tmpLayerdata2, currentRoom->GetLayer(0)->GetLayerData(), datasize2);
            nextroomconfig->LayerData[0] = tmpLayerdata2;
        } else {
            currentroomconfig->LayerData[0] = nullptr;
            nextroomconfig->LayerData[0] = nullptr;
        }
    }

    // Deal with the cases that the new room use Tile8x8 layer 0
    if ((currentroomconfig->Layer0MappingTypeParam & 0x30) != LevelComponents::LayerTile8x8 &&
            (nextroomconfig->Layer0MappingTypeParam & 0x30) == LevelComponents::LayerTile8x8)
    {
        LevelComponents::Layer *currentLayer0 = currentRoom->GetLayer(0);
        delete currentLayer0;
        currentLayer0 = new LevelComponents::Layer(nextroomconfig->Layer0DataPtr, LevelComponents::LayerTile8x8);
        currentRoom->SetLayer(0, currentLayer0);
    }
    else if((currentroomconfig->Layer0MappingTypeParam & 0x30) == LevelComponents::LayerTile8x8 &&
            (nextroomconfig->Layer0MappingTypeParam & 0x30) != LevelComponents::LayerTile8x8)
    {
        currentRoom->GetLayer(0)->CreateNewLayer_type0x10(nextroomconfig->Layer0Width, nextroomconfig->Layer0Height);
    }

    // Layer 0 is disabled (or customized) in the new settings
    if ((currentroomconfig->Layer0MappingTypeParam > 0x10) && (nextroomconfig->Layer0MappingTypeParam < 0x10))
    {
        currentRoom->GetLayer(0)->SetDisabled();
    }

    // Create new Layer 3
    if (nextroomconfig->BackgroundLayerEnable)
    {
        LevelComponents::Layer *currentLayer3 = currentRoom->GetLayer(3);
        delete currentLayer3;
        currentLayer3 =
            new LevelComponents::Layer(nextroomconfig->BackgroundLayerDataPtr, LevelComponents::LayerTile8x8);
        currentRoom->SetLayer(3, currentLayer3);
    }
    else if (currentroomconfig->BackgroundLayerEnable && !nextroomconfig->BackgroundLayerEnable)
    {
        currentRoom->GetLayer(3)->SetDisabled();
    }

    // Need to change layers data and modify the inside doors, entities, camera boxes when Room size changed
    if (nextroomconfig->RoomWidth != currentroomconfig->RoomWidth ||
            nextroomconfig->RoomHeight != currentroomconfig->RoomHeight)
    {
        // Deal with out-of-range Doors, Entities, Camera limitators
        // TODO: support Undo/Redo on these elements
        // -- Door --
        int nxtRoomWidth = nextroomconfig->RoomWidth, nxtRoomHeight = nextroomconfig->RoomHeight;
        std::vector<LevelComponents::Door *> doorlist = currentRoom->GetDoors();
        size_t doornum = currentRoom->CountDoors();
        size_t k = doornum - 1;
        size_t vortexdoorId_needResetPos = 0;
        size_t doorcount = doornum;
        uint *deleteDoorIdlist = new uint[doornum](); // set them all 0, index the door from 1
        for (uint i = 0; i < doornum; i++)
        {
            if ((doorlist[i]->GetX2() >= nxtRoomWidth) || (doorlist[i]->GetY2() >= nxtRoomHeight))
            {
                if (doorlist[i]->IsVortex())
                {
                    vortexdoorId_needResetPos = i + 1;
                    deleteDoorIdlist[k--] = i + 1;
                }
                else
                {
                    deleteDoorIdlist[k--] = i + 1; // the id list will be something like: 0 0 0 8 4 2
                }
            }
        }
        for (uint i = 0; i < doornum; i++)
        {
            if (deleteDoorIdlist[i] != 0)
            {
                if (deleteDoorIdlist[i] != vortexdoorId_needResetPos)
                {
                    if (i == doornum - 1 &&
                        doorcount == 1) // don't delete the last door if there is no vortex door in this Room
                    {
                        currentRoom->GetDoor(deleteDoorIdlist[i] - 1)
                            ->SetDoorPlace(
                                qMin(nxtRoomWidth - 1, currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetX1()),
                                qMin(nxtRoomWidth - 1, currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetX2()),
                                qMin(nxtRoomHeight - 1, currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetY1()),
                                qMin(nxtRoomHeight - 1, currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetY2()));
                        break;
                    }
                    DeleteDoor(currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetGlobalDoorID());
                    --doorcount;
                    // Seems don't need to set Door dirty at least for now
                }
                else
                {
                    currentRoom->GetDoor(vortexdoorId_needResetPos - 1)
                        ->SetDoorPlace(qMin(nxtRoomWidth - 1, currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetX1()),
                                       qMin(nxtRoomWidth - 1, currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetX2()),
                                       qMin(nxtRoomHeight - 1, currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetY1()),
                                       qMin(nxtRoomHeight - 1, currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetY2()));
                    // Seems don't need to set Door dirty at least for now
                }
            }
        }
        delete[] deleteDoorIdlist;

        // -- Entity --
        for (uint i = 0; i < 3; i++)
        {
            std::vector<struct LevelComponents::EntityRoomAttribute> entitylist = currentRoom->GetEntityListData(i);
            size_t entitynum = entitylist.size();
            for (uint j = entitynum; j > 0; j--)
            {
                if ((entitylist[j - 1].XPos > (nxtRoomWidth - 1)) || (entitylist[j - 1].YPos > (nxtRoomHeight - 1)))
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
        k = limitatornum - 1;
        uint *deleteLimitatorIdlist = new uint[limitatornum](); // set them all 0, index the limitator from 1
        for (uint i = 0; i < limitatornum; i++)
        {
            int x2_prime =
                (limitatorlist[i]->ChangeValueOffset == 1) ? (limitatorlist[i]->ChangedValue) : (limitatorlist[i]->x2);
            int y2_prime =
                (limitatorlist[i]->ChangeValueOffset == 3) ? (limitatorlist[i]->ChangedValue) : (limitatorlist[i]->y2);
            if ((x2_prime >= nxtRoomWidth) || (y2_prime >= nxtRoomHeight))
            {
                deleteLimitatorIdlist[k--] = i + 1; // the id list will be something like: 0 0 0 8 4 2
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

        // Reset Layers
        for (int i = 1; i < 3; ++i)
        {
            if ((currentRoom->GetLayer(i)->GetMappingType() & 0x30) == LevelComponents::LayerMap16)
            {
                // save previous Layer data
                size_t datasize1 = 2 * currentroomconfig->RoomWidth * currentroomconfig->RoomHeight;
                unsigned short *tmpLayerdata1 = new unsigned short[datasize1];
                memcpy(tmpLayerdata1, currentRoom->GetLayer(i)->GetLayerData(), datasize1);
                currentroomconfig->LayerData[i] = tmpLayerdata1;

                // reset Layer size
                currentRoom->GetLayer(i)->ChangeDimensions(nextroomconfig->RoomWidth, nextroomconfig->RoomHeight);

                // save result Layer data
                size_t datasize2 = 2 * nextroomconfig->RoomWidth * nextroomconfig->RoomHeight;
                unsigned short *tmpLayerdata2 = new unsigned short[datasize2];
                memcpy(tmpLayerdata2, currentRoom->GetLayer(i)->GetLayerData(), datasize2);
                nextroomconfig->LayerData[i] = tmpLayerdata2;
            }
        }
    }

    // reset all the Parameters in Room class, except new layer data pointers, generate them on saving
    currentRoom->SetHeight(nextroomconfig->RoomHeight);
    currentRoom->SetWidth(nextroomconfig->RoomWidth);
    currentRoom->SetLayer0MappingParam(nextroomconfig->Layer0MappingTypeParam);
    currentRoom->SetLayer0ColorBlendingEnabled(nextroomconfig->Layer0Alpha);
    currentRoom->SetLayerPriorityAndAlphaAttributes(nextroomconfig->LayerPriorityAndAlphaAttr);
    currentRoom->SetLayer2Enabled(nextroomconfig->Layer2Enable);
    if (nextroomconfig->Layer0DataPtr)
        currentRoom->SetRoomHeaderDataPtr(0, nextroomconfig->Layer0DataPtr);
    currentRoom->SetBGLayerEnabled(nextroomconfig->BackgroundLayerEnable);
    currentRoom->SetBGLayerScrollFlag(nextroomconfig->BGLayerScrollFlag);
    currentRoom->SetRoomHeaderDataPtr(3, nextroomconfig->BackgroundLayerDataPtr);
    currentRoom->SetLayerGFXEffect01(nextroomconfig->RasterType);
    currentRoom->SetLayerGFXEffect02(nextroomconfig->Water);
    currentRoom->SetBgmvolume(nextroomconfig->BGMVolume);

    // reset LayerDataPtr in RoomHeader because Layer::SetDisabled() doesn't change the data in RoomHeader
    for (int i = 0; i < 3; ++i)
    {
        if (currentRoom->GetLayer(i)->GetMappingType() == LevelComponents::LayerDisabled)
        {
            currentRoom->SetRoomHeaderDataPtr(
                i, WL4Constants::NormalLayerDefaultPtr); // TODO: need a fix for a Tileset in toxic landfill
        }
    }
    if (currentRoom->GetLayer(3)->GetMappingType() == LevelComponents::LayerDisabled)
    {
        currentRoom->SetRoomHeaderDataPtr(3, WL4Constants::BGLayerDefaultPtr);
    }

    // Mark the layers as dirty
    for (unsigned int i = 0; i < 4; ++i)
        currentRoom->GetLayer(i)->SetDirty(true);
}

/// <summary>
/// Delete a Door from the current Level.
/// </summary>
/// <param name="globalDoorIndex">
/// The global Door id given by current Level.
/// </param>
void WL4EditorWindow::DeleteDoor(int globalDoorIndex)
{
    // You cannot delete the vortex, it is always the first Door.
    if (globalDoorIndex == 0)
    {
        OutputWidget->PrintString(tr("Deleting portal Door not permitted!"));
        return;
    }
    if (CurrentLevel->GetDoors().size() == 1)
    {
        OutputWidget->PrintString(tr("Deleting the last Door in the Room not permitted! Spriteset is based on Doors."));
        return;
    }

    // Delete the Door from the Room Door list
    CurrentLevel->GetRooms()[CurrentLevel->GetDoors()[globalDoorIndex]->GetRoomID()]->DeleteDoor(globalDoorIndex);

    // Disable the destination for all the existing Doors whose DestinationDoor is the Door which is being deleting
    for (unsigned int i = 0; i < CurrentLevel->GetDoors().size(); ++i)
    {
        if (CurrentLevel->GetDoors()[i]->GetDestinationDoor()->GetGlobalDoorID() == globalDoorIndex)
        {
            CurrentLevel->GetDoors()[i]->SetDestinationDoor(CurrentLevel->GetDoors()[0]);
        }
    }

    // Delete the Door from the Level Door list
    CurrentLevel->DeleteDoor(globalDoorIndex);

    // Decline the GlobalDoorId for the Doors indexed after the deleted Door
    if (CurrentLevel->GetDoors().size() - globalDoorIndex)
    {
        for (unsigned int i = globalDoorIndex; i < CurrentLevel->GetDoors().size(); ++i)
        {
            CurrentLevel->GetDoors()[i]->GlobalDoorIdDec();
        }
    }

    // Correct the LinkerDestination in DoorEntry for each Door
    if (CurrentLevel->GetDoors().size() > 1)
    {
        for (unsigned int i = 1; i < CurrentLevel->GetDoors().size(); ++i)
        {
            CurrentLevel->GetDoors()[i]->SetLinkerDestination(
                CurrentLevel->GetDoors()[i]->GetDestinationDoor()->GetGlobalDoorID());
        }
    }
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
    // Check if it is a valid slot function call
    if(filepath == "-/-" || filepath == "") return;

    // Check if the file exist, if not, modify the Recent ROM QAction list
    QFile file(filepath);
    if(!file.exists())
    {
        if(recentROMnum == 1)
        {
            SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(1), "");
            RecentROMs[0]->setText("-/-");
        }
        if(recentROMnum > 1)
        {
            int deletelinenum = -1;
            for(int i = 0; i < 5; i++)
            {
                if(RecentROMs[i]->text() == filepath)
                {
                    deletelinenum = i;
                    break;
                }
            }
            for(int i = deletelinenum; i < (recentROMnum - 1); i++)
            {
                RecentROMs[i]->setText(RecentROMs[i+1]->text());
                SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(i + 1), RecentROMs[i+1]->text());
            }
            SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(recentROMnum), "");
            delete RecentROMs[recentROMnum - 1];
        }
        recentROMnum--;
        QMessageBox::critical(nullptr, QString(tr("Load Error")), QString(tr("This ROM no longer exists!")));
        return;
    }

    LoadROMDataFromFile(filepath);
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
    QGraphicsScene *scene =
        CurrentLevel->GetRooms()[selectedRoom]->RenderGraphicsScene(ui->graphicsView->scene(), &renderParams);
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
    QGraphicsScene *scene =
        CurrentLevel->GetRooms()[selectedRoom]->RenderGraphicsScene(ui->graphicsView->scene(), &renderParams);
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
    QGraphicsScene *scene =
        CurrentLevel->GetRooms()[selectedRoom]->RenderGraphicsScene(ui->graphicsView->scene(), &renderParams);
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
    CurrentLevel->GetRooms()[selectedRoom]->RenderGraphicsScene(ui->graphicsView->scene(), &renderParams);
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

    // Deselect Door and Entity
    ui->graphicsView->DeselectDoorAndEntity(true);

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
        selectedRoom = 0;
        LoadRoomUIUpdate();
        int tmpTilesetID = CurrentLevel->GetRooms()[selectedRoom]->GetTilesetID();
        Tile16SelecterWidget->SetTileset(tmpTilesetID);
        ResetEntitySetDockWidget();
        ResetCameraControlDockWidget();

        // Set program control changes
        UnsavedChanges = false;
        ResetUndoHistory();
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
                return false;
            }
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
            "You just triggered the clear-all shortcut (current room).\nDo you want to delete all the doors, too?\n(One "
            "door will be kept to render camera boxes correctly.\nCamera settings will be unaffected regardless.)"));
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

    // Clear Layers 0, 1, 2
    LevelComponents::Room *currentRoom = CurrentLevel->GetRooms()[selectedRoom];
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
        std::vector<LevelComponents::Door *> doorlist = currentRoom->GetDoors();
        size_t doornum = currentRoom->CountDoors();
        size_t k = doornum - 1;
        size_t vortexdoorId_needResetPos = 0;
        uint *deleteDoorIdlist = new uint[doornum](); // set them all 0, index the door from 1
        for (uint i = 0; i < doornum; i++)
        {
            if (doorlist[i]->IsVortex())
            {
                vortexdoorId_needResetPos = i + 1;
                deleteDoorIdlist[k--] = i + 1;
            }
            else
            {
                deleteDoorIdlist[k--] = i + 1; // the id list will be something like: 0 0 0 8 4 2
            }
        }
        for (uint i = 0; i < doornum; i++)
        {
            if (deleteDoorIdlist[i] != 0)
            {
                if (deleteDoorIdlist[i] != vortexdoorId_needResetPos)
                {
                    if (i == doornum - 1 && !vortexdoorId_needResetPos) // don't delete the last door if there is no
                                                                        // vortex door in this Room
                        break;
                    if (i == doornum - 1 &&
                        vortexdoorId_needResetPos) // delete the last door if there is a vortex door in this Room
                        continue;
                    DeleteDoor(currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetGlobalDoorID());
                    // Seems don't need to set Door dirty at least for now
                }
                else
                {
                    currentRoom->GetDoor(vortexdoorId_needResetPos - 1)
                        ->SetDoorPlace(qMin(currentRoom->GetWidth() - 1,
                                            (uint) currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetX1()),
                                       qMin(currentRoom->GetWidth() - 1,
                                            (uint) currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetX2()),
                                       qMin(currentRoom->GetHeight() - 1,
                                            (uint) currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetY1()),
                                       qMin(currentRoom->GetHeight() - 1,
                                            (uint) currentRoom->GetDoor(deleteDoorIdlist[i] - 1)->GetY2()));
                    // Seems don't need to set Door dirty at least for now
                }
            }
        }
        delete[] deleteDoorIdlist;
    }

    // TODO: add history record

    // UI update
    ResetEntitySetDockWidget();
    RenderScreenFull();

    // Set change flag
    SetUnsavedChanges(true);
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
    if (!selectedRoom)
        return;

    // Deselect rect
    // SetRectSelectMode(ui->actionRect_Select_Mode->isChecked());
    // Deselect Door and Entity
    ui->graphicsView->DeselectDoorAndEntity(true);

    // Load the previous room
    --selectedRoom;
    LoadRoomUIUpdate();
    int tmpTilesetID = CurrentLevel->GetRooms()[selectedRoom]->GetTilesetID();
    Tile16SelecterWidget->SetTileset(tmpTilesetID);
    ResetEntitySetDockWidget();
    ResetCameraControlDockWidget();
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
    if (selectedRoom == (CurrentLevel->GetRooms().size() - 1))
        return;

    // Deselect rect
    // SetRectSelectMode(ui->actionRect_Select_Mode->isChecked());
    // Deselect Door and Entity
    ui->graphicsView->DeselectDoorAndEntity(true);

    // Load the next room
    ++selectedRoom;
    LoadRoomUIUpdate();
    int tmpTilesetID = CurrentLevel->GetRooms()[selectedRoom]->GetTilesetID();
    Tile16SelecterWidget->SetTileset(tmpTilesetID);
    ResetEntitySetDockWidget();
    ResetCameraControlDockWidget();
}

/// <summary>
/// Present the user with a level config dialog for changing the level title and frog timer values.
/// </summary>
/// <remarks>
/// Only set the new parameters from the dialog if OK is pressed.
/// </remarks>
void WL4EditorWindow::on_actionLevel_Config_triggered()
{
    // TODO updates to the level config should go through the undo history queue in Operations.cpp
    // TODO we should probably differentiate between room and level changes

    // Show a level config dialog to the user
    LevelConfigDialog dialog;
    dialog.InitTextBoxes(CurrentLevel->GetLevelName(),
                         CurrentLevel->GetLevelName(1),
                         CurrentLevel->GetTimeCountdownCounter(LevelComponents::HardDifficulty),
                         CurrentLevel->GetTimeCountdownCounter(LevelComponents::NormalDifficulty),
                         CurrentLevel->GetTimeCountdownCounter(LevelComponents::SHardDifficulty));

    // If OK is pressed, then set the level attributes
    auto acc = dialog.exec();
    if (acc == QDialog::Accepted)
    {
        CurrentLevel->SetLevelName(dialog.GetPaddedLevelName());
        CurrentLevel->SetLevelName(dialog.GetPaddedLevelName(1), 1);
        CurrentLevel->SetTimeCountdownCounter(LevelComponents::HardDifficulty, (unsigned int) dialog.GetHModeTimer());
        CurrentLevel->SetTimeCountdownCounter(LevelComponents::NormalDifficulty, (unsigned int) dialog.GetNModeTimer());
        CurrentLevel->SetTimeCountdownCounter(LevelComponents::SHardDifficulty, (unsigned int) dialog.GetSHModeTimer());
        SetUnsavedChanges(true);
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
/// Undo one step of gobal history.
/// </summary>
void WL4EditorWindow::on_actionUndo_global_triggered()
{
    UndoOperationGlobal();
}

/// <summary>
/// Redo one step of gobal history.
/// </summary>
void WL4EditorWindow::on_actionRedo_global_triggered()
{
    RedoOperationGlobal();
}

/// <summary>
/// Show the user a dialog for configuring the current room. If the user clicks OK, apply selected parameters to the
/// room.
/// </summary>
void WL4EditorWindow::on_actionRoom_Config_triggered()
{
    // Set up parameters for the currently selected room, for the purpose of initializing the dialog's selections
    DialogParams::RoomConfigParams *_currentRoomConfigParams =
        new DialogParams::RoomConfigParams(CurrentLevel->GetRooms()[selectedRoom]);

    // Show the dialog
    RoomConfigDialog dialog(this, _currentRoomConfigParams);
    if (dialog.exec() == QDialog::Accepted)
    {
        // Add changes into the operation history
        OperationParams *operation = new OperationParams;
        operation->type = ChangeRoomConfigOperation;
        operation->roomConfigChange = true;
        operation->lastRoomConfigParams = new DialogParams::RoomConfigParams(*_currentRoomConfigParams);
        operation->newRoomConfigParams = new DialogParams::RoomConfigParams(dialog.GetConfigParams());
        ExecuteOperation(operation); // Set UnsavedChanges bool inside
    }
}

/// <summary>
/// Edit current tileset.
/// </summary>
void WL4EditorWindow::on_actionEdit_Tileset_triggered()
{
    // Set up parameters for the currently selected room, for the purpose of initializing the dialog's selections
    DialogParams::TilesetEditParams *_currentRoomTilesetEditParams =
        new DialogParams::TilesetEditParams(CurrentLevel->GetRooms()[selectedRoom]);

    // Show the dialog
    TilesetEditDialog dialog(this, _currentRoomTilesetEditParams);
    if (dialog.exec() == QDialog::Accepted)
    {
        int currentTilesetId = CurrentLevel->GetRooms()[selectedRoom]->GetTilesetID();
        DialogParams::TilesetEditParams *_oldRoomTilesetEditParams = new DialogParams::TilesetEditParams();
        _oldRoomTilesetEditParams->currentTilesetIndex = currentTilesetId;
        _oldRoomTilesetEditParams->newTileset = ROMUtils::singletonTilesets[currentTilesetId];
        _currentRoomTilesetEditParams->newTileset->SetChanged(true);

        // Execute Operation and add changes into the operation history
        OperationParams *operation = new OperationParams;
        operation->type = ChangeTilesetOperation;
        operation->TilesetChange = true;
        operation->lastTilesetEditParams = _oldRoomTilesetEditParams;
        operation->newTilesetEditParams = _currentRoomTilesetEditParams;
        ExecuteOperationGlobal(operation); // Set UnsavedChanges bool inside
    }
    else
    {
        delete _currentRoomTilesetEditParams->newTileset;
    }
}

/// <summary>
/// Add a new Door to the current room.
/// </summary>
void WL4EditorWindow::on_actionNew_Door_triggered()
{
    // Create a new door struct with blank fields
    LevelComponents::__DoorEntry newDoorEntry;
    memset(&newDoorEntry, 0, sizeof(LevelComponents::__DoorEntry));

    // Initialize the fields
    newDoorEntry.DoorTypeByte = (unsigned char) 2;
    newDoorEntry.EntitySetID = (unsigned char) 1;
    newDoorEntry.RoomID = (unsigned char) selectedRoom;
    newDoorEntry.DoorTypeByte = LevelComponents::DoorType::Instant;
    LevelComponents::Door *newDoor =
        new LevelComponents::Door(newDoorEntry, (unsigned char) selectedRoom, CurrentLevel->GetDoors().size());
    newDoor->SetEntitySetID((unsigned char) CurrentLevel->GetRooms()[selectedRoom]->GetCurrentEntitySetID());
    newDoor->SetDestinationDoor(CurrentLevel->GetDoors()[0]);

    // Add the new door to the Level object and re-render the screen
    CurrentLevel->AddDoor(newDoor);
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
    SetUnsavedChanges(true);
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
            dialogInitialPath = qFilePath;
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
    infoPrompt.setText(QString("WL4Editor contributors in alphabetical order are:\n"
                               "    chanchancl\n"
                               "    Goldensunboy\n"
                               "    IamRifki\n"
                               "    Kleyment\n"
                               "    shinespeciall\n"
                               "    xiazhanjian\n\n"
                               "Special Thanks:\n"
                               "    becored\n"
                               "    randrew\n"
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
    // swap Layerdata pointers if possible
    if (!(CurrentLevel->GetRooms()[selectedRoom]->GetLayer(0)->IsEnabled()))
    {
        OutputWidget->PrintString(tr(layerSwapFailureMsg));
        return;
    }
    if (CurrentLevel->GetRooms()[selectedRoom]->GetLayer(0)->GetMappingType() != LevelComponents::LayerMap16)
    {
        OutputWidget->PrintString(tr(layerSwapFailureMsg));
        return;
    }
    unsigned short *dataptr1 = CurrentLevel->GetRooms()[selectedRoom]->GetLayer(0)->GetLayerData();
    unsigned short *dataptr2 = CurrentLevel->GetRooms()[selectedRoom]->GetLayer(1)->GetLayerData();
    CurrentLevel->GetRooms()[selectedRoom]->GetLayer(0)->SetLayerData(dataptr2);
    CurrentLevel->GetRooms()[selectedRoom]->GetLayer(1)->SetLayerData(dataptr1);

    // TODO: add history record

    // UI update
    RenderScreenFull();

    // Set Dirty and change flag
    CurrentLevel->GetRooms()[selectedRoom]->GetLayer(0)->SetDirty(true);
    CurrentLevel->GetRooms()[selectedRoom]->GetLayer(1)->SetDirty(true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Swap the Layerdata for Layer_1 and Layer_2.
/// </summary>
void WL4EditorWindow::on_action_swap_Layer_1_Layer_2_triggered()
{
    // TODO: support swap a disabled Layer with a normal Layer
    // swap Layerdata pointers if possible
    if (!(CurrentLevel->GetRooms()[selectedRoom]->GetLayer(2)->IsEnabled()))
    {
        OutputWidget->PrintString(tr(layerSwapFailureMsg));
        return;
    }
    unsigned short *dataptr1 = CurrentLevel->GetRooms()[selectedRoom]->GetLayer(1)->GetLayerData();
    unsigned short *dataptr2 = CurrentLevel->GetRooms()[selectedRoom]->GetLayer(2)->GetLayerData();
    CurrentLevel->GetRooms()[selectedRoom]->GetLayer(1)->SetLayerData(dataptr2);
    CurrentLevel->GetRooms()[selectedRoom]->GetLayer(2)->SetLayerData(dataptr1);

    // TODO: add history record

    // UI update
    RenderScreenFull();

    // Set Dirty and change flag
    CurrentLevel->GetRooms()[selectedRoom]->GetLayer(1)->SetDirty(true);
    CurrentLevel->GetRooms()[selectedRoom]->GetLayer(2)->SetDirty(true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Swap the Layerdata for Layer_0 and Layer_2.
/// </summary>
void WL4EditorWindow::on_action_swap_Layer_0_Layer_2_triggered()
{
    // TODO: support swap a disabled Layer with a normal Layer
    // swap Layerdata pointers if possible
    if (!(CurrentLevel->GetRooms()[selectedRoom]->GetLayer(0)->IsEnabled()) ||
        !(CurrentLevel->GetRooms()[selectedRoom]->GetLayer(2)->IsEnabled()))
    {
        OutputWidget->PrintString(tr(layerSwapFailureMsg));
        return;
    }
    if (CurrentLevel->GetRooms()[selectedRoom]->GetLayer(0)->GetMappingType() != LevelComponents::LayerMap16)
    {
        OutputWidget->PrintString(tr(layerSwapFailureMsg));
        return;
    }
    unsigned short *dataptr1 = CurrentLevel->GetRooms()[selectedRoom]->GetLayer(0)->GetLayerData();
    unsigned short *dataptr2 = CurrentLevel->GetRooms()[selectedRoom]->GetLayer(2)->GetLayerData();
    CurrentLevel->GetRooms()[selectedRoom]->GetLayer(0)->SetLayerData(dataptr2);
    CurrentLevel->GetRooms()[selectedRoom]->GetLayer(2)->SetLayerData(dataptr1);

    // TODO: add history record

    // UI update
    RenderScreenFull();

    // Set Dirty and change flag
    CurrentLevel->GetRooms()[selectedRoom]->GetLayer(0)->SetDirty(true);
    CurrentLevel->GetRooms()[selectedRoom]->GetLayer(2)->SetDirty(true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Swap Normal and Hard Entity lists.
/// </summary>
void WL4EditorWindow::on_action_swap_Normal_Hard_triggered()
{
    // swap Entity lists
    CurrentLevel->GetRooms()[selectedRoom]->SwapEntityLists(0, 1);

    // TODO: add history record

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    CurrentLevel->GetRooms()[selectedRoom]->SetEntityListDirty(0, true);
    CurrentLevel->GetRooms()[selectedRoom]->SetEntityListDirty(1, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Swap Hard and S-Hard Entity lists.
/// </summary>
void WL4EditorWindow::on_action_swap_Hard_S_Hard_triggered()
{
    // swap Entity lists
    CurrentLevel->GetRooms()[selectedRoom]->SwapEntityLists(0, 2);

    // TODO: add history record

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    CurrentLevel->GetRooms()[selectedRoom]->SetEntityListDirty(0, true);
    CurrentLevel->GetRooms()[selectedRoom]->SetEntityListDirty(2, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Swap Normal and S-Hard Entity lists.
/// </summary>
void WL4EditorWindow::on_action_swap_Normal_S_Hard_triggered()
{
    // swap Entity lists
    CurrentLevel->GetRooms()[selectedRoom]->SwapEntityLists(1, 2);

    // TODO: add history record

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    CurrentLevel->GetRooms()[selectedRoom]->SetEntityListDirty(1, true);
    CurrentLevel->GetRooms()[selectedRoom]->SetEntityListDirty(2, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Clear Layer 0 for the current Room if condition permit.
/// </summary>
void WL4EditorWindow::on_action_clear_Layer_0_triggered()
{
    LevelComponents::Layer *layer0 = CurrentLevel->GetRooms()[selectedRoom]->GetLayer(0);
    if (layer0->GetMappingType() == LevelComponents::LayerMap16)
    {
        layer0->ResetData();
    }
    // TODO: add history record

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
    LevelComponents::Layer *layer1 = CurrentLevel->GetRooms()[selectedRoom]->GetLayer(1);
    if (layer1->GetMappingType() == LevelComponents::LayerMap16)
    {
        layer1->ResetData();
    }
    // TODO: add history record

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
    LevelComponents::Layer *layer2 = CurrentLevel->GetRooms()[selectedRoom]->GetLayer(2);
    if (layer2->GetMappingType() == LevelComponents::LayerMap16)
    {
        layer2->ResetData();
    }
    // TODO: add history record

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
    CurrentLevel->GetRooms()[selectedRoom]->ClearEntitylist(1);

    // TODO: add history record

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    CurrentLevel->GetRooms()[selectedRoom]->SetEntityListDirty(1, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Clear Entity list 0 for the current Room.
/// </summary>
void WL4EditorWindow::on_action_clear_Hard_triggered()
{
    // Delete Entity list
    CurrentLevel->GetRooms()[selectedRoom]->ClearEntitylist(0);

    // TODO: add history record

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    CurrentLevel->GetRooms()[selectedRoom]->SetEntityListDirty(0, true);
    SetUnsavedChanges(true);
}

/// <summary>
/// Clear Entity list 2 for the current Room.
/// </summary>
void WL4EditorWindow::on_action_clear_S_Hard_triggered()
{
    // Delete Entity list
    CurrentLevel->GetRooms()[selectedRoom]->ClearEntitylist(2);

    // TODO: add history record

    // UI update
    RenderScreenElementsLayersUpdate((unsigned int) -1, -1);

    // Set Dirty and change flag
    CurrentLevel->GetRooms()[selectedRoom]->SetEntityListDirty(2, true);
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
        CR_width = CurrentLevel->GetRooms()[selectedRoom]->GetWidth();
        CR_height = CurrentLevel->GetRooms()[selectedRoom]->GetHeight();
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
void WL4EditorWindow::on_actionManager_triggered()
{
    PatchManagerDialog dialog(this);
    auto result = dialog.exec();
}

/// <summary>
/// Reset Theme to Light Theme.
/// </summary>
void WL4EditorWindow::on_actionLight_triggered()
{
    QApplication::setPalette(namedColorSchemePalette(Light));
    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(6), QString("0"));
    ui->actionDark->setChecked(false);
}

/// <summary>
/// Reset Theme to Dark Theme.
/// </summary>
void WL4EditorWindow::on_actionDark_triggered()
{
    QApplication::setPalette(namedColorSchemePalette(Dark));
    SettingsUtils::SetKey(static_cast<SettingsUtils::IniKeys>(6), QString("1"));
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
        OutputWidget->PrintString("Cannot add more Rooms to the current Level!");
        return;
    }
    int offset = WL4Constants::LevelHeaderIndexTable + selectedLevel._PassageIndex * 24 + selectedLevel._LevelIndex * 4;
    int levelHeaderIndex = ROMUtils::IntFromData(offset);
    int levelHeaderPointer = WL4Constants::LevelHeaderTable + levelHeaderIndex * 12;
    int roomCount = ROMUtils::CurrentFile[levelHeaderPointer + 1];
    if (roomCount <= static_cast<int>(selectedRoom))
    {
        OutputWidget->PrintString("Cannot create room, current Room has not been saved in the rom yet!");
        return;
    }

    int roomTableAddress = ROMUtils::PointerFromData(WL4Constants::RoomDataTable + CurrentLevel->GetLevelID() * 4);
    CurrentLevel->AddRoom(new LevelComponents::Room(roomTableAddress + selectedRoom * 0x2C, newRoomId, CurrentLevel->GetLevelID()));

    // Add one Door to the new Room as well as spriteset settings
    {
        // Create a new door struct with blank fields
        LevelComponents::__DoorEntry newDoorEntry;
        memset(&newDoorEntry, 0, sizeof(LevelComponents::__DoorEntry));

        // Initialize the fields
        newDoorEntry.DoorTypeByte = (unsigned char) 2;
        newDoorEntry.EntitySetID = (unsigned char) 1;
        newDoorEntry.RoomID = (unsigned char) newRoomId;
        newDoorEntry.DoorTypeByte = LevelComponents::DoorType::Instant;
        LevelComponents::Door *newDoor =
            new LevelComponents::Door(newDoorEntry, (unsigned char) newRoomId, CurrentLevel->GetDoors().size());
        int entitysetId = CurrentLevel->GetRooms()[selectedRoom]->GetCurrentEntitySetID();
        newDoor->SetEntitySetID(entitysetId);
        newDoor->SetDestinationDoor(CurrentLevel->GetDoors()[0]);

        // Add the new door to the Level object and re-render the screen
        CurrentLevel->AddDoor(newDoor);

        // Set Current Entity list
        CurrentLevel->GetRooms()[newRoomId]->SetCurrentEntitySet(entitysetId);
    }

    // Reset LevelHeader param
    CurrentLevel->GetLevelHeader()->NumOfMap++;

    // Reset pointers in RoomHeader to avoid save chunk invalidation corruption
    int offsetlist[6] = {0, 1, 2, 5, 6, 7};
    for(int _offset: offsetlist)
    {
        CurrentLevel->GetRooms()[newRoomId]->SetRoomHeaderDataPtr(_offset, 0);
    }
    for(int i = 0; i < 3; i++)
    {
        CurrentLevel->GetRooms()[newRoomId]->GetLayer(i)->SetDataPtr(0);
    }
    CurrentLevel->GetRooms()[newRoomId]->SetCameraControlType(LevelComponents::__CameraControlType::NoLimit);

    // UI updates
    SetCurrentRoomId(newRoomId);
    OutputWidget->PrintString("Created a new blank room (# " + QString::number(newRoomId) + ") using the current room's data saved in the rom.");

    // Clear everything in the new room
    ClearEverythingInRoom(true);
}
