#include "RoomConfigDialog.h"
#include "ui_RoomConfigDialog.h"

RoomConfigDialog::RoomConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RoomConfigDialog)
{
    ui->setupUi(this);
}

RoomConfigDialog::~RoomConfigDialog()
{
    delete ui;
}
