#include "TilesetEditDialog.h"
#include "ui_TilesetEditDialog.h"

#include <QFile>
#include <QFileDialog>
#include <QFileDevice>
#include <QMessageBox>

#include "WL4Constants.h"
#include "ROMUtils.h"
#include "FileIOUtils.h"
#include "AssortedGraphicUtils.h"
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
    if (tile16ID < 0)
    {
        tile16ID = 0;
    }
    else if (tile16ID >= Tile16DefaultNum)
    {
        tile16ID = Tile16DefaultNum - 1;
    }

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
    tilesetEditParams->newTileset->GetEventTablePtr()[tile16ID] = 0;
    tilesetEditParams->newTileset->GetTerrainTypeIDTablePtr()[tile16ID] = 0;
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
    for(int i = 0; i < 16; ++i)
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
    for(int i = 0; i < 16; ++i)
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
    if (tile16Id < 0)
    {
        return;
    }

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
                                                     singleton->GetdDialogInitialPath(), tr("PNG file (*.png)"));
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
                                                     singleton->GetdDialogInitialPath(), tr("PNG file (*.png)"));
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
                memcpy(&tmp_current_tile8x8_data[(j - 0x40) * 32], tile8x8array[j]->GetRawPixelData().data(), 32);
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
/// A helper function to find the palette ids which are used by at least one Tile8x8 of the Tile16 set.
/// </summary>
/// <remarks>
/// The Tile16 set stores the palette id of every Tile8x8 corner, so a palette id is used as soon as
/// one corner of one Tile16 references it.
/// </remarks>
/// <return>
/// An array of the used palette ids in ascending order.
/// </return>
QVector<int> TilesetEditDialog::FindUsedPalettes()
{
    bool usedPalettes[16] = { false };
    QVector<LevelComponents::TileMap16*> tile16array = tilesetEditParams->newTileset->GetMap16arrayPtr();
    for (int i = 0; i < Tile16DefaultNum; i++)
    {
        LevelComponents::TileMap16* tile16 = tile16array[i];
        for (int j = 0; j < 4; ++j)
        {
            int pal = tile16->GetTile8X8(j)->GetPaletteIndex() & 0xF;
            usedPalettes[pal] = true;
        }
    }
    QVector<int> result;
    for (int i = 0; i < 16; i++)
    {
        if (usedPalettes[i])
        {
            result.push_back(i);
        }
    }
    return result;
}

/// <summary>
/// A helper function to find palettes which is never used in Tile16 set.
/// </summary>
/// <return>
/// An array of palette id which is never used in Tile16 set.
/// </return>
QVector<int> TilesetEditDialog::FindUnusedPalettes()
{
    QVector<int> usedPalettes = FindUsedPalettes();
    QVector<int> result;
    for (int i = 0; i < 16; i++)
    {
        if (!usedPalettes.contains(i))
        {
            result.push_back(i);
        }
    }
    return result;
}

/// <summary>
/// Count the Tile16s of the Tile16 set which use a specified palette id.
/// </summary>
/// <param name="paletteId">
/// The palette id to check.
/// </param>
/// <return>
/// The number of the Tile16s using the palette, a Tile16 is counted only once.
/// </return>
int TilesetEditDialog::CountTile16UsingPalette(int paletteId)
{
    int result = 0;
    QVector<LevelComponents::TileMap16*> tile16array = tilesetEditParams->newTileset->GetMap16arrayPtr();
    for (int i = 0; i < Tile16DefaultNum; i++)
    {
        LevelComponents::TileMap16* tile16 = tile16array[i];
        for (int j = 0; j < 4; ++j)
        {
            if (tile16->GetTile8X8(j)->GetPaletteIndex() == paletteId)
            {
                ++result;
                break;
            }
        }
    }
    return result;
}

