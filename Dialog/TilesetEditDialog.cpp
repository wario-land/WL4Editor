#include "TilesetEditDialog.h"
#include "ui_TilesetEditDialog.h"

TilesetEditDialog::TilesetEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TilesetEditDialog)
{
    ui->setupUi(this);
}

TilesetEditDialog::~TilesetEditDialog()
{
    delete ui;
}
