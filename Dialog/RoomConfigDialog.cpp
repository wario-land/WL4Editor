#include "RoomConfigDialog.h"
#include "ui_RoomConfigDialog.h"

#include <cstring>
#include <QMessageBox>
#include "AssortedGraphicUtils.h"

// constexpr declarations for the initializers in the header
constexpr const char *RoomConfigDialog::TilesetNamesSetData[0x5C];
constexpr const char *RoomConfigDialog::LayerPrioritySetData[4];
constexpr const char *RoomConfigDialog::AlphaBlendAttrsSetData[12];
constexpr unsigned int RoomConfigDialog::BGLayerdataPtrsData[166];
constexpr unsigned int RoomConfigDialog::VanillaTilesetBGTilesDataAddr[0x5C];

// static variables used by RoomConfigDialog
static QStringList TilesetNamesSet, LayerPrioritySet, AlphaBlendAttrsSet;
static std::vector<int> BGLayerdataPtrs;

// helper function
static unsigned short *ChangeLayerDimensions(int newWidth, int newHeight, int oldWidth, int oldHeight, unsigned short *oldData)
{
    if ((newWidth < 1 || newHeight < 1) || (oldData == nullptr)) return nullptr;

    unsigned short *tmpLayerData = new unsigned short[newWidth * newHeight];
    int boundX = qMin(oldWidth, newWidth), boundY = qMin(oldHeight, newHeight);
    unsigned short defaultValue = 0x0000;

    // init
    memset(tmpLayerData, defaultValue, 2 * newWidth * newHeight);

    // copy old data
    if (oldWidth > 0 && oldHeight > 0)
    {
        for (int i = 0; i < boundY; ++i)
        {
            for (int j = 0; j < boundX; ++j)
            {
                tmpLayerData[i * newWidth + j] = oldData[i * oldWidth + j];
            }
        }
    }
    return tmpLayerData;
}