/// <summary>
/// Swap the 2 palette ids used by the Tile8x8s of the whole Tile16 set.
/// </summary>
/// <remarks>
/// This keeps the rendered Tile16 images unchanged after the color data of the 2 palettes has been
/// swapped, since each Tile8x8 keeps using the same colors.
/// </remarks>
/// <param name="paletteId1">
/// The first palette id to swap.
/// </param>
/// <param name="paletteId2">
/// The second palette id to swap.
/// </param>
void TilesetEditDialog::SwapPaletteIdsInTile16Set(int paletteId1, int paletteId2)
{
    QVector<LevelComponents::TileMap16*> tile16array = tilesetEditParams->newTileset->GetMap16arrayPtr();
    for (int i = 0; i < Tile16DefaultNum; i++)
    {
        LevelComponents::TileMap16* tile16 = tile16array[i];
        for (int j = 0; j < 4; ++j)
        {
            LevelComponents::Tile8x8* tile8x8 = tile16->GetTile8X8(j);
            int pal = tile8x8->GetPaletteIndex();
            if (pal == paletteId1)
            {
                tile8x8->SetPaletteIndex(paletteId2);
            }
            else if (pal == paletteId2)
            {
                tile8x8->SetPaletteIndex(paletteId1);
            }
        }
    }
}

