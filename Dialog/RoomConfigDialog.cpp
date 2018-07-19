#include "RoomConfigDialog.h"
#include "ui_RoomConfigDialog.h"

#include <cstring>

// constexpr declarations for the initializers in the header
constexpr const char *RoomConfigDialog::TilesetNamesSetData[0x5C];
constexpr const char *RoomConfigDialog::LayerPrioritySetData[3];
constexpr const char *RoomConfigDialog::AlphaBlendAttrsSetData[12];
constexpr const char *RoomConfigDialog::Layer0MappingTypeParamSetData[2];
constexpr int RoomConfigDialog::BGLayerdataPtrsData[166];

// static variables used by RoomConfigDialog
static QStringList TilesetNamesSet, LayerPrioritySet, AlphaBlendAttrsSet, Layer0MappingTypeParamSet;
static std::vector<int> BGLayerdataPtrs[0x5C];

/// <summary>
/// Construct the instance of the RoomConfigDialog.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
RoomConfigDialog::RoomConfigDialog(QWidget *parent, DialogParams::RoomConfigParams *CurrentRoomParams) :
    QDialog(parent),
    ui(new Ui::RoomConfigDialog),
    currentParams(CurrentRoomParams)
{
    ui->setupUi(this);

    // Initialize UI elements
    ui->ComboBox_TilesetID->addItems(TilesetNamesSet); // sets current tileset index to 0
    ui->ComboBox_LayerPriority->addItems(LayerPrioritySet);
    ui->ComboBox_AlphaBlendAttribute->addItems(AlphaBlendAttrsSet);
    ui->ComboBox_Layer0MappingType->addItems(Layer0MappingTypeParamSet);
    ui->ComboBox_TilesetID->setCurrentIndex(currentParams->CurrentTilesetIndex);
    ui->CheckBox_Layer0Enable->setChecked(currentParams->Layer0Enable);
    ui->CheckBox_Layer0Alpha->setChecked(currentParams->Layer0Alpha);
    int LayerPriorityID = currentParams->LayerPriorityAndAlphaAttr & 3;
    ui->ComboBox_LayerPriority->setCurrentIndex((LayerPriorityID < 2) ? LayerPriorityID : (LayerPriorityID - 1));
    ui->ComboBox_AlphaBlendAttribute->setCurrentIndex((currentParams->LayerPriorityAndAlphaAttr & 0x78) >> 3);
    if(CurrentRoomParams->Layer0Enable)
    {
        ui->ComboBox_Layer0MappingType->setCurrentIndex((((currentParams->Layer0MappingTypeParam) & 0x30) >> 4) - 1);
    }
    ui->CheckBox_Layer0AutoScroll->setChecked(currentParams->Layer0MappingTypeParam == 0x22);
    ui->ComboBox_Layer0Picker->setEnabled(currentParams->Layer0MappingTypeParam >= 0x20);
    ui->SpinBox_RoomWidth->setValue(currentParams->RoomWidth);
    ui->SpinBox_RoomHeight->setValue(currentParams->RoomHeight);
    ui->CheckBox_Layer2Enable->setChecked(currentParams->Layer2Enable);
    ui->CheckBox_BGLayerEnable->setChecked(currentParams->BackgroundLayerEnable);
    ui->CheckBox_BGLayerAutoScroll->setChecked(currentParams->BackgroundLayerAutoScrollEnable);

    // Initialize the selection for the BG selection combobox
    bool CurrentBGSelectionAvailable = false;
    std::vector<int> CurrentBGLayerdataPtrs = BGLayerdataPtrs[currentParams->CurrentTilesetIndex];
    for(unsigned int i = 0; i < CurrentBGLayerdataPtrs.size(); ++i)
    {
        if(currentParams->BackgroundLayerDataPtr == CurrentBGLayerdataPtrs[i])
        {
            CurrentBGSelectionAvailable = true;
            ui->ComboBox_BGLayerPicker->setCurrentIndex(i);
            break;
        }
    }
    if(!CurrentBGSelectionAvailable)
    {
        ui->ComboBox_BGLayerPicker->addItem(QString::number(currentParams->BackgroundLayerDataPtr, 16).toUpper());
        ui->ComboBox_BGLayerPicker->setCurrentIndex(ui->ComboBox_BGLayerPicker->count() - 1);
    }

    // TODO Does not work
    ui->graphicsView->verticalScrollBar()->setValue(0);
    ui->graphicsView->horizontalScrollBar()->setValue(0);
}

