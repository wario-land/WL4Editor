#include "PatchEditDialog.h"
#include "ui_PatchEditDialog.h"

PatchEditDialog::PatchEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PatchEditDialog)
{
    ui->setupUi(this);
}

PatchEditDialog::~PatchEditDialog()
{
    delete ui;
}
