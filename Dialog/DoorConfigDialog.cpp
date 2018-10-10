#include "DoorConfigDialog.h"
#include "ui_DoorConfigDialog.h"

// constexpr declarations for the initializers in the header
constexpr const char *DoorConfigDialog::DoortypeSetData[5];
constexpr const char *DoorConfigDialog::EntitynameSetData[128];

// static variables used by DoorConfigDialog
static QStringList DoortypeSet;
static QStringList EntitynameSet;

LevelComponents::EntitySet *DoorConfigDialog::entitiessets[90];
LevelComponents::Entity *DoorConfigDialog::entities[129];

/// <summary>
/// Construct an instance of DoorConfigDialog.
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
    EntityFilterTable = new EntityFilterTableModel(ui->TableView_EntityFilter);
    ui->TableView_EntityFilter->setModel(EntityFilterTable);
    IsInitialized = false;

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
    ui->SpinBox_DoorWidth->setMaximum(tmpCurrentRoom->GetWidth() - currentdoor->GetX1());
    ui->SpinBox_DoorHeight->setMaximum(tmpCurrentRoom->GetHeight() - currentdoor->GetY1());
    ui->SpinBox_DoorX->setMaximum(tmpCurrentRoom->GetWidth() - doorwidth);
    ui->SpinBox_DoorY->setMaximum(tmpCurrentRoom->GetHeight() - doorheight);
    ui->SpinBox_WarioX->setValue(currentdoor->GetDeltaX());
    ui->SpinBox_WarioY->setValue(currentdoor->GetDeltaY());
    ui->SpinBox_BGM_ID->setValue(currentdoor->GetBGM_ID());

    // Initialize the selections for destination door combobox
    QStringList doorofLevelSet;
    for(unsigned int i = 0; i < _level->GetDoors().size(); ++i)
    {
        doorofLevelSet << _level->GetDoors()[i]->GetDoorname();
    }
    ui->ComboBox_DoorDestinationPicker->addItems(doorofLevelSet);
    ui->ComboBox_DoorDestinationPicker->setCurrentIndex(currentdoor->GetDestinationDoor()->GetGlobalDoorID());
    RenderGraphicsView_Preview();

    // Initialize the entity list drop-down
    for(unsigned int i = 1; i < sizeof(entities)/sizeof(entities[0]); ++i)
    {
        EntityFilterTable->AddEntity(entities[i]);
    }
    //unsigned char entitySetID = currentdoor->GetEntitySetID();
    // TODO set the index here

    IsInitialized = true;
}

/// <summary>
/// Deconstruct the Door Config Dialog.
/// </summary>
DoorConfigDialog::~DoorConfigDialog()
{
    delete tmpCurrentRoom;
    delete tmpDestinationRoom;
    delete EntityFilterTable;
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

    // Initialize the selections for the Entity name
    for(unsigned int i = 0; i < sizeof(EntitynameSetData)/sizeof(EntitynameSetData[0]); ++i)
    {
        EntitynameSet << EntitynameSetData[i];
    }
}

/// <summary>
/// Perform static initializtion of EntitySets and Entities for the dialog.
/// </summary>
void DoorConfigDialog::EntitySetsInitialization()
{
    // Initialize all the entitysets
    for(int i = 0; i < 90; ++i)
    {
        entitiessets[i] = new LevelComponents::EntitySet(i, WL4Constants::UniversalSpritesPalette);
    }

    // Initialize all the Entity
    for(int i = 0; i < 129; ++i)
    {
        struct LevelComponents::EntitySetAndEntitylocalId tmpEntitysetAndEntitylocalId = LevelComponents::EntitySet::EntitySetFromEntityID(i);
        entities[i] = new LevelComponents::Entity(tmpEntitysetAndEntitylocalId.entitylocalId, i, entitiessets[tmpEntitysetAndEntitylocalId.entitysetId]);
    }
}

/// <summary>
/// Render Room and Doors in GraphicsView_Preview.
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
/// Render Room and Doors in GraphicsView_DestinationDoor.
/// </summary>
/// /// <param name="doorIDinRoom">
/// door Id in Room.
/// </param>
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

