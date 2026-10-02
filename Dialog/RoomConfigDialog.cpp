#include "RoomConfigDialog.h"
#include "ui_RoomConfigDialog.h"

#include <cstring>
#include <QMessageBox>
#include "AssortedGraphicUtils.h"
#include "WL4EditorWindow.h"

extern WL4EditorWindow *singleton;

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
unsigned short *RoomConfigDialog::ChangeLayerDimensions(int newWidth, int newHeight, int oldWidth, int oldHeight, unsigned short *oldData)
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
/// Check if a Layer 0 mapping type parameter makes Layer 0 use a Tile8x8 mapping data.
/// </summary>
/// <param name="layer0MappingTypeParam">
/// The Layer 0 mapping type parameter value.
/// </param>
/// <returns>
/// Return true if Layer 0 uses a Tile8x8 mapping data (mapping type 0x20 to 0x2F).
/// </returns>
bool RoomConfigDialog::IsLayer0Tile8x8MappingType(int layer0MappingTypeParam)
{
    return (layer0MappingTypeParam >= LevelComponents::LayerTile8x8) && (layer0MappingTypeParam <= 0x2F);
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
    ui->ComboBox_Layer0Picker->setEnabled(false); // enabled again after the picker is filled below
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

    // Initialize the items for the BG selection combobox and the Layer 0 selection combobox.
    // Both pickers share one list of the mapping data usable by the background Tile8x8 set of the
    // current Tileset, because a mapping data built on that Tile8x8 set works for Layer 0 and
    // Layer 3 alike. The current layer data pointers are kept selectable even when the list does
    // not contain them.
    // The data pointer of the live Layer instance is the pointer the Room uses, the Layer0Data and
    // Layer3Data fields of the room header are only refreshed when the Room is loaded or reset, so
    // the Layer instance has to be looked up here. A Layer 0 data pointer is only meaningful when
    // Layer 0 uses a Tile8x8 mapping data, a Map16 data pointer cannot be used by the picker.
    LevelComponents::Room *currentRoom = singleton->GetCurrentLevel()->GetRooms()[CurrentRoomParams->roomID];
    CurrentBGLayerPtr = (unsigned int) CurrentRoomParams->BackgroundLayerDataPtr;
    CurrentLayer0Ptr = (IsLayer0Tile8x8MappingType(CurrentRoomParams->Layer0MappingTypeParam) && currentRoom)
            ? (unsigned int) currentRoom->GetLayer(0)->GetDataPtr() : 0;
    ResetBGLayerPickerComboBox(CurrentRoomParams->CurrentTilesetIndex);
    UpdateLayer0PickerAvailability(CurrentRoomParams->CurrentTilesetIndex);

    // Initialize the graphic view layers
    ui->graphicsView->infoLabel = ui->graphicViewDetailsLabel;
    currentTileset = ROMUtils::singletonTilesets[CurrentRoomParams->CurrentTilesetIndex];
    int L0ptr = IsLayer0Tile8x8MappingType(ui->spinBox_Layer0MappingType->value()) ? CurrentRoomParams->Layer0DataPtr : 0;
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
    configParams->roomID = prevRoomParams->roomID;

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
    else if (IsLayer0Tile8x8MappingType(configParams->Layer0MappingTypeParam))
    {
        configParams->Layer0Width = configParams->Layer0Height = 0;
        configParams->Layer0DataPtr = ui->ComboBox_Layer0Picker->currentData().toUInt();
        if (!configParams->Layer0DataPtr)
        {
            // The picker cannot show any layer data, which happens when the current one is
            // unknown, so keep the data pointer the Room currently uses
            LevelComponents::Room *currentRoom = singleton->GetCurrentLevel()->GetRooms()[prevRoomParams->roomID];
            if (currentRoom && (currentRoom->GetLayer(0)->GetMappingType() == LevelComponents::LayerTile8x8))
            {
                configParams->Layer0DataPtr = (int) currentRoom->GetLayer(0)->GetDataPtr();
            }
        }
    }
    else
    {
        // Layer 0 mapping types out of the 0x20 to 0x2F range use no mapping data
        configParams->Layer0DataPtr = configParams->Layer0Width = configParams->Layer0Height = 0;
    }

    configParams->Layer2MappingTypeParam = ui->spinBox_Layer2MappingType->value();
    configParams->LayerPriorityAndAlphaAttr = ui->ComboBox_LayerPriority->currentIndex() + 4;
    configParams->LayerPriorityAndAlphaAttr += (qMax(ui->ComboBox_AlphaBlendAttribute->currentIndex(), 0) << 2);
    configParams->BackgroundLayerEnable = ui->CheckBox_BGLayerEnable->isChecked();
    configParams->BGLayerScrollFlag = ui->spinBox_BGLayerScrollingFlag->value();
    if (configParams->BackgroundLayerEnable)
    {
        configParams->BackgroundLayerDataPtr = ui->ComboBox_BGLayerPicker->currentData().toUInt();
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

        // Update the available BG layers and Layer 0 data to choose from
        ResetBGLayerPickerComboBox(index);

        // Update the available FG layers to choose from
        UpdateLayer0PickerAvailability(index);

        int BGptr = ui->ComboBox_BGLayerPicker->currentData().toUInt();
        int L0ptr = ui->ComboBox_Layer0Picker->currentData().toUInt();
        if ((ui->spinBox_Layer0MappingType->value() & 0x20) == 0)
            L0ptr = 0;
        ui->graphicsView->UpdateGraphicsItems(currentTileset, BGptr, L0ptr);
    }
}

