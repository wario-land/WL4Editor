#include "TilesetEditDialog.h"
#include "ui_TilesetEditDialog.h"

#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

TilesetEditDialog::TilesetEditDialog(QWidget *parent, DialogParams::TilesetEditParams *tilesetEditParam) :
    QDialog(parent),
    ui(new Ui::TilesetEditDialog)
{
    ui->setupUi(this);
    this->tilesetEditParams = tilesetEditParam;
    ui->label_TilesetID->setText("Tileset ID: " + QString::number(tilesetEditParam->currentTilesetIndex, 10));
    ui->graphicsView_TilesetAllTile16->SetCurrentTilesetEditor(this);
    ui->graphicsView_TilesetAllTile8x8->SetCurrentTilesetEditor(this);
    ui->graphicsView_paletteBar->SetCurrentTilesetEditor(this);
    ui->graphicsView_Tile8x8Editor->SetCurrentTilesetEditor(this);

    // render
    RenderInitialization();

    //re-initialize widgets
    SetSelectedTile8x8(0, true);
    SetSelectedTile16(0, true);
    SetSelectedColorId(0);

    HasInitialized = true;
}

/// <summary>
/// Deconstructor of TilesetEditDialog class.
/// </summary>
TilesetEditDialog::~TilesetEditDialog()
{
    delete ui;
}

/// <summary>
/// Set a selected tile16 and update the UI accordingly
/// </summary>
/// <param name="tile16ID">
/// The tile16ID index.
/// </param>
/// <param name="resetscrollbar">
/// Set this to true if you want the editor to set the scrollbar automatically.
/// </param>
void TilesetEditDialog::SetSelectedTile16(int tile16ID, bool resetscrollbar)
{
    IsSelectingTile16 = true;

    // Paint red Box to show selected Tile16
    int X = tile16ID & 7;
    int Y = tile16ID >> 3;
    SelectionBox_Tile16->setPos(X * 16, Y * 16);
    SelectionBox_Tile16->setVisible(true);
    SelectedTile16 = tile16ID;

    // Set vertical scrollbar of braphicview
    if (resetscrollbar)
    {
        ui->graphicsView_TilesetAllTile16->verticalScrollBar()->setValue(16 * (tile16ID / 16));
        ui->graphicsView_TilesetAllTile16->horizontalScrollBar()->setValue(0);
    }

    ui->spinBox->setValue(tile16ID);
    ui->spinBox_EventId->setValue(tilesetEditParams->newTileset->GetEventTablePtr()[tile16ID]);
    ui->spinBox_TerrainId->setValue(tilesetEditParams->newTileset->GetTerrainTypeIDTablePtr()[tile16ID]);
    LevelComponents::TileMap16* tile16Data=tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID];
    LevelComponents::Tile8x8* tile8_TL=tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPLEFT);
    LevelComponents::Tile8x8* tile8_TR=tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPRIGHT);
    LevelComponents::Tile8x8* tile8_BL=tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMLEFT);
    LevelComponents::Tile8x8* tile8_BR=tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMRIGHT);

    SetSpinboxesTile8x8sInfo(tile8_TL,ui->spinBox_TopLeftTileId,ui->spinBox_TopLeftpaletteId,ui->checkBox_TopLeftHFlip,ui->checkBox_TopLeftVFlip);
    SetSpinboxesTile8x8sInfo(tile8_TR,ui->spinBox_TopRightTileId,ui->spinBox_TopRightpaletteId,ui->checkBox_TopRightHFlip,ui->checkBox_TopRightVFlip);
    SetSpinboxesTile8x8sInfo(tile8_BL,ui->spinBox_BottomLeftTileId,ui->spinBox_BottomLeftpaletteId,ui->checkBox_BottomLeftHFlip,ui->checkBox_BottomLeftVFlip);
    SetSpinboxesTile8x8sInfo(tile8_BR,ui->spinBox_BottomRightTileId,ui->spinBox_BottomRightpaletteId,ui->checkBox_BottomRightHFlip,ui->checkBox_BottomRightVFlip);

    IsSelectingTile16 = false;
}

/// <summary>
/// Put a tile8 on a line of spinbox and checkbox
/// </summary>
/// <param name="tile8">
/// The tile8 object.
/// </param>
/// <param name="spinBoxID">
/// The spinbox needed to select the id of the tile8.
/// </param>
/// <param name="spinBoxPaletteID">
/// The spinbox needed to select the palette of the tile8.
/// </param>
/// <param name="checkBoxHFlip">
/// The checkbox that determine if there is a horizontal flip on hte selected tile
/// </param>
/// <param name="checkBoxVFlip">
/// The checkbox that determine if there is a vertical flip on hte selected tile
/// </param>
void TilesetEditDialog::SetSpinboxesTile8x8sInfo(LevelComponents::Tile8x8* tile8, QSpinBox* spinBoxID, QSpinBox* spinBoxPaletteID, QCheckBox* checkBoxHFlip, QCheckBox* checkBoxVFlip)
{
    spinBoxID->setValue(tile8->GetIndex());
    spinBoxPaletteID->setValue(tile8->GetPaletteIndex());
    checkBoxHFlip->setChecked(tile8->GetFlipX());
    checkBoxVFlip->setChecked(tile8->GetFlipY());
}

