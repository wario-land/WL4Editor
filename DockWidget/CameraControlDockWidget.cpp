#include "CameraControlDockWidget.h"
#include "ui_CameraControlDockWidget.h"

#include "WL4EditorWindow.h"
extern WL4EditorWindow *singleton;

// constexpr declarations for the initializers in the header
constexpr const char *CameraControlDockWidget::CameraLimitatorResetSideTypeNameData[5];

// static variables used by CameraControlDockWidget
static QStringList CameraLimitatorTypeNameSet;

/// <summary>
/// Constructor function of the CameraControlDockWidget.
/// </summary>
CameraControlDockWidget::CameraControlDockWidget(QWidget *parent) :
        QDockWidget(parent), ui(new Ui::CameraControlDockWidget)
{
    // Set UI
    ui->setupUi(this);
    ui->CameraLimitatorTypePicker_comboBox->addItems(CameraLimitatorTypeNameSet);
}

/// <summary>
/// Deconstructor function of the CameraControlDockWidget.
/// </summary>
CameraControlDockWidget::~CameraControlDockWidget() { delete ui; }

/// <summary>
/// Reset Camera Control Infomation in the Dock Widget.
/// </summary>
/// <param name="currentroom">
/// The ptr of the current Room instance, don't delete it in this class.
/// </param>
void CameraControlDockWidget::SetCameraControlInfo(LevelComponents::Room *currentroom)
{
    currentRoom = currentroom;
    CurrentRoomWidth = (int) currentroom->GetWidth();
    CurrentRoomHeight = (int) currentroom->GetHeight();
    ClearListView();
    if (ListViewItemModel)
    {
        ListViewItemModel->removeRows(0, ListViewItemModel->rowCount());
        delete ListViewItemModel;
        ListViewItemModel = nullptr;
    }
    SelectedLimitator = -1;
    ClearCurrentLimitatorSetting();
    ui->LimitatorSetting_groupBox->setEnabled(false);
    enum LevelComponents::__CameraControlType currentcameracontroltype = currentroom->GetCameraControlType();

    // Enable/disable the appropriate radio button for the room's camera type
    if (currentcameracontroltype == LevelComponents::__CameraControlType::FixedY)
    {
        ui->CameraYFixed_radioButton->setChecked(true);
    }
    else if (currentcameracontroltype == LevelComponents::__CameraControlType::NoLimit)
    {
        ui->FollowWario_radioButton->setChecked(true);
    }
    else if (currentcameracontroltype == LevelComponents::__CameraControlType::HasControlAttrs)
    {
        ui->UseCameraLimitators_radioButton->setChecked(true);
    }
    else if (currentcameracontroltype == LevelComponents::__CameraControlType::Vertical_Seperated)
    {
        ui->VerticalSeperate_radioButton->setChecked(true);
    }

    // Enable/disable the existing limitators groupbox depending on the room's camera type
    if (currentcameracontroltype != LevelComponents::__CameraControlType::HasControlAttrs)
    {
        ui->ExistingLimitators_groupBox->setEnabled(false);
    }
    else
    {
        ui->ExistingLimitators_groupBox->setEnabled(true);
        PaintListView();
    }
}

/// <summary>
/// Perform static initialization of constant data structures for the dock widget.
/// </summary>
void CameraControlDockWidget::StaticInitialization()
{
    // Initialize the selections for the CameraLimitatorType ComboBox
    for (unsigned int i = 0;
         i < sizeof(CameraLimitatorResetSideTypeNameData) / sizeof(CameraLimitatorResetSideTypeNameData[0]); ++i)
    {
        CameraLimitatorTypeNameSet << CameraLimitatorResetSideTypeNameData[i];
    }
}

/// <summary>
/// Clear all the information inside LimitatorSetting_groupBox.
/// </summary>
void CameraControlDockWidget::ClearCurrentLimitatorSetting()
{
    IsSavingData = false;
    ui->CameraLimitatorTypePicker_comboBox->setCurrentIndex(0);
    ui->spinBox_x1->setValue(2);
    ui->spinBox_y1->setValue(2);
    ui->spinBox_width->setValue(1);
    ui->spinBox_height->setValue(1);
    ui->LimitatorSideOffset_spinBox->setValue(0);
    ui->TriggerBlockPositionX_spinBox->setValue(0);
    ui->TriggerBlockPositionY_spinBox->setValue(0);
    IsSavingData = true;
}