/// <summary>
/// Parse a list of hex numbers separated by English commas.
/// </summary>
/// <param name="text">
/// The text to be parsed.
/// </param>
/// <param name="result">
/// The parsed values.
/// </param>
/// <param name="maxValue">
/// The biggest value allowed.
/// </param>
/// <param name="valueName">
/// The name of the values, which is used in the error messages.
/// </param>
/// <returns>
/// Return true if all the values in the text are parsed successfully.
/// </returns>
bool TilesetEditDialog::ParseCommaSeparatedHexValues(const QString &text, QVector<unsigned int> &result,
                                                     unsigned int maxValue, const QString &valueName)
{
    result.clear();
    QStringList valueStrings = text.split(",", Qt::SkipEmptyParts);
    for (const QString &s : valueStrings)
    {
        bool ok;
        unsigned int value = s.trimmed().toUInt(&ok, 16);
        if (!ok)
        {
            QMessageBox::warning(this, tr("Warning"),
                                 tr("A part of text cannot be converted to ") + valueName + tr(": ") + s);
            result.clear();
            return false;
        }
        else if (value > maxValue)
        {
            QMessageBox::warning(this, tr("Warning"),
                                 valueName + tr(" must be between 0 and ") + QString::number(maxValue)
                                 + tr(": ") + s);
            result.clear();
            return false;
        }
        else
        {
            result.push_back(value);
        }
    }
    return true;
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
                                            tr("Input a tolerance value for the number of different pixels between 2 Tile8x8s.\n"
                                               "The editor will merge similar Tile8x8s to reduce tile count aggressively.\n"
                                               "Use a bigger value to reduce more tiles. However, the quality of the Tile16s will drop.\n"
                                               "To perform regular tile reduction, use a value of 0."),
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
        memcpy(&tmp_current_tile8x8_data[(j - 0x40) * 32], tile8x8array[j]->GetRawPixelData().data(), 32);
    }

    // Compare through all the existing foreground Tile8x8s
    for(int i = existingTile8x8Num; i > 0; i--)
    {
        // Generate 4 possible existing Tile8x8 graphic data for comparison
        memcpy(newtmpdata, &tmp_current_tile8x8_data[32 * i], 32);
        ROMUtils::Tile8x8DataXFlip(newtmpdata, newtmpXFlipdata);
        ROMUtils::Tile8x8DataYFlip(newtmpdata, newtmpYFlipdata);
        ROMUtils::Tile8x8DataYFlip(newtmpXFlipdata, newtmpXYFlipdata);
        int old_tileid = i + 0x40;

        // loop from the first blank tile to the tile right before the current tile being checked, excluding those animated tiles
        for (int j = 0; j < i; j++)
        {
            unsigned char tile_data[32];
            memcpy(tile_data, &tmp_current_tile8x8_data[32 * j], 32);

            int result0 = FileIOUtils::quasi_memcmp(newtmpdata, tile_data, 32);
            int result1 = FileIOUtils::quasi_memcmp(newtmpXFlipdata, tile_data, 32);
            int result2 = FileIOUtils::quasi_memcmp(newtmpYFlipdata, tile_data, 32);
            int result3 = FileIOUtils::quasi_memcmp(newtmpXYFlipdata, tile_data, 32);
            bool find_eqaul = false;
            bool xflip = false;
            bool yflip = false;
            auto tile16array = tmp_newTilesetPtr->GetMap16arrayPtr();
            auto tile8x8array = tmp_newTilesetPtr->GetTile8x8arrayPtr();

            int find_tileid = j + 0x40;
            int reserved_tileid = find_tileid;

            if (result0 <= diff_upbound)
            {
                find_eqaul = true;
                if (newtmpdata == FileIOUtils::find_less_feature_buff(newtmpdata, tile_data, 32))
                {
                    reserved_tileid = old_tileid;
                }
            }
            else if (result1 <= diff_upbound)
            {
                find_eqaul = true;
                if (newtmpXFlipdata == FileIOUtils::find_less_feature_buff(newtmpXFlipdata, tile_data, 32))
                {
                    reserved_tileid = old_tileid;
                }
                xflip = true;
            }
            else if (result2 <= diff_upbound)
            {
                find_eqaul = true;
                if (newtmpYFlipdata == FileIOUtils::find_less_feature_buff(newtmpYFlipdata, tile_data, 32))
                {
                    reserved_tileid = old_tileid;
                }
                yflip = true;
            }
            else if (result3 <= diff_upbound)
            {
                find_eqaul = true;
                if (newtmpXYFlipdata == FileIOUtils::find_less_feature_buff(newtmpXYFlipdata, tile_data, 32))
                {
                    reserved_tileid = old_tileid;
                }
                xflip = true;
                yflip = true;
            }

            if (find_eqaul)
            {
                // we need to change the tmp_current_tile8x8_data and tile8x8array
                // if the old_tileid needs to be reserved
                if (reserved_tileid == old_tileid)
                {
                    memcpy(&tmp_current_tile8x8_data[32 * j], newtmpdata, 32);

                    LevelComponents::Tile8x8 *tmptile = tile8x8array[old_tileid];
                    tile8x8array[old_tileid] = tile8x8array[find_tileid];
                    tile8x8array[find_tileid] = tmptile;
                }

                // always replace the old_tileid instance with the find_tileid instance
                for (int k = 0; k < Tile16DefaultNum; k++)
                {
                    for (int pos = 0; pos < 4; pos++)
                    {
                        bool tmp_xflip = xflip;
                        bool tmp_yflip = yflip;
                        auto tile8 = tile16array[k]->GetTile8X8(pos);
                        if (tile8->GetIndex() == old_tileid)
                        {
                            if (tile8->GetFlipX())
                            {
                                tmp_xflip = !xflip;
                            }
                            if (tile8->GetFlipY())
                            {
                                tmp_yflip = !yflip;
                            }
                            tile16array[k]->ResetTile8x8(tile8x8array[find_tileid], pos, find_tileid,
                                                         tile8->GetPaletteIndex(), tmp_xflip, tmp_yflip);
                        }
                    }
                }

                // delete the Tile8x8 from the Tile8x8 set
                tilesetEditParams->newTileset->DelTile8x8(old_tileid);
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
/// Change BG Tile8x8 set and the last palette by searching legal assorted graphic entries.
/// </summary>
void TilesetEditDialog::on_pushButton_ResetBGTile8x8setAndPals_clicked()
{
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> graphicEntries = AssortedGraphicUtils::GetAssortedGraphicsFromROM();

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
        if (graphicEntries[i].TileDataType != AssortedGraphicUtils::Tile8x8_4bpp_no_comp_Tileset_text_bg)
        {
            continue;
        }

        // the entry contains no Tile8x8 data, so there is nothing to be used as bg tiles
        if (!graphicEntries[i].TileDataSizeInByte)
        {
            continue;
        }

        // the new bg tiles will overwrite the existing fg tiles
        if ((graphicEntries[i].TileDataRAMOffsetNum + 0x200) <= (fgTileNum + 0x40))
        {
            continue;
        }

        // check if the palettes used by bg tiles are occupied by foreground Tile16s
        QVector<int> unusedpal = FindUnusedPalettes();
        bool skipthisentry = false;
        for (int j = 0; j < graphicEntries[i].PaletteSlotIDs.size(); j++)
        {
            if (!unusedpal.contains(graphicEntries[i].PaletteSlotIDs[j]))
            {
                skipthisentry = true;
                break;
            }
        }
        if (skipthisentry) continue;

        // available entries need to be pushed into the list
        // split the string with " " and we can always get the correct id on the first slice.
        items << QString(QString::number(i) + "  " + graphicEntries[i].TileDataName + " - " + graphicEntries[i].MappingDataName);
    }

    // if the size == 0, the dialog will become a regular inputbox
    if (!items.size())
    {
        QMessageBox::information(this, tr("Error"), tr("Cannot find any graphic entry suitable for the current Tileset!\n"
                                                       "only when the current Tileset has corresponding unused palettes used by graphics,\n"
                                                       "then you can import the tiles."));
        return;
    }

    QInputDialog dialog;
    dialog.setOptions(QInputDialog::UseListViewForComboBoxItems);
    dialog.setComboBoxItems(items);
    dialog.setLabelText(tr("Select a graphic entry to use its background tiles."));
    dialog.setWindowTitle("WL4Editor");
    int bgTileNum = tmp_newTilesetPtr->GetbgGFXlen() / 32;
    if(dialog.exec() == QDialog::Accepted)
    {
        // cleanup old bg tile instances
        LevelComponents::Tile8x8 *blanktile = tmp_newTilesetPtr->GetblankTile();
        for (int n = 0; n < bgTileNum; n++)
        {
            LevelComponents::Tile8x8 *tile = tmp_newTilesetPtr->GetTile8x8arrayPtr()[0x5FE - n];
            if(tile != blanktile)
                delete tile;
            tilesetEditParams->newTileset->SetTile8x8(blanktile, 0x5FE - n);
        }

        // instanciate new tiles and put them into the Tileset
        QStringList result = dialog.textValue().split(QChar(' '), Qt::SkipEmptyParts);
        unsigned int id = result.at(0).toUInt(nullptr, 10);

        unsigned char newtmpdata[32];
        unsigned int bgTileNum = graphicEntries[id].TileDataSizeInByte / 32;
        unsigned int startId = graphicEntries[id].TileDataRAMOffsetNum + 0x200;
        for(int i = 0; i < bgTileNum; ++i)
        {
            memcpy(newtmpdata, graphicEntries[id].tileData.data() + 32 * i, 32);
            OverwriteATile8x8InTile8x8MapAndUpdateTile16Map(startId + i, newtmpdata);
        }

        // Reset the palette(s) according to the entry, all the 16 colors are copied
        for (int n = 0; n < graphicEntries[id].PaletteSlotIDs.size(); n++)
        {
            int palId = graphicEntries[id].PaletteSlotIDs[n];
            for (int i = 0; i < 16; i++)
            {
                tmp_newTilesetPtr->SetColor(palId, i, graphicEntries[id].palettes[palId][i]);
            }
        }

        // update settings in the new tileset instance
        tmp_newTilesetPtr->SetbgGFXlen(graphicEntries[id].TileDataSizeInByte);
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

/// <summary>
/// Delete the foreground Tile8x8s which are never used by any Tile16.
/// </summary>
void TilesetEditDialog::on_pushButton_CleanUpUnusedTile8x8_clicked()
{
    LevelComponents::Tileset *tmp_newTilesetPtr = tilesetEditParams->newTileset;
    int existingTile8x8Num = tmp_newTilesetPtr->GetfgGFXlen() / 32;

    // go through all the existing foreground Tile8x8s
    for(int i = existingTile8x8Num; i > 0; i--)
    {
        int old_tileid = i + 0x40;
        auto tile16array = tmp_newTilesetPtr->GetMap16arrayPtr();
        for (int k = 0; k < Tile16DefaultNum; k++)
        {
            for (int pos = 0; pos < 4; pos++)
            {
                auto tile8 = tile16array[k]->GetTile8X8(pos);
                if (tile8->GetIndex() == old_tileid)
                {
                    goto find_next_tile8x8;
                }
            }
        }

        // delete the Tile8x8 from the Tile8x8 set
        tilesetEditParams->newTileset->DelTile8x8(old_tileid);

find_next_tile8x8: {} // do nothing here
    }

    // update graphicview
    ReRenderTile16Map();
    ReRenderTile8x8Map(SelectedPaletteId);
    SetSelectedTile16(0, true);
    SetSelectedTile8x8(0, true);
}

/// <summary>
/// Find the first Tile16 which uses a specified palette id.
/// </summary>
void TilesetEditDialog::on_pushButton_FindTile16ByItsUsingPaletteId_clicked()
{
    if(!HasInitialized || IsSelectingTile16) return;
    int palette_id = QInputDialog::getInt(this,
                                          tr("WL4Editor"),
                                          tr("Input the (decimal) palette id to find the first Tile16 which uses the specified palette:"),
                                          0, 0, 15);
    QVector<LevelComponents::TileMap16*> tile16array = tilesetEditParams->newTileset->GetMap16arrayPtr();
    for (int i = 0; i < Tile16DefaultNum; i++)
    {
        LevelComponents::TileMap16* tile16 = tile16array[i];
        for(int j = 0; j < 4; ++j)
        {
            int pal = tile16->GetTile8X8(j)->GetPaletteIndex();
            if (palette_id == pal)
            {
                SetSelectedTile16(i, true);
                return;
            }
        }
    }
    QMessageBox::information(this, tr("WL4Editor"), tr("Cannot find any Tile16 using the palette id you specified."));
}


/// <summary>
/// Swap 2 palettes of the Tileset, and let the user decide whether the palette ids saved in the
/// Tile16 set are swapped as well.
/// </summary>
/// <remarks>
/// Only the color data of the 2 palettes is swapped, the Tile8x8 set is not changed because it
/// stores color ids only. The Tile16 set stores the palette id of every Tile8x8 corner, so swapping
/// those ids keeps the rendered Tile16 images unchanged, while keeping the ids makes the rendered
/// Tile16 images use the swapped colors.
/// </remarks>
void TilesetEditDialog::on_pushButton_SwapPalettes_clicked()
{
    if (!HasInitialized)
    {
        return;
    }

    // Ask the user for the 2 palette ids to swap
    bool ok;
    QString input = QInputDialog::getText(this,
                                          tr("WL4Editor"),
                                          tr("Input the ids of the 2 palettes to swap:\n"
                                             "Use hexadecimal palette ids without the \"0x\" prefix,\n"
                                             "and split the 2 ids with an English comma, such as \"4,6\"."),
                                          QLineEdit::Normal, "", &ok);
    if (!ok)
    {
        return;
    }

    QVector<unsigned int> paletteIds;
    if (!ParseCommaSeparatedHexValues(input, paletteIds, 0xF, tr("palette id")))
    {
        return;
    }
    if (paletteIds.size() != 2)
    {
        QMessageBox::warning(this, tr("Warning"),
                             tr("2 palette ids have to be input to perform the swapping!"));
        return;
    }
    if (paletteIds[0] == paletteIds[1])
    {
        QMessageBox::warning(this, tr("Warning"),
                             tr("The 2 palettes to swap cannot be the same one!"));
        return;
    }

    int firstPaletteId = static_cast<int>(paletteIds[0]);
    int secondPaletteId = static_cast<int>(paletteIds[1]);

    // Ask the user whether the palette ids saved in the Tile16 set should be swapped as well
    QMessageBox messageBox(this);
    messageBox.setWindowTitle(tr("Swap Palettes"));
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setText(tr("Swap palette 0x%1 and palette 0x%2 of the Tileset?")
                       .arg(firstPaletteId, 1, 16).arg(secondPaletteId, 1, 16));
    messageBox.setInformativeText(tr("The Tile8x8 set is not affected because it stores color ids only.\n"
                                     "The Tile16 set stores a palette id for every Tile8x8 corner: swap those "
                                     "ids as well to keep the rendered Tile16 images unchanged, or keep the ids "
                                     "to let the Tile16 images use the swapped colors.\n"
                                     "Room layers which use the background Tile8x8 set of this Tileset are not "
                                     "changed here, their colors follow the swapped palette data."));
    QPushButton *keepImagesButton = messageBox.addButton(tr("Keep Tile16 images"), QMessageBox::AcceptRole);
    QPushButton *keepIdsButton = messageBox.addButton(tr("Keep Tile16 palette ids"), QMessageBox::AcceptRole);
    QPushButton *cancelButton = messageBox.addButton(QMessageBox::Cancel);
    messageBox.setDefaultButton(cancelButton);
    messageBox.exec();
    QAbstractButton *clickedButton = messageBox.clickedButton();
    if ((clickedButton != keepImagesButton) && (clickedButton != keepIdsButton))
    {
        return;
    }
    bool swapTile16PaletteIds = (clickedButton == keepImagesButton);

    // Swap the color data of the 2 palettes
    tilesetEditParams->newTileset->SwapPalettes(firstPaletteId, secondPaletteId);

    // Swap the palette ids used by the Tile16 set if the user asks for it
    if (swapTile16PaletteIds)
    {
        SwapPaletteIdsInTile16Set(firstPaletteId, secondPaletteId);
    }

    // Update all the graphicviews
    int selectedTile16 = SelectedTile16;
    ResetPaletteBarGraphicView(SelectedPaletteId);
    SetSelectedColorId(SelectedColorId);
    ReRenderTile8x8Map(SelectedPaletteId);
    SetSelectedTile8x8(SelectedTile8x8, false);
    ReRenderTile16Map();
    SetSelectedTile16(selectedTile16, true);
}

/// <summary>
/// Overwrite 1 or more palettes of the Tileset with the palettes of a chosen graphic manager entry,
/// without changing any Tile8x8 or Tile16 data.
/// </summary>
/// <remarks>
/// This is used to make the palettes of the Tileset match another graphic entry built on the shared
/// background Tile8x8 set (for example the Layer 0 / Layer 3 graphics of a room), or to reuse the
/// palettes of an entry for other customized features. Only entries which carry palette data are
/// selectable, the destination palette slots come from the PaletteSlotIDs of the entry.
/// </remarks>
void TilesetEditDialog::on_pushButton_UseAdditionalBGPals_clicked()
{
    if (!HasInitialized)
    {
        return;
    }

    QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> graphicEntries =
            AssortedGraphicUtils::GetAssortedGraphicsFromROM();
    if (!graphicEntries.size())
    {
        QMessageBox::information(this, tr("Error"), tr("Cannot find graphic entry from the ROM!"));
        return;
    }

    // Collect the entries which carry palette data
    QStringList items;
    QVector<int> entryIds;
    for (int i = 0; i < graphicEntries.size(); ++i)
    {
        if (!graphicEntries[i].PaletteAddress || !graphicEntries[i].PaletteSlotIDs.size())
        {
            continue; // this entry has no palette data which can be used
        }

        // List all the palette ids which can be provided by this entry
        QString paletteIdList;
        for (int j = 0; j < graphicEntries[i].PaletteSlotIDs.size(); ++j)
        {
            if (j)
            {
                paletteIdList += ", ";
            }
            paletteIdList += "0x" + QString::number(graphicEntries[i].PaletteSlotIDs[j], 16);
        }

        // The numeric prefix also keeps the item texts unique, the entry is found by the list index
        items << QString::number(i) + "  " + graphicEntries[i].TileDataName + " - " + graphicEntries[i].MappingDataName
                 + "   " + tr("[palette(s): %1]").arg(paletteIdList);
        entryIds.push_back(i);
    }
    if (!items.size())
    {
        QMessageBox::information(this, tr("Error"),
                                 tr("Cannot find any graphic entry with palette data!\n"
                                    "Import the palette data of the graphic entry in the Graphic Manager first."));
        return;
    }

    // Let the user choose a graphic entry
    QInputDialog dialog;
    dialog.setOptions(QInputDialog::UseListViewForComboBoxItems);
    dialog.setComboBoxItems(items);
    dialog.setLabelText(tr("Select a graphic entry to use its palette(s)."));
    dialog.setWindowTitle("WL4Editor");
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }
    int listIndex = items.indexOf(dialog.textValue());
    if ((listIndex < 0) || (listIndex >= entryIds.size()))
    {
        return;
    }
    AssortedGraphicUtils::AssortedGraphicEntryItem &entry = graphicEntries[entryIds[listIndex]];

    // Check if the palettes to overwrite are used by the current Tile16 set
    QString usedPaletteInfo;
    bool hasUsedPalette = false;
    for (int i = 0; i < entry.PaletteSlotIDs.size(); ++i)
    {
        int paletteId = static_cast<int>(entry.PaletteSlotIDs[i]);
        if ((paletteId < 0) || (paletteId >= 16))
        {
            continue;
        }
        int tile16Count = CountTile16UsingPalette(paletteId);
        if (tile16Count)
        {
            hasUsedPalette = true;
            usedPaletteInfo += tr("  Palette 0x%1: used by %2 Tile16(s)\n")
                    .arg(paletteId, 1, 16).arg(tile16Count);
        }
    }
    if (hasUsedPalette)
    {
        QMessageBox messageBox(this);
        messageBox.setWindowTitle(tr("Confirm Overwrite"));
        messageBox.setIcon(QMessageBox::Warning);
        messageBox.setText(tr("Some of the palettes to overwrite are used by the Tile16 set of this Tileset:"));
        messageBox.setInformativeText(usedPaletteInfo + "\n"
                                      + tr("Overwriting them will change the rendered images of those Tile16s.\n"
                                           "Do you want to continue?"));
        messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        messageBox.setDefaultButton(QMessageBox::No);
        if (messageBox.exec() != QMessageBox::Yes)
        {
            return;
        }
    }

    // Overwrite the palettes, all the 16 colors are copied from the palette data of the entry
    for (int i = 0; i < entry.PaletteSlotIDs.size(); ++i)
    {
        int paletteId = static_cast<int>(entry.PaletteSlotIDs[i]);
        if ((paletteId < 0) || (paletteId >= 16))
        {
            continue;
        }
        if (entry.palettes[paletteId].size() < 16)
        {
            continue;
        }
        for (int colorId = 0; colorId < 16; ++colorId)
        {
            tilesetEditParams->newTileset->SetColor(paletteId, colorId, entry.palettes[paletteId][colorId]);
        }
    }

    // Update all the graphicviews
    int selectedTile16 = SelectedTile16;
    ResetPaletteBarGraphicView(SelectedPaletteId);
    SetSelectedColorId(SelectedColorId);
    ReRenderTile8x8Map(SelectedPaletteId);
    SetSelectedTile8x8(SelectedTile8x8, false);
    ReRenderTile16Map();
    SetSelectedTile16(selectedTile16, true);
}