/// <summary>
/// Set Tile16 Palette id for all the 4 TIle8x8 in it
/// </summary>
/// <param name="tile16ID">
/// Tile16's Id which are going to be set.
/// </param>
void TilesetEditDialog::SetTile16PaletteId(int tile16ID)
{
    UpdateATile8x8ForSelectedTile16InTilesetData(tile16ID,
                                                 tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID]->GetTile8X8(0)->GetIndex(),
                                                 0,
                                                 ui->spinBox_paletteBrushValue->value(),
                                                 tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID]->GetTile8X8(0)->GetFlipX(),
                                                 tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID]->GetTile8X8(0)->GetFlipY());
    UpdateATile8x8ForSelectedTile16InTilesetData(tile16ID,
                                                 tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID]->GetTile8X8(1)->GetIndex(),
                                                 1,
                                                 ui->spinBox_paletteBrushValue->value(),
                                                 tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID]->GetTile8X8(1)->GetFlipX(),
                                                 tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID]->GetTile8X8(1)->GetFlipY());
    UpdateATile8x8ForSelectedTile16InTilesetData(tile16ID,
                                                 tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID]->GetTile8X8(2)->GetIndex(),
                                                 2,
                                                 ui->spinBox_paletteBrushValue->value(),
                                                 tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID]->GetTile8X8(2)->GetFlipX(),
                                                 tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID]->GetTile8X8(2)->GetFlipY());
    UpdateATile8x8ForSelectedTile16InTilesetData(tile16ID,
                                                 tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID]->GetTile8X8(3)->GetIndex(),
                                                 3,
                                                 ui->spinBox_paletteBrushValue->value(),
                                                 tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID]->GetTile8X8(3)->GetFlipX(),
                                                 tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID]->GetTile8X8(3)->GetFlipY());
}

void TilesetEditDialog::on_spinBox_valueChanged(int arg1)
{
    if(!HasInitialized || IsSelectingTile16) return;
    SetSelectedTile16(arg1, true);
    SelectedTile16 = (unsigned short) arg1;
}

void TilesetEditDialog::on_spinBox_EventId_valueChanged(int arg1)
{
    if(!HasInitialized || IsSelectingTile16) return;
    tilesetEditParams->newTileset->GetEventTablePtr()[SelectedTile16] = (unsigned short) arg1;
}

void TilesetEditDialog::on_spinBox_TerrainId_valueChanged(int arg1)
{
    if(!HasInitialized || IsSelectingTile16) return;
    tilesetEditParams->newTileset->GetTerrainTypeIDTablePtr()[SelectedTile16] = (unsigned char) arg1;
}