/// <summary>
/// Construct the instance of the RoomConfigDialog.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
RoomConfigDialog::RoomConfigDialog(QWidget *parent, DialogParams::RoomConfigParams *CurrentRoomParams) :
        QDialog(parent), ui(new Ui::RoomConfigDialog)
{
    ui->setupUi(this);

    // Initialize UI elements
    ui->ComboBox_TilesetID->addItems(TilesetNamesSet);
    ui->ComboBox_LayerPriority->addItems(LayerPrioritySet);
    ui->ComboBox_AlphaBlendAttribute->addItems(AlphaBlendAttrsSet);
    ui->ComboBox_TilesetID->setCurrentIndex(CurrentRoomParams->CurrentTilesetIndex);
    ui->CheckBox_Layer0Alpha->setChecked(CurrentRoomParams->Layer0Alpha);
    int LayerPriorityID = CurrentRoomParams->LayerPriorityAndAlphaAttr & 3;
    ui->ComboBox_LayerPriority->setCurrentIndex(LayerPriorityID);
    ui->ComboBox_AlphaBlendAttribute->setCurrentIndex(qMax((CurrentRoomParams->LayerPriorityAndAlphaAttr - 4), 0) >> 2);  // == (LayerPriorityAndAlphaAttr - 8) >> 2 + 1
    ui->spinBox_Layer0MappingType->setValue(CurrentRoomParams->Layer0MappingTypeParam);
    ui->ComboBox_Layer0Picker->setEnabled(CurrentRoomParams->Layer0MappingTypeParam >= 0x20);
    ui->spinBox_Layer0Width->setValue(CurrentRoomParams->Layer0Width);
    ui->spinBox_Layer0Height->setValue(CurrentRoomParams->Layer0Height);
    ui->SpinBox_RoomWidth->setValue(CurrentRoomParams->RoomWidth);
    ui->SpinBox_RoomHeight->setValue(CurrentRoomParams->RoomHeight);
    ui->spinBox_Layer2MappingType->setValue(CurrentRoomParams->Layer2MappingTypeParam);
    ui->CheckBox_BGLayerEnable->setChecked(CurrentRoomParams->BackgroundLayerEnable);
    ui->spinBox_BGLayerScrollingFlag->setValue(CurrentRoomParams->BGLayerScrollFlag);
    ui->spinBox_RasterType->setValue(CurrentRoomParams->RasterType);
    ui->spinBox_Water->setValue(CurrentRoomParams->Water);
    ui->spinBox_BgmVolume->setValue(CurrentRoomParams->BGMVolume);

    // Initialize the items for the BG selection combobox
    // The hardcode layer 3 pointers have been added into the combobox when setting Tileset combobox id
    // Add the current layer 3 pointer if it is not record and hardcode in the editor
    ResetBGLayerPickerComboBox(CurrentRoomParams->CurrentTilesetIndex);
    bool CurrentBGSelectionAvailable = false;
    for (unsigned int i = 0; i < BGLayerdataPtrs.size(); ++i)
    {
        if (CurrentRoomParams->BackgroundLayerDataPtr == BGLayerdataPtrs[i])
        {
            CurrentBGSelectionAvailable = true;
            ui->ComboBox_BGLayerPicker->setCurrentIndex(i);
            break;
        }
    }
    if (!CurrentBGSelectionAvailable)
    {
        ui->ComboBox_BGLayerPicker->setCurrentIndex(ui->ComboBox_BGLayerPicker->count() - 1);
    }

    //  Initialize the items for the Layer 0 selection combobox
    ui->ComboBox_Layer0Picker->addItem(
                QString::number(WL4Constants::BGLayerDefaultPtr, 16).toUpper());
    unsigned int bgtiledataaddr = ROMUtils::singletonTilesets[CurrentRoomParams->CurrentTilesetIndex]->GetbgGFXptr();
    if(bgtiledataaddr == WL4Constants::Tileset_BGTile_0x21)
    {
        ui->ComboBox_Layer0Picker->addItem(
                    QString::number(WL4Constants::ToxicLandfillDustyLayer0Ptr, 16).toUpper());
    }
    else if(bgtiledataaddr == WL4Constants::Tileset_BGTile_0x45)
    {
        ui->ComboBox_Layer0Picker->addItem(
            QString::number(WL4Constants::FieryCavernDustyLayer0Ptr, 16).toUpper());
    }
    if(CurrentRoomParams->Layer0DataPtr &&
            CurrentRoomParams->Layer0DataPtr != WL4Constants::ToxicLandfillDustyLayer0Ptr &&
            CurrentRoomParams->Layer0DataPtr != WL4Constants::FieryCavernDustyLayer0Ptr)
    {
        ui->ComboBox_Layer0Picker->addItem(
            QString::number(CurrentRoomParams->Layer0DataPtr, 16).toUpper());
    }
    ui->ComboBox_Layer0Picker->setCurrentIndex(ui->ComboBox_Layer0Picker->count() - 1); // use the last one in the list for now

    // Initialize the graphic view layers
    ui->graphicsView->infoLabel = ui->graphicViewDetailsLabel;
    currentTileset = ROMUtils::singletonTilesets[CurrentRoomParams->CurrentTilesetIndex];
    int L0ptr = ((ui->spinBox_Layer0MappingType->value() & 0x30) == LevelComponents::LayerTile8x8) ? CurrentRoomParams->Layer0DataPtr : 0;
    ;
    ui->graphicsView->UpdateGraphicsItems(currentTileset, CurrentRoomParams->BackgroundLayerDataPtr, L0ptr);

    ComboBoxInitialized = true;
}

/// <summary>
/// Deconstruct the RoomConfigDialog and clean up its instance objects on the heap.
/// </summary>
RoomConfigDialog::~RoomConfigDialog() { delete ui; }

