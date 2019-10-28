#include "TilesetEditDialog.h"
#include "ui_TilesetEditDialog.h"


TilesetEditDialog::TilesetEditDialog(QWidget *parent, DialogParams::TilesetEditParams *tilesetEditParam) :
    QDialog(parent),
    ui(new Ui::TilesetEditDialog)
{
    ui->setupUi(this);
    this->tilesetEditParams = tilesetEditParam;
    ui->label_TilesetID->setText("Tileset ID" + QString::number(tilesetEditParam->currentTilesetIndex, 10));

    // render
    RenderAllTile8x8();

    //re-initialize widgets
    SetSelectedTile8x8(0, true);
    setSelectedTile16(0);
}

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
void TilesetEditDialog::setSelectedTile16(int tile16ID)
{
    ui->spinBox_EventId->setValue(tilesetEditParams->selectedTileset->GetEventTablePtr()[tile16ID]);
    ui->spinBox_TerrainId->setValue(tilesetEditParams->selectedTileset->GetTerrainTypeIDTablePtr()[tile16ID]);
    LevelComponents::TileMap16* tile16Data=tilesetEditParams->selectedTileset->GetMap16Data()[tile16ID];
    LevelComponents::Tile8x8* tile8_TL=tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPLEFT);
    LevelComponents::Tile8x8* tile8_TR=tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPRIGHT);
    LevelComponents::Tile8x8* tile8_BL=tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMLEFT);
    LevelComponents::Tile8x8* tile8_BR=tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMRIGHT);

    setTile8x8OnSpinBox(tile8_TL,ui->spinBox_TopLeftTileId,ui->spinBox_TopLeftpaletteId,ui->checkBox_TopLeftHFlip,ui->checkBox_TopLeftVFlip);
    setTile8x8OnSpinBox(tile8_TR,ui->spinBox_TopRightTileId,ui->spinBox_TopRightpaletteId,ui->checkBox_TopRightHFlip,ui->checkBox_TopRightVFlip);
    setTile8x8OnSpinBox(tile8_BL,ui->spinBox_BottomLeftTileId,ui->spinBox_BottomLeftpaletteId,ui->checkBox_BottomLeftHFlip,ui->checkBox_BottomLeftVFlip);
    setTile8x8OnSpinBox(tile8_BR,ui->spinBox_BottomRightTileId,ui->spinBox_BottomRightpaletteId,ui->checkBox_BottomRightHFlip,ui->checkBox_BottomRightVFlip);
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
void TilesetEditDialog::setTile8x8OnSpinBox(LevelComponents::Tile8x8* tile8, QSpinBox* spinBoxID, QSpinBox* spinBoxPaletteID, QCheckBox* checkBoxHFlip, QCheckBox* checkBoxVFlip)
{
    spinBoxID->setValue(tile8->GetIndex());
    spinBoxPaletteID->setValue(tile8->GetPaletteIndex());
    checkBoxHFlip->setChecked(tile8->GetFlipX());
    checkBoxVFlip->setChecked(tile8->GetFlipY());
}

void TilesetEditDialog::on_spinBox_valueChanged(int arg1)
{
    setSelectedTile16(arg1);
}

void TilesetEditDialog::on_spinBox_EventId_valueChanged(int arg1)
{
    // TODO
}

void TilesetEditDialog::on_spinBox_TerrainId_valueChanged(int arg1)
{
    // TODO
}

void TilesetEditDialog::setTile8x8TileID(LevelComponents::Tile8x8* tile8, int tileID)
{
    tile8->SetIndex(tileID);
}

void TilesetEditDialog::setTile8x8PaletteID(LevelComponents::Tile8x8* tile8, int paletteIndex)
{
    tile8->SetPaletteIndex(paletteIndex);
}

void TilesetEditDialog::setTile8x8HFlip(LevelComponents::Tile8x8* tile8, bool hFlip)
{
    tile8->SetFlipX(hFlip);
}

void TilesetEditDialog::setTile8x8VFlip(LevelComponents::Tile8x8* tile8, bool vFlip)
{
    tile8->SetFlipY(vFlip);
}

void TilesetEditDialog::on_spinBox_TopLeftTileId_valueChanged(int arg1)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_TL = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPLEFT);
    setTile8x8TileID(tile8_TL, arg1);
}

void TilesetEditDialog::on_spinBox_TopRightTileId_valueChanged(int arg1)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_TR = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPRIGHT);
    setTile8x8TileID(tile8_TR, arg1);
}

