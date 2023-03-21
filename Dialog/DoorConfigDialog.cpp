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
DoorConfigDialog::DoorConfigDialog(QWidget *parent, LevelComponents::Room *currentroom, int localDoorID,
                                   LevelComponents::Level *_level) :
        QDialog(parent),
        ui(new Ui::DoorConfigDialog), _currentLevel(_level),
        tmpDoorVec(_level->GetDoorList()),
        tmpCurrentRoom(new LevelComponents::Room(currentroom)),
        tmpDestinationRoom(new LevelComponents::Room(
            _level->GetRooms()[tmpDoorVec.GetDestinationDoor(currentroom->GetRoomID(), localDoorID).RoomID])),
        LocalDoorID(localDoorID)
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

    // Initialize UI elements
    ui->ComboBox_DoorType->addItems(DoortypeSet);
    int roomId = tmpCurrentRoom->GetRoomID();
    LevelComponents::DoorEntry curDoorData = tmpDoorVec.GetDoor(roomId, localDoorID);
    ui->label_CurrentDoorGlobalID->setText("0x" + QString::number(tmpDoorVec.GetGlobalIDByLocalID(roomId, localDoorID), 16));
    ui->ComboBox_DoorType->setCurrentIndex(curDoorData.DoorTypeByte - 1);
    ui->SpinBox_DoorX->setValue(curDoorData.x1);
    ui->SpinBox_DoorY->setValue(curDoorData.y1);
    int doorwidth = curDoorData.x2 - curDoorData.x1 + 1;
    int doorheight = curDoorData.y2 - curDoorData.y1 + 1;
    ui->SpinBox_DoorWidth->setValue(doorwidth);
    ui->SpinBox_DoorHeight->setValue(doorheight);
    ui->SpinBox_DoorWidth->setMaximum(tmpCurrentRoom->GetLayer1Width() - curDoorData.x1);
    ui->SpinBox_DoorHeight->setMaximum(tmpCurrentRoom->GetLayer1Height() - curDoorData.y1);
    ui->SpinBox_DoorX->setMaximum(tmpCurrentRoom->GetLayer1Width() - doorwidth);
    ui->SpinBox_DoorY->setMaximum(tmpCurrentRoom->GetLayer1Height() - doorheight);
    ui->SpinBox_WarioX->setValue(curDoorData.HorizontalDeltaWario);
    ui->SpinBox_WarioY->setValue(curDoorData.VerticalDeltaWario);
    ui->SpinBox_BGM_ID->setValue(curDoorData.BGM_ID);

    // Initialize the selections for destination door combobox
    QStringList doorofLevelSet;
    doorofLevelSet << "Disable destination door";
    LevelComponents::LevelDoorVector &doorvec = _level->GetDoorListRef();
    if (int doorsize = doorvec.size(); doorsize > 1)
    {
        for (unsigned int i = 1; i < doorsize; ++i)
        {
            doorofLevelSet << doorvec.GetDoorName(i);
        }
    }
    ui->ComboBox_DoorDestinationPicker->addItems(doorofLevelSet);
    ui->ComboBox_DoorDestinationPicker->setCurrentIndex(curDoorData.DestinationDoorGlobalID);
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
    ui->ComboBox_EntitySetID->setCurrentIndex(curDoorData.EntitySetID);

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
/// return the tmpDoorVec and let the Level instance get it to complete the DoorVector editing
/// </summary>
LevelComponents::LevelDoorVector &DoorConfigDialog::GetChangedDoorVectorResult()
{
    return tmpDoorVec;
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
    tparam.SelectedDoorID = (unsigned int) LocalDoorID; // ID in Room
    tparam.mode.editMode = Ui::DoorEditMode;
    tparam.mode.ExtraHintsEnabled = tparam.mode.entitiesEnabled = tparam.mode.cameraAreasEnabled = false;
    tparam.mode.entitiesboxesDisabled = true;
    tparam.localDoors = tmpDoorVec.GetDoorsByRoomID(tmpCurrentRoom->GetRoomID());
    QGraphicsScene *scene = tmpCurrentRoom->RenderGraphicsScene(ui->GraphicsView_Preview->scene(), &tparam);
    ui->GraphicsView_Preview->setScene(scene);
    ui->GraphicsView_Preview->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // Set scrollbars magic
    LevelComponents::DoorEntry curDoorData = tmpDoorVec.GetDoor(tmpCurrentRoom->GetRoomID(), LocalDoorID);
    int door_CurX1 = curDoorData.x1;
    int door_CurX2 = curDoorData.x2;
    int door_CurY1 = curDoorData.y1;
    int door_CurY2 = curDoorData.y2;
    int X_av = (door_CurX1 + door_CurX2) / 2;
    int Y_av = (door_CurY1 + door_CurY2) / 2;
    float X_av_rate = static_cast<float>(X_av) / static_cast<float>(tmpCurrentRoom->GetLayer1Width());
    float Y_av_rate = static_cast<float>(Y_av) / static_cast<float>(tmpCurrentRoom->GetLayer1Height());
    float Heightrate_L1_over_L0 = qMin(1.0f, static_cast<float>(tmpCurrentRoom->GetLayer1Height()) / static_cast<float>(tmpCurrentRoom->GetLayer0Height()));
    float Widthrate_L1_over_L0 = qMin(1.0f, static_cast<float>(tmpCurrentRoom->GetLayer1Width()) / static_cast<float>(tmpCurrentRoom->GetLayer0Width()));
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
    tparam.localDoors = tmpDoorVec.GetDoorsByRoomID(tmpDestinationRoom->GetRoomID());
    QGraphicsScene *scene = tmpDestinationRoom->RenderGraphicsScene(ui->GraphicsView_DestinationDoor->scene(), &tparam);
    ui->GraphicsView_DestinationDoor->setScene(scene);
    ui->GraphicsView_DestinationDoor->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // Set scrollbars
    LevelComponents::DoorEntry destDoorData = tmpDoorVec.GetDoor(tmpDestinationRoom->GetRoomID(), doorIDinRoom);
    int door_DesX1 = destDoorData.x1;
    int door_DesX2 = destDoorData.x2;
    int door_DesY1 = destDoorData.y1;
    int door_DesY2 = destDoorData.y2;
    int X_av = (door_DesX1 + door_DesX2) / 2;
    int Y_av = (door_DesY1 + door_DesY2) / 2;
    float X_av_rate = static_cast<float>(X_av) / static_cast<float>(tmpDestinationRoom->GetLayer1Width());
    float Y_av_rate = static_cast<float>(Y_av) / static_cast<float>(tmpDestinationRoom->GetLayer1Height());
    float Heightrate_L1_over_L0 = qMin(1.0f, static_cast<float>(tmpDestinationRoom->GetLayer1Height()) / static_cast<float>(tmpDestinationRoom->GetLayer0Height()));
    float Widthrate_L1_over_L0 = qMin(1.0f, static_cast<float>(tmpDestinationRoom->GetLayer1Width()) / static_cast<float>(tmpDestinationRoom->GetLayer0Width()));
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
    tmpDoorVec.SetDoorPlace(tmpDoorVec.GetGlobalIDByLocalID(tmpCurrentRoom->GetRoomID(), LocalDoorID),
                static_cast<unsigned char>(ui->SpinBox_DoorX->value()),
                static_cast<unsigned char>((ui->SpinBox_DoorX->value() + ui->SpinBox_DoorWidth->value() - 1)),
                static_cast<unsigned char>(ui->SpinBox_DoorY->value()),
                static_cast<unsigned char>((ui->SpinBox_DoorY->value() + ui->SpinBox_DoorHeight->value() - 1)));
    LevelComponents::DoorEntry curDoorData = tmpDoorVec.GetDoor(tmpCurrentRoom->GetRoomID(), LocalDoorID);
    int doorwidth = curDoorData.x2 - curDoorData.x1 + 1;
    int doorheight = curDoorData.y2 - curDoorData.y1 + 1;
    ui->SpinBox_DoorX->setMaximum(tmpCurrentRoom->GetLayer1Width() - doorwidth);
    ui->SpinBox_DoorY->setMaximum(tmpCurrentRoom->GetLayer1Height() - doorheight);
    ui->SpinBox_DoorWidth->setMaximum(tmpCurrentRoom->GetLayer1Width() - curDoorData.x1);
    ui->SpinBox_DoorHeight->setMaximum(tmpCurrentRoom->GetLayer1Height() - curDoorData.y1);
    UpdateDoorLayerGraphicsView_Preview();

    // when DestinationDoor and currentDoor are in the same Room, the DestinationDoor graphicview also needs an update.
    if (!ui->ComboBox_DoorDestinationPicker->currentIndex()) return;
    if(tmpDestinationRoom->GetRoomID() == tmpCurrentRoom->GetRoomID())
    {
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
    tparam.SelectedDoorID = (unsigned int) LocalDoorID; // ID in Room
    tparam.mode.editMode = Ui::DoorEditMode;
    tparam.mode.ExtraHintsEnabled = tparam.mode.entitiesEnabled = tparam.mode.cameraAreasEnabled = false;
    tparam.mode.entitiesboxesDisabled = true;
    tparam.localDoors = tmpDoorVec.GetDoorsByRoomID(tmpCurrentRoom->GetRoomID());
    tmpCurrentRoom->RenderGraphicsScene(ui->GraphicsView_Preview->scene(), &tparam);
}

/// <summary>
/// Update Door layer in GraphicsView_DestinationDoor.
/// </summary>
void DoorConfigDialog::UpdateDoorLayerGraphicsView_DestinationDoor()
{
    struct LevelComponents::RenderUpdateParams tparam(LevelComponents::ElementsLayersUpdate);
    tparam.tilechangelist.clear();
    int destDoorGlobalId = ui->ComboBox_DoorDestinationPicker->currentIndex();
    tparam.SelectedDoorID = tmpDoorVec.GetLocalIDByGlobalID(destDoorGlobalId); // ID in Room
    tparam.mode.editMode = Ui::DoorEditMode;
    tparam.mode.ExtraHintsEnabled = tparam.mode.entitiesEnabled = tparam.mode.cameraAreasEnabled = false;
    tparam.mode.entitiesboxesDisabled = true;
    tparam.localDoors = tmpDoorVec.GetDoorsByRoomID(tmpDestinationRoom->GetRoomID());
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
        new LevelComponents::Room(_currentLevel->GetRooms()[tmpDoorVec.GetDoor(index).RoomID]);
    if (index != 0)
    {
        RenderGraphicsView_DestinationDoor(tmpDoorVec.GetLocalIDByGlobalID(index));
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
    tmpDoorVec.SetDoorType(tmpDoorVec.GetGlobalIDByLocalID(tmpCurrentRoom->GetRoomID(), LocalDoorID), index + 1);
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
    int globalDoorId = tmpDoorVec.GetGlobalIDByLocalID(tmpCurrentRoom->GetRoomID(), LocalDoorID);
    tmpDoorVec.SetWarioDelta(globalDoorId, (signed char) ui->SpinBox_WarioX->value(), (signed char) ui->SpinBox_WarioY->value());
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
    int globalDoorId = tmpDoorVec.GetGlobalIDByLocalID(tmpCurrentRoom->GetRoomID(), LocalDoorID);
    tmpDoorVec.SetWarioDelta(globalDoorId, (signed char) ui->SpinBox_WarioX->value(), (signed char) ui->SpinBox_WarioY->value());
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
    int globalDoorId = tmpDoorVec.GetGlobalIDByLocalID(tmpCurrentRoom->GetRoomID(), LocalDoorID);
    tmpDoorVec.SetBGM(globalDoorId, arg1);

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
    int globalDoorId = tmpDoorVec.GetGlobalIDByLocalID(tmpCurrentRoom->GetRoomID(), LocalDoorID);
    int currentEntitySetId = tmpDoorVec.GetDoor(globalDoorId).EntitySetID;
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
    tmpDoorVec.SetEntitySetID(globalDoorId, currentEntitySetId);
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
