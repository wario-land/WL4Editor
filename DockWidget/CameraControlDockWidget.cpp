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
    QDockWidget(parent),
    ui(new Ui::CameraControlDockWidget)
{
    // Set UI
    ui->setupUi(this);
    ui->CameraLimitatorTypePicker_comboBox->addItems(CameraLimitatorTypeNameSet);
}

/// <summary>
/// Deconstructor function of the CameraControlDockWidget.
/// </summary>
CameraControlDockWidget::~CameraControlDockWidget()
{
    delete ui;
}

/// <summary>
/// Reset Camera Control Infomation in the Dock Widget.
/// </summary>
/// <param name="currentroom">
/// The ptr of the current Room instance, don't delete it in this class.
/// </param>
void CameraControlDockWidget::SetCameraControlInfo(LevelComponents::Room *currentroom)
{
    currentRoom = currentroom;
    ClearListView();
    if(ListViewItemModel != nullptr)
    {
        ListViewItemModel->removeRows(0,ListViewItemModel->rowCount());
        delete ListViewItemModel;
        ListViewItemModel = nullptr;
    }
    SelectedLimitator = -1;
    ClearCurrentLimitatorSetting();
    ui->LimitatorSetting_groupBox->setEnabled(false);
    enum LevelComponents::__CameraControlType currentcameracontroltype = currentroom->GetCameraControlType();
    if(currentcameracontroltype == LevelComponents::__CameraControlType::FixedY)
    {
        ui->CameraYFixed_radioButton->setChecked(true);
    }
    else if(currentcameracontroltype == LevelComponents::__CameraControlType::NoLimit)
    {
        ui->FollowWario_radioButton->setChecked(true);
    }
    else if(currentcameracontroltype == LevelComponents::__CameraControlType::HasControlAttrs)
    {
        ui->UseCameraLimitators_radioButton->setChecked(true);
    }

    if(currentcameracontroltype != LevelComponents::__CameraControlType::HasControlAttrs)
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
    for(unsigned int i = 0; i < sizeof(CameraLimitatorResetSideTypeNameData)/sizeof(CameraLimitatorResetSideTypeNameData[0]); ++i)
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
    if(SelectedLimitator > -1)
    {
        // Convey current_limitator_data the CurrentRoom
        LevelComponents::__CameraControlRecord current_limitator_data;
        current_limitator_data.TransboundaryControl = (unsigned char) 2;
        current_limitator_data.x1 = (unsigned char) ui->spinBox_x1->value();
        current_limitator_data.y1 = (unsigned char) ui->spinBox_y1->value();
        current_limitator_data.x2 = (unsigned char) (ui->spinBox_x1->value() + ui->spinBox_width->value() - 1);
        current_limitator_data.y2 = (unsigned char) (ui->spinBox_y1->value() + ui->spinBox_height->value() - 1);
        int limitator_type = ui->CameraLimitatorTypePicker_comboBox->currentIndex() - 1;
        if(limitator_type > 0)
        {
            current_limitator_data.ChangeValueOffset = (unsigned char) limitator_type;
            switch(limitator_type)
            {
            case 0: // Reset left side
                current_limitator_data.ChangedValue = (unsigned char) (ui->spinBox_x1->value() + ui->LimitatorSideOffset_spinBox->value());
                break;
            case 1: // Reset right side
                current_limitator_data.ChangedValue = (unsigned char) (ui->spinBox_x1->value() + ui->spinBox_width->value() - 1 + ui->LimitatorSideOffset_spinBox->value());
                break;
            case 2: // Reset upper side
                current_limitator_data.ChangedValue = (unsigned char) (ui->spinBox_y1->value() + ui->LimitatorSideOffset_spinBox->value());
                break;
            case 3: // Reset lower side
                current_limitator_data.ChangedValue = (unsigned char) (ui->spinBox_y1->value() + ui->spinBox_height->value() - 1 + ui->LimitatorSideOffset_spinBox->value());
            }
            current_limitator_data.x3 = (unsigned char) ui->TriggerBlockPositionX_spinBox->value();
            current_limitator_data.y3 = (unsigned char) ui->TriggerBlockPositionY_spinBox->value();
        }
        else
        {
            current_limitator_data.ChangeValueOffset = current_limitator_data.ChangedValue = current_limitator_data.x3 = current_limitator_data.y3 = (unsigned char) 0xFF;
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
    QString string = "(" +
            QString::number(ui->spinBox_x1->value(), 10) + ", " +
            QString::number(ui->spinBox_y1->value(), 10) + ") - (" +
            QString::number(ui->spinBox_x1->value() + ui->spinBox_width->value() - 1, 10) + ", " +
            QString::number(ui->spinBox_y1->value() + ui->spinBox_height->value() - 1, 10) + ")";
    ListViewItemModel->item(row, 0)->setText(string);
}

/// <summary>
/// Get limitator information from the current room and list all of them into the Listview.
/// </summary>
void CameraControlDockWidget::PaintListView()
{
    std::vector<struct LevelComponents::__CameraControlRecord*> currentCameraLimitators = currentRoom->GetCameraControlRecords();
    ClearListView();
    ListViewItemModel = new QStandardItemModel(this);
    QStringList List_strs;
    if(currentCameraLimitators.size() > (unsigned int) 0)
    {
        for(int i = 0; i < (int) currentCameraLimitators.size(); ++i)
        {
            QString string = "(" +
                    QString::number((int) currentCameraLimitators[i]->x1, 10) + ", " +
                    QString::number((int) currentCameraLimitators[i]->y1, 10) + ") - (" +
                    QString::number((int) currentCameraLimitators[i]->x2, 10) + ", " +
                    QString::number((int) currentCameraLimitators[i]->y2, 10) + ")";
            List_strs << string;
        }
        int nCount = List_strs.size();

        for(int i = 0; i < nCount; i++)
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
    if(ListViewItemModel)
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
    std::vector<struct LevelComponents::__CameraControlRecord*> currentCameraLimitators = currentRoom->GetCameraControlRecords();
    int linenum = index.row();
    SelectedLimitator = linenum;
    LevelComponents::__CameraControlRecord *currentLimitator = currentCameraLimitators[linenum];
    int currentLimitatorTypeid = (currentLimitator->ChangeValueOffset == 0xFF ? -1: currentLimitator->ChangeValueOffset);
    ui->CameraLimitatorTypePicker_comboBox->setCurrentIndex(currentLimitatorTypeid + 1);
    ui->spinBox_x1->setValue(currentLimitator->x1);
    ui->spinBox_y1->setValue(currentLimitator->y1);
    ui->spinBox_width->setValue(currentLimitator->x2 - currentLimitator->x1 + 1);
    ui->spinBox_height->setValue(currentLimitator->y2 - currentLimitator->y1 + 1);
    if((currentLimitatorTypeid > -1) && currentLimitatorTypeid != 0xFF)
    {
        ui->LimitatorSideOffset_spinBox->setValue(currentLimitator->ChangedValue - *(((unsigned char *) &(currentLimitator->x1)) + currentLimitatorTypeid));
        ui->TriggerBlockPositionX_spinBox->setValue(currentLimitator->x3);
        ui->TriggerBlockPositionY_spinBox->setValue(currentLimitator->y3);
    }
    else
    {
        ui->LimitatorSideOffset_spinBox->setValue(0);
        ui->TriggerBlockPositionX_spinBox->setValue(0);
        ui->TriggerBlockPositionY_spinBox->setValue(0);
    }
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
    (void) index;
    if(!IsSavingData) return;
    ui->LimitatorSideOffset_spinBox->setValue(0);
    ui->TriggerBlockPositionX_spinBox->setValue(0);
    ui->TriggerBlockPositionY_spinBox->setValue(0);
    SetCurrentLimitator();
    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
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
    if(!IsSavingData) return;
    SetListviewItemText(SelectedLimitator);
    SetCurrentLimitator();
    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    // TODO: Reset the text in the listview
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
    if(!IsSavingData) return;
    SetListviewItemText(SelectedLimitator);
    SetCurrentLimitator();
    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    // TODO: Reset the text in the listview
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
    if(!IsSavingData) return;
    SetListviewItemText(SelectedLimitator);
    SetCurrentLimitator();
    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    // TODO: Reset the text in the listview
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
    if(!IsSavingData) return;
    SetListviewItemText(SelectedLimitator);
    SetCurrentLimitator();
    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
    // TODO: Reset the text in the listview
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
    if(!IsSavingData) return;
    SetCurrentLimitator();
    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
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
    if(!IsSavingData) return;
    SetCurrentLimitator();
    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
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
    if(!IsSavingData) return;
    SetCurrentLimitator();
    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
}

/// <summary>
/// Called when CameraYFixed_radioButton is clicked.
/// </summary>
/// <param name="checked">
/// Show if CameraYFixed_radioButton is been checked.
/// </param>
void CameraControlDockWidget::on_CameraYFixed_radioButton_clicked(bool checked)
{
    if(checked)
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
}

/// <summary>
/// Called when FollowWario_radioButton is clicked.
/// </summary>
/// <param name="checked">
/// Show if FollowWario_radioButton is been checked.
/// </param>
void CameraControlDockWidget::on_FollowWario_radioButton_clicked(bool checked)
{
    if(checked)
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
}

/// <summary>
/// Called when UseCameraLimitators_radioButton is clicked.
/// </summary>
/// <param name="checked">
/// Show if UseCameraLimitators_radioButton is been checked.
/// </param>
void CameraControlDockWidget::on_UseCameraLimitators_radioButton_clicked(bool checked)
{
    if(checked)
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
}

/// <summary>
/// Called when DeleteCameraLimitator_pushButton is clicked.
/// </summary>
void CameraControlDockWidget::on_DeleteCameraLimitator_pushButton_clicked()
{
    if(SelectedLimitator == -1) return;
    ui->LimitatorSetting_groupBox->setEnabled(false);
    currentRoom->DeleteCameraLimitator(SelectedLimitator);
    SelectedLimitator = -1;
    ClearCurrentLimitatorSetting();
    singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
    PaintListView();
    singleton->GetCurrentRoom()->SetCameraBoundaryDirty(true);
}
