#include "WL4EditorWindow.h"
#include "ui_WL4EditorWindow.h"

#include <QFileDialog>
#include <QGraphicsScene>

// Prototype for main.cpp function
void LoadROMFile(std::string);

WL4EditorWindow::WL4EditorWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WL4EditorWindow)
{
    ui->setupUi(this);
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
    std::string filePath = qFilePath.toStdString();

    LoadROMFile(filePath);
}

#include <iostream>
void WL4EditorWindow::RenderScreen(LevelComponents::Room *room)
{
    QGraphicsScene *scene = new QGraphicsScene(0, 0, 24 * 16, 32 * 16);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    LevelComponents::Tileset *t = room->GetTileset();
    /*
    // Draw tile8x8 with palette 0
    for(int i = 0; i < 48; ++i)
    {
        for(int j = 0; j < 32; ++j)
        {
            int x = j * 8, y = i * 8;
            t->GetTile8x8Data()[i * 32 + j]->DrawTile(scene, x, y);
        }
    }
    */
    // Draw map16
    // /*
    LevelComponents::TileMap16 **map16data = t->GetMap16Data();
    for(int i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 32; ++j)
        {
            for(int k = 0; k < 8; ++k)
            {
                int x = i * 128 + k * 16, y = j * 16;
                map16data[i * 256 + j * 8 + k]->DrawTile(scene, x, y);
            }
        }
    }
    // */
}
