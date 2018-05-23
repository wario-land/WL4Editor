#include "WL4EditorWindow.h"
#include "ui_WL4EditorWindow.h"

#include <QFileDialog>
#include <iostream>
#include <fstream>

#include "ROMUtils.h"

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

    // Read ROM file into current file array
    std::ifstream ifs(filePath, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();
    int length = pos;
    ROMUtils::CurrentFile = new unsigned char[length];
    ifs.seekg(0, std::ios::beg);
    ifs.read((char*) ROMUtils::CurrentFile, length);
    ifs.close();
}
