#include "WL4EditorWindow.h"
#include "ui_WL4EditorWindow.h"

#include "LevelComponents/Level.h"

#include <QFileDialog>
#include <QGraphicsScene>

// Prototype for main.cpp function
void LoadROMFile(std::string);

LevelComponents::Level *CurrentLevel;

WL4EditorWindow::WL4EditorWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WL4EditorWindow)
{
    ui->setupUi(this);
    ui->graphicsView->scale(2, 2);
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

    // Load the first level and render the screen
    CurrentLevel = new LevelComponents::Level(LevelComponents::EntryPassage, LevelComponents::FirstLevel);
    RenderScreen(CurrentLevel->GetRooms()[0]);

    // Set the program title
    std::string fileName = filePath.substr(filePath.rfind('/') + 1);
    this->setWindowTitle(fileName.c_str());
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