/// <summary>
/// Get the selected config parameters based on the UI selections.
/// </summary>
/// <param name="prevRoomParams">
/// Use the prevRoomParams to config layerdata in the new configParams.
/// </param>
/// <returns>
/// A RoomConfigParams struct containing the selected parameters from the dialog.
/// </returns>
DialogParams::RoomConfigParams *RoomConfigDialog::GetConfigParams(DialogParams::RoomConfigParams *prevRoomParams)
{
    DialogParams::RoomConfigParams *configParams = new DialogParams::RoomConfigParams();

    // Get all the Room Configuration data
    configParams->CurrentTilesetIndex = ui->ComboBox_TilesetID->currentIndex();
    configParams->Layer0Alpha = ui->CheckBox_Layer0Alpha->isChecked();
    configParams->Layer0MappingTypeParam = ui->spinBox_Layer0MappingType->value();
    if((configParams->Layer0MappingTypeParam & 0x30) == LevelComponents::LayerMap16)
    {
        configParams->Layer0Width = ui->spinBox_Layer0Width->value();
        configParams->Layer0Height = ui->spinBox_Layer0Height->value();
        configParams->Layer0DataPtr = 0;
    }
    else if ((configParams->Layer0MappingTypeParam & 0x30) == LevelComponents::LayerTile8x8)
    {
        configParams->Layer0Width = configParams->Layer0Height = 0;
        configParams->Layer0DataPtr = ui->ComboBox_Layer0Picker->currentText().toUInt(nullptr, 16);
    }
    else
    {
        configParams->Layer0DataPtr = configParams->Layer0Width = configParams->Layer0Height = 0;
    }

    configParams->Layer2MappingTypeParam = ui->spinBox_Layer2MappingType->value();
    configParams->LayerPriorityAndAlphaAttr = ui->ComboBox_LayerPriority->currentIndex() + 4;
    configParams->LayerPriorityAndAlphaAttr += (qMax(ui->ComboBox_AlphaBlendAttribute->currentIndex(), 0) << 2);
    configParams->BackgroundLayerEnable = ui->CheckBox_BGLayerEnable->isChecked();
    configParams->BGLayerScrollFlag = ui->spinBox_BGLayerScrollingFlag->value();
    if (configParams->BackgroundLayerEnable)
    {
        configParams->BackgroundLayerDataPtr = ui->ComboBox_BGLayerPicker->currentText().toUInt(nullptr, 16);
    }
    else
    {
        configParams->BackgroundLayerDataPtr = WL4Constants::BGLayerDefaultPtr;
    }
    configParams->RoomHeight = ui->SpinBox_RoomHeight->value();
    configParams->RoomWidth = ui->SpinBox_RoomWidth->value();
    configParams->RasterType = ui->spinBox_RasterType->value();
    configParams->Water = ui->spinBox_Water->value();
    configParams->BGMVolume = ui->spinBox_BgmVolume->value();

    // Reset Layers, iterate 0, 1, 2
    if((configParams->Layer0MappingTypeParam & 0x30) == LevelComponents::LayerMap16) {
        configParams->LayerData[0] = ChangeLayerDimensions(configParams->Layer0Width, configParams->Layer0Height,
                                                          prevRoomParams->Layer0Width, prevRoomParams->Layer0Height, prevRoomParams->LayerData[0]);
    } else {
        configParams->LayerData[0] = nullptr;
    }
    configParams->LayerData[1] = ChangeLayerDimensions(configParams->RoomWidth, configParams->RoomHeight,
                                                      prevRoomParams->RoomWidth, prevRoomParams->RoomHeight, prevRoomParams->LayerData[1]);
    if((configParams->Layer2MappingTypeParam & 0x30) == LevelComponents::LayerMap16) {
        configParams->LayerData[2] = ChangeLayerDimensions(configParams->RoomWidth, configParams->RoomHeight,
                                                          prevRoomParams->RoomWidth, prevRoomParams->RoomHeight, prevRoomParams->LayerData[2]);
    } else {
        configParams->LayerData[2] = nullptr;
    }

    return configParams;
}