/// <summary>
/// Set Current Limitator and Update graphicview in the MainWindow.
/// </summary>
void CameraControlDockWidget::SetCurrentLimitator()
{
    if (SelectedLimitator > -1)
    {
        // Convey current_limitator_data the CurrentRoom
        ui->TriggerBlockPositionX_spinBox->setMaximum(CurrentRoomWidth - 3);
        ui->TriggerBlockPositionY_spinBox->setMaximum(CurrentRoomHeight - 3);
        LevelComponents::__CameraControlRecord current_limitator_data;
        current_limitator_data.TransboundaryControl = (unsigned char) 2;
        current_limitator_data.x1 = (unsigned char) ui->spinBox_x1->value();
        ui->spinBox_width->setMaximum(CurrentRoomWidth - ui->spinBox_x1->value() - 2);
        current_limitator_data.y1 = (unsigned char) ui->spinBox_y1->value();
        ui->spinBox_height->setMaximum(CurrentRoomHeight - ui->spinBox_y1->value() - 2);
        current_limitator_data.x2 = (unsigned char) (ui->spinBox_x1->value() + ui->spinBox_width->value() - 1);
        ui->spinBox_x1->setMaximum(CurrentRoomWidth - 2 - 15);
        current_limitator_data.y2 = (unsigned char) (ui->spinBox_y1->value() + ui->spinBox_height->value() - 1);
        ui->spinBox_y1->setMaximum(CurrentRoomHeight - 2 - 10);
        int limitator_type = ui->CameraLimitatorTypePicker_comboBox->currentIndex() - 1;
        if (limitator_type >= 0)
        {
            current_limitator_data.ChangeValueOffset = (unsigned char) limitator_type;
            switch (limitator_type)
            {
            case 0: // Reset left side
                current_limitator_data.ChangedValue =
                    (unsigned char) (ui->spinBox_x1->value() + ui->LimitatorSideOffset_spinBox->value());
                ui->LimitatorSideOffset_spinBox->setMinimum(2 - ui->spinBox_x1->value());
                ui->LimitatorSideOffset_spinBox->setMaximum(ui->spinBox_width->value() - 15);
                break;
            case 1: // Reset right side
                current_limitator_data.ChangedValue =
                    (unsigned char) (ui->spinBox_x1->value() + ui->spinBox_width->value() - 1 +
                                     ui->LimitatorSideOffset_spinBox->value());
                ui->LimitatorSideOffset_spinBox->setMinimum(15 - ui->spinBox_width->value());
                ui->LimitatorSideOffset_spinBox->setMaximum(CurrentRoomWidth - ui->spinBox_x1->value() -
                                                            ui->spinBox_width->value() - 2);
                break;
            case 2: // Reset upper side
                current_limitator_data.ChangedValue =
                    (unsigned char) (ui->spinBox_y1->value() + ui->LimitatorSideOffset_spinBox->value());
                ui->LimitatorSideOffset_spinBox->setMinimum(2 - ui->spinBox_y1->value());
                ui->LimitatorSideOffset_spinBox->setMaximum(ui->spinBox_height->value() - 10);
                break;
            case 3: // Reset lower side
                current_limitator_data.ChangedValue =
                    (unsigned char) (ui->spinBox_y1->value() + ui->spinBox_height->value() - 1 +
                                     ui->LimitatorSideOffset_spinBox->value());
                ui->LimitatorSideOffset_spinBox->setMinimum(10 - ui->spinBox_height->value());
                ui->LimitatorSideOffset_spinBox->setMaximum(CurrentRoomHeight - ui->spinBox_y1->value() -
                                                            ui->spinBox_height->value() - 2);
            }
            current_limitator_data.x3 = (unsigned char) ui->TriggerBlockPositionX_spinBox->value();
            current_limitator_data.y3 = (unsigned char) ui->TriggerBlockPositionY_spinBox->value();
        }
        else
        {
            current_limitator_data.ChangeValueOffset = current_limitator_data.ChangedValue = current_limitator_data.x3 =
                current_limitator_data.y3 = (unsigned char) 0xFF;
        }
        currentRoom->SetCameraLimitator(SelectedLimitator, current_limitator_data);

        // Rerender graphicview in MainWindow
        singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
    }
}

