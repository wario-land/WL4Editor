#include "RoomConfigDialog.h"
#include "ui_RoomConfigDialog.h"

#include <cstring>
#include <cstdlib>

RoomConfigDialog::RoomConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RoomConfigDialog)
{
    ui->setupUi(this);
}

RoomConfigDialog::~RoomConfigDialog()
{
    delete ui;
}

RoomConfigDialog::InitDialog()
{
    currentParams = (struct DialogParams::RoomConfigParams *) malloc(sizeof(struct DialogParams::RoomConfigParams));
    memcpy(currentParams, CurrentRoomParams, sizeof(struct DialogParams::RoomConfigParams));
    InitComboBoxItems();
    ui->ComboBox_AlphaBlendAttribute->setCurrentIndex(0);
    ui->ComboBox_Layer0MappingType->setCurrentIndex(0);
    // TODO

}

RoomConfigDialog::InitDialog(DialogParams::RoomConfigParams *CurrentRoomParams)
{
    currentParams = (struct DialogParams::RoomConfigParams *) malloc(sizeof(struct DialogParams::RoomConfigParams));
    memcpy(currentParams, CurrentRoomParams, sizeof(struct DialogParams::RoomConfigParams));
    InitComboBoxItems();
    SetBGLayerdataPtrs();
    ui->ComboBox_TilesetID->setCurrentIndex(CurrentRoomParams->CurrentTilesetIndex);
    ui->CheckBox_Layer0Enable->setChecked(CurrentRoomParams->Layer0Enable);
    ui->CheckBox_Layer0Alpha->setChecked(CurrentRoomParams->Layer0Alpha);
    int LayerPriorityID = CurrentRoomParams->LayerPriorityAndAlphaAttr & 3;
    ui->ComboBox_LayerPriority->setCurrentIndex((LayerPriorityID < 2) ? LayerPriorityID : (LayerPriorityID - 1));
    int AlphaBlendID = (CurrentRoomParams->LayerPriorityAndAlphaAttr & 0x78) >> 3;
    ui->ComboBox_AlphaBlendAttribute->setCurrentIndex(AlphaBlendID);
    if(CurrentRoomParams->Layer0Enable) ui->ComboBox_Layer0MappingType->setCurrentIndex(((CurrentRoomParams->Layer0MappingTypeParam) & 0x30) >> 4);
    if(CurrentRoomParams->Layer0MappingTypeParam == 0x22) ui->CheckBox_Layer0AutoScroll->setChecked(true);
    ui->ComboBox_Layer0Picker->setEnabled((CurrentRoomParams->Layer0MappingTypeParam >= 0x20) ? true : false);
    ui->SpinBox_RoomWidth->setValue(CurrentRoomParams->RoomWidth);
    ui->SpinBox_RoomHeight->setValue(CurrentRoomParams->RoomHeight);
    ui->CheckBox_Layer2Enable->setChecked(CurrentRoomParams->Layer2Enable);
    ui->CheckBox_BGLayerEnable->setChecked(CurrentRoomParams->BackgroundLayerEnable);
    ui->CheckBox_BGLayerAutoScroll->setChecked(CurrentRoomParams->BackgroundLayerAutoScrollEnable);
    // TODO
}

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

    ui->graphicsView->repaint();
    ui->graphicsView->setScene(tmpGraphicviewScene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    delete tmpTileset;
}

