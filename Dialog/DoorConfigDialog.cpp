#include "DoorConfigDialog.h"
#include "ui_DoorConfigDialog.h"

// constexpr declarations for the initializers in the header
constexpr const char *DoorConfigDialog::DoortypeSetData[5];

// static variables used by DoorConfigDialog
static QStringList DoortypeSet;

DoorConfigDialog::DoorConfigDialog(QWidget *parent, LevelComponents::Room *currentroom, int doorID) :
    QDialog(parent),
    ui(new Ui::DoorConfigDialog)
{
    ui->setupUi(this);

    // Initialize UI elements
    ui->ComboBox_DoorType->addItems(DoortypeSet);
    ui->ComboBox_DoorType->setCurrentIndex(currentroom->GetDoor(doorID)->GetDoortypeNum());
    ui->SpinBox_DoorX->setValue(currentroom->GetDoor(doorID)->GetX1());
    ui->SpinBox_DoorY->setValue(currentroom->GetDoor(doorID)->GetY1());
    int doorwidth = currentroom->GetDoor(doorID)->GetX2() - currentroom->GetDoor(doorID)->GetX1() + 1;
    int doorheight = currentroom->GetDoor(doorID)->GetY2() - currentroom->GetDoor(doorID)->GetY1() + 1;
    ui->SpinBox_DoorWidth->setValue(doorwidth);
    ui->SpinBox_DoorHeight->setValue(doorheight);
    ui->SpinBox_WarioX->setValue(currentroom->GetDoor(doorID)->GetDeltaX());
    ui->SpinBox_WarioY->setValue(currentroom->GetDoor(doorID)->GetDeltaY());
    ui->SpinBox_BGM_ID->setValue(currentroom->GetDoor(doorID)->GetBGM_ID());
    // TODOs
}

DoorConfigDialog::~DoorConfigDialog()
{
    delete ui;
}

void DoorConfigDialog::StaticInitialization()
{
    // Initialize the selections for the Door type
    for(unsigned int i = 0; i < sizeof(DoortypeSetData)/sizeof(DoortypeSetData[0]); ++i)
    {
        DoortypeSet << DoortypeSetData[i];
    }
}
