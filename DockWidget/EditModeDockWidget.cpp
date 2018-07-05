#include "EditModeDockWidget.h"
#include "ui_EditModeDockWidget.h"

static bool initialized = false;

/// <summary>
/// Construct the instance of the EditModeDockWidget.
/// </summary>
/// <remarks>
/// Also initializes the enum and index mappings to quickly create EditModeParams structs.
/// </remarks>
/// <param name="parent">
/// The parent QWidget.
/// </param>
EditModeDockWidget::EditModeDockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::EditModeDockWidget)
{
    ui->setupUi(this);

    modeEnums[ui->RadioButton_LayerMode]  = Ui::LayerEditMode;
    modeEnums[ui->RadioButton_EntityMode] = Ui::EntityEditMode;
    modeEnums[ui->RadioButton_DoorMode]   = Ui::DoorEditMode;
    modeEnums[ui->RadioButton_CameraMode] = Ui::CameraEditMode;
    layerIndices[ui->RadioButton_EditOnLayer0] = 0;
    layerIndices[ui->RadioButton_EditOnLayer1] = 1;
    layerIndices[ui->RadioButton_EditOnLayer2] = 2;
    layerIndices[ui->RadioButton_EditOnLayer3] = 3;
    difficultyIndices[ui->RadioButton_NMode] = 0;
    difficultyIndices[ui->RadioButton_HMode] = 1;
    difficultyIndices[ui->RadioButton_SHMode] = 2;

    //Set the widget height
    QFontMetrics fontMetrics(ui->CheckBox_AlphaView->font());
    int rowHeight = fontMetrics.lineSpacing();
    ui->dockWidgetContents->setFixedHeight(16 * rowHeight); // TODO: Make this exact, calculate using margins
}

/// <summary>
/// Deconstruct the EditModeDockWidget and clean up its instance objects on the heap.
/// </summary>
EditModeDockWidget::~EditModeDockWidget()
{
    delete ui;
}

/// <summary>
/// Retrieve the selected edit mode options as a structure.
/// </summary>
/// <return>
/// A struct filled out with the selected edit mode options.
/// </return>
struct Ui::EditModeParams EditModeDockWidget::GetEditModeParams()
{
    QAbstractButton *selectedModeButton = ui->RadioButton_LayerMode->group()->checkedButton();
    QAbstractButton *selectedLayerButton = ui->RadioButton_EditOnLayer0->group()->checkedButton();
    QAbstractButton *selectedDifficultyButton = ui->RadioButton_NMode->group()->checkedButton();
    struct Ui::EditModeParams params;
    params.editMode = modeEnums[selectedModeButton];
    params.selectedLayer = layerIndices[selectedLayerButton];
    params.layersEnabled[0] = ui->CheckBox_Layer0View->isChecked();
    params.layersEnabled[1] = ui->CheckBox_Layer1View->isChecked();
    params.layersEnabled[2] = ui->CheckBox_Layer2View->isChecked();
    params.layersEnabled[3] = ui->CheckBox_Layer3View->isChecked();
    params.entitiesEnabled = ui->CheckBox_EntityView->isChecked();
    params.doorsEnabled = ui->CheckBox_DoorView->isChecked();
    params.alphaBlendingEnabled = ui->CheckBox_AlphaView->isChecked();
    params.cameraAreasEnabled = ui->CheckBox_CameraView->isChecked();
    params.seleteddifficulty = difficultyIndices[selectedDifficultyButton];
    return params;
}