/// <summary>
/// Perform static initializtion of constant data structures for the dialog.
/// </summary>
void RoomConfigDialog::StaticComboBoxesInitialization()
{
    // Initialize the selections for the tilesets
    for (unsigned int i = 0; i < sizeof(TilesetNamesSetData) / sizeof(TilesetNamesSetData[0]); ++i)
    {
        TilesetNamesSet << TilesetNamesSetData[i];
    }

    // Initialize the selections for the layer priority types
    for (unsigned int i = 0; i < sizeof(LayerPrioritySetData) / sizeof(LayerPrioritySetData[0]); ++i)
    {
        LayerPrioritySet << LayerPrioritySetData[i];
    }

    // Initialize the selections for the alpha blending types
    for (unsigned int i = 0; i < sizeof(AlphaBlendAttrsSetData) / sizeof(AlphaBlendAttrsSetData[0]); ++i)
    {
        AlphaBlendAttrsSet << AlphaBlendAttrsSetData[i];
    }
}

/// <summary>
/// Slot function for CheckBox_Layer0Alpha_stateChanged.
/// </summary>
void RoomConfigDialog::on_CheckBox_Layer0Alpha_stateChanged(int state)
{
    ui->ComboBox_AlphaBlendAttribute->setEnabled(ui->CheckBox_Layer0Alpha->isChecked());
    if (state == Qt::Unchecked)
    {
        ui->ComboBox_AlphaBlendAttribute->setCurrentIndex(0);
    }
}

/// <summary>
/// Slot function for ComboBox_TilesetID_currentIndexChanged.
/// </summary>
void RoomConfigDialog::on_ComboBox_TilesetID_currentIndexChanged(int index)
{
    if (ComboBoxInitialized)
    {
        // Update the graphic view
        currentTileset = ROMUtils::singletonTilesets[index];

        // Update the available BG layers to choose from
        ResetBGLayerPickerComboBox(index);

        // Update the available FG layers to choose from
        if(index == 0x21 && ui->spinBox_Layer0MappingType->value() >= 0x20)
        {
            ui->ComboBox_Layer0Picker->setEnabled(true);
        }
        else
        {
            ui->ComboBox_Layer0Picker->setEnabled(false);
        }

        // Extra UI changes for Toxic Landfill dust Layer0
        unsigned int bgtiledataaddr = ROMUtils::singletonTilesets[index]->GetbgGFXptr();
        ui->ComboBox_Layer0Picker->clear();
        if(bgtiledataaddr == WL4Constants::Tileset_BGTile_0x21)
        {
            ui->ComboBox_Layer0Picker->addItem(
                        QString::number(WL4Constants::ToxicLandfillDustyLayer0Ptr, 16).toUpper());
        }
        else if(bgtiledataaddr == WL4Constants::Tileset_BGTile_0x45)
        {
            ui->ComboBox_Layer0Picker->addItem(
                QString::number(WL4Constants::FieryCavernDustyLayer0Ptr, 16).toUpper());
        }
        int BGptr = ui->ComboBox_BGLayerPicker->currentText().toUInt(nullptr, 16);
        int L0ptr = ui->ComboBox_Layer0Picker->currentText().toUInt(nullptr, 16);
        if ((ui->spinBox_Layer0MappingType->value() & 0x20) == 0)
            L0ptr = 0;
        ui->graphicsView->UpdateGraphicsItems(currentTileset, BGptr, L0ptr);
    }
}

/// <summary>
/// Slot function for CheckBox_BGLayerEnable_stateChanged.
/// </summary>
void RoomConfigDialog::on_CheckBox_BGLayerEnable_stateChanged(int state)
{
    ui->ComboBox_BGLayerPicker->setEnabled(state == Qt::Checked);
}