void TilesetEditDialog::on_spinBox_TopLeftTileId_valueChanged(int arg1)
{
    if(!HasInitialized || IsSelectingTile16) return;
    TLTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_TopRightTileId_valueChanged(int arg1)
{
    if(!HasInitialized || IsSelectingTile16) return;
    TRTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_BottomLeftTileId_valueChanged(int arg1)
{
    if(!HasInitialized || IsSelectingTile16) return;
    BLTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_BottomRightTileId_valueChanged(int arg1)
{
    if(!HasInitialized || IsSelectingTile16) return;
    BRTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_TopLeftpaletteId_valueChanged(int arg1)
{
    if(!HasInitialized || IsSelectingTile16) return;
    TLTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_TopRightpaletteId_valueChanged(int arg1)
{
    if(!HasInitialized || IsSelectingTile16) return;
    TRTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_BottomLeftpaletteId_valueChanged(int arg1)
{
    if(!HasInitialized || IsSelectingTile16) return;
    BLTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_BottomRightpaletteId_valueChanged(int arg1)
{
    if(!HasInitialized || IsSelectingTile16) return;
    BRTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_TopLeftHFlip_toggled(bool checked)
{
    if(!HasInitialized || IsSelectingTile16) return;
    TLTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_TopRightHFlip_toggled(bool checked)
{
    if(!HasInitialized || IsSelectingTile16) return;
    TRTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_BottomLeftHFlip_toggled(bool checked)
{
    if(!HasInitialized || IsSelectingTile16) return;
    BLTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_BottomRightHFlip_toggled(bool checked)
{
    if(!HasInitialized || IsSelectingTile16) return;
    BRTile8x8Reset();
}


void TilesetEditDialog::on_checkBox_TopLeftVFlip_toggled(bool checked)
{
    if(!HasInitialized || IsSelectingTile16) return;
    TLTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_TopRightVFlip_toggled(bool checked)
{
    if(!HasInitialized || IsSelectingTile16) return;
    TRTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_BottomLeftVFlip_toggled(bool checked)
{
    if(!HasInitialized || IsSelectingTile16) return;
    BLTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_BottomRightVFlip_toggled(bool checked)
{
    if(!HasInitialized || IsSelectingTile16) return;
    BRTile8x8Reset();
}

/// <summary>
/// Render All Tile8x8 into graphicsView_TilesetAllTile8x8.
/// </summary>
void TilesetEditDialog::RenderInitialization()
{
    // draw pixmaps
    QPixmap Tile8x8Pixmap(8 * 16, 0x600 / 2);
    Tile8x8Pixmap.fill(Qt::transparent);
    QPainter Tile8x8PixmapPainter(&Tile8x8Pixmap);
    Tile8x8PixmapPainter.drawImage(0, 0, tilesetEditParams->newTileset->RenderAllTile8x8(0).toImage());
    QPixmap Tile16Pixmap(16 * 8, 0x300 * 2);
    Tile16Pixmap.fill(Qt::transparent);
    QPainter Tile16PixmapPainter(&Tile16Pixmap);
    Tile16PixmapPainter.drawImage(0, 0, tilesetEditParams->newTileset->RenderAllTile16(1).toImage());

    // draw palette Bar
    QPixmap PaletteBarpixmap(8 * 16, 16);
    PaletteBarpixmap.fill(Qt::transparent);
    QPainter PaletteBarPainter(&PaletteBarpixmap);
    QVector<QRgb> *palettetable = tilesetEditParams->newTileset->GetPalettes();
    for(int i = 1; i < 16; ++i)
    {
        PaletteBarPainter.fillRect(8 * i, 0, 8, 16, palettetable[0][i]);
    }

    // draw Tile8x8 in Tile8x8 editor
    QPixmap CurTile8x8Pixmap(8, 8);
    CurTile8x8Pixmap.fill(Qt::transparent);
    QPainter CurTile8x8PixmapPainter(&CurTile8x8Pixmap);
    CurTile8x8PixmapPainter.drawImage(0, 0, tilesetEditParams->newTileset->RenderTile8x8(0, 0).toImage());

    // Set up scenes
    PaletteBarScene = new QGraphicsScene(0, 0, 16 * 8, 16);
    Palettemapping = PaletteBarScene->addPixmap(PaletteBarpixmap);
    Tile8x8MAPScene = new QGraphicsScene(0, 0, 8 * 16, 0x600 / 2);
    Tile8x8mapping = Tile8x8MAPScene->addPixmap(Tile8x8Pixmap);
    Tile16MAPScene = new QGraphicsScene(0, 0, 16 * 8, 0x300 * 2);
    Tile16mapping = Tile16MAPScene->addPixmap(Tile16Pixmap);
    Tile8x8EditorScene = new QGraphicsScene(0, 0, 8, 8);
    Tile8x8Editormapping = Tile8x8EditorScene->addPixmap(CurTile8x8Pixmap);

    // Add the highlighted tile rectangle
    QPixmap selectionPixmap(8, 8);
    QPixmap selectionPixmap2(16, 16);
    const QColor highlightColor(0xFF, 0, 0, 0x7F);
    selectionPixmap.fill(highlightColor);
    selectionPixmap2.fill(highlightColor);
    SelectionBox_Tile8x8 = Tile8x8MAPScene->addPixmap(selectionPixmap);
    SelectionBox_Tile8x8->setVisible(false);
    SelectionBox_Tile16 = Tile16MAPScene->addPixmap(selectionPixmap2);
    SelectionBox_Tile16->setVisible(false);
    QPixmap selectionPixmap3(8, 16);
    selectionPixmap3.fill(Qt::transparent);
    QPainter SelectionBoxRectPainter(&selectionPixmap3);
    SelectionBoxRectPainter.setPen(QPen(QBrush(Qt::blue), 2));
    SelectionBoxRectPainter.drawRect(1, 1, 7, 15);
    SelectionBox_Color = PaletteBarScene->addPixmap(selectionPixmap3);
    SelectionBox_Color->setVisible(false);

    // show Rneder
    ui->graphicsView_TilesetAllTile8x8->setScene(Tile8x8MAPScene);
    ui->graphicsView_TilesetAllTile8x8->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->graphicsView_TilesetAllTile8x8->scale(2, 2);
    ui->graphicsView_TilesetAllTile16->setScene(Tile16MAPScene);
    ui->graphicsView_TilesetAllTile16->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->graphicsView_TilesetAllTile16->scale(2, 2);
    ui->graphicsView_paletteBar->setScene(PaletteBarScene);
    ui->graphicsView_paletteBar->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->graphicsView_paletteBar->scale(2, 2);
    ui->graphicsView_Tile8x8Editor->setScene(Tile8x8EditorScene);
    ui->graphicsView_Tile8x8Editor->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->graphicsView_Tile8x8Editor->scale(24, 24);
}

/// <summary>
/// Reset Palette Bar.
/// </summary>
void TilesetEditDialog::ResetPaletteBarGraphicView(int paletteId)
{
    PaletteBarScene->clear();

    // draw palette Bar
    QPixmap PaletteBarpixmap(8 * 16, 16);
    PaletteBarpixmap.fill(Qt::transparent);
    QPainter PaletteBarPainter(&PaletteBarpixmap);
    QVector<QRgb> *palettetable = tilesetEditParams->newTileset->GetPalettes();
    for(int i = 1; i < 16; ++i)
    {
        PaletteBarPainter.fillRect(8 * i, 0, 8, 16, palettetable[paletteId][i]);
    }
    Palettemapping = PaletteBarScene->addPixmap(PaletteBarpixmap);

    // Add the highlighted tile rectangle in SelectionBox_Color
    QPixmap selectionPixmap3(8, 16);
    selectionPixmap3.fill(Qt::transparent);
    const QColor highlightColor(0xFF, 0, 0, 0x7F);
    QPainter SelectionBoxRectPainter(&selectionPixmap3);
    SelectionBoxRectPainter.setPen(QPen(QBrush(Qt::blue), 2));
    SelectionBoxRectPainter.drawRect(1, 1, 7, 15);
    SelectionBox_Color = PaletteBarScene->addPixmap(selectionPixmap3);
    SelectionBox_Color->setVisible(false);
}

/// <summary>
/// Re-Render Tile16 Map if the TIle8x8 Map updated.
/// </summary>
void TilesetEditDialog::ReRenderTile16Map()
{
    // draw pixmaps
    QPixmap Tile16Pixmap(16 * 8, 0x300 * 2);
    Tile16Pixmap.fill(Qt::transparent);
    QPainter Tile16PixmapPainter(&Tile16Pixmap);
    Tile16PixmapPainter.drawImage(0, 0, tilesetEditParams->newTileset->RenderAllTile16(1).toImage());

    // Set up scenes
    Tile16MAPScene->clear();
    Tile16mapping = Tile16MAPScene->addPixmap(Tile16Pixmap);

    // Add the highlighted tile rectangle
    QPixmap selectionPixmap2(16, 16);
    const QColor highlightColor(0xFF, 0, 0, 0x7F);
    selectionPixmap2.fill(highlightColor);
    SelectionBox_Tile16 = Tile16MAPScene->addPixmap(selectionPixmap2);
    SelectionBox_Tile16->setVisible(false);

    SetSelectedTile16(0, true);
}

void TilesetEditDialog::ReRenderTile8x8Map(int paletteId)
{
    // draw pixmaps
    QPixmap Tile8x8Pixmap(8 * 16, 0x600 / 2);
    Tile8x8Pixmap.fill(Qt::transparent);
    QPainter Tile8x8PixmapPainter(&Tile8x8Pixmap);
    Tile8x8PixmapPainter.drawImage(0, 0, tilesetEditParams->newTileset->RenderAllTile8x8(paletteId).toImage());

    // Set up scenes
    Tile8x8MAPScene->clear();
    Tile8x8mapping = Tile8x8MAPScene->addPixmap(Tile8x8Pixmap);

    // Add the highlighted tile rectangle
    QPixmap selectionPixmap(8, 8);
    const QColor highlightColor(0xFF, 0, 0, 0x7F);
    selectionPixmap.fill(highlightColor);
    SelectionBox_Tile8x8 = Tile8x8MAPScene->addPixmap(selectionPixmap);
    SelectionBox_Tile8x8->setVisible(false);
}

/// <summary>
/// Copy A Tile16 triggered by mouse drag and drop action
/// </summary>
/// <param name="from_Tile16">
/// Tile16 copy from.
/// </param>
/// <param name="To_Tile16">
/// Tile16 copy to.
/// </param>
void TilesetEditDialog::CopyTile16AndUpdateGraphic(int from_Tile16, int To_Tile16)
{
    IsSelectingTile16 = true;

    // Paint red Box to show selected Tile16
    int X = To_Tile16 & 7;
    int Y = To_Tile16 >> 3;
    SelectionBox_Tile16->setPos(X * 16, Y * 16);
    SelectionBox_Tile16->setVisible(true);
    SelectedTile16 = (unsigned short) To_Tile16;

    LevelComponents::TileMap16* from_tile16Data = tilesetEditParams->newTileset->GetMap16arrayPtr()[from_Tile16];
    LevelComponents::TileMap16* to_tile16Data = tilesetEditParams->newTileset->GetMap16arrayPtr()[To_Tile16];
    for(int i = 0; i < 4; ++i)
    {
        // Update Tile8x8 data in Tile16
        LevelComponents::Tile8x8* oldtile = from_tile16Data->GetTile8X8(i);
        to_tile16Data->ResetTile8x8(oldtile,
                                    i,
                                    oldtile->GetIndex(),
                                    oldtile->GetPaletteIndex(),
                                    oldtile->GetFlipX(),
                                    oldtile->GetFlipY());
    }

    // Update Graphicview
    QPixmap pm(Tile16mapping->pixmap());
    LevelComponents::TileMap16 *newtile16 = tilesetEditParams->newTileset->GetMap16arrayPtr()[To_Tile16];
    newtile16->DrawTile(&pm, (To_Tile16 & 7) << 4, (To_Tile16 >> 3) << 4);
    Tile16mapping->setPixmap(pm);

    // Update UI
    ui->spinBox->setValue(To_Tile16);
    unsigned short eventid = tilesetEditParams->newTileset->GetEventTablePtr()[from_Tile16];
    tilesetEditParams->newTileset->GetEventTablePtr()[To_Tile16] = eventid;
    ui->spinBox_EventId->setValue(eventid);
    unsigned char terrainId = tilesetEditParams->newTileset->GetTerrainTypeIDTablePtr()[from_Tile16];
    tilesetEditParams->newTileset->GetTerrainTypeIDTablePtr()[To_Tile16] = terrainId;
    ui->spinBox_TerrainId->setValue(terrainId);
    LevelComponents::Tile8x8* tile8_TL = to_tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPLEFT);
    LevelComponents::Tile8x8* tile8_TR = to_tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPRIGHT);
    LevelComponents::Tile8x8* tile8_BL = to_tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMLEFT);
    LevelComponents::Tile8x8* tile8_BR = to_tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMRIGHT);

    SetSpinboxesTile8x8sInfo(tile8_TL, ui->spinBox_TopLeftTileId, ui->spinBox_TopLeftpaletteId, ui->checkBox_TopLeftHFlip, ui->checkBox_TopLeftVFlip);
    SetSpinboxesTile8x8sInfo(tile8_TR, ui->spinBox_TopRightTileId, ui->spinBox_TopRightpaletteId, ui->checkBox_TopRightHFlip, ui->checkBox_TopRightVFlip);
    SetSpinboxesTile8x8sInfo(tile8_BL, ui->spinBox_BottomLeftTileId, ui->spinBox_BottomLeftpaletteId, ui->checkBox_BottomLeftHFlip, ui->checkBox_BottomLeftVFlip);
    SetSpinboxesTile8x8sInfo(tile8_BR, ui->spinBox_BottomRightTileId, ui->spinBox_BottomRightpaletteId, ui->checkBox_BottomRightHFlip, ui->checkBox_BottomRightVFlip);

    IsSelectingTile16 = false;
}

/// <summary>
/// Reset One Tile8x8 for Selected Tile16 in Tileset data and Update Graphics in Tile16Map GraphicView at the same time
/// </summary>
/// <param name="tile16Id">
/// Used to find a Tile16 to reset a Tile8x8
/// </param>
/// <param name="newTile8x8_Id">
/// Use a new tile8x8 id to find a Tile8x8 to replace the current Tile8x8
/// </param>
/// <param name="position">
/// The tile8x8 position id, from 0 to 3.
/// </param>
/// <param name="new_paletteIndex">
/// set a new palette index
/// </param>
/// <param name="xflip">
/// set xflip
/// </param>
/// <param name="yflip">
/// set yflip
/// </param>
void TilesetEditDialog::UpdateATile8x8ForSelectedTile16InTilesetData(int tile16Id, int newTile8x8_Id, int position, int new_paletteIndex, bool xflip, bool yflip)
{
    // Update Data
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16Id];
    tile16Data->ResetTile8x8(tilesetEditParams->newTileset->GetTile8x8arrayPtr()[newTile8x8_Id], position & 3, newTile8x8_Id, new_paletteIndex, xflip, yflip);

    // Update Graphic
    QPixmap pm(Tile16mapping->pixmap());
    tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16Id]->DrawTile(&pm, (tile16Id & 7) << 4, (tile16Id >> 3) << 4);
    Tile16mapping->setPixmap(pm);
}

/// <summary>
/// Overwrite One Tile8x8 in Tile8x8 map and update tile16 map at the same time
/// </summary>
/// <param name="posId">
/// The id of a Tile8x8 needs to reset
/// </param>
/// <param name="tiledata">
/// data for generating a new Tile8x8
/// </param>
void TilesetEditDialog::OverwriteATile8x8InTile8x8MapAndUpdateTile16Map(int posId, unsigned char *tiledata)
{
    LevelComponents::Tile8x8** tilearray = tilesetEditParams->newTileset->GetTile8x8arrayPtr();
    LevelComponents::Tile8x8* tile = tilearray[posId];
    if(tile != tilesetEditParams->newTileset->GetblankTile())
        delete tile;
    tile = new LevelComponents::Tile8x8(tiledata, tilesetEditParams->newTileset->GetPalettes());
    tilearray[posId] = tile;

    // update Tile16 map
    for(int i = 0; i < 0x300; ++i)
    {
        LevelComponents::TileMap16* tile16 = tilesetEditParams->newTileset->GetMap16arrayPtr()[i];
        for(int j = 0; j < 4; ++j)
        {
            LevelComponents::Tile8x8* tmptile = tile16->GetTile8X8(j);
            if(tmptile->GetIndex() == posId)
            {
                int pal = tmptile->GetPaletteIndex();
                bool xflip = tmptile->GetFlipX();
                bool yflip = tmptile->GetFlipY();
                tile16->ResetTile8x8(tile, j, posId, pal, xflip, yflip);
            }
        }
    }
}

/// <summary>
/// Set the selected tile8x8 index and update the position of the highlight square.
/// </summary>
/// <param name="tileId">
/// The tile8x8 index that was selected in the graphicsView_Tile8x8Editor.
/// </param>
/// <param name="resetscrollbar">
/// Set this to true if you want the editor to set the scrollbar automatically.
/// </param>
void TilesetEditDialog::SetSelectedTile8x8(unsigned short tileId, bool resetscrollbar)
{
    // Paint red Box to show selected Tile16
    int X = tileId & 15;
    int Y = tileId >> 4;
    SelectionBox_Tile8x8->setPos(X * 8, Y * 8);
    SelectionBox_Tile8x8->setVisible(true);
    SelectedTile8x8 = tileId;

    // Set vertical scrollbar of braphicview
    if (resetscrollbar)
    {
        ui->graphicsView_TilesetAllTile8x8->verticalScrollBar()->setValue(8 * (tileId / 32));
        ui->graphicsView_TilesetAllTile8x8->horizontalScrollBar()->setValue(0);
    }

    ui->label_Tile8x8_ID->setText("Selected Tile8x8 Id: " + QString::number(tileId));

    // Tile8x8 Editor graphicview update
    QPixmap CurTile8x8Pixmap(8, 8);
    CurTile8x8Pixmap.fill(Qt::transparent);
    QPainter CurTile8x8PixmapPainter(&CurTile8x8Pixmap);
    CurTile8x8PixmapPainter.drawImage(0, 0, tilesetEditParams->newTileset->RenderTile8x8(tileId, SelectedPaletteId).toImage());
    Tile8x8EditorScene->clear();
    Tile8x8Editormapping = Tile8x8EditorScene->addPixmap(CurTile8x8Pixmap);

    // Animated Tile staff
    if(tileId < 64)
    {
        int slotId = tileId >> 2;
        int tilegroupId = tilesetEditParams->newTileset->GetAnimatedTileData()[slotId];
        ui->spinBox_AnimatedTileSlot->setValue(slotId);
        ui->spinBox_AnimatedTileGroupId->setValue(tilegroupId);
    }
}

/// <summary>
/// Set Selected Color Id, also update the Selection box in the palette bar.
/// </summary>
/// <param name="newcolorId">
/// new selected color Id.
/// </param>
void TilesetEditDialog::SetSelectedColorId(int newcolorId)
{
    // Paint red Box to show selected Color
    int X = newcolorId * 8;
    SelectionBox_Color->setPos(X, 0);
    SelectionBox_Color->setVisible(true);
    SelectedColorId = newcolorId;

    QColor color = tilesetEditParams->newTileset->GetPalettes()[SelectedPaletteId][newcolorId];
    ui->label_RGB888Value->setText(QString("RGB888: (") +
                                   QString::number(color.red(), 10) + QString(", ") +
                                   QString::number(color.green(), 10) + QString(", ") +
                                   QString::number(color.blue(), 10) + QString(") RGB555: (") +
                                   QString::number(color.red() >> 3, 10) + QString(", ") +
                                   QString::number(color.green() >> 3, 10) + QString(", ") +
                                   QString::number(color.blue() >> 3, 10) + QString(")"));
}

/// <summary>
/// Set Color for right-clicked Color via color dialog.
/// </summary>
/// <param name="newcolorId">
/// new selected color Id.
/// </param>
void TilesetEditDialog::SetColor(int newcolorId)
{
    QColor color = QColorDialog::getColor(Qt::black, this);
    color.setAlpha(0xFF);
    if(color.isValid())
    {
        tilesetEditParams->newTileset->SetColor(SelectedPaletteId, newcolorId, color.rgba());

        // Update Palette Graphicview
        QPixmap pm(Palettemapping->pixmap());
        QPainter PaletteBarPainter(&pm);
        PaletteBarPainter.fillRect(8 * newcolorId, 0, 8, 16, color.rgba());
        Palettemapping->setPixmap(pm);
        ui->label_RGB888Value->setText(QString("RGB888: (") +
                                       QString::number(color.red(), 10) + QString(", ") +
                                       QString::number(color.green(), 10) + QString(", ") +
                                       QString::number(color.blue(), 10) + QString(")"));

        ReRenderTile8x8Map(SelectedPaletteId);
        ReRenderTile16Map();
    }
}

/// <summary>
/// This function will be called when slider moving.
/// </summary>
/// <param name="value">
/// slider value gives automatically.
/// </param>
void TilesetEditDialog::on_horizontalSlider_valueChanged(int value)
{
    if(!HasInitialized) return;
    SelectedPaletteId = value;
    ReRenderTile8x8Map(value);
    ResetPaletteBarGraphicView(value);
    SetSelectedTile8x8(0, true);
    SetSelectedColorId(0);
}

void TilesetEditDialog::TLTile8x8Reset()
{
    UpdateATile8x8ForSelectedTile16InTilesetData(SelectedTile16,
                                                 ui->spinBox_TopLeftTileId->value(),
                                                 LevelComponents::TileMap16::TILE8_TOPLEFT,
                                                 ui->spinBox_TopLeftpaletteId->value(),
                                                 ui->checkBox_TopLeftHFlip->isChecked(),
                                                 ui->checkBox_TopLeftVFlip->isChecked());
}

void TilesetEditDialog::TRTile8x8Reset()
{
    UpdateATile8x8ForSelectedTile16InTilesetData(SelectedTile16,
                                                 ui->spinBox_TopRightTileId->value(),
                                                 LevelComponents::TileMap16::TILE8_TOPRIGHT,
                                                 ui->spinBox_TopRightpaletteId->value(),
                                                 ui->checkBox_TopRightHFlip->isChecked(),
                                                 ui->checkBox_TopRightVFlip->isChecked());
}

void TilesetEditDialog::BLTile8x8Reset()
{
    UpdateATile8x8ForSelectedTile16InTilesetData(SelectedTile16,
                                                 ui->spinBox_BottomLeftTileId->value(),
                                                 LevelComponents::TileMap16::TILE8_BOTTOMLEFT,
                                                 ui->spinBox_BottomLeftpaletteId->value(),
                                                 ui->checkBox_BottomLeftHFlip->isChecked(),
                                                 ui->checkBox_BottomLeftVFlip->isChecked());
}

void TilesetEditDialog::BRTile8x8Reset()
{
    UpdateATile8x8ForSelectedTile16InTilesetData(SelectedTile16,
                                                 ui->spinBox_BottomRightTileId->value(),
                                                 LevelComponents::TileMap16::TILE8_BOTTOMRIGHT,
                                                 ui->spinBox_BottomRightpaletteId->value(),
                                                 ui->checkBox_BottomRightHFlip->isChecked(),
                                                 ui->checkBox_BottomRightVFlip->isChecked());
}

void TilesetEditDialog::on_checkBox_paletteBrush_toggled(bool checked)
{
    if(checked)
        paletteBrushVal = ui->spinBox_paletteBrushValue->value();
    else
        paletteBrushVal = -1;  // Disable
}

/// <summary>
/// Set Animated Tile Slot.
/// </summary>
void TilesetEditDialog::on_pushButton_SetAnimatedTileSlot_clicked()
{
    tilesetEditParams->newTileset->SetAnimatedTile(ui->spinBox_AnimatedTileGroupId->value(), 4 * ui->spinBox_AnimatedTileSlot->value());
    ReRenderTile16Map();
    ReRenderTile8x8Map(SelectedPaletteId);
}

/// <summary>
/// Save Tile8x8 map into file.
/// </summary>
void TilesetEditDialog::on_pushButton_ExportTile8x8Map_clicked()
{
    QString qFilePath = QFileDialog::getSaveFileName(this, tr("Save current Tile8x8 map to a file"),
                                                     QString(""), tr("PNG files (*.png)"));
    if (qFilePath.compare(""))
    {
        int CR_width, CR_height;
        CR_width = 8 * 16;
        CR_height = 0x600 / 2;
        QGraphicsScene *tmpscene = Tile8x8MAPScene;
        QPixmap currentTile8x8mapPixmap(CR_width, CR_height);
        QPainter tmppainter(&currentTile8x8mapPixmap);
        tmpscene->render(&tmppainter);
        currentTile8x8mapPixmap.save(qFilePath, "PNG", 100);
    }
}

/// <summary>
/// Save Tile16 map into file.
/// </summary>
void TilesetEditDialog::on_pushButton_ExportTile16Map_clicked()
{
    QString qFilePath = QFileDialog::getSaveFileName(this, tr("Save current Tile16 map to a file"),
                                                     QString(""), tr("PNG files (*.png)"));
    if (qFilePath.compare(""))
    {
        int CR_width, CR_height;
        CR_width = 8 * 16;
        CR_height = 0x300 * 2;
        QGraphicsScene *tmpscene = Tile16MAPScene;
        QPixmap currentTile16mapPixmap(CR_width, CR_height);
        QPainter tmppainter(&currentTile16mapPixmap);
        tmpscene->render(&tmppainter);
        currentTile16mapPixmap.save(qFilePath, "PNG", 100);
    }
}

void TilesetEditDialog::on_pushButton_ImportTile8x8Graphic_clicked()
{
    // Check SelectedTile8x8, cannot overwrite animated Tile8x8s
    if(SelectedTile8x8 < 65)
    {
        QMessageBox::critical(this, QString("Error"), QString("Overwrite animated tiles not permit!"));
        return;
    }

    // Load gfx bin file
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Load Tileset bin file"), QString(""),
                                                    tr("bin files (*.bin)"));

    // load data into QBytearray
    QByteArray tmptile8x8data, tmptile8x8data_final;
    QFile gfxbinfile(fileName);
    if(!gfxbinfile.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, QString("Error"), QString("Cannot open file!"));
        return;
    }
    tmptile8x8data = gfxbinfile.readAll();
    tmptile8x8data_final = gfxbinfile.readAll(); // Init
    gfxbinfile.close();

    // Check size
    if(tmptile8x8data.size() % 8)
    {
        QMessageBox::critical(this, QString("Error"), QString("Illegal file size!"));
        return;
    }

    // Load palette data from bin file
    fileName = QFileDialog::getOpenFileName(this,
                                            tr("Load palette bin file"), QString(""),
                                            tr("bin files (*.bin)"));
    QByteArray tmppalettedata;
    QFile palbinfile(fileName);
    if(!palbinfile.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, QString("Error"), QString("Cannot open file!"));
        return;
    }
    tmppalettedata = palbinfile.readAll();
    palbinfile.close();

    QVector<QRgb> tmppalette;
    unsigned short *tmppaldata = new unsigned short[16];
    memcpy(tmppaldata, tmppalettedata.data(), 32);

    // Get transparent color id in the palette
    bool ok;
    int transparentcolorId = QInputDialog::getInt(this, tr("Dialog"),
                                         tr("Input the transparent-substitute color id in the palette bin file:"), 0,
                                         0, 15, 1, &ok);
    if (!ok)
        return;

    // transparent-substitute color replacement and load palette
    tmppaldata[transparentcolorId] = 0x7FFF;
    ROMUtils::LoadPalette(&tmppalette, tmppaldata);
    delete[] tmppaldata;
    tmppalette[0] = 0xFF000000;
    tmppalette[transparentcolorId] = 0;

    // half-byte exchange not needed
    // reset bytearray according to the palette bin file
    for(int i = 0; i != 16; ++i)
    {
        char count = 0;
        while(1)
        {
            if(tmppalette[i] == tilesetEditParams->newTileset->GetPalettes()[SelectedPaletteId][count])
            {
                break;
            }
            ++count;
            if(count == 16)
            {
                if((tmppalette[i] != 0xFF000000) && (tmppalette[i] != 0xFFFFFFFF) && (tmppalette[i] != 0))
                {
                    QMessageBox::critical(this, QString("Error"), QString("Palette not suitable!"));
                    return;
                }
                else if(tmppalette[i] == 0xFF000000) // black
                {
                    auto iter = std::find_if(tilesetEditParams->newTileset->GetPalettes()[SelectedPaletteId].begin(),
                                             tilesetEditParams->newTileset->GetPalettes()[SelectedPaletteId].end(), [&](const QRgb& value) {
                                    return value == tmppalette[i]; });
                    if (tmppalette.end() != iter) {
                        count = iter - tmppalette.begin();
                    } else {
                    count = 0;
                    }
                    break;
                }
                else if(tmppalette[i] == 0xFFFFFFFF) // white
                {
                    auto iter = std::find_if(tilesetEditParams->newTileset->GetPalettes()[SelectedPaletteId].begin(),
                                             tilesetEditParams->newTileset->GetPalettes()[SelectedPaletteId].end(), [&](const QRgb& value) {
                                    return value == tmppalette[i]; });
                    if (tmppalette.end() != iter) {
                        count = iter - tmppalette.begin();
                    } else {
                    count = 0;
                    }
                    break;
                }
                else if(tmppalette[i] == 0)
                {
                    count = 0;
                    break;
                }
            }
        }
        if(transparentcolorId == i)
        {
            count = 0;
        }
        for(int j = 0; j < tmptile8x8data.size(); ++j) // TODO: bugfix here
        {
            char tmpchr = tmptile8x8data[j];
            char l4b, h4b;
            h4b = (tmpchr >> 4) & 0xF;
            l4b = tmpchr & 0xF;
            if (l4b == i) {
                l4b = count;
            } else {
                l4b = tmptile8x8data_final[j] & 0xF;
            }
            if (h4b == i) {
                h4b = count;
            } else {
                h4b = (tmptile8x8data_final[j] >> 4) & 0xF;
            }
            tmptile8x8data_final[j] = (h4b << 4) | l4b;
        }
    }

    // find the first blank Tile8x8
    int k;
    int newtilenum = 0;
    for(int i = 0; i < (tmptile8x8data_final.size() / 32); ++i)
    {
        k = 0;
        while((tmptile8x8data.at(i * 32 + k) == 0x11) || (tmptile8x8data.at(i * 32 + k) == 0x22)) // tmptile8x8data[i * 32 + k] == 0x11 or 0x22
        {
            ++k;
            if(k == 16) break;
        }
        if(k == 16)
        {
            newtilenum = i; break;
        }
    }
    if(!newtilenum) // if newtilenum == 0
    {
        newtilenum = tmptile8x8data_final.size() / 32;
    }

    // compare (number of the new Tile8x8 + selected Tile8x8 Id + 1) with (tilesetEditParams->newTileset->GetfgGFXlen() / 32)
    // if (number of the new Tile8x8 + selected Tile8x8 Id + 1) > (tilesetEditParams->newTileset->GetfgGFXlen() / 32) then
    // tilesetEditParams->newTileset->SetfgGFXlen(number of the new Tile8x8 + selected Tile8x8 Id)
    // also (number of the new Tile8x8 + selected Tile8x8 Id) should be < (0x600 - tilesetEditParams->newTileset->GetbgGFXlen() / 32) or return
    // create new Tile8x8 by using 32-byte length data
    // overwrite and replace the old TIle8x8 instances down-through from selected Tile8x8
    unsigned char* newtmpdata = new unsigned char[32];
    if((newtilenum + SelectedTile8x8 + 1) > (tilesetEditParams->newTileset->GetfgGFXlen() / 32))
    {
        if((newtilenum + SelectedTile8x8 + 1 + tilesetEditParams->newTileset->GetbgGFXlen() / 32 + 1) > 0x600)
        {
            QMessageBox::critical(this, QString("Load Error"), QString("Cannot overwrite background Tiles!"));
            return;
        }
        else
        {
            tilesetEditParams->newTileset->SetfgGFXlen(32 * (SelectedTile8x8 - 65 + newtilenum));
        }
    }
    for(int i = 0; i < newtilenum; ++i)
    {
        memcpy(newtmpdata, tmptile8x8data_final.data() + 32 * i, 32);
        OverwriteATile8x8InTile8x8MapAndUpdateTile16Map(SelectedTile8x8 + i, newtmpdata);
    }
    delete[] newtmpdata;

    // update all the graphicviews
    ReRenderTile8x8Map(SelectedPaletteId);
    SetSelectedTile8x8(SelectedTile8x8, false);
    ReRenderTile16Map();
}
