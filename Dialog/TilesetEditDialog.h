#ifndef TILESETEDITDIALOG_H
#define TILESETEDITDIALOG_H

#include <QDialog>
#include <QString>
#include <QImage>
#include <QSpinBox>
#include <QCheckBox>
#include <QScrollBar>
#include <QColorDialog>
#include "LevelComponents/Layer.h"
#include "LevelComponents/Room.h"
#include "LevelComponents/Tileset.h"
#include "LevelComponents/Tile.h"
#include "ROMUtils.h"
#include "RoomPreviewGraphicsView.h"
#include "WL4Constants.h"

namespace DialogParams
{
    struct TilesetEditParams
    {
        int currentTilesetIndex;
        LevelComponents::Tileset *newTileset = nullptr;

        // Default constructor
        TilesetEditParams() { memset(this, 0, sizeof(struct TilesetEditParams)); }

        // Construct this param struct using a Room object
        TilesetEditParams(LevelComponents::Room *room) {
                currentTilesetIndex = room->GetTilesetID();
                newTileset = new LevelComponents::Tileset(room->GetTileset(), room->GetTilesetID());
        }
        ~TilesetEditParams() {}
    };
}

namespace Ui
{
    class RoomConfigDialog;
}


namespace Ui {
class TilesetEditDialog;
}

class TilesetEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TilesetEditDialog(QWidget *parent, DialogParams::TilesetEditParams *tilesetEditParams);
    void SetSelectedTile16(int tile16ID, bool resetscrollbar);
    void SetSelectedTile8x8(unsigned short tileId, bool resetscrollbar);
    void SetSelectedColorId(int newcolorId);
    void SetColor(int newcolorId);
    void CopyTile16AndUpdateGraphic(int from_Tile16, int To_Tile16);
    void SetSpinboxesTile8x8sInfo(LevelComponents::Tile8x8* tile8, QSpinBox* spinBoxID, QSpinBox* spinBoxTextureID, QCheckBox* checkBoxHFlip, QCheckBox* checkBoxVFlip);
    int PaletteBrushValue() {return paletteBrushVal; }
    void SetTile16PaletteId(int tile16ID);
    ~TilesetEditDialog();

private slots:
    void on_spinBox_valueChanged(int arg1);
    void on_spinBox_EventId_valueChanged(int arg1);
    void on_spinBox_TerrainId_valueChanged(int arg1);
    void on_spinBox_TopLeftTileId_valueChanged(int arg1);
    void on_spinBox_TopRightTileId_valueChanged(int arg1);
    void on_spinBox_BottomLeftTileId_valueChanged(int arg1);
    void on_spinBox_BottomRightTileId_valueChanged(int arg1);
    void on_spinBox_TopLeftpaletteId_valueChanged(int arg1);
    void on_spinBox_TopRightpaletteId_valueChanged(int arg1);
    void on_spinBox_BottomLeftpaletteId_valueChanged(int arg1);
    void on_spinBox_BottomRightpaletteId_valueChanged(int arg1);
    void on_checkBox_TopLeftHFlip_toggled(bool checked);
    void on_checkBox_TopRightHFlip_toggled(bool checked);
    void on_checkBox_BottomLeftHFlip_toggled(bool checked);
    void on_checkBox_BottomRightHFlip_toggled(bool checked);
    void on_checkBox_TopLeftVFlip_toggled(bool checked);
    void on_checkBox_TopRightVFlip_toggled(bool checked);
    void on_checkBox_BottomLeftVFlip_toggled(bool checked);
    void on_checkBox_BottomRightVFlip_toggled(bool checked);
    void on_horizontalSlider_valueChanged(int value);
    void on_checkBox_paletteBrush_toggled(bool checked);
    void on_pushButton_SetAnimatedTileSlot_clicked();
    void on_pushButton_ExportTile8x8Map_clicked();
    void on_pushButton_ExportTile16Map_clicked();
    void on_pushButton_ImportTile8x8Graphic_clicked();

private:
    Ui::TilesetEditDialog *ui;

    // members
    DialogParams::TilesetEditParams* tilesetEditParams;
    bool HasInitialized = false;
    QGraphicsScene *PaletteBarScene = nullptr;
    QGraphicsPixmapItem *SelectionBox_Color = nullptr;
    QGraphicsPixmapItem *Palettemapping = nullptr;
    QGraphicsScene *Tile8x8MAPScene = nullptr;
    QGraphicsPixmapItem *SelectionBox_Tile8x8 = nullptr;
    QGraphicsPixmapItem *Tile8x8mapping = nullptr;
    QGraphicsScene *Tile16MAPScene = nullptr;
    QGraphicsPixmapItem *SelectionBox_Tile16 = nullptr;
    QGraphicsPixmapItem *Tile16mapping = nullptr;
    QGraphicsScene *Tile8x8EditorScene = nullptr;
    QGraphicsPixmapItem *Tile8x8Editormapping = nullptr;

    unsigned short SelectedTile8x8 = 0;
    unsigned short SelectedTile16 = 0;
    bool IsSelectingTile16 = false;
    int SelectedColorId = 0;
    int SelectedPaletteId = 0;
    int paletteBrushVal = -1;

    // Setters for Selected Tile16
    void TLTile8x8Reset();
    void TRTile8x8Reset();
    void BLTile8x8Reset();
    void BRTile8x8Reset();

    // functions
    void RenderInitialization();
    void ResetPaletteBarGraphicView(int paletteId);
    void ReRenderTile16Map();
    void ReRenderTile8x8Map(int paletteId);
    void UpdateATile8x8ForSelectedTile16InTilesetData(int tile16Id, int newTile8x8_Id, int position, int new_paletteIndex, bool xflip, bool yflip);
    void OverwriteATile8x8InTile8x8MapAndUpdateTile16Map(int posId, unsigned char *tiledata);
};

#endif // TILESETEDITDIALOG_H