void RoomConfigDialog::ShowMappingType20LayerDetails(int _layerdataAddr, LevelComponents::Layer *_tmpLayer)
{
    // TODO: Put these 2 lines somewhere else
    if(_tmpLayer != nullptr) delete _tmpLayer;
    _tmpLayer = new LevelComponents::Layer(_layerdataAddr, LevelComponents::LayerTile8x8);

    int _tilesetPtr = WL4Constants::TilesetDataTable + currentParams->CurrentTilesetIndex * 36;
    LevelComponents::Tileset *tmpTileset = new LevelComponents::Tileset(_tilesetPtr, currentParams->CurrentTilesetIndex);
    QPixmap tmplayerpixmap = _tmpLayer->RenderLayer(tmpTileset);
    int sceneWidth = 64 * 16;
    int sceneHeight = 64 * 16;
    if(tmpGraphicviewScene != nullptr) delete tmpGraphicviewScene;
    tmpGraphicviewScene = new QGraphicsScene(0, 0, sceneWidth, sceneHeight);
    tmpGraphicviewScene->addPixmap(tmplayerpixmap);
    ui->graphicsView->repaint();
    ui->graphicsView->setScene(tmpGraphicviewScene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    delete tmpTileset;
}

void RoomConfigDialog::on_CheckBox_Layer0Enable_stateChanged(int arg1)
{
    (void) arg1;
    if(ui->CheckBox_Layer0Enable->isChecked())
    {
        ui->CheckBox_Layer0Alpha->setEnabled(true);
        ui->ComboBox_Layer0MappingType->setEnabled(true);
    }else{
        ui->CheckBox_Layer0Alpha->setChecked(false);
        ui->CheckBox_Layer0Alpha->setEnabled(false);
        ui->ComboBox_Layer0MappingType->setEnabled(false);
        currentParams->Layer0MappingTypeParam = 0;
    }
}

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

void RoomConfigDialog::on_ComboBox_Layer0MappingType_currentIndexChanged(int index)
{
    (void) index;
    currentParams->Layer0MappingTypeParam = ui->ComboBox_Layer0MappingType->currentIndex() << 4;
    if(ui->ComboBox_Layer0MappingType->currentIndex() == 0)
    {
        ui->CheckBox_Layer0AutoScroll->setChecked(false);
        ui->CheckBox_Layer0AutoScroll->setEnabled(false);
        ui->ComboBox_Layer0Picker->setEnabled(false);
        currentParams->Layer0MappingTypeParam = LevelComponents::LayerMap16;
        currentParams->Layer0DataPtr = 0;
    }else{
        ui->CheckBox_Layer0AutoScroll->setEnabled(true);
        ui->ComboBox_Layer0Picker->setEnabled(true);
        currentParams->Layer0MappingTypeParam = LevelComponents::LayerTile8x8;
    }
}

void RoomConfigDialog::on_CheckBox_Layer0AutoScroll_stateChanged(int arg1)
{
    (void) arg1;
    if(ui->CheckBox_Layer0AutoScroll->isChecked()) currentParams->Layer0MappingTypeParam = 0x22;
}

void RoomConfigDialog::on_ComboBox_AlphaBlendAttribute_currentIndexChanged(int index)
{
    (void) index;
    currentParams->LayerPriorityAndAlphaAttr = (currentParams->LayerPriorityAndAlphaAttr & 3) | (ui->ComboBox_AlphaBlendAttribute->currentIndex() << 3);
}

void RoomConfigDialog::on_ComboBox_TilesetID_currentIndexChanged(int index)
{
    (void) index;
    currentParams->CurrentTilesetIndex = ui->ComboBox_TilesetID->currentIndex();
    ShowTilesetDetails();
}

void RoomConfigDialog::on_SpinBox_RoomWidth_valueChanged(int arg1)
{
    currentParams->RoomWidth = arg1;
}

void RoomConfigDialog::on_SpinBox_RoomHeight_valueChanged(int arg1)
{
    currentParams->RoomHeight = arg1;
}

void RoomConfigDialog::on_CheckBox_Layer2Enable_stateChanged(int arg1)
{
    (void) arg1;
    currentParams->Layer2Enable = ui->CheckBox_Layer2Enable->isChecked();
}

void RoomConfigDialog::on_CheckBox_BGLayerAutoScroll_stateChanged(int arg1)
{
    (void) arg1;
    currentParams->BackgroundLayerAutoScrollEnable = ui->CheckBox_BGLayerAutoScroll->isChecked();
}

void RoomConfigDialog::on_CheckBox_BGLayerEnable_stateChanged(int arg1)
{
    (void) arg1;
    currentParams->BackgroundLayerEnable = ui->CheckBox_BGLayerEnable->isChecked();
    ui->ComboBox_BGLayerPicker->setEnabled(ui->CheckBox_BGLayerEnable->isChecked() & (BGLayerdataPtrs[currentParams->CurrentTilesetIndex][0] != 0));
    ui->CheckBox_BGLayerAutoScroll->setEnabled(ui->CheckBox_BGLayerEnable->isChecked());
    if(ui->CheckBox_BGLayerEnable->isChecked() == 0)
        currentParams->BackgroundLayerDataPtr = WL4Constants::BGLayerDisableDefaultPtr;
    if(ui->ComboBox_BGLayerPicker->count() > 0)
    {
        for(int i = ui->ComboBox_BGLayerPicker->count() - 1; i >= 0 ; i++)
            ui->ComboBox_BGLayerPicker->removeItem(i);
    }
    for(int i = 0; i < 3; i++)
    {
        if(BGLayerdataPtrs[currentParams->CurrentTilesetIndex][i] != 0)
            ui->ComboBox_BGLayerPicker->addItem(QString::number(BGLayerdataPtrs[currentParams->CurrentTilesetIndex][i], 16).toUpper());
    }
}

void RoomConfigDialog::on_ComboBox_LayerPriority_currentIndexChanged(int index)
{
    (void) index;
    if(ui->ComboBox_LayerPriority->currentIndex() >= 0)
        currentParams->LayerPriorityAndAlphaAttr = (currentParams->LayerPriorityAndAlphaAttr & 0x78) | ((ui->ComboBox_LayerPriority->currentIndex() < 2) ? ui->ComboBox_LayerPriority->currentIndex(): 3);
}

void RoomConfigDialog::SetBGLayerdataPtrs()
{
    for(int i = 0; i < 0x5C; i++)
        for(int j = 0; j < 3; j++)
            BGLayerdataPtrs[i][j] = 0;
    BGLayerdataPtrs[0x50][0] = BGLayerdataPtrs[0x11][0] = BGLayerdataPtrs[0x47][0] = 0x5FA6D0;
    BGLayerdataPtrs[1][0] = 0x5FB2CC; BGLayerdataPtrs[2][0] = 0x5FB8DC;
    BGLayerdataPtrs[6][0] = 0x5FC9A0; BGLayerdataPtrs[6][1] = 0x5FC2D0;
    BGLayerdataPtrs[0xB][0] = 0x5FD484; BGLayerdataPtrs[0xA][0] = 0x5FD078; BGLayerdataPtrs[0x52][0] = 0x5FD484;
    BGLayerdataPtrs[0x1F][0] = 0x5FD680; BGLayerdataPtrs[0x1F][1] = 0x5FD9BC;
    BGLayerdataPtrs[0x28][0] = 0x5FE540; BGLayerdataPtrs[0x34][0] = 0x5FB8DC; BGLayerdataPtrs[0x51][0] = 0x5FE008;
    BGLayerdataPtrs[0x26][0] = 0x5FE918; BGLayerdataPtrs[0x26][1] = 0x5FEED8;
    BGLayerdataPtrs[0x35][0] = 0x5FF264; BGLayerdataPtrs[0x36][0] = 0x5FF960; BGLayerdataPtrs[0x38][0] = 0x5FF684;
    BGLayerdataPtrs[8][0] = BGLayerdataPtrs[0x3F][0] = 0x5FFD94;
    BGLayerdataPtrs[0x1E][0] = BGLayerdataPtrs[0x40][0] = 0x600EF8;
    BGLayerdataPtrs[0x20][0] = 0x6006C4; BGLayerdataPtrs[0x1D][0] = 0x600388;
    BGLayerdataPtrs[0x21][0] = 0x6013D4; BGLayerdataPtrs[0x22][0] = 0x601A0C;
    BGLayerdataPtrs[0x27][0] = 0x60221C; BGLayerdataPtrs[0x3E][0] = 0x602858;
    BGLayerdataPtrs[0xC][0] = 0x603E98; BGLayerdataPtrs[0x56][0] = 0x605270; BGLayerdataPtrs[7][0] = 0x6045C4; BGLayerdataPtrs[0xE][0] = 0x604ACC;
    BGLayerdataPtrs[3][0] = BGLayerdataPtrs[4][0] = 0x603064; BGLayerdataPtrs[5][0] = 0x60368C;
    BGLayerdataPtrs[0xF][0] = 0x605A7C; BGLayerdataPtrs[0xF][1] = 0x6063B0; BGLayerdataPtrs[0xF][2] = 0x606CF4;
    BGLayerdataPtrs[0x10][0] = 0x6074C4; BGLayerdataPtrs[0x1A][0] = 0x607CD0; BGLayerdataPtrs[0x1B][0] = 0x6084DC; BGLayerdataPtrs[0x1C][0] = 0x608CE8;
    BGLayerdataPtrs[0x13][0] = BGLayerdataPtrs[0x54][0] = BGLayerdataPtrs[0x55][0] = 0x60A1E8;
    BGLayerdataPtrs[0x53][0] = 0x60A350; BGLayerdataPtrs[0x12][0] = 0x60A1D8; BGLayerdataPtrs[0x14][0] = 0x60A4B4;
    BGLayerdataPtrs[0x15][0] = BGLayerdataPtrs[0x17][0] = BGLayerdataPtrs[0x18][0] = BGLayerdataPtrs[0x19][0] = 0x6094F4;
    BGLayerdataPtrs[0x41][0] = 0x609A84; BGLayerdataPtrs[0x2F][0] = 0x60AD10; BGLayerdataPtrs[0x46][0] = 0x60E044;
    BGLayerdataPtrs[0x37][0] = 0x60BC54; BGLayerdataPtrs[0x37][1] = 0x60B29C;
    BGLayerdataPtrs[0x45][0] = 0x60CF98; BGLayerdataPtrs[0x45][1] = 0x60C5FC;
    BGLayerdataPtrs[0x4A][0] = BGLayerdataPtrs[0x4B][0] = BGLayerdataPtrs[0x4C][0] = BGLayerdataPtrs[0x4D][0] = BGLayerdataPtrs[0x4E][0] = BGLayerdataPtrs[0x43][0] = 0x60E96C;
    BGLayerdataPtrs[0x29][0] = BGLayerdataPtrs[0x49][0] = BGLayerdataPtrs[0x44][0] = BGLayerdataPtrs[0x48][0] = BGLayerdataPtrs[0x42][0] = 0x60E860;
    BGLayerdataPtrs[0x4F][0] = 0x60E870; BGLayerdataPtrs[0x39][0] = 0x60ED78;
}

void RoomConfigDialog::InitComboBoxItems()
{
    QStringList TilesetNamesSet;
    TilesetNamesSet << "00  Debug room" << "01  Palm Tree Paradise" << "02  Caves" << "03  The Big Board" << "04  The Big Board" <<
                       "05  The Big Board (indoor)" << "06  Wildflower Fields" << "07  Toy Block Tower" << "08  Factory" << "09  Wildflower Underground" <<
                       "0A  Wildflower WaterPlace" << "0B  Underwater" << "0C  Toy Block Tower" << "0D  Toy Block Tower" << "0E  Toy Block Tower" <<
                       "0F  Doodle woods" << "10  Dominoes" << "11  Hall of Hieroglyphs" << "12  Haunte House" <<
                       "13  Crescent Moon Village outside" << "14  Drain" << "15  Arabian outside" << "16  Arabian inside" << "17  Arabian" <<
                       "18  Arabian" << "19  Arabian" << "1A  Dominoes (blue)" << "1B  Dominoes (purple)" << "1C  Dominoes (teal)" << "1D  Factory" <<
                       "1E  Factory" << "1F  Jungle" << "20  Factory" << "21  Toxic Landfill" << "22  Toxic Landfill" << "23  Pinball" <<
                       "24  Pinball" << "25  Pinball (with Gorilla)" << "26  Jungle" << "27  40 Below Fridge" << "28  Jungle" <<
                       "29  Jungle caves" << "2A  Hotel" << "2B  Hotel" << "2C  Hotel" << "2D  Hotel" << "2E  Hotel" << "2F  Hotel (outside)" <<
                       "30  Unused in-game (Haunted House)" << "31  Unused in-game (Haunted House)" << "32  Unused in-game (Cardboard)" <<
                       "33  Cardboard" << "34  Caves" << "35  Jungle" << "36  Caves" << "37  Lava level" << "38  Caves" << "39  Golden Passage" <<
                       "3A  Hotel" << "3B  Hotel" << "3C  Hotel" << "3D  Hotel" << "3E  40 Below Fridge" << "3F  Factory" << "40  Factory" <<
                       "41  Arabian" << "42  Boss room" << "43  Boss corridor" << "44  Boss room" << "45  Frozen lava level" << "46  Lava level" <<
                       "47  Hall of Hieroglyphs" << "48  Boss room" << "49  Boss room" << "4A  Boss corridor" << "4B  Boss corridor" <<
                       "4C  Boss corridor" << "4D  Boss corridor" << "4E  Boss corridor" << "4F  Boss room (Diva)" << "50  Hall of Hieroglyphs" <<
                       "51  Jungle" << "52  Wildflower" << "53  Crescent Moon Village" << "54  Crescent Moon Village" << "55  Crescent Moon Village" <<
                       "56  Toy Block Tower" << "57  Pinball" << "58  Bonus room" << "59  Bonus room" << "5A  Final level" << "5B  The Big Board end";
    ui->ComboBox_TilesetID->addItems(TilesetNamesSet);
    QStringList LayerPrioritySet;
    LayerPrioritySet << "layer 0 (Top) > layer 1 > layer 2 > Layer 3 (Bottom)" <<
                        "layer 1 (Top) > layer 0 > layer 2 > Layer 3 (Bottom)" <<
                        "layer 1 (Top) > layer 2 > layer 0 > Layer 3 (Bottom)";
    ui->ComboBox_LayerPriority->addItems(LayerPrioritySet);
    QStringList AlphaBlendAttrsSet;
    AlphaBlendAttrsSet << "No Alpha Blending    "
                          "EVA = 0.44;  EVB = 1.00; " <<
                          "EVA = 0.63;  EVB = 1.00; " <<
                          "EVA = 0.81;  EVB = 1.00; " <<
                          "EVA = 1.00;  EVB = 1.00; " <<
                          "EVA = 1.00;  EVB = 0.00; " <<
                          "EVA = 0.81;  EVB = 0.19; " <<
                          "EVA = 0.63;  EVB = 0.37; " <<
                          "EVA = 0.44;  EVB = 0.56; " <<
                          "EVA = 0.31;  EVB = 0.68; " <<
                          "EVA = 0.19;  EVB = 0.81; " <<
                          "EVA = 0.00;  EVB = 1.00; ";
    ui->ComboBox_AlphaBlendAttribute->addItems(AlphaBlendAttrsSet);
    QStringList Layer0MappingTypeParamSet;
    Layer0MappingTypeParamSet << "LayerMap16" << "LayerTile8x8";
    ui->ComboBox_Layer0MappingType->addItems(Layer0MappingTypeParamSet);
}

void RoomConfigDialog::on_ComboBox_BGLayerPicker_currentIndexChanged(int index)
{
    (void) index;
    currentParams->BackgroundLayerDataPtr = ui->ComboBox_BGLayerPicker->currentText().toInt();
}

void RoomConfigDialog::on_ComboBox_Layer0Picker_currentIndexChanged(int index)
{
    (void) index;
    currentParams->Layer0DataPtr = WL4Constants::ToxicLandfillDustyLayer0Ptr;
    ui->CheckBox_Layer0AutoScroll->setEnabled(true);
}