/// <summary>
/// Set Item test in Listview.
/// </summary>
/// <param name="row">
/// item index in current listview.
/// </param>
void CameraControlDockWidget::SetListviewItemText(int row)
{
    QString string = "(" + QString::number(ui->spinBox_x1->value()) + ", " + QString::number(ui->spinBox_y1->value()) +
                     ") - (" + QString::number(ui->spinBox_x1->value() + ui->spinBox_width->value() - 1) + ", " +
                     QString::number(ui->spinBox_y1->value() + ui->spinBox_height->value() - 1) + ")";
    ListViewItemModel->item(row, 0)->setText(string);
}

/// <summary>
/// Get limitator information from the current room and list all of them into the Listview.
/// </summary>
void CameraControlDockWidget::PaintListView()
{
    std::vector<struct LevelComponents::__CameraControlRecord *> currentCameraLimitators =
        currentRoom->GetCameraControlRecords();
    ClearListView();
    ListViewItemModel = new QStandardItemModel(this);
    QStringList List_strs;
    if (currentCameraLimitators.size() > (unsigned int) 0)
    {
        for (int i = 0; i < (int) currentCameraLimitators.size(); ++i)
        {
            QString string = "(" + QString::number((int) currentCameraLimitators[i]->x1) + ", " +
                             QString::number((int) currentCameraLimitators[i]->y1) + ") - (" +
                             QString::number((int) currentCameraLimitators[i]->x2) + ", " +
                             QString::number((int) currentCameraLimitators[i]->y2) + ")";
            List_strs << string;
        }
        int nCount = List_strs.size();

        for (int i = 0; i < nCount; i++)
        {
            QString string = static_cast<QString>(List_strs.at(i));
            QStandardItem *item = new QStandardItem(string);
            ListViewItemModel->appendRow(item);
        }
    }
    ui->CameraLimitators_listView->setModel(ListViewItemModel);
}

/// <summary>
/// Clear Listview.
/// </summary>
void CameraControlDockWidget::ClearListView()
{
    if (ListViewItemModel)
    {
        ListViewItemModel->clear();
        delete ListViewItemModel;
        ListViewItemModel = nullptr;
    }
}

/// <summary>
/// Be called the listview is clicked and a limitator is selected.
/// </summary>
/// <param name="index">
/// Reference of the selected QModelIndex from the listview.
/// </param>
void CameraControlDockWidget::on_CameraLimitators_listView_clicked(const QModelIndex &index)
{
    IsSavingData = false;
    ui->LimitatorSetting_groupBox->setEnabled(false);
    std::vector<struct LevelComponents::__CameraControlRecord *> currentCameraLimitators =
        currentRoom->GetCameraControlRecords();
    int linenum = index.row();
    SelectedLimitator = linenum;
    LevelComponents::__CameraControlRecord *currentLimitator = currentCameraLimitators[linenum];
    int currentLimitatorTypeid =
        (currentLimitator->ChangeValueOffset == 0xFF ? -1 : currentLimitator->ChangeValueOffset);
    ui->CameraLimitatorTypePicker_comboBox->setCurrentIndex(currentLimitatorTypeid + 1);
    ui->spinBox_x1->setValue(currentLimitator->x1);
    ui->spinBox_y1->setValue(currentLimitator->y1);
    ui->spinBox_width->setValue(currentLimitator->x2 - currentLimitator->x1 + 1);
    ui->spinBox_height->setValue(currentLimitator->y2 - currentLimitator->y1 + 1);
    if ((currentLimitatorTypeid > -1) && currentLimitatorTypeid != 0xFF)
    {
        ui->LimitatorSideOffset_spinBox->setValue(
            currentLimitator->ChangedValue - *(((unsigned char *) &(currentLimitator->x1)) + currentLimitatorTypeid));
        ui->TriggerBlockPositionX_spinBox->setValue(currentLimitator->x3);
        ui->TriggerBlockPositionY_spinBox->setValue(currentLimitator->y3);
        ui->LimitatorSideOffset_spinBox->setEnabled(true);
        ui->TriggerBlockPositionX_spinBox->setEnabled(true);
        ui->TriggerBlockPositionY_spinBox->setEnabled(true);
    }
    else
    {
        ui->LimitatorSideOffset_spinBox->setValue(0);
        ui->TriggerBlockPositionX_spinBox->setValue(0);
        ui->TriggerBlockPositionY_spinBox->setValue(0);
        ui->LimitatorSideOffset_spinBox->setEnabled(false);
        ui->TriggerBlockPositionX_spinBox->setEnabled(false);
        ui->TriggerBlockPositionY_spinBox->setEnabled(false);
    }
    SetCurrentLimitator(); // only used to set maximums for all the spinboxes
    ui->LimitatorSetting_groupBox->setEnabled(true);
    IsSavingData = true;
}

