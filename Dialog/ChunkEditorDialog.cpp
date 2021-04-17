#include "ChunkEditorDialog.h"
#include "ui_ChunkEditorDialog.h"

ChunkEditorDialog::ChunkEditorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChunkEditorDialog)
{
    ui->setupUi(this);

    setWindowTitle("Chunk Manager");
    TreeView = ui->treeView_chunkData;
}

ChunkEditorDialog::~ChunkEditorDialog()
{
    delete ui;
}