/// <summary>
/// Reset Door Rect according to the value from the door properties SpinBoxes.
/// </summary>
void DoorConfigDialog::ResetDoorRect()
{
    LevelComponents::Door *currentdoor0 = tmpCurrentRoom->GetDoor(DoorID);
    currentdoor0->SetDoorPlace((unsigned char) ui->SpinBox_DoorX->value(), (unsigned char) (ui->SpinBox_DoorX->value() + ui->SpinBox_DoorWidth->value() - 1),
                               (unsigned char) ui->SpinBox_DoorY->value(), (unsigned char) (ui->SpinBox_DoorY->value() + ui->SpinBox_DoorHeight->value() - 1));
    int doorwidth = currentdoor0->GetX2() - currentdoor0->GetX1() + 1;
    int doorheight = currentdoor0->GetY2() - currentdoor0->GetY1() + 1;
    ui->SpinBox_DoorX->setMaximum(tmpCurrentRoom->GetWidth() - doorwidth);
    ui->SpinBox_DoorY->setMaximum(tmpCurrentRoom->GetHeight() - doorheight);
    ui->SpinBox_DoorWidth->setMaximum(tmpCurrentRoom->GetWidth() - currentdoor0->GetX1());
    ui->SpinBox_DoorHeight->setMaximum(tmpCurrentRoom->GetHeight() - currentdoor0->GetY1());
    UpdateDoorLayerGraphicsView_Preview();
    // when DestinationDoor and currentDoor are in the same Room, the DestinationDoor also needs an update.
    UpdateDoorLayerGraphicsView_DestinationDoor();
}

/// <summary>
/// Update Door layer in GraphicsView_Preview.
/// </summary>
void DoorConfigDialog::UpdateDoorLayerGraphicsView_Preview()
{
    struct LevelComponents::RenderUpdateParams tparam(LevelComponents::ElementsLayersUpdate);
    tparam.tileX = tparam.tileY = 0; tparam.tileID = (unsigned short) 0;
    tparam.SelectedDoorID = (unsigned int) DoorID; //ID in Room
    tparam.mode.editMode = Ui::DoorEditMode;
    tparam.mode.entitiesEnabled = tparam.mode.cameraAreasEnabled = false;
    tmpCurrentRoom->RenderGraphicsScene(ui->GraphicsView_Preview->scene(), &tparam);
}

/// <summary>
/// Update Door layer in GraphicsView_DestinationDoor.
/// </summary>
void DoorConfigDialog::UpdateDoorLayerGraphicsView_DestinationDoor()
{
    struct LevelComponents::RenderUpdateParams tparam(LevelComponents::ElementsLayersUpdate);
    tparam.tileX = tparam.tileY = 0; tparam.tileID = (unsigned short) 0;
    tparam.SelectedDoorID = (unsigned int) tmpDestinationRoom->GetLocalDoorID(ui->ComboBox_DoorDestinationPicker->currentIndex()); //ID in Room
    tparam.mode.editMode = Ui::DoorEditMode;
    tparam.mode.entitiesEnabled = tparam.mode.cameraAreasEnabled = false;
    tmpDestinationRoom->RenderGraphicsScene(ui->GraphicsView_DestinationDoor->scene(), &tparam);
}

/// <summary>
/// Reset currentDoor destination according to the currentIndex of the ComboBox_DoorDestinationPicker.
/// </summary>
/// <param name="index">
/// currentIndex of the ComboBox_DoorDestinationPicker.
/// </param>
void DoorConfigDialog::on_ComboBox_DoorDestinationPicker_currentIndexChanged(int index)
{
    delete tmpDestinationRoom;
    tmpDestinationRoom = new LevelComponents::Room(_currentLevel->GetRooms()[_currentLevel->GetDoors()[index]->GetRoomID()]);
    tmpDestinationRoom->SetDoors(_currentLevel->GetRoomDoors((unsigned int) _currentLevel->GetDoors()[index]->GetRoomID()));
    _currentLevel->GetDoors()[index]->SetDestinationDoor(_currentLevel->GetDoors()[index]);
    RenderGraphicsView_DestinationDoor(tmpDestinationRoom->GetLocalDoorID(index));
}

/// <summary>
/// Reset X value of Door position.
/// </summary>
/// <param name="arg1">
/// unused.
/// </param>
void DoorConfigDialog::on_SpinBox_DoorX_valueChanged(int arg1)
{
    (void) arg1;
    if(!IsInitialized) return;
    ResetDoorRect();
}

/// <summary>
/// Reset Y value of Door position.
/// </summary>
/// <param name="arg1">
/// unused.
/// </param>
void DoorConfigDialog::on_SpinBox_DoorY_valueChanged(int arg1)
{
    (void) arg1;
    if(!IsInitialized) return;
    ResetDoorRect();
}

