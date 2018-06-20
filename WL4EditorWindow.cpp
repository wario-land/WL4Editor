#include "WL4EditorWindow.h"
#include "ui_WL4EditorWindow.h"

#include "LevelComponents/Level.h"
#include "ROMUtils.h"

#include <cstdio>

#include <QFileDialog>
#include <QGraphicsScene>

// Prototype for main.cpp function
void LoadROMFile(std::string);

LevelComponents::Level *CurrentLevel;
QString statusBarText("Open a ROM file");
struct DialogParams::PassageAndLevelIndex selectedLevel = { 0, 0 };
int selectedRoom;

void WL4EditorWindow::SetStatusBarText(char *str)
{
    QLabel *old = (QLabel*) ui->statusBar->children()[0];
    QLabel *newStr = new QLabel(str);
    ui->statusBar->removeWidget(old);
    ui->statusBar->addWidget(newStr);
    delete old;
}

void WL4EditorWindow::LoadRoom()
{
    char tmpStr[30];
    sprintf(tmpStr, "Room %d", selectedRoom);
    ui->selectedRoomLabel->setText(tmpStr);
    sprintf(tmpStr, "Level ID: %d-%d", selectedLevel._PassageIndex, selectedLevel._LevelIndex);
    statusBarLabel->setText(tmpStr);
    ui->roomDecreaseButton->setEnabled(selectedRoom);
    ui->roomIncreaseButton->setEnabled(CurrentLevel->GetRooms().size() > selectedRoom + 1);
    RenderScreen(CurrentLevel->GetRooms()[selectedRoom]);
}

WL4EditorWindow::WL4EditorWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WL4EditorWindow)
{
    ui->setupUi(this);
    ui->graphicsView->scale(2, 2);
    statusBarLabel = new QLabel("Open a ROM file");
    ui->statusBar->addWidget(statusBarLabel);
    Tile16DockWidget *tmpdocker = new Tile16DockWidget;
    Tile16SelecterDockWidget = tmpdocker;
}

WL4EditorWindow::~WL4EditorWindow()
{
    delete ui;
}

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
    this->setWindowTitle(fileName.c_str());

    // Load the first level and render the screen
    CurrentLevel = new LevelComponents::Level(
        static_cast<enum LevelComponents::__passage>(selectedLevel._PassageIndex),
        static_cast<enum LevelComponents::__stage>(selectedLevel._LevelIndex)
    );
    selectedRoom = 0;
    LoadRoom();

    // Enable UI that requires a ROM file to be loaded
    ui->loadLevelButton->setEnabled(true);

    // Load Dock widget
    this->addDockWidget(Qt::RightDockWidgetArea, Tile16SelecterDockWidget);
    Tile16SelecterDockWidget->SetTileset(80);
}

void WL4EditorWindow::RenderScreen(LevelComponents::Room *room)
{
    QGraphicsScene *oldScene = ui->graphicsView->scene();
    if(oldScene == nullptr)
    {
        delete oldScene;
    }

    QGraphicsScene *scene = room->GetGraphicsScene();
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
}

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
        LoadRoom();
    }
}

void WL4EditorWindow::on_roomDecreaseButton_clicked()
{
    --selectedRoom;
    LoadRoom();
}

void WL4EditorWindow::on_roomIncreaseButton_clicked()
{
    ++selectedRoom;
    LoadRoom();
}
