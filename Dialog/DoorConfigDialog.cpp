#include "DoorConfigDialog.h"
#include "ui_DoorConfigDialog.h"

DoorConfigDialog::DoorConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DoorConfigDialog)
{
    ui->setupUi(this);
}

DoorConfigDialog::~DoorConfigDialog()
{
    delete ui;
}
