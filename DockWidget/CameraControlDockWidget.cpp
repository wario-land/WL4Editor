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
    if(ListViewItemModel != nullptr)
    {
        ListViewItemModel->removeRows(0,ListViewItemModel->rowCount());
        delete ListViewItemModel;
        ListViewItemModel = nullptr;
    }
    ClearCurrentLimitatorSetting();
    ui->LimitatorSetting_groupBox->setEnabled(false);
    enum LevelComponents::__CameraControlType currentcameracontroltype = currentroom->GetCameraControlType();
    if(currentcameracontroltype == LevelComponents::FixedY)
    {
        ui->CameraYFixed_radioButton->setChecked(true);
    }
    else if(currentcameracontroltype == LevelComponents::NoLimit)
    {
        ui->FollowWario_radioButton->setChecked(true);
    }
    else if(currentcameracontroltype == LevelComponents::HasControlAttrs)
    {
        ui->UseCameraLimitators_radioButton->setChecked(true);
    }

    if(currentcameracontroltype != LevelComponents::HasControlAttrs)
    {
        ui->ExistingLimitators_groupBox->setEnabled(false);
    }
    else
    {
        ui->ExistingLimitators_groupBox->setEnabled(true);
        std::vector<struct LevelComponents::__CameraControlRecord*> currentCameraLimitators = currentroom->GetCameraControlRecords();
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
            ui->CameraLimitators_listView->setModel(ListViewItemModel);
        }
    }
}

/// <summary>
/// Perform static initializtion of constant data structures for the dock widget.
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
    ui->CameraLimitatorTypePicker_comboBox->setCurrentIndex(0);
    ui->spinBox_x1->setValue(0);
    ui->spinBox_y1->setValue(0);
    ui->spinBox_width->setValue(1);
    ui->spinBox_height->setValue(1);
    ui->LimitatorSideOffset_spinBox->setValue(0);
    ui->TriggerBlockPositionX_spinBox->setValue(0);
    ui->TriggerBlockPositionY_spinBox->setValue(0);
}

/// <summary>
/// Be called the listview is clicked and a limitator is selected.
/// </summary>
/// <param name="index">
/// Referrence of the selected QModelIndex from the listview.
/// </param>
void CameraControlDockWidget::on_CameraLimitators_listView_clicked(const QModelIndex &index)
{
    ui->LimitatorSetting_groupBox->setEnabled(true);
    std::vector<struct LevelComponents::__CameraControlRecord*> currentCameraLimitators = currentRoom->GetCameraControlRecords();
    int linenum = index.row();
    LevelComponents::__CameraControlRecord *currentLimitator = currentCameraLimitators[linenum];
    int currentLimitatorTypeid = (signed char) currentLimitator->ChangeValueOffset;
    ui->CameraLimitatorTypePicker_comboBox->setCurrentIndex(currentLimitatorTypeid + 1);
    ui->spinBox_x1->setValue(currentLimitator->x1);
    ui->spinBox_y1->setValue(currentLimitator->y1);
    ui->spinBox_width->setValue(currentLimitator->x2 - currentLimitator->x1 + 1);
    ui->spinBox_height->setValue(currentLimitator->y2 - currentLimitator->y1 + 1);
    if(currentLimitatorTypeid >= 0)
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
}