/// <summary>
/// Slot function for ComboBox_BGLayerPicker_currentIndexChanged.
/// </summary>
void RoomConfigDialog::on_ComboBox_BGLayerPicker_currentIndexChanged(int index)
{
    (void) index;
    if (ComboBoxInitialized)
    {
        int BGptr = ui->ComboBox_BGLayerPicker->currentText().toUInt(nullptr, 16);
        int L0ptr = ui->ComboBox_Layer0Picker->currentText().toUInt(nullptr, 16);
        if ((ui->spinBox_Layer0MappingType->value() & 0x30) == LevelComponents::LayerMap16)
            L0ptr = 0;
        ui->graphicsView->UpdateGraphicsItems(currentTileset, BGptr, L0ptr);
    }
}

/// <summary>
/// Slot function for ComboBox_Layer0Picker_currentIndexChanged.
/// </summary>
void RoomConfigDialog::on_ComboBox_Layer0Picker_currentIndexChanged(int index)
{
    (void) index;
    if (ComboBoxInitialized)
    {
        int BGptr = ui->ComboBox_BGLayerPicker->currentText().toUInt(nullptr, 16);
        int L0ptr = ui->ComboBox_Layer0Picker->currentText().toUInt(nullptr, 16);
        if ((ui->spinBox_Layer0MappingType->value() & 0x30) == LevelComponents::LayerMap16)
            L0ptr = 0;
        ui->graphicsView->UpdateGraphicsItems(currentTileset, BGptr, L0ptr);
    }
}

/// <summary>
/// Slot function for ComboBox_LayerPriority_currentIndexChanged.
/// </summary>
void RoomConfigDialog::on_ComboBox_LayerPriority_currentIndexChanged(int index)
{
    (void) index;
    if ((ui->spinBox_Layer0MappingType->value() & 0x30) == LevelComponents::LayerTile8x8)
        ui->ComboBox_LayerPriority->setCurrentIndex(0);
}

/// <summary>
/// Slot function for SpinBox_RoomWidth_valueChanged.
/// </summary>
/// <param name="arg1">
/// The spinbox value.
/// </param>
void RoomConfigDialog::on_SpinBox_RoomWidth_valueChanged(int arg1)
{
    int heightmax = 0x1400 / arg1;
    if (ui->SpinBox_RoomHeight->value() > heightmax)
    {
        ui->SpinBox_RoomWidth->setStyleSheet("background-color: red");
        ui->SpinBox_RoomHeight->setStyleSheet("background-color: red");
    }
    else
    {
        ui->SpinBox_RoomWidth->setStyleSheet(""); // TODO: need a better solution, for example, add an extra label to show the warning
        ui->SpinBox_RoomHeight->setStyleSheet("");
    }
}

/// <summary>
/// Slot function for SpinBox_RoomHeight_valueChanged.
/// </summary>
/// <param name="arg1">
/// The spinbox value.
/// </param>
void RoomConfigDialog::on_SpinBox_RoomHeight_valueChanged(int arg1)
{
    int widthmax = 0x1400 / arg1;
    if (ui->SpinBox_RoomWidth->value() > widthmax)
    {
        ui->SpinBox_RoomWidth->setStyleSheet("background-color: red");
        ui->SpinBox_RoomHeight->setStyleSheet("background-color: red");
    }
    else
    {
        ui->SpinBox_RoomWidth->setStyleSheet(""); // TODO: need a better solution, for example, add an extra label to show the warning
        ui->SpinBox_RoomHeight->setStyleSheet("");
    }
}

/// <summary>
/// Slot function for spinBox_Layer0Width_valueChanged.
/// </summary>
/// <param name="arg1">
/// The spinbox value.
/// </param>
void RoomConfigDialog::on_spinBox_Layer0Width_valueChanged(int arg1)
{
    int heightmax = 0x1400 / arg1;
    if (ui->spinBox_Layer0Height->value() > heightmax)
    {
        ui->spinBox_Layer0Width->setStyleSheet("background-color: red");
        ui->spinBox_Layer0Height->setStyleSheet("background-color: red");
    }
    else
    {
        ui->spinBox_Layer0Width->setStyleSheet(""); // TODO: need a better solution, for example, add an extra label to show the warning
        ui->spinBox_Layer0Height->setStyleSheet("");
    }
}