/// <summary>
/// Reset Door Width according to the value from the door properties SpinBoxes.
/// </summary>
/// <param name="arg1">
/// unused.
/// </param>
void DoorConfigDialog::on_SpinBox_DoorWidth_valueChanged(int arg1)
{
    (void) arg1;
    if(!IsInitialized) return;
    ResetDoorRect();
}

/// <summary>
/// Reset Door Height according to the value from the door properties SpinBoxes.
/// </summary>
/// <param name="arg1">
/// unused.
/// </param>
void DoorConfigDialog::on_SpinBox_DoorHeight_valueChanged(int arg1)
{
    (void) arg1;
    if(!IsInitialized) return;
    ResetDoorRect();
}

/// <summary>
/// Reset currentDoor DoorType.
/// </summary>
/// <param name="index">
/// currentIndex of the ComboBox_DoorType.
/// </param>
void DoorConfigDialog::on_ComboBox_DoorType_currentIndexChanged(int index)
{
    if(!IsInitialized) return;
    LevelComponents::Door *currentdoor0 = tmpCurrentRoom->GetDoor(DoorID);
    if((index == 0) && (currentdoor0->GetDoortypeNum() != 1))
    {
        QMessageBox::information(this, QString("Info"), QString("Unless you know what you are doing, don't put more than 1 Portal-type Door (vortex) in one level."));
    }
    // TODOs: need more auto-reset to some of the Door attributes when select DoorType 4 or 5.
    currentdoor0->SetDoorType(static_cast<LevelComponents::DoorType>(index + 1));
}

/// <summary>
/// Reset Wario appearing place X delta.
/// </summary>
/// <param name="arg1">
/// unused.
/// </param>
void DoorConfigDialog::on_SpinBox_WarioX_valueChanged(int arg1)
{
    (void) arg1;
    tmpCurrentRoom->GetDoor(DoorID)->SetDelta((unsigned char) ui->SpinBox_WarioX->value(), (unsigned char) ui->SpinBox_WarioY->value());
}

/// <summary>
/// Reset Wario appearing place Y delta.
/// </summary>
/// <param name="arg1">
/// unused.
/// </param>
void DoorConfigDialog::on_SpinBox_WarioY_valueChanged(int arg1)
{
    (void) arg1;
    tmpCurrentRoom->GetDoor(DoorID)->SetDelta((unsigned char) ui->SpinBox_WarioX->value(), (unsigned char) ui->SpinBox_WarioY->value());
}

/// <summary>
/// Set a new BGM_ID for the Door if the BGM need a reset.
/// </summary>
/// <param name="arg1">
/// unused.
/// </param>
void DoorConfigDialog::on_SpinBox_BGM_ID_valueChanged(int arg1)
{
    (void) arg1;
    tmpCurrentRoom->GetDoor(DoorID)->SetBGM((unsigned char) ui->SpinBox_BGM_ID->value());
}

//---------------------------------------------------------------------------------------------------------------------------
// EntityFilterTableModel functions
//---------------------------------------------------------------------------------------------------------------------------

/// <summary>
/// Construct an instance of EntityFilterTableModel.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
EntityFilterTableModel::EntityFilterTableModel(QWidget *_parent) : QAbstractTableModel(_parent), parent(_parent)
{
    // TODO
}

/// <summary>
/// Deconstruct the EntityFilterTableModel.
/// </summary>
EntityFilterTableModel::~EntityFilterTableModel()
{
    // TODO
}

/// <summary>
/// Add an Entity to EntityFilterTableModel.
/// </summary>
/// <param name="entity">
/// The entity to add.
/// </param>
void EntityFilterTableModel::AddEntity(LevelComponents::Entity *entity)
{
    beginInsertRows((const QModelIndex&)*parent, entities.size(), entities.size());
    entities.push_back(entity);
    endInsertRows();
}

/// <summary>
/// Return the data for some cell in the table.
/// <summary>
/// <param name="index">
/// The 2D indexer for the table.
/// </param>
/// <returns>
/// The data at X = index.column(), Y = index.row()
/// </returns>
QVariant EntityFilterTableModel::data(const QModelIndex &index, int) const
{
    if(index.column())
    {
        return entities[index.row()]->Render();
    }
    else
    {
        return DoorConfigDialog::EntitynameSetData[entities[index.row()]->GetEntityGlobalID() - 1];
    }
}
