#include "CameraControlDockWidget.h"
#include "ui_CameraControlDockWidget.h"

#include "WL4EditorWindow.h"
extern WL4EditorWindow *singleton;

// constexpr declarations for the initializers in the header
constexpr const char *CameraControlDockWidget::CameraLimitatorResetSideTypeNameData[5];

// static variables used by CameraControlDockWidget
static QStringList CameraLimitatorTypeNameSet;

CameraControlDockWidget::CameraControlDockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::CameraControlDockWidget)
{
    // Set UI
    ui->setupUi(this);
    ui->CameraLimitatorTypePicker_comboBox->addItems(CameraLimitatorTypeNameSet);
}

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
void CameraControlDockWidget::ResetCameraControlInfo(LevelComponents::Room *currentroom)
{
    currentRoom = currentroom;
//    enum LevelComponents::__CameraControlType currentcameracontroltype = currentroom->GetCameraControlType();
//    if(currentcameracontroltype == LevelComponents::FixedY)
//    {
//        ui->CameraYFixed_radioButton->setChecked(true);
//    }
//    else if(currentcameracontroltype == LevelComponents::NoLimit)
//    {
//        ui->FollowWario_radioButton->setChecked(true);
//    }
//    else if(currentcameracontroltype == LevelComponents::HasControlAttrs)
//    {
//        ui->UseCameraLimitators_radioButton->setChecked(true);
//    }
//    if(currentcameracontroltype != LevelComponents::HasControlAttrs)
//    {
//        ui->ExistingLimitators_groupBox->setEnabled(false);
//    }
//    else
//    {
        ui->ExistingLimitators_groupBox->setEnabled(true);
        std::vector<struct LevelComponents::__CameraControlRecord*> currentCameraLimitators = currentroom->GetCameraControlRecords();
        if(ListViewItemModel != nullptr)
        {
            ListViewItemModel->clear();
            delete ListViewItemModel;
        }
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
            ui->verticalLayout_LimitatorSetting->setEnabled(false);
        }
//    }
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

void CameraControlDockWidget::on_CameraLimitators_listView_clicked(const QModelIndex &index)
{

}