/// <summary>
/// Slot function for spinBox_Layer0Height_valueChanged.
/// </summary>
/// <param name="arg1">
/// The spinbox value.
/// </param>
void RoomConfigDialog::on_spinBox_Layer0Height_valueChanged(int arg1)
{
    int widthmax = 0x1400 / arg1;
    if (ui->spinBox_Layer0Width->value() > widthmax)
    {
        ui->spinBox_Layer0Width->setStyleSheet("background-color: red");
        ui->spinBox_Layer0Height->setStyleSheet("background-color: red");
    }
    else
    {
        ui->spinBox_Layer0Width->setStyleSheet(""); // TODO: need a better solution, for example, add an extra label to show the warning
        ui->spinBox_Layer0Height->setStyleSheet("");
    }
}

/// <summary>
/// Slot function for spinBox_BGLayerScrollingFlag_valueChanged.
/// </summary>
/// <param name="arg1">
/// The spinbox value.
/// </param>
void RoomConfigDialog::on_spinBox_BGLayerScrollingFlag_valueChanged(int arg1)
{
    switch(arg1)
    {
        case 0: ui->label_CurBGLayerScrollingType->setText("No scrolling"); break;
        case 1: ui->label_CurBGLayerScrollingType->setText("H speed: 1/2 of BG1"); break;
        case 2: ui->label_CurBGLayerScrollingType->setText("V speed: 1/2 of BG1"); break;
        case 3: ui->label_CurBGLayerScrollingType->setText("H and V speed: 1/2 of BG1"); break;
        case 4: ui->label_CurBGLayerScrollingType->setText("H speed sync wih BG1, V speed: 1/2 of BG1"); break;
        case 5: ui->label_CurBGLayerScrollingType->setText("V speed sync wih BG1, H speed: 1/2 of BG1"); break;
        case 6: ui->label_CurBGLayerScrollingType->setText("H and V speed sync wih BG1"); break;
        case 7: ui->label_CurBGLayerScrollingType->setText("H autoscroll (to left, top half only): 1/8 of BG1"); break;
        default: ui->label_CurBGLayerScrollingType->setText("Unknown");
    }
}