void TilesetEditDialog::on_spinBox_BottomLeftTileId_valueChanged(int arg1)
{
    LevelComponents::TileMap16* tile16Data=tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_BL = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMLEFT);
    setTile8x8TileID(tile8_BL, arg1);
}

void TilesetEditDialog::on_spinBox_BottomRightTileId_valueChanged(int arg1)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_BR = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMRIGHT);
    setTile8x8TileID(tile8_BR, arg1);
}

void TilesetEditDialog::on_spinBox_TopLeftpaletteId_valueChanged(int arg1)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_TL = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPLEFT);
    setTile8x8PaletteID(tile8_TL, arg1);
}

void TilesetEditDialog::on_spinBox_TopRightpaletteId_valueChanged(int arg1)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_TR = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPRIGHT);
    setTile8x8PaletteID(tile8_TR, arg1);
}

void TilesetEditDialog::on_spinBox_BottomLeftpaletteId_valueChanged(int arg1)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_BL = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMLEFT);
    setTile8x8PaletteID(tile8_BL, arg1);
}

void TilesetEditDialog::on_spinBox_BottomRightpaletteId_valueChanged(int arg1)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_BR = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMRIGHT);
    setTile8x8PaletteID(tile8_BR, arg1);
}

void TilesetEditDialog::on_checkBox_TopLeftHFlip_toggled(bool checked)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_TL = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPLEFT);
    setTile8x8HFlip(tile8_TL, checked);
}

void TilesetEditDialog::on_checkBox_TopRightHFlip_toggled(bool checked)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_TR = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPRIGHT);
    setTile8x8HFlip(tile8_TR, checked);
}

void TilesetEditDialog::on_checkBox_BottomLeftHFlip_toggled(bool checked)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_BL = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMLEFT);
    setTile8x8HFlip(tile8_BL, checked);
}

void TilesetEditDialog::on_checkBox_BottomRightHFlip_toggled(bool checked)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_BR = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMRIGHT);
    setTile8x8HFlip(tile8_BR, checked);
}


void TilesetEditDialog::on_checkBox_TopLeftVFlip_toggled(bool checked)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_TL = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPLEFT);
    setTile8x8VFlip(tile8_TL, checked);
}

void TilesetEditDialog::on_checkBox_TopRightVFlip_toggled(bool checked)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_TR = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPRIGHT);
    setTile8x8VFlip(tile8_TR, checked);
}

void TilesetEditDialog::on_checkBox_BottomLeftVFlip_toggled(bool checked)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_BL = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMLEFT);
    setTile8x8VFlip(tile8_BL, checked);
}

void TilesetEditDialog::on_checkBox_BottomRightVFlip_toggled(bool checked)
{
    LevelComponents::TileMap16* tile16Data = tilesetEditParams->selectedTileset->GetMap16Data()[SelectedTile16];
    LevelComponents::Tile8x8* tile8_BR = tile16Data->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMRIGHT);
    setTile8x8VFlip(tile8_BR, checked);
}

void TilesetEditDialog::RenderAllTile8x8()
{
    // Set up scene
    Tile8x8MAPScene = new QGraphicsScene(0, 0, 8 * 32, 0x600 / 4);
    Tile8x8MAPScene->addPixmap(tilesetEditParams->selectedTileset->RenderTile8x8());

    // Add the highlighted tile rectangle
    QPixmap selectionPixmap(8, 8);
    const QColor highlightColor(0xFF, 0, 0, 0x7F);
    selectionPixmap.fill(highlightColor);
    SelectionBox_Tile8x8 = Tile8x8MAPScene->addPixmap(selectionPixmap);
    SelectionBox_Tile8x8->setVisible(false);

    ui->graphicsView_TilesetAllTile8x8->setScene(Tile8x8MAPScene);
    ui->graphicsView_TilesetAllTile8x8->setAlignment(Qt::AlignTop | Qt::AlignLeft);
}

/// <summary>
/// Set the selected tile8x8 index and update the position of the highlight square.
/// </summary>
/// <param name="tileId">
/// The tile8x8 index that was selected in the graphicsView_Tile8x8Editor.
/// </param>
void TilesetEditDialog::SetSelectedTile8x8(unsigned short tileId, bool resetscrollbar)
{
    // Paint red Box to show selected Tile16
    int X = tileId & 7;
    int Y = tileId >> 3;
    SelectionBox_Tile8x8->setPos(X * 16, Y * 16);
    SelectionBox_Tile8x8->setVisible(true);
    SelectedTile8x8 = tileId;

    // Set vertical scrollbar of braphicview
    if (resetscrollbar)
        ui->graphicsView_TilesetAllTile8x8->verticalScrollBar()->setValue(8 * (tileId / 32));
}
