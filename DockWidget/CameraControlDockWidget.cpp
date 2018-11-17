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