/// <summary>
/// Slot function for spinBox_Layer0MappingType_valueChanged.
/// </summary>
/// <param name="arg1">
/// The spinbox value.
/// </param>
void RoomConfigDialog::on_spinBox_Layer0MappingType_valueChanged(int arg1)
{
    switch(arg1)
    {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D:
    case 0x0E:
    case 0x0F:
    {
        ui->label_CurLayer0MappingType->setText("Disabled"); break;
    }
    case 0x10:
//    case 0x11:
//    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
    {
        ui->spinBox_Layer0Width->setValue(ui->SpinBox_RoomWidth->value());
        ui->spinBox_Layer0Height->setValue(ui->SpinBox_RoomHeight->value());
        ui->label_CurLayer0MappingType->setText("Map16");
        break;
    }
    case 0x11: ui->label_CurLayer0MappingType->setText("Map16 & The Big Board result Bar control"); break;
    case 0x12: ui->label_CurLayer0MappingType->setText("Map16 & asyn cam-based H-scroll"); break;
    case 0x20:
//    case 0x21:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x2B:
    case 0x2C:
    case 0x2D:
    case 0x2E:
    case 0x2F:
    {
        ui->label_CurLayer0MappingType->setText("Tile8x8"); break;
    }
    case 0x21: ui->label_CurLayer0MappingType->setText("Tile8x8 & Layer 0 temp ASC-BOSS"); break;
    case 0x22: ui->label_CurLayer0MappingType->setText("Tile8x8 & autoscroll"); break;
    }

    int BGptr = ui->ComboBox_BGLayerPicker->currentText().toUInt(nullptr, 16);
    int L0ptr = ui->ComboBox_Layer0Picker->currentText().toUInt(nullptr, 16);
    if (arg1 >= LevelComponents::LayerMap16) // Enable L0
    {
        ui->CheckBox_Layer0Alpha->setEnabled(true);
        if(arg1 >= LevelComponents::LayerMap16 && arg1 < LevelComponents::LayerTile8x8) // Map16
        {
            ui->spinBox_Layer0Width->setEnabled(true);
            ui->spinBox_Layer0Height->setEnabled(true);
            ui->spinBox_Layer0Width->setValue(ui->SpinBox_RoomWidth->value());
            ui->spinBox_Layer0Height->setValue(ui->SpinBox_RoomHeight->value());
            ui->ComboBox_Layer0Picker->setEnabled(false);
            ui->graphicsView->UpdateGraphicsItems(currentTileset, BGptr, 0);
        } else if (arg1 >= LevelComponents::LayerTile8x8) { //Map8
            ui->spinBox_Layer0Width->setEnabled(false);
            ui->spinBox_Layer0Height->setEnabled(false);
            ui->ComboBox_Layer0Picker->setEnabled(true);
            ui->graphicsView->UpdateGraphicsItems(currentTileset, BGptr, L0ptr);
        }
    }
    else // Disable L0
    {
        ui->spinBox_Layer0Width->setEnabled(false);
        ui->spinBox_Layer0Height->setEnabled(false);
        ui->ComboBox_Layer0Picker->setEnabled(false);
        ui->graphicsView->UpdateGraphicsItems(currentTileset, BGptr, 0);
        ui->CheckBox_Layer0Alpha->setChecked(false);
        ui->CheckBox_Layer0Alpha->setEnabled(false);
    }
}

/// <summary>
/// Slot function for spinBox_RasterType.
/// </summary>
/// <param name="arg1">
/// The spinbox value.
/// </param>
void RoomConfigDialog::on_spinBox_RasterType_valueChanged(int arg1)
{
    switch(arg1)
    {
    case 0x00: ui->label_CurRasterType->setText("No Raster type effect"); break;
    case 0x01: ui->label_CurRasterType->setText("Layer 3 Water effect 1"); break;
    case 0x02: ui->label_CurRasterType->setText("Layer 3 Water effect 2"); break;
    case 0x03: ui->label_CurRasterType->setText("Layer 0 Fog effect"); break;
    case 0x04: ui->label_CurRasterType->setText("Layer 3 Fire effect 1"); break;
    case 0x05: ui->label_CurRasterType->setText("Layer 3 Fire effect 2"); break;
    case 0x06: ui->label_CurRasterType->setText("Layer 3 DOUBLE-SCR(A): top AUTO(1/8),bottom(non)"); break;
    case 0x07: ui->label_CurRasterType->setText("Layer 3 DOUBLE-SCR(A): top(non),bottom AUTO(1/8)"); break;
    case 0x08: ui->label_CurRasterType->setText("alpha Fire effect 1"); break;
    case 0x09: ui->label_CurRasterType->setText("alpha Fire effect 2"); break;
    default: ui->label_CurRasterType->setText("Undefined");
    }
}