/// <summary>
/// Deconstruct the RoomConfigDialog and clean up its instance objects on the heap.
/// </summary>
RoomConfigDialog::~RoomConfigDialog()
{
    delete ui;
    delete currentParams;
}

/// <summary>
/// Perform static initializtion of constant data structures for the dialog.
/// </summary>
void RoomConfigDialog::StaticInitialization()
{
    // Initialize the selections for the tilesets
    for(unsigned int i = 0; i < sizeof(TilesetNamesSetData)/sizeof(TilesetNamesSetData[0]); ++i)
    {
        TilesetNamesSet << TilesetNamesSetData[i];
    }
    // Initialize the selections for the layer priority types
    for(unsigned int i = 0; i < sizeof(LayerPrioritySetData)/sizeof(LayerPrioritySetData[0]); ++i)
    {
        LayerPrioritySet << LayerPrioritySetData[i];
    }
    // Initialize the selections for the alpha blending types
    for(unsigned int i = 0; i < sizeof(AlphaBlendAttrsSetData)/sizeof(AlphaBlendAttrsSetData[0]); ++i)
    {
        AlphaBlendAttrsSet << AlphaBlendAttrsSetData[i];
    }
    // Initialize the selections for the types of layer mapping for layer 0
    for(unsigned int i = 0; i < sizeof(Layer0MappingTypeParamSetData)/sizeof(Layer0MappingTypeParamSetData[0]); ++i)
    {
        Layer0MappingTypeParamSet << Layer0MappingTypeParamSetData[i];
    }
    // Initialize the selections for the tilesets's available BGs
    for(unsigned int i = 0, idx = 0; idx < sizeof(BGLayerdataPtrsData)/sizeof(BGLayerdataPtrsData[0]); ++i)
    {
        std::vector<int> vec;
        int count = BGLayerdataPtrsData[idx++];
        while(count--)
        {
            vec.push_back(BGLayerdataPtrsData[idx++]);
        }
        BGLayerdataPtrs[i] = vec;
    }
}