/// <summary>
/// Called when current camera limitator type is changed.
/// </summary>
/// <param name="index">
/// Index of the current selected item in CameraLimitatorTypePicker_comboBox.
/// </param>
void CameraControlDockWidget::on_CameraLimitatorTypePicker_comboBox_currentIndexChanged(int index)
{
    if (!IsSavingData)
        return;
    ui->LimitatorSideOffset_spinBox->setValue(0);
    ui->TriggerBlockPositionX_spinBox->setValue(0);
    ui->TriggerBlockPositionY_spinBox->setValue(0);
    if (index == 0)
    {
        ui->LimitatorSideOffset_spinBox->setEnabled(false);
        ui->TriggerBlockPositionX_spinBox->setEnabled(false);
        ui->TriggerBlockPositionY_spinBox->setEnabled(false);
    }
    else
    {
        ui->LimitatorSideOffset_spinBox->setEnabled(true);
        ui->TriggerBlockPositionX_spinBox->setEnabled(true);
        ui->TriggerBlockPositionY_spinBox->setEnabled(true);
    }
    SetCurrentLimitator();

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Called when setting the x1 value of the current camera limitator.
/// </summary>
/// <param name="arg1">
/// Spinbox value of the widget.
/// </param>
void CameraControlDockWidget::on_spinBox_x1_valueChanged(int arg1)
{
    (void) arg1;
    if (!IsSavingData)
        return;
    SetListviewItemText(SelectedLimitator);
    SetCurrentLimitator();
    // TODO: Reset the text in the listview

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Called when setting the y1 value of the current camera limitator.
/// </summary>
/// <param name="arg1">
/// Spinbox value of the widget.
/// </param>
void CameraControlDockWidget::on_spinBox_y1_valueChanged(int arg1)
{
    (void) arg1;
    if (!IsSavingData)
        return;
    SetListviewItemText(SelectedLimitator);
    SetCurrentLimitator();
    // TODO: Reset the text in the listview

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Called when setting the width of the current camera limitator.
/// </summary>
/// <param name="arg1">
/// Spinbox value of the widget.
/// </param>
void CameraControlDockWidget::on_spinBox_width_valueChanged(int arg1)
{
    (void) arg1;
    if (!IsSavingData)
        return;
    SetListviewItemText(SelectedLimitator);
    SetCurrentLimitator();
    // TODO: Reset the text in the listview

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Called when setting the height of the current camera limitator.
/// </summary>
/// <param name="arg1">
/// Spinbox value of the widget.
/// </param>
void CameraControlDockWidget::on_spinBox_height_valueChanged(int arg1)
{
    (void) arg1;
    if (!IsSavingData)
        return;
    SetListviewItemText(SelectedLimitator);
    SetCurrentLimitator();
    // TODO: Reset the text in the listview

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Called when setting the sideoffset of the current camera limitator.
/// </summary>
/// <param name="arg1">
/// Spinbox value of the widget.
/// </param>
void CameraControlDockWidget::on_LimitatorSideOffset_spinBox_valueChanged(int arg1)
{
    (void) arg1;
    if (!IsSavingData)
        return;
    SetCurrentLimitator();

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Called when setting the TriggerBlockPositionX of the current camera limitator.
/// </summary>
/// <param name="arg1">
/// Spinbox value of the widget.
/// </param>
void CameraControlDockWidget::on_TriggerBlockPositionX_spinBox_valueChanged(int arg1)
{
    (void) arg1;
    if (!IsSavingData)
        return;
    SetCurrentLimitator();

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Called when setting the TriggerBlockPositionY of the current camera limitator.
/// </summary>
/// <param name="arg1">
/// Spinbox value of the widget.
/// </param>
void CameraControlDockWidget::on_TriggerBlockPositionY_spinBox_valueChanged(int arg1)
{
    (void) arg1;
    if (!IsSavingData)
        return;
    SetCurrentLimitator();

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Called when CameraYFixed_radioButton is clicked.
/// </summary>
/// <param name="checked">
/// Rerender current Room if CameraYFixed_radioButton is been checked.
/// </param>
void CameraControlDockWidget::on_CameraYFixed_radioButton_clicked(bool checked)
{
    if (checked)
    {
        singleton->GetCurrentRoom()->SetCameraControlType(LevelComponents::__CameraControlType::FixedY);
        SelectedLimitator = -1;
        ClearCurrentLimitatorSetting();
        ClearListView();
        ui->ExistingLimitators_groupBox->setEnabled(false);
        ui->LimitatorSetting_groupBox->setEnabled(false);
        // Rerender graphicview in MainWindow
        singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
    }

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Called when FollowWario_radioButton is clicked.
/// </summary>
/// <param name="checked">
/// Rerender current Room if FollowWario_radioButton is been checked.
/// </param>
void CameraControlDockWidget::on_FollowWario_radioButton_clicked(bool checked)
{
    if (checked)
    {
        singleton->GetCurrentRoom()->SetCameraControlType(LevelComponents::__CameraControlType::NoLimit);
        SelectedLimitator = -1;
        ClearCurrentLimitatorSetting();
        ClearListView();
        ui->ExistingLimitators_groupBox->setEnabled(false);
        ui->LimitatorSetting_groupBox->setEnabled(false);
        // Rerender graphicview in MainWindow
        singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
    }

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Called when UseCameraLimitators_radioButton is clicked.
/// </summary>
/// <param name="checked">
/// Rerender current Room if UseCameraLimitators_radioButton is been checked.
/// </param>
void CameraControlDockWidget::on_UseCameraLimitators_radioButton_clicked(bool checked)
{
    if (checked)
    {
        singleton->GetCurrentRoom()->SetCameraControlType(LevelComponents::__CameraControlType::HasControlAttrs);
        SelectedLimitator = -1;
        ClearCurrentLimitatorSetting();
        PaintListView();
        ui->ExistingLimitators_groupBox->setEnabled(true);
        ui->LimitatorSetting_groupBox->setEnabled(false);
        // Rerender graphicview in MainWindow
        singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
    }

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Called when AddCameraLimitator_pushButton is clicked.
/// </summary>
void CameraControlDockWidget::on_AddCameraLimitator_pushButton_clicked()
{
    currentRoom->AddCameraLimitator();
    singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
    PaintListView();

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Called when DeleteCameraLimitator_pushButton is clicked.
/// </summary>
void CameraControlDockWidget::on_DeleteCameraLimitator_pushButton_clicked()
{
    if (SelectedLimitator == -1)
        return;
    ui->LimitatorSetting_groupBox->setEnabled(false);
    currentRoom->DeleteCameraLimitator(SelectedLimitator);
    SelectedLimitator = -1;
    ClearCurrentLimitatorSetting();
    singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
    PaintListView();

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}

/// <summary>
/// Called when VerticalSeperate_radioButton is clicked.
/// </summary>
/// <param name="checked">
/// Rerender current Room if VerticalSeperate_radioButton is been checked.
/// </param>
void CameraControlDockWidget::on_VerticalSeperate_radioButton_clicked(bool checked)
{
    if (checked)
    {
        singleton->GetCurrentRoom()->SetCameraControlType(LevelComponents::__CameraControlType::Vertical_Seperated);
        SelectedLimitator = -1;
        ClearCurrentLimitatorSetting();
        ClearListView();
        ui->ExistingLimitators_groupBox->setEnabled(false);
        ui->LimitatorSetting_groupBox->setEnabled(false);
        // Rerender graphicview in MainWindow
        singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
    }

    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    singleton->SetUnsavedChanges(true);
}
