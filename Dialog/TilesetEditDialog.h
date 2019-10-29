#ifndef TILESETEDITDIALOG_H
#define TILESETEDITDIALOG_H

#include <QDialog>
#include <QString>
#include <QSpinBox>
#include <QCheckBox>
#include <QScrollBar>
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
        LevelComponents::Tileset *selectedTileset = nullptr;

        // Default constructor
        TilesetEditParams() { memset(this, 0, sizeof(struct TilesetEditParams)); }

        // Construct this param struct using a Room object
        TilesetEditParams(LevelComponents::Room *room) {
                currentTilesetIndex = room->GetTilesetID();
                selectedTileset = new LevelComponents::Tileset(room->GetTileset(), room->GetTilesetID());
        }
        ~TilesetEditParams(){};
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
    void setTile8x8OnSpinBox(LevelComponents::Tile8x8* tile8, QSpinBox* spinBoxID, QSpinBox* spinBoxTextureID, QCheckBox* checkBoxHFlip, QCheckBox* checkBoxVFlip);
    void setTile8x8TileID(LevelComponents::Tile8x8* tile8, int tileID);
    void setTile8x8PaletteID(LevelComponents::Tile8x8* tile8, int paletteIndex);
    void setTile8x8HFlip(LevelComponents::Tile8x8* tile8, bool hFlip);
    void setTile8x8VFlip(LevelComponents::Tile8x8* tile8, bool vFlip);
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

private:
    Ui::TilesetEditDialog *ui;

    // members
    DialogParams::TilesetEditParams* tilesetEditParams;

    QGraphicsScene *Tile8x8MAPScene = nullptr;
    QGraphicsPixmapItem *SelectionBox_Tile8x8 = nullptr;
    QGraphicsPixmapItem *Tile8x8mapping = nullptr;
    QGraphicsScene *Tile16MAPScene = nullptr;
    QGraphicsPixmapItem *SelectionBox_Tile16 = nullptr;
    QGraphicsPixmapItem *Tile16mapping = nullptr;
    unsigned short SelectedTile8x8 = 0;
    unsigned short SelectedTile16 = 0;

    // functions
    void RenderInitialization();
    void SetSelectedTile8x8(unsigned short tileId, bool resetscrollbar);
};

#endif // TILESETEDITDIALOG_H