/// <summary>
/// Render the current MAP16 Tileset.
/// </summary>
/// <remarks>
/// The tileset ID is directly get from DialogParams::RoomConfigParams *currentParams.
/// </remarks>
void RoomConfigDialog::ShowTilesetDetails()
{
    // Set up tileset
    int _tilesetPtr = WL4Constants::TilesetDataTable + currentParams->CurrentTilesetIndex * 36;
    LevelComponents::Tileset *tmpTileset = new LevelComponents::Tileset(_tilesetPtr, currentParams->CurrentTilesetIndex);

    // Set up scene
    if(tmpGraphicviewScene != nullptr) delete tmpGraphicviewScene;
    tmpGraphicviewScene = new QGraphicsScene(0, 0, 8 * 16, (48 * 2) * 16);
    QPixmap layerPixmap(8 * 16, (48 * 2) * 16);
    layerPixmap.fill(Qt::transparent);

    // Draw the tiles to the QPixmap
    for(int i = 0; i < (48 * 2); ++i)
    {
        for(int j = 0; j < 8; ++j)
        {
            tmpTileset->GetMap16Data()[i * 8 + j]->DrawTile(&layerPixmap, j * 16, i * 16);
        }
    }
    tmpGraphicviewScene->addPixmap(layerPixmap);

    // Repaint and set the position of the scene in the graphics view
    ui->graphicsView->repaint();
    ui->graphicsView->setScene(tmpGraphicviewScene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    delete tmpTileset;
}

/// <summary>
/// Render a Layer with mapping type 0x20.
/// </summary>
/// <remarks>
/// use it to render either Layer 0 or background Layer when the mapping type is 0x20.
/// </remarks>
/// <param name="_layerdataAddr">
/// Layer RLE data.
/// </param>
/// /// <param name="_tmpLayer">
/// choose a Layer ptr in the class, I don't think it is a must.
/// </param>
void RoomConfigDialog::ShowMappingType20LayerDetails(int _layerdataAddr, LevelComponents::Layer *_tmpLayer)
{
    // Re-initialize
    if(_tmpLayer)
    {
        delete _tmpLayer;
    }
    _tmpLayer = new LevelComponents::Layer(_layerdataAddr, LevelComponents::LayerTile8x8);

    int _tilesetPtr = WL4Constants::TilesetDataTable + currentParams->CurrentTilesetIndex * 36;
    LevelComponents::Tileset *tmpTileset = new LevelComponents::Tileset(_tilesetPtr, currentParams->CurrentTilesetIndex);
    QPixmap tmplayerpixmap = _tmpLayer->RenderLayer(tmpTileset);
    int sceneWidth = 64 * 16;
    int sceneHeight = 64 * 16;
    if(tmpGraphicviewScene != nullptr)
    {
        delete tmpGraphicviewScene;
    }
    tmpGraphicviewScene = new QGraphicsScene(0, 0, sceneWidth, sceneHeight);
    tmpGraphicviewScene->addPixmap(tmplayerpixmap);
    ui->graphicsView->repaint();
    ui->graphicsView->setScene(tmpGraphicviewScene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    delete tmpTileset;
}

/// <summary>
/// Slot function for CheckBox_Layer0Enable_stateChanged.
/// </summary>
void RoomConfigDialog::on_CheckBox_Layer0Enable_stateChanged(int arg1)
{
    (void) arg1;
    if(ui->CheckBox_Layer0Enable->isChecked())
    {
        ui->CheckBox_Layer0Alpha->setEnabled(true);
        ui->ComboBox_Layer0MappingType->setEnabled(true);
    }
    else
    {
        ui->CheckBox_Layer0Alpha->setChecked(false);
        ui->CheckBox_Layer0Alpha->setEnabled(false);
        ui->ComboBox_Layer0MappingType->setEnabled(false);
        currentParams->Layer0MappingTypeParam = 0;
    }
}

/// <summary>
/// Slot function for CheckBox_Layer0Alpha_stateChanged.
/// </summary>
void RoomConfigDialog::on_CheckBox_Layer0Alpha_stateChanged(int arg1)
{
    (void) arg1;
    currentParams->Layer0Alpha = ui->CheckBox_Layer0Alpha->isChecked();
    ui->ComboBox_AlphaBlendAttribute->setEnabled(ui->CheckBox_Layer0Alpha->isChecked());
    if(!ui->CheckBox_Layer0Alpha->isChecked())
    {
        ui->ComboBox_AlphaBlendAttribute->setCurrentIndex(0);
    }
}

/// <summary>
/// Slot function for ComboBox_Layer0MappingType_currentIndexChanged.
/// </summary>
void RoomConfigDialog::on_ComboBox_Layer0MappingType_currentIndexChanged(int index)
{
    (void) index;
    currentParams->Layer0MappingTypeParam = ui->ComboBox_Layer0MappingType->currentIndex() << 4;
    if(!ui->ComboBox_Layer0MappingType->currentIndex())
    {
        ui->CheckBox_Layer0AutoScroll->setChecked(false);
        ui->CheckBox_Layer0AutoScroll->setEnabled(false);
        ui->ComboBox_Layer0Picker->setEnabled(false);
        currentParams->Layer0MappingTypeParam = LevelComponents::LayerMap16;
        currentParams->Layer0DataPtr = 0;
    }
    else
    {
        ui->CheckBox_Layer0AutoScroll->setEnabled(true);
        ui->ComboBox_Layer0Picker->setEnabled(true);
        currentParams->Layer0MappingTypeParam = LevelComponents::LayerTile8x8;
    }
}

/// <summary>
/// Slot function for CheckBox_Layer0AutoScroll_stateChanged.
/// </summary>
void RoomConfigDialog::on_CheckBox_Layer0AutoScroll_stateChanged(int arg1)
{
    (void) arg1;
    if(ui->CheckBox_Layer0AutoScroll->isChecked()) currentParams->Layer0MappingTypeParam = 0x22;
}

/// <summary>
/// Slot function for ComboBox_AlphaBlendAttribute_currentIndexChanged.
/// </summary>
void RoomConfigDialog::on_ComboBox_AlphaBlendAttribute_currentIndexChanged(int index)
{
    (void) index;
    currentParams->LayerPriorityAndAlphaAttr = (currentParams->LayerPriorityAndAlphaAttr & 3) | (ui->ComboBox_AlphaBlendAttribute->currentIndex() << 3);
}

/// <summary>
/// Slot function for ComboBox_TilesetID_currentIndexChanged.
/// </summary>
void RoomConfigDialog::on_ComboBox_TilesetID_currentIndexChanged(int index)
{
    (void) index;
    currentParams->CurrentTilesetIndex = ui->ComboBox_TilesetID->currentIndex();
    ShowTilesetDetails();
}

/// <summary>
/// Slot function for SpinBox_RoomWidth_valueChanged.
/// </summary>
void RoomConfigDialog::on_SpinBox_RoomWidth_valueChanged(int arg1)
{
    currentParams->RoomWidth = arg1;
}

/// <summary>
/// Slot function for SpinBox_RoomHeight_valueChanged.
/// </summary>
void RoomConfigDialog::on_SpinBox_RoomHeight_valueChanged(int arg1)
{
    currentParams->RoomHeight = arg1;
}

/// <summary>
/// Slot function for CheckBox_Layer2Enable_stateChanged.
/// </summary>
void RoomConfigDialog::on_CheckBox_Layer2Enable_stateChanged(int arg1)
{
    (void) arg1;
    currentParams->Layer2Enable = ui->CheckBox_Layer2Enable->isChecked();
}

/// <summary>
/// Slot function for CheckBox_BGLayerAutoScroll_stateChanged.
/// </summary>
void RoomConfigDialog::on_CheckBox_BGLayerAutoScroll_stateChanged(int arg1)
{
    (void) arg1;
    currentParams->BackgroundLayerAutoScrollEnable = ui->CheckBox_BGLayerAutoScroll->isChecked();
}

/// <summary>
/// Slot function for CheckBox_BGLayerEnable_stateChanged.
/// </summary>
void RoomConfigDialog::on_CheckBox_BGLayerEnable_stateChanged(int arg1)
{
    (void) arg1;
    currentParams->BackgroundLayerEnable = ui->CheckBox_BGLayerEnable->isChecked();
    ui->ComboBox_BGLayerPicker->setEnabled(ui->CheckBox_BGLayerEnable->isChecked() & (BGLayerdataPtrs[currentParams->CurrentTilesetIndex].size()));
    ui->CheckBox_BGLayerAutoScroll->setEnabled(ui->CheckBox_BGLayerEnable->isChecked());
    if(!ui->CheckBox_BGLayerEnable->isChecked())
    {
        currentParams->BackgroundLayerDataPtr = WL4Constants::BGLayerDisableDefaultPtr;
    }
    if(ui->ComboBox_BGLayerPicker->count() > 0)
    {
        for(int i = ui->ComboBox_BGLayerPicker->count() - 1; i >= 0 ; ++i)
        {
            ui->ComboBox_BGLayerPicker->removeItem(i);
        }
    }
    for(unsigned int i = 0; i < BGLayerdataPtrs[currentParams->CurrentTilesetIndex].size(); ++i)
    {
        if(BGLayerdataPtrs[currentParams->CurrentTilesetIndex][i])
        {
            ui->ComboBox_BGLayerPicker->addItem(QString::number(BGLayerdataPtrs[currentParams->CurrentTilesetIndex][i], 16).toUpper());
        }
    }
}

/// <summary>
/// Slot function for ComboBox_LayerPriority_currentIndexChanged.
/// </summary>
void RoomConfigDialog::on_ComboBox_LayerPriority_currentIndexChanged(int index)
{
    (void) index;
    if(ui->ComboBox_LayerPriority->currentIndex() >= 0)
    {
        currentParams->LayerPriorityAndAlphaAttr =
            (currentParams->LayerPriorityAndAlphaAttr & 0x78) | ((ui->ComboBox_LayerPriority->currentIndex() < 2) ?
            ui->ComboBox_LayerPriority->currentIndex() : 3);
    }
}

/// <summary>
/// Slot function for ComboBox_BGLayerPicker_currentIndexChanged.
/// </summary>
void RoomConfigDialog::on_ComboBox_BGLayerPicker_currentIndexChanged(int index)
{
    (void) index;
    currentParams->BackgroundLayerDataPtr = ui->ComboBox_BGLayerPicker->currentText().toInt();
    //ShowMappingType20LayerDetails(currentParams->BackgroundLayerDataPtr, tmpBGLayer);  //crash after this function is added
}

/// <summary>
/// Slot function for ComboBox_Layer0Picker_currentIndexChanged.
/// </summary>
void RoomConfigDialog::on_ComboBox_Layer0Picker_currentIndexChanged(int index)
{
    (void) index;
    currentParams->Layer0DataPtr = WL4Constants::ToxicLandfillDustyLayer0Ptr;
    ui->CheckBox_Layer0AutoScroll->setEnabled(true);
    ShowMappingType20LayerDetails(currentParams->Layer0DataPtr, tmpLayer0);  //crash after this function is added
}
