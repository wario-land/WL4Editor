#include "DoorConfigDialog.h"
#include "ui_DoorConfigDialog.h"

// constexpr declarations for the initializers in the header
constexpr const char *DoorConfigDialog::DoortypeSetData[5];

// static variables used by DoorConfigDialog
static QStringList DoortypeSet;

/// <summary>
/// Construct the instance of the DoorConfigDialog.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
DoorConfigDialog::DoorConfigDialog(QWidget *parent, LevelComponents::Room *currentroom, int doorID, LevelComponents::Level *_level) :
    QDialog(parent),
    ui(new Ui::DoorConfigDialog),
    _currentLevel(_level),
    tmpCurrentRoom(new LevelComponents::Room(currentroom)),
    tmpDestinationRoom(new LevelComponents::Room(_level->GetRooms()[currentroom->GetDoor(doorID)->GetDestinationDoor()->GetRoomID()])),
    DoorID(doorID)
{
    ui->setupUi(this);

    // Distribute Doors into the temp CurrentRoom
    tmpCurrentRoom->SetDoors(_level->GetRoomDoors(currentroom->GetRoomID()));
    tmpDestinationRoom->SetDoors(_level->GetRoomDoors(currentroom->GetDoor(doorID)->GetDestinationDoor()->GetRoomID()));

    // Initialize UI elements
    ui->ComboBox_DoorType->addItems(DoortypeSet);
    LevelComponents::Door *currentdoor = tmpCurrentRoom->GetDoor(doorID);
    ui->ComboBox_DoorType->setCurrentIndex(currentdoor->GetDoortypeNum() - 1);
    ui->SpinBox_DoorX->setValue(currentdoor->GetX1());
    ui->SpinBox_DoorY->setValue(currentdoor->GetY1());
    int doorwidth = currentdoor->GetX2() - currentdoor->GetX1() + 1;
    int doorheight = currentdoor->GetY2() - currentdoor->GetY1() + 1;
    ui->SpinBox_DoorWidth->setValue(doorwidth);
    ui->SpinBox_DoorHeight->setValue(doorheight);
    ui->SpinBox_WarioX->setValue(currentdoor->GetDeltaX());
    ui->SpinBox_WarioY->setValue(currentdoor->GetDeltaY());
    ui->SpinBox_BGM_ID->setValue(currentdoor->GetBGM_ID());
    RenderGraphicsView_Preview();
    RenderGraphicsView_DestinationDoor(tmpDestinationRoom->GetLocalDoorID(currentdoor->GetDestinationDoor()->GetGlobalDoorID()));
    // TODOs
}

/// <summary>
/// Deconstruct the Door Config Dialog.
/// </summary>
DoorConfigDialog::~DoorConfigDialog()
{
    delete tmpCurrentRoom;
    delete tmpDestinationRoom;
    delete ui;
}

/// <summary>
/// Perform static initializtion of constant data structures for the dialog.
/// </summary>
void DoorConfigDialog::StaticInitialization()
{
    // Initialize the selections for the Door type
    for(unsigned int i = 0; i < sizeof(DoortypeSetData)/sizeof(DoortypeSetData[0]); ++i)
    {
        DoortypeSet << DoortypeSetData[i];
    }
}

/// <summary>
/// Render Room and Doors on GraphicsView_Preview.
/// </summary>
void DoorConfigDialog::RenderGraphicsView_Preview()
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
    QGraphicsScene *scene = tmpCurrentRoom->RenderGraphicsScene(ui->GraphicsView_Preview->scene(), &tparam);
    ui->GraphicsView_Preview->setScene(scene);
    ui->GraphicsView_Preview->setAlignment(Qt::AlignTop | Qt::AlignLeft);
}

/// <summary>
/// Render Room and Doors on GraphicsView_DestinationDoor.
/// </summary>
void DoorConfigDialog::RenderGraphicsView_DestinationDoor(int doorIDinRoom)
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
    QGraphicsScene *scene = tmpDestinationRoom->RenderGraphicsScene(ui->GraphicsView_DestinationDoor->scene(), &tparam);
    ui->GraphicsView_DestinationDoor->setScene(scene);
    ui->GraphicsView_DestinationDoor->setAlignment(Qt::AlignTop | Qt::AlignLeft);
}
