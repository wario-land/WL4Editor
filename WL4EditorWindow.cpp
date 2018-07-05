#include "WL4EditorWindow.h"
#include "ui_WL4EditorWindow.h"
#include "LevelComponents/Level.h"
#include "ROMUtils.h"

#include <cstdio>

#include <QFileDialog>
#include <QGraphicsScene>

void LoadROMFile(std::string); // Prototype for main.cpp function
LevelComponents::Level *CurrentLevel;
QString statusBarText("Open a ROM file");
struct DialogParams::PassageAndLevelIndex selectedLevel = { 0, 0 };
int selectedRoom;
bool firstROMLoaded = false;
bool editModeWidgetInitialized = false;

/// <summary>
/// Construct the instance of the WL4EditorWindow.
/// </summary>
/// <remarks>
/// The graphics view is hardcoded to scale at 2x size.
/// </remarks>
/// <param name="parent">
/// The parent QWidget.
/// </param>
WL4EditorWindow::WL4EditorWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WL4EditorWindow)
{
    ui->setupUi(this);
    ui->graphicsView->scale(2, 2);
    statusBarLabel = new QLabel("Open a ROM file");
    statusBarLabel->setMargin(3);
    ui->statusBar->addWidget(statusBarLabel);
    EditModeWidget = new EditModeDockWidget();
    Tile16SelecterWidget = new Tile16DockWidget;
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
    QLabel *old = (QLabel*) ui->statusBar->children()[0];
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
    ui->roomIncreaseButton->setEnabled((int) CurrentLevel->GetRooms().size() > selectedRoom + 1);

    // Render the screen
    RenderScreen();
}

/// <summary>
/// Present the user with an "open file" dialog, and perform necessary level loading actions if a ROM in successfully loaded.
/// </summary>
/// <remarks>
/// Load the ROM file into CurrentFile.
/// Set the title of the main window.
/// Load level 0-0.
/// Render room 0 of the level.
/// On first successful ROM load, add and update UI that requires a ROM to have been loaded.
/// </remarks>
void WL4EditorWindow::on_actionOpen_ROM_triggered()
{
    // Select a ROM file to open
    QString qFilePath = QFileDialog::getOpenFileName(
        this,
        tr("Open ROM file"),
        "C:\\",
        tr("GBA ROM files (*.gba)")
    );
    if(!qFilePath.compare(""))
    {
        return;
    }

    // Load the ROM file
    std::string filePath = qFilePath.toStdString();
    LoadROMFile(filePath);

    // Set the program title
    std::string fileName = filePath.substr(filePath.rfind('/') + 1);
    setWindowTitle(fileName.c_str());

    // Load the first level and render the screen
    if(CurrentLevel) delete CurrentLevel;
    CurrentLevel = new LevelComponents::Level(
        static_cast<enum LevelComponents::__passage>(selectedLevel._PassageIndex),
        static_cast<enum LevelComponents::__stage>(selectedLevel._LevelIndex)
    );
    selectedRoom = 0;
    LoadRoomUIUpdate();
    int tmpTilesetID = CurrentLevel->GetRooms()[selectedRoom]->GetTilesetID();

    // Only modify UI on the first time a ROM is loaded
    if(!firstROMLoaded)
    {
        firstROMLoaded = true;

        // Enable UI that requires a ROM file to be loaded
        ui->loadLevelButton->setEnabled(true);
        ui->actionLevel_Config->setEnabled(true);

        // Load Dock widget
        addDockWidget(Qt::RightDockWidgetArea, EditModeWidget);
        addDockWidget(Qt::RightDockWidgetArea, Tile16SelecterWidget);
        Tile16SelecterWidget->SetTileset(tmpTilesetID);
    }
}

/// <summary>
/// Perform a full render of the currently selected room.
/// </summary>
/// <param name="str">
/// The string contents to put in the status bar.
/// </param>
void WL4EditorWindow::RenderScreen()
{
    // Delete the old scene, if it exists
    QGraphicsScene *oldScene = ui->graphicsView->scene();
    if(oldScene == nullptr)
    {
        delete oldScene;
    }

    // Perform a full render of the screen
    struct LevelComponents::RenderUpdateParams renderParams(LevelComponents::FullRender);
    QGraphicsScene *scene = CurrentLevel->GetRooms()[selectedRoom]->RenderGraphicsScene(nullptr, &renderParams);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
}

/// <summary>
/// Present the user with an "open level" dialog, in which a level can be selected to load.
/// </summary>
/// <remarks>
/// The newly loaded level will start by loading room 0 into the editor.
/// </remarks>
void WL4EditorWindow::on_loadLevelButton_clicked()
{
    // Load the first level and render the screen
    ChooseLevelDialog tmpdialog(selectedLevel);
    if(tmpdialog.exec() == QDialog::Accepted) {
        selectedLevel = tmpdialog.GetResult();
        CurrentLevel = new LevelComponents::Level(
            static_cast<enum LevelComponents::__passage>(selectedLevel._PassageIndex),
            static_cast<enum LevelComponents::__stage>(selectedLevel._LevelIndex)
        );
        selectedRoom = 0;
        LoadRoomUIUpdate();
        int tmpTilesetID = CurrentLevel->GetRooms()[selectedRoom]->GetTilesetID();
        Tile16SelecterWidget->SetTileset(tmpTilesetID);
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
    --selectedRoom;
    LoadRoomUIUpdate();
    int tmpTilesetID = CurrentLevel->GetRooms()[selectedRoom]->GetTilesetID();
    Tile16SelecterWidget->SetTileset(tmpTilesetID);
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
    ++selectedRoom;
    LoadRoomUIUpdate();
    int tmpTilesetID = CurrentLevel->GetRooms()[selectedRoom]->GetTilesetID();
    Tile16SelecterWidget->SetTileset(tmpTilesetID);
}

/// <summary>
/// Present the user with a level config dialog for changing the level title and frog timer values.
/// </summary>
/// <remarks>
/// Only set the new parameters from the dialog if OK is pressed.
/// </remarks>
void WL4EditorWindow::on_actionLevel_Config_triggered()
{
    // Show a level config dialog to the user
    LevelConfigDialog dialog;
    dialog.InitTextBoxes(
        CurrentLevel->GetLevelName(),
        CurrentLevel->GetTimeCountdownCounter(LevelComponents::HardDifficulty),
        CurrentLevel->GetTimeCountdownCounter(LevelComponents::NormalDifficulty),
        CurrentLevel->GetTimeCountdownCounter(LevelComponents::SHardDifficulty)
    );

    // If OK is pressed, then set the level attributes
    if(dialog.exec() == QDialog::Accepted)
    {
        CurrentLevel->SetLevelName(dialog.GetPaddedLevelName());
        CurrentLevel->SetTimeCountdownCounter(LevelComponents::HardDifficulty, (unsigned int)dialog.GetHModeTimer());
        CurrentLevel->SetTimeCountdownCounter(LevelComponents::NormalDifficulty, (unsigned int)dialog.GetNModeTimer());
        CurrentLevel->SetTimeCountdownCounter(LevelComponents::SHardDifficulty, (unsigned int)dialog.GetSHModeTimer());
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
    if(firstROMLoaded)
    {
        resizeDocks({EditModeWidget}, {1}, Qt::Vertical);
    }
}
