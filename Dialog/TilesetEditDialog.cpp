#include "TilesetEditDialog.h"
#include "ui_TilesetEditDialog.h"

#include <QFile>
#include <QFileDialog>
#include <QFileDevice>
#include <QMessageBox>

#include "WL4Constants.h"
#include "ROMUtils.h"
#include "FileIOUtils.h"
#include "ScatteredGraphicUtils.h"
#include "WL4EditorWindow.h"
extern WL4EditorWindow *singleton;

/// <summary>
/// Constructor of TilesetEditDialog class.
/// </summary>
TilesetEditDialog::TilesetEditDialog(QWidget *parent, DialogParams::TilesetEditParams *tilesetEditParam) :
    QDialog(parent),
    ui(new Ui::TilesetEditDialog)
{
    ui->setupUi(this);
    this->tilesetEditParams = tilesetEditParam;
    ui->label_TilesetID->setText("Tileset ID: 0x" + QString::number(tilesetEditParam->currentTilesetIndex, 16));
    ui->graphicsView_TilesetAllTile16->SetCurrentTilesetEditor(this);
    ui->graphicsView_TilesetAllTile8x8->SetCurrentTilesetEditor(this);
    ui->graphicsView_paletteBar->SetCurrentTilesetEditor(this);
    ui->graphicsView_Tile8x8Editor->SetCurrentTilesetEditor(this);
    ui->label_Tile8x8SetPaletteId->setText("0x0");

    // render
    RenderInitialization();

    //re-initialize widgets
    SetSelectedTile8x8(0, true);
    SetSelectedTile16(0, true);
    SetSelectedColorId(0);

    // update Info Textbox
    UpdateInfoTextBox();

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
/// Delete a foreground TIle8x8 from the Tileset and keep all the Tile16 using the correct tile8x8s
/// </summary>
/// <param name="tile8x8id">
/// id of the Tile8x8 which are going to be deleted, the first foreground tile8x8 is indexed 0x41.
/// </param>
void TilesetEditDialog::DeleteFGTile8x8(int tile8x8id)
{
    tilesetEditParams->newTileset->DelTile8x8(tile8x8id);

    // UI update
    ReRenderTile16Map();
    ReRenderTile8x8Map(SelectedPaletteId);
    if(tile8x8id <= (0x40 + tilesetEditParams->newTileset->GetfgGFXlen() / 32))
    {
        SetSelectedTile8x8(tile8x8id, false);
    }
    else
    {
        SetSelectedTile8x8(tile8x8id - 1, false);
    }
}

/// <summary>
/// Set Tile16 Palette id for all the 4 TIle8x8 in it
/// </summary>
/// <param name="tile16ID">
/// Tile16's Id which are going to be set.
/// </param>
void TilesetEditDialog::SetTile16PaletteId(int tile16ID)
{
    auto tile16 = tilesetEditParams->newTileset->GetMap16arrayPtr()[tile16ID];
    for (int i = 0; i < 4; i++)
    {
        UpdateATile8x8ForSelectedTile16InTilesetData(tile16ID,
                                                     tile16->GetTile8X8(i)->GetIndex(),
                                                     i,
                                                     ui->spinBox_paletteBrushValue->value(),
                                                     tile16->GetTile8X8(i)->GetFlipX(),
                                                     tile16->GetTile8X8(i)->GetFlipY());
    }
}

/// <summary>
/// Clear the current Tile16 data, reset it to blank tile16
/// </summary>
/// <param name="tile16ID">
/// Tile16's Id which are going to be set.
/// </param>
void TilesetEditDialog::ClearTile16Data(int tile16ID)
{
    for (int i = 0; i < 4; i++)
    {
        UpdateATile8x8ForSelectedTile16InTilesetData(tile16ID, 0x40, i, 0, false, false);
    }
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
    (void) arg1;
    if(!HasInitialized || IsSelectingTile16) return;
    TLTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_TopRightTileId_valueChanged(int arg1)
{
    (void) arg1;
    if(!HasInitialized || IsSelectingTile16) return;
    TRTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_BottomLeftTileId_valueChanged(int arg1)
{
    (void) arg1;
    if(!HasInitialized || IsSelectingTile16) return;
    BLTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_BottomRightTileId_valueChanged(int arg1)
{
    (void) arg1;
    if(!HasInitialized || IsSelectingTile16) return;
    BRTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_TopLeftpaletteId_valueChanged(int arg1)
{
    (void) arg1;
    if(!HasInitialized || IsSelectingTile16) return;
    TLTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_TopRightpaletteId_valueChanged(int arg1)
{
    (void) arg1;
    if(!HasInitialized || IsSelectingTile16) return;
    TRTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_BottomLeftpaletteId_valueChanged(int arg1)
{
    (void) arg1;
    if(!HasInitialized || IsSelectingTile16) return;
    BLTile8x8Reset();
}

void TilesetEditDialog::on_spinBox_BottomRightpaletteId_valueChanged(int arg1)
{
    (void) arg1;
    if(!HasInitialized || IsSelectingTile16) return;
    BRTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_TopLeftHFlip_toggled(bool checked)
{
    (void) checked;
    if(!HasInitialized || IsSelectingTile16) return;
    TLTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_TopRightHFlip_toggled(bool checked)
{
    (void) checked;
    if(!HasInitialized || IsSelectingTile16) return;
    TRTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_BottomLeftHFlip_toggled(bool checked)
{
    (void) checked;
    if(!HasInitialized || IsSelectingTile16) return;
    BLTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_BottomRightHFlip_toggled(bool checked)
{
    (void) checked;
    if(!HasInitialized || IsSelectingTile16) return;
    BRTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_TopLeftVFlip_toggled(bool checked)
{
    (void) checked;
    if(!HasInitialized || IsSelectingTile16) return;
    TLTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_TopRightVFlip_toggled(bool checked)
{
    (void) checked;
    if(!HasInitialized || IsSelectingTile16) return;
    TRTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_BottomLeftVFlip_toggled(bool checked)
{
    (void) checked;
    if(!HasInitialized || IsSelectingTile16) return;
    BLTile8x8Reset();
}

void TilesetEditDialog::on_checkBox_BottomRightVFlip_toggled(bool checked)
{
    (void) checked;
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

    // update Info Textbox
    UpdateInfoTextBox();
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

    // update Info Textbox
    UpdateInfoTextBox();
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
    QVector<LevelComponents::Tile8x8 *> tilearray = tilesetEditParams->newTileset->GetTile8x8arrayPtr();
    LevelComponents::Tile8x8* tile = tilearray[posId];
    if(tile != tilesetEditParams->newTileset->GetblankTile())
        delete tile;
    tile = new LevelComponents::Tile8x8(tiledata, tilesetEditParams->newTileset->GetPalettes());
    tilesetEditParams->newTileset->SetTile8x8(tile, posId);

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
/// Update Info Textbox
/// </summary>
void TilesetEditDialog::UpdateInfoTextBox()
{
    // clean up textbox
    ui->textEdit_Infos->setText("");

    // Generate new Infos
    QString output;
    output += tr("Unused Palette(s): (don't forget to clean up those unused Tile16 to free up the room for new palettes.)\n");
    QVector<int> unused_palettes = FindUnusedPalettes();
    for (auto palette_id: unused_palettes)
    {
        output += "0x" + QString::number(palette_id, 16) + ", ";
    }
    output += "\n";

    // set textbox
    ui->textEdit_Infos->setText(output);
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

    int fgtilenum = tilesetEditParams->newTileset->GetfgGFXlen() / 32;
    int bgtilenum = tilesetEditParams->newTileset->GetbgGFXlen() / 32;
    if((SelectedTile8x8 > (0x40 + fgtilenum)) && (SelectedTile8x8 < (0x5FF - bgtilenum)))
    {
        ui->label_Tile8x8_ID->setText("Selected Tile8x8 Id: 0x" + QString::number(tileId, 16) + " (undefined)");
    }
    else if(SelectedTile8x8 <= (0x40 + fgtilenum))
    {
        ui->label_Tile8x8_ID->setText("Selected Foreground Tile8x8 Id: 0x" + QString::number(tileId, 16));
    }
    else
    {
        ui->label_Tile8x8_ID->setText("Selected Background Tile8x8 Id: 0x" + QString::number(tileId, 16));
    }

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
        int tilegroupId = tilesetEditParams->newTileset->GetAnimatedTileData(0)[slotId];
        int tilegroup2Id = tilesetEditParams->newTileset->GetAnimatedTileData(1)[slotId];
        unsigned char SWId = tilesetEditParams->newTileset->GetAnimatedTileSwitchTable()[slotId];
        ui->spinBox_AnimatedTileSlot->setValue(slotId);
        ui->spinBox_AnimatedTileGroupId->setValue(tilegroupId);
        ui->spinBox_AnimatedTileGroup2Id->setValue(tilegroup2Id);
        ui->spinBox_AnimatedTileSwitchId->setValue(SWId);
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
    ReRenderTile16Map();
    ui->label_Tile8x8SetPaletteId->setText("0x" + QString::number(value, 16));
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
    tilesetEditParams->newTileset->SetAnimatedTile(ui->spinBox_AnimatedTileGroupId->value(),
                                                   ui->spinBox_AnimatedTileGroup2Id->value(),
                                                   ui->spinBox_AnimatedTileSwitchId->value(),
                                                   ui->spinBox_AnimatedTileSlot->value() << 2);
    ReRenderTile16Map();
    ReRenderTile8x8Map(SelectedPaletteId);
}

/// <summary>
/// Save Tile8x8 map into file.
/// </summary>
void TilesetEditDialog::on_pushButton_ExportTile8x8Map_clicked()
{
    QString qFilePath = QFileDialog::getSaveFileName(this, tr("Save current Tile8x8 map to a file"),
                                                     QString(""), tr("PNG file (*.png)"));
    if (qFilePath.compare(""))
    {
        QPixmap Tile8x8Pixmap(8 * 16, 0x600 / 2);
        Tile8x8Pixmap.fill(Qt::transparent);
        QPainter Tile8x8PixmapPainter(&Tile8x8Pixmap);
        Tile8x8PixmapPainter.drawImage(0, 0, FileIOUtils::RenderBGColor(tilesetEditParams->newTileset->RenderAllTile8x8(SelectedPaletteId).toImage(), this));
        Tile8x8Pixmap.save(qFilePath, "PNG", 100);
    }
}

/// <summary>
/// Save Tile16 map into file.
/// </summary>
void TilesetEditDialog::on_pushButton_ExportTile16Map_clicked()
{
    QString qFilePath = QFileDialog::getSaveFileName(this, tr("Save current Tile16 map to a file"),
                                                     QString(""), tr("PNG file (*.png)"));
    if (qFilePath.compare(""))
    {
        // draw pixmaps
        QPixmap Tile16Pixmap(16 * 8, 0x300 * 2);
        Tile16Pixmap.fill(Qt::transparent);
        QPainter Tile16PixmapPainter(&Tile16Pixmap);
        Tile16PixmapPainter.drawImage(0, 0, FileIOUtils::RenderBGColor(tilesetEditParams->newTileset->RenderAllTile16(1).toImage(), this));
        Tile16Pixmap.save(qFilePath, "PNG", 100);
    }
}

/// <summary>
/// Inport Tile8x8 graphic data from bin files.
/// </summary>
void TilesetEditDialog::on_pushButton_ImportTile8x8Graphic_clicked()
{
    /** Check SelectedTile8x8, cannot overwrite condition:
     * animated Tile8x8s
     * blanktile (indexed 0x40)
     * blanktiles not attached to the tail of the foreground tileset
     * background tileset
     * blanktile (indexed 0x5FF)
    **/
    if(SelectedTile8x8 < 65)
    {
        QMessageBox::critical(this, tr("Error"), tr("Overwrite animated tiles not permitted in this area!"));
        return;
    }
    else if(SelectedTile8x8 > GetFGTile8x8Num() + 0x41)
    {
        QMessageBox::critical(this, tr("Error"), tr("Skipped adding new tiles not allowed!\nYou cannot leave blank tiles in tileset data or overwrite the background tiles."));
        return;
    }

    LevelComponents::Tileset *tmp_newTilesetPtr = tilesetEditParams->newTileset;
    TilesetEditDialog *currenteditor = this;
    int selTile8x8 = SelectedTile8x8;
    FileIOUtils::ImportTile8x8GfxData(this,
        tmp_newTilesetPtr->GetPalettes()[SelectedPaletteId],
        tr("Choose a color to covert to transparent:"),
        [selTile8x8, tmp_newTilesetPtr, currenteditor] (QByteArray finaldata, QWidget *parentPtr)
        {
            // Assume the file is fully filled with tiles
            int newtilenum = finaldata.size() / 32;

            // compare (number of the new Tile8x8 + selected Tile8x8 Id + 1) with (tilesetEditParams->newTileset->GetfgGFXlen() / 32)
            // if (number of the new Tile8x8 + selected Tile8x8 Id + 1) > (tilesetEditParams->newTileset->GetfgGFXlen() / 32) then
            // tilesetEditParams->newTileset->SetfgGFXlen(number of the new Tile8x8 + selected Tile8x8 Id)
            // also (newtilenum + SelectedTile8x8 + 1) should be less than or equal to 0x400 or return
            // create new Tile8x8 by using 32-byte length data
            // overwrite and replace the old TIle8x8 instances down-through from selected Tile8x8
            unsigned char newtmpdata[32];
            if((newtilenum + selTile8x8 + 1) > (tmp_newTilesetPtr->GetfgGFXlen() / 32))
            {
                if((newtilenum + selTile8x8 + 1) > 0x400)
                {
                    QMessageBox::critical(parentPtr, tr("Load Error"), tr("You can only use 0x400 foreground tiles at most!"));
                    return;
                }
                else
                {
                    tmp_newTilesetPtr->SetfgGFXlen(32 * (selTile8x8 - 65 + newtilenum));
                }
            }
            for(int i = 0; i < newtilenum; ++i)
            {
                memcpy(newtmpdata, finaldata.data() + 32 * i, 32);
                currenteditor->OverwriteATile8x8InTile8x8MapAndUpdateTile16Map(selTile8x8 + i, newtmpdata);
            }
        });

    // update all the graphicviews
    ReRenderTile8x8Map(SelectedPaletteId);
    SetSelectedTile8x8(SelectedTile8x8, false);
    ReRenderTile16Map();
}

/// <summary>
/// Export current palette data to a file.
/// </summary>
void TilesetEditDialog::on_pushButton_ExportPalette_clicked()
{
    FileIOUtils::ExportPalette(this, tilesetEditParams->newTileset->GetPalettes()[SelectedPaletteId]);
}

/// <summary>
/// Import current palette data from a file.
/// </summary>
void TilesetEditDialog::on_pushButton_ImportPalette_clicked()
{
    LevelComponents::Tileset *tmp_newTilesetPtr = tilesetEditParams->newTileset;
    FileIOUtils::ImportPalette(this,
        [tmp_newTilesetPtr] (int selectedPalId, int colorId, QRgb newColor)
        {
            tmp_newTilesetPtr->SetColor(selectedPalId, colorId, newColor);
        },
        SelectedPaletteId);
    ResetPaletteBarGraphicView(SelectedPaletteId);
    SetSelectedColorId(0);
    ReRenderTile8x8Map(SelectedPaletteId);
    ReRenderTile16Map();
}

/// <summary>
/// Inport Tile16 graphic data from bin files.
/// </summary>
void TilesetEditDialog::on_pushButton_ImportTile16Graphic_clicked()
{
    /** rule(s):
     *  don't overwrite the first Tile16
     *  don't let imported Tile16 id goes out of bound (should be smaller than 0x2FF)
     *  TODO: support flexible Tile16 set length in the future, and we can read each of its length from the RATS struct
    **/
    LevelComponents::Tileset *tmp_newTilesetPtr = tilesetEditParams->newTileset;
    int selTile16 = SelectedTile16;
    int selPalId = SelectedPaletteId;
    FileIOUtils::ImportTile8x8GfxData(this,
        tmp_newTilesetPtr->GetPalettes()[SelectedPaletteId],
        tr("Choose a color to covert to transparent:"),
        [selTile16, tmp_newTilesetPtr, selPalId] (QByteArray finaldata, QWidget *parentPtr)
        {
            // Assume the file is fully filled with tiles
            int newtile8x8num = finaldata.size() / 32;
            int newtile16num = newtile8x8num / 4;
            if (newtile8x8num != newtile16num * 4)
            {
                QMessageBox::critical(parentPtr, tr("Load Error"), tr("Illegal Tile8x8 number!"));
                return;
            }

            // Assume we insert Tile16s after the selected Tile16
            if ((newtile16num + selTile16 + 1) > Tile16DefaultNum)
            {
                QMessageBox::critical(parentPtr, tr("Load Error"), tr("Too many Tile16, new Tile16s indexed out of bound!"));
                return;
            }

            // not include animated tiles
            int existingTile8x8Num = tmp_newTilesetPtr->GetfgGFXlen() / 32;

            unsigned char newtmpdata[32];
            unsigned char newtmpXFlipdata[32];
            unsigned char newtmpYFlipdata[32];
            unsigned char newtmpXYFlipdata[32];

            // Generate tile8x8 data for all the existing foreground Tile8x8s
            int data_size = (existingTile8x8Num + 1) * 32;
            unsigned char *tmp_current_tile8x8_data = new unsigned char[data_size];
            memset(&tmp_current_tile8x8_data[0], 0, data_size);
            auto tile8x8array = tmp_newTilesetPtr->GetTile8x8arrayPtr();
            for (int j = 0x40; j < (0x41 + existingTile8x8Num); j++)
            {
                memcpy(&tmp_current_tile8x8_data[(j - 0x40) * 32], tile8x8array[j]->CreateGraphicsData().data(), 32);
            }

            // ask user how many Tile16 per row, then update Tile16s' data to the Tile16 set
            int Tile16_per_row = QInputDialog::getInt(parentPtr,
                                                      tr("WL4Editor"),
                                                      tr("Input the Tile16 number per row in your source file"),
                                                      1, 1, 8);
            int Tile16_per_col = newtile16num / Tile16_per_row;
            if (Tile16_per_row * Tile16_per_col != newtile16num)
            {
                QMessageBox::critical(parentPtr, tr("Error"), tr("incorrect Tile16 number per row!"));
                delete[] tmp_current_tile8x8_data;
                return;
            }

            // Compare through all the existing foreground Tile8x8s
            // if exist, generate Tile16 data
            // if not exist, show dialog and return
            for(int i = 0; i < newtile8x8num; ++i)
            {
                // Generate 4 possible existing Tile8x8 graphic data for comparison
                memcpy(newtmpdata, finaldata.data() + 32 * i, 32);
                ROMUtils::Tile8x8DataXFlip(newtmpdata, newtmpXFlipdata);
                ROMUtils::Tile8x8DataYFlip(newtmpdata, newtmpYFlipdata);
                ROMUtils::Tile8x8DataYFlip(newtmpXFlipdata, newtmpXYFlipdata);

                bool find_eqaul = false;
                // loop from the first blank tile, excluding those animated tiles
                for (int j = 0; j < (existingTile8x8Num + 1); j++)
                {
                    int result0 = memcmp(newtmpdata, &tmp_current_tile8x8_data[j * 32], 32);
                    int result1 = memcmp(newtmpXFlipdata, &tmp_current_tile8x8_data[j * 32], 32);
                    int result2 = memcmp(newtmpYFlipdata, &tmp_current_tile8x8_data[j * 32], 32);
                    int result3 = memcmp(newtmpXYFlipdata, &tmp_current_tile8x8_data[j * 32], 32);
                    int cur_row = i / Tile16_per_row / 2;
                    int cur_col = i % (Tile16_per_row * 2);
                    int position = (cur_row & 1) << 1 | (cur_col & 1);
                    int row_tile16 = i / Tile16_per_row / 4;
                    int col_tile16 = i / 2 - Tile16_per_row * (cur_row);

                    LevelComponents::TileMap16* tile16Data = tmp_newTilesetPtr->GetMap16arrayPtr()[selTile16 + row_tile16 * 8 + col_tile16];
                    LevelComponents::Tile8x8* tile8x8_ptr = tmp_newTilesetPtr->GetTile8x8arrayPtr()[j + 0x40];
                    if (!result0)
                    {
                        tile16Data->ResetTile8x8(tile8x8_ptr, position & 3, j + 0x40, selPalId, false, false);
                        find_eqaul = true;
                        break;
                    }
                    else if (!result1)
                    {
                        tile16Data->ResetTile8x8(tile8x8_ptr, position & 3, j + 0x40, selPalId, true, false);
                        find_eqaul = true;
                        break;
                    }
                    else if (!result2)
                    {
                        tile16Data->ResetTile8x8(tile8x8_ptr, position & 3, j + 0x40, selPalId, false, true);
                        find_eqaul = true;
                        break;
                    }
                    else if (!result3)
                    {
                        tile16Data->ResetTile8x8(tile8x8_ptr, position & 3, j + 0x40, selPalId, true, true);
                        find_eqaul = true;
                        break;
                    }
                }
                if (!find_eqaul)
                {// not find any existing tile8x8 eqaul to the current tile8x8
                    QMessageBox::critical(parentPtr, tr("Load Error"),
                                          tr("Detect a Tile8x8 cannot be found in the current Tile8x8 set!"));
                    delete[] tmp_current_tile8x8_data;
                    return;
                }
            }
            delete[] tmp_current_tile8x8_data;
        });

    // update graphicview
    SetSelectedTile16(SelectedTile16, false);
    ReRenderTile16Map();
}

/// <summary>
/// A helper fucntion to find palettes which is never used in Tile16 set.
/// </summary>
/// <return>
/// An array of palette id which is never used in Tile16 set.
/// </return>
QVector<int> TilesetEditDialog::FindUnusedPalettes()
{
    QVector<int> tmp = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    QVector<LevelComponents::TileMap16*> tile16array = tilesetEditParams->newTileset->GetMap16arrayPtr();
    for (int i = 0; i < Tile16DefaultNum; i++)
    {
        LevelComponents::TileMap16* tile16 = tile16array[i];
        for(int j = 0; j < 4; ++j)
        {
            int pal = tile16->GetTile8X8(j)->GetPaletteIndex();
            if (tmp[pal] == pal)
            {
                tmp[pal] = -1; // we clean the used one from the tmp array
            }
        }
    }
    QVector<int> result;
    for (int i = 0; i < 16; i++)
    {
        if (tmp[i] == i)
        {
            result.push_back(i);
        }
    }
    return result;
}

/// <summary>
/// Clean up duplicated Tile8x8 from both Tile8x8 set and Tile16 set.
/// </summary>
void TilesetEditDialog::on_pushButton_CleanUpDuplicatedTile8x8_clicked()
{
    // ask user if eliminate similar tiles to reduce more tiles
    bool ok;
    int diff_upbound = QInputDialog::getInt(this,
                                              tr("WL4Editor"),
                                              tr("Input a number to eliminate similar tiles to reducing tiles aggressively.\n"
                                                 "use a bigger number to reduce more tiles. but Tile16s' quality will drop more.\n"
                                                 "do strictly tile reduce by set 0 here."),
                                              0, 0, 64, 1, &ok);
    if (!ok) return;

    LevelComponents::Tileset *tmp_newTilesetPtr = tilesetEditParams->newTileset;
    int existingTile8x8Num = tmp_newTilesetPtr->GetfgGFXlen() / 32;
    unsigned char newtmpdata[32];
    unsigned char newtmpXFlipdata[32];
    unsigned char newtmpYFlipdata[32];
    unsigned char newtmpXYFlipdata[32];

    // Generate tile8x8 data for all the existing foreground Tile8x8s
    int data_size = (existingTile8x8Num + 1) * 32;
    unsigned char *tmp_current_tile8x8_data = new unsigned char[data_size];
    memset(&tmp_current_tile8x8_data[0], 0, data_size);
    auto tile8x8array = tmp_newTilesetPtr->GetTile8x8arrayPtr();
    for (int j = 0x40; j < (0x41 + existingTile8x8Num); j++)
    {
        memcpy(&tmp_current_tile8x8_data[(j - 0x40) * 32], tile8x8array[j]->CreateGraphicsData().data(), 32);
    }

    // Compare through all the existing foreground Tile8x8s
    for(int i = existingTile8x8Num; i > 0; i--)
    {
        // Generate 4 possible existing Tile8x8 graphic data for comparison
        memcpy(newtmpdata, &tmp_current_tile8x8_data[32 * i], 32);
        ROMUtils::Tile8x8DataXFlip(newtmpdata, newtmpXFlipdata);
        ROMUtils::Tile8x8DataYFlip(newtmpdata, newtmpYFlipdata);
        ROMUtils::Tile8x8DataYFlip(newtmpXFlipdata, newtmpXYFlipdata);

        // loop from the first blank tile to the tile right before the current tile being checked, excluding those animated tiles
        for (int j = 0; j < i; j++)
        {
            int result0 = FileIOUtils::quasi_memcmp(newtmpdata, &tmp_current_tile8x8_data[j * 32], 32);
            int result1 = FileIOUtils::quasi_memcmp(newtmpXFlipdata, &tmp_current_tile8x8_data[j * 32], 32);
            int result2 = FileIOUtils::quasi_memcmp(newtmpYFlipdata, &tmp_current_tile8x8_data[j * 32], 32);
            int result3 = FileIOUtils::quasi_memcmp(newtmpXYFlipdata, &tmp_current_tile8x8_data[j * 32], 32);
            bool find_eqaul = false;
            bool xflip = false;
            bool yflip = false;
            auto tile16array = tmp_newTilesetPtr->GetMap16arrayPtr();
            auto tile8x8array = tmp_newTilesetPtr->GetTile8x8arrayPtr();

            if (result0 <= diff_upbound)
            {
                find_eqaul = true;
            }
            else if (result1 <= diff_upbound)
            {
                find_eqaul = true;
                xflip = true;
            }
            else if (result2 <= diff_upbound)
            {
                find_eqaul = true;
                yflip = true;
            }
            else if (result3 <= diff_upbound)
            {
                find_eqaul = true;
                xflip = true;
                yflip = true;
            }

            if (find_eqaul)
            {
                for (int k = 0; k < Tile16DefaultNum; k++)
                {
                    for (int pos = 0; pos < 4; pos++)
                    {
                        bool tmp_xflip = xflip;
                        bool tmp_yflip = yflip;
                        auto tile8 = tile16array[k]->GetTile8X8(pos);
                        if (tile8->GetIndex() == (i + 0x40))
                        {
                            if (tile8->GetFlipX())
                            {
                                tmp_xflip = !xflip;
                            }
                            if (tile8->GetFlipY())
                            {
                                tmp_yflip = !yflip;
                            }
                            tile16array[k]->ResetTile8x8(tile8x8array[j + 0x40], pos, j + 0x40,
                                                         tile8->GetPaletteIndex(), tmp_xflip, tmp_yflip);
                        }
                    }
                }

                // delete the Tile8x8 from the Tile8x8 set
                tilesetEditParams->newTileset->DelTile8x8(i + 0x40);
                break;
            }
        }
    }
    delete[] tmp_current_tile8x8_data;

    // update graphicview
    ReRenderTile16Map();
    ReRenderTile8x8Map(SelectedPaletteId);
    SetSelectedTile16(0, true);
    SetSelectedTile8x8(0, true);
}

/// <summary>
/// Change BG Tile8x8 set and the last palette by searching legal scattered graphic entries.
/// </summary>
void TilesetEditDialog::on_pushButton_changeBGTile8x8set_clicked()
{
    QVector<struct ScatteredGraphicUtils::ScatteredGraphicEntryItem> graphicEntries = ScatteredGraphicUtils::GetScatteredGraphicsFromROM();

    // Open a dialog to let user to choose a graphic entry
    if (!graphicEntries.size())
    {
        QMessageBox::information(this, tr("Error"), tr("Cannot find graphic entry from the ROM!"));
        return;
    }

    LevelComponents::Tileset *tmp_newTilesetPtr = tilesetEditParams->newTileset;
    int fgTileNum = tmp_newTilesetPtr->GetfgGFXlen() / 32;
    QStringList items;
    for (int i = 0; i < graphicEntries.size(); i++)
    {
        // incorrect tile data type
        if (graphicEntries[i].TileDataType != ScatteredGraphicUtils::Tile8x8_4bpp_no_comp_Tileset_text_bg)
        {
            continue;
        }

        // the new bg tiles will overwrite the existing fg tiles
        if ((graphicEntries[i].TileDataRAMOffsetNum + 0x200) <= (fgTileNum + 0x40))
        {
            continue;
        }

        // available entries need to be pushed into the list
        // split the string with " " and we can always get the correct id on the first slice.
        items << QString(QString::number(i) + "  " + graphicEntries[i].TileDataName + " - " + graphicEntries[i].MappingDataName);
    }

    // if the size == 0, the dialog will become a regular inputbox
    if (!items.size())
    {
        QMessageBox::information(this, tr("Error"), tr("Cannot find any graphic entry suitable for the current Tileset!"));
        return;
    }

    QInputDialog dialog;
    dialog.setOptions(QInputDialog::UseListViewForComboBoxItems);
    dialog.setComboBoxItems(items);
    dialog.setWindowTitle(tr("Select a graphic entry to use its background tiles."));
    if(dialog.exec() == QDialog::Accepted)
    {
        QStringList result = dialog.textValue().split(QChar(' '), Qt::SkipEmptyParts);
        unsigned int id = result.at(0).toUInt(nullptr, 10);

        unsigned char newtmpdata[32];
        unsigned int bgTileNum = graphicEntries[id].TileDataSize_Byte / 32;
        unsigned int startId = graphicEntries[id].TileDataRAMOffsetNum + 0x200;
        for(int i = 0; i < bgTileNum; ++i)
        {
            memcpy(newtmpdata, graphicEntries[id].tileData.data() + 32 * i, 32);
            OverwriteATile8x8InTile8x8MapAndUpdateTile16Map(startId + i, newtmpdata);
        }

        // Reset the last palette
        SelectedPaletteId = 15;
        for (int i = 1; i < 16; i++)
        {
            tmp_newTilesetPtr->SetColor(SelectedPaletteId, i, graphicEntries[id].palettes[15][i]);
        }

        // update settings in the new tileset instance
        tmp_newTilesetPtr->SetbgGFXlen(graphicEntries[id].TileDataSize_Byte);
        tmp_newTilesetPtr->SetbgGFXptr(graphicEntries[id].TileDataAddress);

        // update all the graphicviews
        ResetPaletteBarGraphicView(SelectedPaletteId);
        SetSelectedColorId(0);
        ReRenderTile16Map();
        ReRenderTile8x8Map(SelectedPaletteId);
        SetSelectedTile16(0, true);
        SetSelectedTile8x8(0, true);
    }
}
