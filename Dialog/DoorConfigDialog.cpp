#include "DoorConfigDialog.h"
#include "ui_DoorConfigDialog.h"
#include "ROMUtils.h"
#include "SettingsUtils.h"

// constexpr declarations for the initializers in the header
constexpr const char *DoorConfigDialog::DoortypeSetData[5];
constexpr const char *DoorConfigDialog::EntitynameSetData[129];

// static variables used by DoorConfigDialog
static QStringList DoortypeSet;
static QStringList EntitynameSet;

/// <summary>
/// Construct an instance of DoorConfigDialog.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
DoorConfigDialog::DoorConfigDialog(QWidget *parent, LevelComponents::Room *currentroom, int doorID,
                                   LevelComponents::Level *_level) :
        QDialog(parent),
        ui(new Ui::DoorConfigDialog), _currentLevel(_level), CurrentRoom(currentroom),
        tmpCurrentRoom(new LevelComponents::Room(currentroom)),
        tmpDestinationRoom(new LevelComponents::Room(
            _level->GetRooms()[currentroom->GetDoor(doorID)->GetDestinationDoor()->GetRoomID()])),
        DoorID(doorID)
{
    ui->setupUi(this);

    // TableView
    EntityFilterTable = new EntityFilterTableModel(ui->TableView_EntityFilter);
    // Header
    EntityFilterTable->setHorizontalHeaderLabels(QStringList() << ""
                                                               << "Entity Name"
                                                               << "Entity Image");
    EntityFilterTable->setColumnCount(3);
    // set col width
    ui->TableView_EntityFilter->setColumnWidth(0, 30);
    ui->TableView_EntityFilter->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->TableView_EntityFilter->resizeColumnsToContents();
    // set row height
    ui->TableView_EntityFilter->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->TableView_EntityFilter->resizeRowsToContents();
    IsInitialized = false;
    // connect CheckBox in TableView to action
    connect(EntityFilterTable, SIGNAL(itemChanged(QStandardItem *)), this,
            SLOT(on_TableView_Checkbox_stateChanged(QStandardItem *)));
    // set model
    ui->TableView_EntityFilter->setModel(EntityFilterTable);

    // Distribute Doors into the temp CurrentRoom
    tmpCurrentRoom->SetDoorsVector(_level->GetRoomDoors(currentroom->GetRoomID()));
    tmpDestinationRoom->SetDoorsVector(
        _level->GetRoomDoors(currentroom->GetDoor(doorID)->GetDestinationDoor()->GetRoomID()));

    // Initialize UI elements
    ui->ComboBox_DoorType->addItems(DoortypeSet);
    LevelComponents::Door *currentdoor = tmpCurrentRoom->GetDoor(doorID);
    ui->ComboBox_DoorType->setCurrentIndex(currentdoor->GetDoorTypeNum() - 1);
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
    doorofLevelSet << "Disable destination door";
    for (unsigned int i = 1; i < _level->GetDoors().size(); ++i)
    {
        doorofLevelSet << _level->GetDoors()[i]->GetDoorName();
    }
    ui->ComboBox_DoorDestinationPicker->addItems(doorofLevelSet);
    ui->ComboBox_DoorDestinationPicker->setCurrentIndex(currentdoor->GetDestinationDoor()->GetGlobalDoorID());
    RenderGraphicsView_Preview();

    // Initialize the EntitySet ComboBox
    for (unsigned int i = 0; i < sizeof(ROMUtils::entitiessets) / sizeof(ROMUtils::entitiessets[0]); ++i)
    {
        comboboxEntitySet.push_back({ (int) i, true });
    }
    UpdateComboBoxEntitySet();

    // Initialize the entity list drop-down
    for (unsigned int i = 0; i < sizeof(ROMUtils::entities) / sizeof(ROMUtils::entities[0]); ++i)
    {
        EntityFilterTable->AddEntity(ROMUtils::entities[i]);
    }
    UpdateTableView();

    // Set the current EntitySet in the ComboBox
    int entitySetID = currentdoor->GetEntitySetID();
    ui->ComboBox_EntitySetID->setCurrentIndex(entitySetID);

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
/// All the changes in the dialog are made on the temp-created room,
/// so the current Door needs to get data from the dialog only when user clicks okay,
/// and this function get called.
/// </summary>
void DoorConfigDialog::UpdateCurrentDoorData()
{
    CurrentRoom->GetDoor(DoorID)->SetDoorType(
        static_cast<LevelComponents::DoorType>(ui->ComboBox_DoorType->currentIndex() + 1));
    CurrentRoom->GetDoor(DoorID)->SetDelta((signed char) ui->SpinBox_WarioX->value(),
                                           (signed char) ui->SpinBox_WarioY->value());
    CurrentRoom->GetDoor(DoorID)->SetDoorPlace(
        (unsigned char) ui->SpinBox_DoorX->value(),
        (unsigned char) (ui->SpinBox_DoorX->value() + ui->SpinBox_DoorWidth->value() - 1),
        (unsigned char) ui->SpinBox_DoorY->value(),
        (unsigned char) (ui->SpinBox_DoorY->value() + ui->SpinBox_DoorHeight->value() - 1));
    CurrentRoom->GetDoor(DoorID)->SetBGM((unsigned short) ui->SpinBox_BGM_ID->value());
    int resetEntitysetId = tmpCurrentRoom->GetDoor(DoorID)->GetEntitySetID();
    if (resetEntitysetId > 0)
    {
        CurrentRoom->GetDoor(DoorID)->SetEntitySetID((unsigned char) resetEntitysetId);
        CurrentRoom->SetCurrentEntitySet(resetEntitysetId);
    }
    int index = ui->ComboBox_DoorDestinationPicker->currentIndex();
    CurrentRoom->GetDoor(DoorID)->SetLinkerDestination(index);
    CurrentRoom->GetDoor(DoorID)->SetDestinationDoor(_currentLevel->GetDoors()[index]);
}

/// <summary>
/// Perform static initialization of constant data structures for the dialog.
/// </summary>
void DoorConfigDialog::StaticInitialization()
{
    // Initialize the selections for the Door type
    for (unsigned int i = 0; i < sizeof(DoortypeSetData) / sizeof(DoortypeSetData[0]); ++i)
    {
        DoortypeSet << DoortypeSetData[i];
    }

    // Initialize the selections for the Entity name
    for (unsigned int i = 0; i < sizeof(EntitynameSetData) / sizeof(EntitynameSetData[0]); ++i)
    {
        EntitynameSet << EntitynameSetData[i];
    }
}

/// <summary>
/// Render Room and Doors in GraphicsView_Preview.
/// </summary>
void DoorConfigDialog::RenderGraphicsView_Preview()
{
    QGraphicsScene *oldScene = ui->GraphicsView_Preview->scene();
    if (oldScene)
    {
        delete oldScene;
    }
    struct LevelComponents::RenderUpdateParams tparam(LevelComponents::FullRender);
    tparam.tilechangelist.clear();
    tparam.SelectedDoorID = (unsigned int) DoorID; // ID in Room
    tparam.mode.editMode = Ui::DoorEditMode;
    tparam.mode.ExtraHintsEnabled = tparam.mode.entitiesEnabled = tparam.mode.cameraAreasEnabled = false;
    tparam.mode.entitiesboxesDisabled = true;
    QGraphicsScene *scene = tmpCurrentRoom->RenderGraphicsScene(ui->GraphicsView_Preview->scene(), &tparam);
    ui->GraphicsView_Preview->setScene(scene);
    ui->GraphicsView_Preview->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // Set scrollbars
    LevelComponents::Door *tmpdoor = tmpCurrentRoom->GetDoor(DoorID);
    door_CurX1 = tmpdoor->GetX1();
    door_CurX2 = tmpdoor->GetX2();
    door_CurY1 = tmpdoor->GetY1();
    door_CurY2 = tmpdoor->GetY2();
    int X_av = (door_CurX1 + door_CurX2) / 2;
    int Y_av = (door_CurY1 + door_CurY2) / 2;
    float X_av_rate = static_cast<float>(X_av) / static_cast<float>(tmpCurrentRoom->GetWidth());
    float Y_av_rate = static_cast<float>(Y_av) / static_cast<float>(tmpCurrentRoom->GetHeight());
    float Heightrate_L1_over_L0 = qMin(1.0f, static_cast<float>(tmpCurrentRoom->GetHeight()) / static_cast<float>(tmpCurrentRoom->GetLayer0Height()));
    float Widthrate_L1_over_L0 = qMin(1.0f, static_cast<float>(tmpCurrentRoom->GetWidth()) / static_cast<float>(tmpCurrentRoom->GetLayer0Width()));
    int V_all = Heightrate_L1_over_L0 * (ui->GraphicsView_Preview->verticalScrollBar()->pageStep() +
            ui->GraphicsView_Preview->verticalScrollBar()->maximum());
    int H_all = Widthrate_L1_over_L0 * (ui->GraphicsView_Preview->horizontalScrollBar()->pageStep() +
            ui->GraphicsView_Preview->horizontalScrollBar()->maximum());
    int X_val = static_cast<int>(H_all * X_av_rate);
    int Y_val = static_cast<int>(V_all * Y_av_rate);
    ui->GraphicsView_Preview->verticalScrollBar()->setSliderPosition(
                qMin(qMax(Y_val, 1) - ui->GraphicsView_Preview->verticalScrollBar()->pageStep() / 2,
                     (int)(Heightrate_L1_over_L0 * (float)ui->GraphicsView_Preview->verticalScrollBar()->maximum())));
    ui->GraphicsView_Preview->horizontalScrollBar()->setSliderPosition(
                qMin(qMax(X_val, 1) - ui->GraphicsView_Preview->horizontalScrollBar()->pageStep() / 2,
                     (int)(Widthrate_L1_over_L0 * (float)ui->GraphicsView_Preview->horizontalScrollBar()->maximum())));
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
    if (oldScene)
    {
        delete oldScene;
    }
    struct LevelComponents::RenderUpdateParams tparam(LevelComponents::FullRender);
    tparam.tilechangelist.clear();
    tparam.SelectedDoorID = (unsigned int) doorIDinRoom; // ID in Room
    tparam.mode.editMode = Ui::DoorEditMode;
    tparam.mode.ExtraHintsEnabled = tparam.mode.entitiesEnabled = tparam.mode.cameraAreasEnabled = false;
    tparam.mode.entitiesboxesDisabled = true;
    QGraphicsScene *scene = tmpDestinationRoom->RenderGraphicsScene(ui->GraphicsView_DestinationDoor->scene(), &tparam);
    ui->GraphicsView_DestinationDoor->setScene(scene);
    ui->GraphicsView_DestinationDoor->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    if(tmpDestinationRoom->GetRoomID() == tmpCurrentRoom->GetRoomID())
    {
        tmpDestinationRoom->GetDoor(DoorID)->SetDoorPlace(static_cast<unsigned char>(ui->SpinBox_DoorX->value()),
                                                          static_cast<unsigned char>((ui->SpinBox_DoorX->value() + ui->SpinBox_DoorWidth->value() - 1)),
                                                          static_cast<unsigned char>(ui->SpinBox_DoorY->value()),
                                                          static_cast<unsigned char>((ui->SpinBox_DoorY->value() + ui->SpinBox_DoorHeight->value() - 1)));
        UpdateDoorLayerGraphicsView_DestinationDoor();
    }

    // Set scrollbars
    LevelComponents::Door *tmpdoor = tmpDestinationRoom->GetDoor(doorIDinRoom);
    door_DesX1 = tmpdoor->GetX1();
    door_DesX2 = tmpdoor->GetX2();
    door_DesY1 = tmpdoor->GetY1();
    door_DesY2 = tmpdoor->GetY2();
    int X_av = (door_DesX1 + door_DesX2) / 2;
    int Y_av = (door_DesY1 + door_DesY2) / 2;
    float X_av_rate = static_cast<float>(X_av) / static_cast<float>(tmpDestinationRoom->GetWidth());
    float Y_av_rate = static_cast<float>(Y_av) / static_cast<float>(tmpDestinationRoom->GetHeight());
    float Heightrate_L1_over_L0 = qMin(1.0f, static_cast<float>(tmpDestinationRoom->GetHeight()) / static_cast<float>(tmpDestinationRoom->GetLayer0Height()));
    float Widthrate_L1_over_L0 = qMin(1.0f, static_cast<float>(tmpDestinationRoom->GetWidth()) / static_cast<float>(tmpDestinationRoom->GetLayer0Width()));
    int V_all = Heightrate_L1_over_L0 * (ui->GraphicsView_DestinationDoor->verticalScrollBar()->pageStep() +
            ui->GraphicsView_DestinationDoor->verticalScrollBar()->maximum());
    int H_all = Widthrate_L1_over_L0 * (ui->GraphicsView_DestinationDoor->horizontalScrollBar()->pageStep() +
            ui->GraphicsView_DestinationDoor->horizontalScrollBar()->maximum());
    int X_val = static_cast<int>(H_all * X_av_rate);
    int Y_val = static_cast<int>(V_all * Y_av_rate);
    ui->GraphicsView_DestinationDoor->verticalScrollBar()->setSliderPosition(
                qMin(qMax(Y_val, 1) - ui->GraphicsView_DestinationDoor->verticalScrollBar()->pageStep() / 2,
                     (int)(Heightrate_L1_over_L0 * (float)ui->GraphicsView_DestinationDoor->verticalScrollBar()->maximum())));
    ui->GraphicsView_DestinationDoor->horizontalScrollBar()->setSliderPosition(
                qMin(qMax(X_val, 1) - ui->GraphicsView_DestinationDoor->horizontalScrollBar()->pageStep() / 2,
                     (int)(Widthrate_L1_over_L0 * (float)ui->GraphicsView_DestinationDoor->horizontalScrollBar()->maximum())));
}

/// <summary>
/// Reset Door Rect according to the value from the door properties SpinBoxes.
/// </summary>
void DoorConfigDialog::ResetDoorRect()
{
    LevelComponents::Door *currentdoor0 = tmpCurrentRoom->GetDoor(DoorID);
    currentdoor0->SetDoorPlace(static_cast<unsigned char>(ui->SpinBox_DoorX->value()),
                               static_cast<unsigned char>((ui->SpinBox_DoorX->value() + ui->SpinBox_DoorWidth->value() - 1)),
                               static_cast<unsigned char>(ui->SpinBox_DoorY->value()),
                               static_cast<unsigned char>((ui->SpinBox_DoorY->value() + ui->SpinBox_DoorHeight->value() - 1)));
    int doorwidth = currentdoor0->GetX2() - currentdoor0->GetX1() + 1;
    int doorheight = currentdoor0->GetY2() - currentdoor0->GetY1() + 1;
    ui->SpinBox_DoorX->setMaximum(tmpCurrentRoom->GetWidth() - doorwidth);
    ui->SpinBox_DoorY->setMaximum(tmpCurrentRoom->GetHeight() - doorheight);
    ui->SpinBox_DoorWidth->setMaximum(tmpCurrentRoom->GetWidth() - currentdoor0->GetX1());
    ui->SpinBox_DoorHeight->setMaximum(tmpCurrentRoom->GetHeight() - currentdoor0->GetY1());
    UpdateDoorLayerGraphicsView_Preview();

    // when DestinationDoor and currentDoor are in the same Room, the DestinationDoor also needs an update.
    if (!ui->ComboBox_DoorDestinationPicker->currentIndex()) return;
    if(tmpDestinationRoom->GetRoomID() == tmpCurrentRoom->GetRoomID())
    {
        tmpDestinationRoom->GetDoor(DoorID)->SetDoorPlace(static_cast<unsigned char>(ui->SpinBox_DoorX->value()),
                                                          static_cast<unsigned char>((ui->SpinBox_DoorX->value() + ui->SpinBox_DoorWidth->value() - 1)),
                                                          static_cast<unsigned char>(ui->SpinBox_DoorY->value()),
                                                          static_cast<unsigned char>((ui->SpinBox_DoorY->value() + ui->SpinBox_DoorHeight->value() - 1)));
        UpdateDoorLayerGraphicsView_DestinationDoor();
    }
}

/// <summary>
/// Update Door layer in GraphicsView_Preview.
/// </summary>
void DoorConfigDialog::UpdateDoorLayerGraphicsView_Preview()
{
    struct LevelComponents::RenderUpdateParams tparam(LevelComponents::ElementsLayersUpdate);
    tparam.tilechangelist.clear();
    tparam.SelectedDoorID = (unsigned int) DoorID; // ID in Room
    tparam.mode.editMode = Ui::DoorEditMode;
    tparam.mode.ExtraHintsEnabled = tparam.mode.entitiesEnabled = tparam.mode.cameraAreasEnabled = false;
    tparam.mode.entitiesboxesDisabled = true;
    tmpCurrentRoom->RenderGraphicsScene(ui->GraphicsView_Preview->scene(), &tparam);
}

/// <summary>
/// Update Door layer in GraphicsView_DestinationDoor.
/// </summary>
void DoorConfigDialog::UpdateDoorLayerGraphicsView_DestinationDoor()
{
    struct LevelComponents::RenderUpdateParams tparam(LevelComponents::ElementsLayersUpdate);
    tparam.tilechangelist.clear();
    tparam.SelectedDoorID = (unsigned int) tmpDestinationRoom->GetLocalDoorID(
        ui->ComboBox_DoorDestinationPicker->currentIndex()); // ID in Room
    tparam.mode.editMode = Ui::DoorEditMode;
    tparam.mode.ExtraHintsEnabled = tparam.mode.entitiesEnabled = tparam.mode.cameraAreasEnabled = false;
    tparam.mode.entitiesboxesDisabled = true;
    tmpDestinationRoom->RenderGraphicsScene(ui->GraphicsView_DestinationDoor->scene(), &tparam);
}

/// <summary>
/// Return the current entity set ID in ComboBox_EntitySetID.
/// </summary>
/// <return>
/// return -1 if there is no entity ID in ComboBox_EntitySetID.
/// </return>
int DoorConfigDialog::GetSelectedComboBoxEntitySetID()
{
    QString str = ui->ComboBox_EntitySetID->currentText();
    bool ok = false;
    int ret = str.toInt(&ok, 16);
    if (!ok)
        ret = -1;

    return ret;
}

/// <sumary>
/// Update items in ComboBox_EntitySetID using comboboxEntitySet
/// <sumary>
void DoorConfigDialog::UpdateComboBoxEntitySet()
{
    QComboBox *model = ui->ComboBox_EntitySetID;
    model->clear();
    for (const auto &item : comboboxEntitySet)
    {
        if (!item.visible)
            continue;
        model->addItem("0x" + QString::number(item.id, 16));
    }
}

/// <sumary>
/// Update items in TableView_EntityFilter using entities
/// <sumary>
void DoorConfigDialog::UpdateTableView()
{
    EntityFilterTableModel *model = static_cast<EntityFilterTableModel *>(ui->TableView_EntityFilter->model());
    model->clear();
    for (const auto &item : model->entities)
    {
        // skip invisible item
        if (!item.visible)
            continue;

        int row = model->rowCount();
        QStandardItem *checkbox = new QStandardItem;
        checkbox->setCheckable(true);
        checkbox->setCheckState(Qt::Unchecked);
        model->setItem(row, 0, checkbox);

        // entity name item
        QStandardItem *entityName = new QStandardItem(item.entityName);
        // QStandardItem *entityName = new QStandardItem(QString::number(row));
        model->setItem(row, 1, entityName);

        // pixmap item
        QStandardItem *imageItem = new QStandardItem;
        imageItem->setData(QVariant(item.entityImage), Qt::DecorationRole);
        model->setItem(row, 2, imageItem);

        // disable edit
        checkbox->setEditable(false);
        entityName->setEditable(false);
        imageItem->setEditable(false);
    }
}

/// <summary>
/// Called when state of checkbox changed.
/// </summary>
/// <param name="item">
/// The checkbox that state changed.
/// </param>
void DoorConfigDialog::on_TableView_Checkbox_stateChanged(QStandardItem *item)
{
    EntityFilterTableModel *model = static_cast<EntityFilterTableModel *>(ui->TableView_EntityFilter->model());
    const TableEntityItem &it = model->entities[item->row()];

    if (item->checkState() == Qt::Checked)
    {
        for (auto &set : comboboxEntitySet)
        {
            if (set.visible && !ROMUtils::entitiessets[set.id]->FindEntity(it.entity->GetEntityGlobalID()))
            {
                set.visible = false;
            }
        }
        UpdateComboBoxEntitySet();
    }
    else if (item->checkState() == Qt::Unchecked)
    {
        // for every set
        // if there are any entity that is checked and
        // the set isn't inside it
        // then the set is still invisible
        for (auto &set : comboboxEntitySet)
        {
            if (set.visible)
                continue;

            bool visible = true;
            for (int i = 0; i < model->rowCount(); ++i)
            {
                // the entity is checked
                bool checked = model->item(i, 0)->checkState();
                if (checked)
                {
                    if (!ROMUtils::entitiessets[set.id]->FindEntity(model->entities[i].entity->GetEntityGlobalID()))
                    {
                        visible = false;
                        break;
                    }
                }
            }
            set.visible = visible;
        }
        UpdateComboBoxEntitySet();
    }
}

/// <summary>
/// Called when current index of ComboBox_DoorDestinationPicker is changed even on its initialization time.
/// </summary>
/// <param name="index">
/// The selected item's index in ComboBox_DoorDestinationPicker.
/// </param>
void DoorConfigDialog::on_ComboBox_DoorDestinationPicker_currentIndexChanged(int index)
{
    delete tmpDestinationRoom;
    tmpDestinationRoom =
        new LevelComponents::Room(_currentLevel->GetRooms()[_currentLevel->GetDoors()[index]->GetRoomID()]);
    tmpDestinationRoom->SetDoorsVector(
        _currentLevel->GetRoomDoors((unsigned int) _currentLevel->GetDoors()[index]->GetRoomID()));
    if (index != 0)
    {
        RenderGraphicsView_DestinationDoor(tmpDestinationRoom->GetLocalDoorID(index));
    }
    else
    {
        QGraphicsScene *oldScene = ui->GraphicsView_DestinationDoor->scene();
        if (oldScene)
        {
            delete oldScene; // UI automatically update
        }
    }
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
    if (!IsInitialized)
        return;
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
    if (!IsInitialized)
        return;
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
    if (!IsInitialized)
        return;
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
    if (!IsInitialized)
        return;
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
    if (!IsInitialized)
        return;
    LevelComponents::Door *currentdoor0 = tmpCurrentRoom->GetDoor(DoorID);

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
    if (!IsInitialized)
        return;
    tmpCurrentRoom->GetDoor(DoorID)->SetDelta((signed char) ui->SpinBox_WarioX->value(),
                                              (signed char) ui->SpinBox_WarioY->value());
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
    if (!IsInitialized)
        return;
    tmpCurrentRoom->GetDoor(DoorID)->SetDelta((signed char) ui->SpinBox_WarioX->value(),
                                              (signed char) ui->SpinBox_WarioY->value());
}

/// <summary>
/// Set a new BGM_ID for the Door if the BGM need a reset.
/// </summary>
/// <param name="arg1">
/// unused.
/// </param>
void DoorConfigDialog::on_SpinBox_BGM_ID_valueChanged(int arg1)
{
    if (!IsInitialized)
        return;
    tmpCurrentRoom->GetDoor(DoorID)->SetBGM((unsigned char) arg1);

    // set bgm name for the label
    if (SettingsUtils::projectSettings::bgmNameList.find(arg1) != SettingsUtils::projectSettings::bgmNameList.end())
    {
        ui->label_bgm_name->setText(SettingsUtils::projectSettings::bgmNameList[arg1]);
    }
    else
    {
        ui->label_bgm_name->setText("no name");
    }
}

/// <summary>
/// Show All the Entities' names in the selected EntitySet.
/// </summary>
/// <param name="index">
/// currentIndex of the ComboBox_EntitySetID.
/// </param>
void DoorConfigDialog::on_ComboBox_EntitySetID_currentIndexChanged(int index)
{
    if (index == -1)
        return;
    int currentEntitySetId = tmpCurrentRoom->GetDoor(DoorID)->GetEntitySetID();
    if (IsInitialized == true)
        currentEntitySetId = ui->ComboBox_EntitySetID->currentText().toInt(nullptr, 16);
    ui->TextEdit_AllTheEntities->clear();
    QVector<LevelComponents::EntitySetinfoTableElement> currentEntityTable =
        ROMUtils::entitiessets[currentEntitySetId]->GetEntityTable();
    for (unsigned int i = 0; i < currentEntityTable.size(); ++i)
    {
        QString currentname = EntitynameSetData[currentEntityTable[i].Global_EntityID];
        ui->TextEdit_AllTheEntities->append(currentname);
    }
    tmpCurrentRoom->GetDoor(DoorID)->SetEntitySetID((unsigned char) currentEntitySetId);
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
EntityFilterTableModel::EntityFilterTableModel(QWidget *_parent) : QStandardItemModel(_parent), parent(_parent)
{
    // nothing
}

/// <summary>
/// Perform cleanup for deconstruction of EntityFilterTableModel.
/// </summary>
EntityFilterTableModel::~EntityFilterTableModel()
{
    foreach (TableEntityItem item, entities)
    {
        // don't delete
        item.entity = NULL;
    }
}

/// <summary>
/// Add an Entity to EntityFilterTableModel.
/// </summary>
/// <param name="entity">
/// The entity to add.
/// </param>
void EntityFilterTableModel::AddEntity(LevelComponents::Entity *entity)
{
    entities.push_back(
        { entity, DoorConfigDialog::EntitynameSetData[entity->GetEntityGlobalID()], entity->Render(), true });
}

/// <summary>
/// Deselect All Entities.
/// </summary>
void DoorConfigDialog::on_pushButton_DeselectAll_clicked()
{
    EntityFilterTableModel *model = static_cast<EntityFilterTableModel *>(ui->TableView_EntityFilter->model());
    for (int i = 0; i < model->rowCount(); ++i)
    {
        if (model->item(i, 0)->checkState() == Qt::Checked)
        {
            model->item(i, 0)->setCheckState(Qt::Unchecked);
        }
    }
}
