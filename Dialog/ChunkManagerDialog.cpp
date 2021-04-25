#include "ChunkManagerDialog.h"
#include "ui_ChunkManagerDialog.h"

ChunkManagerDialog::ChunkManagerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChunkManagerDialog)
{
    ui->setupUi(this);

    setWindowTitle("Chunk Manager");
    TreeView = ui->treeView_chunkData;
}

ChunkManagerDialog::~ChunkManagerDialog()
{
    delete ui;
}
