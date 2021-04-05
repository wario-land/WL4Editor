#include "ChunkEditorDialog.h"
#include "ui_ChunkEditorDialog.h"

ChunkEditorDialog::ChunkEditorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChunkEditorDialog)
{
    ui->setupUi(this);
}

ChunkEditorDialog::~ChunkEditorDialog()
{
    delete ui;
}