/// <summary>
/// Reset ComboBox_BGLayerPicker with available items.
/// </summary>
/// <param name="newTilesetId">
/// The tileset id to generate items.
/// </param>
void RoomConfigDialog::ResetBGLayerPickerComboBox(int newTilesetId)
{
    // if the tileset is using some vanilla bg tile8x8 set
    unsigned int bgtiledataAddr = ROMUtils::singletonTilesets[newTilesetId]->GetbgGFXptr();
    BGLayerdataPtrs.clear();
    bool find_using_vanilla_bg_tiledata = false;
    int find_using_vanilla_bg_from_vanilla_tileset_id = -1;
    for ( int i = 0; i < (sizeof(VanillaTilesetBGTilesDataAddr) / sizeof(VanillaTilesetBGTilesDataAddr[0])); i++)
    {
        if (bgtiledataAddr == VanillaTilesetBGTilesDataAddr[i])
        {
            find_using_vanilla_bg_tiledata = true;
            find_using_vanilla_bg_from_vanilla_tileset_id = i;
            break;
        }
    }
    if (find_using_vanilla_bg_tiledata)
    {
        // Initialize the selections for the current tileset's available BGs
        // it is possible that user set a bg tileset from another vanilla tileset
        // so we need to go thru all the data and find the suitable BG Layer data pointer(s) to push into BGLayerdataPtrs
        int curTilesetId = 0;
        int count = 0;
        int graphicNum = 0;
        while (curTilesetId != find_using_vanilla_bg_from_vanilla_tileset_id)
        {
            graphicNum = BGLayerdataPtrsData[count];
            count += (graphicNum + 1);
            curTilesetId++;
        }
        graphicNum = BGLayerdataPtrsData[count++];
        if (graphicNum > 0)
        {
            for (int i = 0; i < graphicNum; i++)
            {
                BGLayerdataPtrs.push_back(BGLayerdataPtrsData[count + i]);
            }
        }
        bool find_default_ptr = false;
        if (int num = BGLayerdataPtrs.size())
        {
            for (int i = 0; i < num; i++)
            {
                if (BGLayerdataPtrs[i] == WL4Constants::BGLayerDefaultPtr)
                {
                    find_default_ptr = true;
                    break;
                }
            }
        }
        if (!find_default_ptr)
        {
            BGLayerdataPtrs.push_back(WL4Constants::BGLayerDefaultPtr);
        }
    }
    else/* if (bgtiledataAddr >= WL4Constants::AvailableSpaceBeginningInROM)*/
    {   // use custom tile data
        QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> graphicEntries = AssortedGraphicUtils::GetAssortedGraphicsFromROM();

        // some bug case should never happen
        if (!graphicEntries.size())
        {
            QMessageBox::information(this, tr("Error"), tr("Unknown BG tile data pointer!"));
            return;
        }

        // search through graphic entries
        for (int i = 0; i < graphicEntries.size(); i++)
        {
            if (bgtiledataAddr == graphicEntries[i].TileDataAddress &&
                    graphicEntries[i].TileDataType == AssortedGraphicUtils::Tile8x8_4bpp_no_comp_Tileset_text_bg &&
                    graphicEntries[i].MappingDataCompressType == AssortedGraphicUtils::RLE_mappingtype_0x20)
            {
                BGLayerdataPtrs.push_back(graphicEntries[i].MappingDataAddress);
            }
        }
        BGLayerdataPtrs.push_back(WL4Constants::BGLayerDefaultPtr);
    }

    // update ComboBox_BGLayerPicker
    ui->ComboBox_BGLayerPicker->clear();
    QStringList elements;
    if (BGLayerdataPtrs.size())
    {
        for (auto item : BGLayerdataPtrs)
        {
            elements << QString::number(item, 16).toUpper();
        }
    }
    ui->ComboBox_BGLayerPicker->clear();
    ui->ComboBox_BGLayerPicker->addItems(elements);

    // TODO: deal with layer 0 edge cases
    // when Layer 0 mnapping type is 0x20, it uses the bg tiles too
    // Initialize the Tileset list which contains map8x8 layer 0
}

void RoomConfigDialog::on_spinBox_Layer2MappingType_valueChanged(int arg1)
{
    switch(arg1)
    {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
        case 0x0F:
        {
            ui->label_CurLayer2MappingType->setText("Disabled"); break;
        }
        case 0x10:
        case 0x11:
        case 0x12:
//        case 0x13: // we define it below
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
        {
            ui->label_CurLayer2MappingType->setText("Map16");
            break;
        }
        case 0x13: ui->label_CurLayer2MappingType->setText("Map16 & Boss Room Layer 2 X shifting"); break;
    }
}

