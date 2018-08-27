#include "DoorConfigDialog.h"
#include "ui_DoorConfigDialog.h"

// constexpr declarations for the initializers in the header
constexpr const char *DoorConfigDialog::DoortypeSetData[5];

// static variables used by DoorConfigDialog
static QStringList DoortypeSet;

DoorConfigDialog::DoorConfigDialog(QWidget *parent, LevelComponents::Room *currentroom, int doorID, std::vector<LevelComponents::Room *> _levelrooms) :
    QDialog(parent),
    ui(new Ui::DoorConfigDialog),
    Levelrooms(_levelrooms),
    CurrentRoom(currentroom),
    DoorID(doorID)
{
    ui->setupUi(this);

    // Initialize UI elements
    ui->ComboBox_DoorType->addItems(DoortypeSet);
    ui->ComboBox_DoorType->setCurrentIndex(currentroom->GetDoor(doorID)->GetDoortypeNum() - 1);
    ui->SpinBox_DoorX->setValue(currentroom->GetDoor(doorID)->GetX1());
    ui->SpinBox_DoorY->setValue(currentroom->GetDoor(doorID)->GetY1());
    int doorwidth = currentroom->GetDoor(doorID)->GetX2() - currentroom->GetDoor(doorID)->GetX1() + 1;
    int doorheight = currentroom->GetDoor(doorID)->GetY2() - currentroom->GetDoor(doorID)->GetY1() + 1;
    ui->SpinBox_DoorWidth->setValue(doorwidth);
    ui->SpinBox_DoorHeight->setValue(doorheight);
    ui->SpinBox_WarioX->setValue(currentroom->GetDoor(doorID)->GetDeltaX());
    ui->SpinBox_WarioY->setValue(currentroom->GetDoor(doorID)->GetDeltaY());
    ui->SpinBox_BGM_ID->setValue(currentroom->GetDoor(doorID)->GetBGM_ID());
    InitRenderGraphicsView_Preview();
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

void DoorConfigDialog::InitRenderGraphicsView_Preview()
{
    QGraphicsScene *oldScene = ui->GraphicsView_Preview->scene();
    if(oldScene)
    {
        delete oldScene;
    }
    struct LevelComponents::RenderUpdateParams tparam(LevelComponents::FullRender);
    tparam.tileX = tparam.tileY = 0; tparam.tileID = (unsigned short) 0;
    tparam.SelectedDoorID = (unsigned int) DoorID; //ID in Room
    tparam.mode.editMode = Ui::DoorEditMode;
    tparam.mode.entitiesEnabled = tparam.mode.cameraAreasEnabled = false;
    QGraphicsScene *scene = CurrentRoom->RenderGraphicsScene(ui->GraphicsView_Preview->scene(), &tparam);
    ui->GraphicsView_Preview->setScene(scene);
    ui->GraphicsView_Preview->setAlignment(Qt::AlignTop | Qt::AlignLeft);
}

void DoorConfigDialog::InitRenderGraphicsView_DestinationDoor(LevelComponents::Room *currentRoom, int doorIDinRoom)
{
    QGraphicsScene *oldScene = ui->GraphicsView_DestinationDoor->scene();
    if(oldScene)
    {
        delete oldScene;
    }
    struct LevelComponents::RenderUpdateParams tparam(LevelComponents::FullRender);
    tparam.tileX = tparam.tileY = 0; tparam.tileID = (unsigned short) 0;
    tparam.SelectedDoorID = (unsigned int) doorIDinRoom; //ID in Room
    tparam.mode.editMode = Ui::DoorEditMode;
    tparam.mode.entitiesEnabled = tparam.mode.cameraAreasEnabled = false;
    QGraphicsScene *scene = currentRoom->RenderGraphicsScene(ui->GraphicsView_DestinationDoor->scene(), &tparam);
    ui->GraphicsView_DestinationDoor->setScene(scene);
    ui->GraphicsView_DestinationDoor->setAlignment(Qt::AlignTop | Qt::AlignLeft);
}