/// <summary>
/// Set whether "Use existing Layer 0" can be chosen with the current Tileset and Layer 0 mapping type.
/// </summary>
/// <param name="tilesetId">
/// The id of the current Tileset.
/// </param>
void RoomConfigDialog::UpdateLayer0PickerAvailability(int tilesetId)
{
    (void) tilesetId;
    // Layer 0 needs a Tile8x8 mapping data, and the picker lists the mapping data of the Tileset
    // background Tile8x8 set, so any mapping data of the list can be chosen when the current
    // Layer 0 mapping type is a Tile8x8 one
    bool usable = IsLayer0Tile8x8MappingType(ui->spinBox_Layer0MappingType->value())
            && ui->ComboBox_Layer0Picker->count();
    ui->ComboBox_Layer0Picker->setEnabled(usable);
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
        if (ui->ComboBox_BGLayerPicker->currentIndex() >= 0)
        {
            CurrentBGLayerPtr = ui->ComboBox_BGLayerPicker->currentData().toUInt();
        }
        int BGptr = ui->ComboBox_BGLayerPicker->currentData().toUInt();
        int L0ptr = ui->ComboBox_Layer0Picker->currentData().toUInt();
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
        if (ui->ComboBox_Layer0Picker->currentIndex() >= 0)
        {
            CurrentLayer0Ptr = ui->ComboBox_Layer0Picker->currentData().toUInt();
        }
        int BGptr = ui->ComboBox_BGLayerPicker->currentData().toUInt();
        int L0ptr = ui->ComboBox_Layer0Picker->currentData().toUInt();
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

    int BGptr = ui->ComboBox_BGLayerPicker->currentData().toUInt();
    int L0ptr = ui->ComboBox_Layer0Picker->currentData().toUInt();
    if (arg1 >= LevelComponents::LayerMap16) // Enable L0
    {
        ui->CheckBox_Layer0Alpha->setEnabled(true);
        if(arg1 >= LevelComponents::LayerMap16 && arg1 < LevelComponents::LayerTile8x8) // Map16
        {
            ui->spinBox_Layer0Width->setEnabled(true);
            ui->spinBox_Layer0Height->setEnabled(true);
            ui->spinBox_Layer0Width->setValue(ui->SpinBox_RoomWidth->value());
            ui->spinBox_Layer0Height->setValue(ui->SpinBox_RoomHeight->value());
        }
        else if (IsLayer0Tile8x8MappingType(arg1)) // Map8
        {
            ui->spinBox_Layer0Width->setEnabled(false);
            ui->spinBox_Layer0Height->setEnabled(false);
        }
        else
        {
            // Layer 0 mapping types out of the 0x20 to 0x2F range cannot use a mapping data
            ui->spinBox_Layer0Width->setEnabled(false);
            ui->spinBox_Layer0Height->setEnabled(false);
        }
        UpdateLayer0PickerAvailability(ui->ComboBox_TilesetID->currentIndex());
        ui->graphicsView->UpdateGraphicsItems(currentTileset, BGptr, IsLayer0Tile8x8MappingType(arg1) ? L0ptr : 0);
    }
    else // Disable L0
    {
        ui->spinBox_Layer0Width->setEnabled(false);
        ui->spinBox_Layer0Height->setEnabled(false);
        UpdateLayer0PickerAvailability(ui->ComboBox_TilesetID->currentIndex());
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
/// Collect the mapping data addresses usable by the background Tile8x8 set of a Tileset.
/// </summary>
/// <remarks>
/// A mapping data built on the background Tile8x8 set of the Tileset draws the same tiles for
/// Layer 0 and Layer 3, so both "Use existing Layer 0" and "Use existing Background Layer" list the
/// same addresses. Graphic entries sharing one Tile8x8 set (the duplicated entries made by the
/// Graphic Manager for example) all contribute their own mapping data address, since their Tile8x8
/// data are several copies in the ROM which must not be merged.
/// </remarks>
/// <param name="newTilesetId">
/// The tileset id which provides the background Tile8x8 set.
/// </param>
/// <returns>
/// The mapping data addresses in use order, the addresses of the vanilla background mapping data
/// included.
/// </returns>
QVector<unsigned int> RoomConfigDialog::FindLayerMappingDataAddresses(int newTilesetId)
{
    BGLayerdataPtrs.clear();

    unsigned int bgtiledataAddr = ROMUtils::singletonTilesets[newTilesetId]->GetbgGFXptr();

    // go through all the vanilla background tile data pointer and see if the current Tileset is using several of them
    // push all the available background mapping data pointer into the BGLayerdataPtrsData[] as long as the bg tile data pointer matches
    for ( int i = 0; i < (sizeof(VanillaTilesetBGTilesDataAddr) / sizeof(VanillaTilesetBGTilesDataAddr[0])); i++)
    {
        if (bgtiledataAddr == VanillaTilesetBGTilesDataAddr[i])
        {
            int curTilesetId = 0;
            int count = 0;
            int graphicNum = 0;
            while (curTilesetId != i)
            {
                graphicNum = BGLayerdataPtrsData[count];
                count += (graphicNum + 1);
                curTilesetId++;
            }
            graphicNum = BGLayerdataPtrsData[count++];
            if (graphicNum > 0)
            {
                for (int j = 0; j < graphicNum; j++)
                {
                    std::vector<int>::iterator it = std::find(BGLayerdataPtrs.begin(), BGLayerdataPtrs.end(), BGLayerdataPtrsData[count + j]);
                    if(it == BGLayerdataPtrs.end())
                    {
                        BGLayerdataPtrs.push_back(BGLayerdataPtrsData[count + j]);
                    }
                }
            }
        }
    }

    // graphic entries: a Tile8x8 set can be shared by several entries, which have different ROM
    // data pointers but different mapping data, so all of their mapping data can be listed here
    QVector<unsigned int> sharedMappingDataPtrs =
            AssortedGraphicUtils::FindCompatibleMappingDataAddresses(ROMUtils::singletonTilesets[newTilesetId]);
    for (unsigned int addr : sharedMappingDataPtrs)
    {
        std::vector<int>::iterator it = std::find(BGLayerdataPtrs.begin(), BGLayerdataPtrs.end(), (int) addr);
        if(it == BGLayerdataPtrs.end())
        {
            BGLayerdataPtrs.push_back((int) addr);
        }
    }

    // add the default bg mapping data pointer
    std::vector<int>::iterator it = std::find(BGLayerdataPtrs.begin(), BGLayerdataPtrs.end(), WL4Constants::BGLayerDefaultPtr);
    if(it == BGLayerdataPtrs.end())
    {
        BGLayerdataPtrs.push_back(WL4Constants::BGLayerDefaultPtr);
    }

    QVector<unsigned int> result;
    for (auto item : BGLayerdataPtrs)
    {
        result.push_back((unsigned int) item);
    }
    return result;
}

/// <summary>
/// Reset a layer data picker with the available mapping data addresses.
/// </summary>
/// <param name="picker">
/// The picker to reset.
/// </param>
/// <param name="mappingDataAddresses">
/// The addresses to list in the picker.
/// </param>
/// <param name="currentAddress">
/// The address the picker has to select, which is added into the list when the list does not
/// contain it.
/// </param>
void RoomConfigDialog::PopulateLayerPickerComboBox(QComboBox *picker, const QVector<unsigned int> &mappingDataAddresses,
                                                   unsigned int currentAddress)
{
    picker->clear();
    for (unsigned int addr : mappingDataAddresses)
    {
        picker->addItem(QString::number(addr, 16).toUpper(), addr);
    }
    if (currentAddress && !mappingDataAddresses.contains(currentAddress))
    {
        // keep the layer data the Room currently uses selectable
        picker->addItem(QString::number(currentAddress, 16).toUpper(), currentAddress);
    }

    int selectionId = picker->findData(currentAddress);
    if (selectionId < 0)
    {
        // there is no layer data of the Room in the list, so select the first usable one
        selectionId = 0;
    }
    picker->setCurrentIndex(selectionId);
}

/// <summary>
/// Reset ComboBox_BGLayerPicker and ComboBox_Layer0Picker with available items.
/// </summary>
/// <param name="newTilesetId">
/// The tileset id to generate items.
/// </param>
void RoomConfigDialog::ResetBGLayerPickerComboBox(int newTilesetId)
{
    QVector<unsigned int> mappingDataAddresses = FindLayerMappingDataAddresses(newTilesetId);

    // update ComboBox_BGLayerPicker
    PopulateLayerPickerComboBox(ui->ComboBox_BGLayerPicker, mappingDataAddresses, CurrentBGLayerPtr);

    // update ComboBox_Layer0Picker, the vanilla dusty Layer 0 data are usable by Layer 0 only
    QVector<unsigned int> layer0MappingDataAddresses = mappingDataAddresses;
    unsigned int bgtiledataaddr = ROMUtils::singletonTilesets[newTilesetId]->GetbgGFXptr();
    if(bgtiledataaddr == WL4Constants::Tileset_BGTile_0x21)
    {
        layer0MappingDataAddresses.push_back(WL4Constants::ToxicLandfillDustyLayer0Ptr);
    }
    else if(bgtiledataaddr == WL4Constants::Tileset_BGTile_0x45)
    {
        layer0MappingDataAddresses.push_back(WL4Constants::FieryCavernDustyLayer0Ptr);
    }
    PopulateLayerPickerComboBox(ui->ComboBox_Layer0Picker, layer0MappingDataAddresses, CurrentLayer0Ptr);
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

