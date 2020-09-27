#ifndef CREDITSEDITDIALOG_H
#define CREDITSEDITDIALOG_H

#include <QDialog>
#include <QString>
#include <QImage>
#include <QSpinBox>
#include <QCheckBox>
#include <QScrollBar>
#include <QColorDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QTableView>
#include "LevelComponents/Layer.h"
#include "LevelComponents/Room.h"
#include "LevelComponents/Tileset.h"
#include "LevelComponents/Tile.h"
#include "ROMUtils.h"
#include "RoomPreviewGraphicsView.h"
#include "WL4Constants.h"
#include "Dialog/SelectColorDialog.h"
#include <unordered_map>

namespace DialogParams
{
    struct CreditsEditParams
    {
        /*int currentTilesetIndex;
        LevelComponents::Tileset *newTileset = nullptr;*/

        // Default constructor
        CreditsEditParams() { memset(this, 0, sizeof(struct CreditsEditParams)); }
        ~CreditsEditParams() {}
    };
}

namespace Ui
{
    class RoomConfigDialog;
}


namespace Ui {
class CreditsEditDialog;
}

class CreditsEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreditsEditDialog(QWidget *parent, DialogParams::CreditsEditParams *creditsEditParams);
    /*void SetSelectedTile16(int tile16ID, bool resetscrollbar);
    void SetSelectedTile8x8(unsigned short tileId, bool resetscrollbar);
    void SetSelectedColorId(int newcolorId);
    void SetColor(int newcolorId);
    void CopyTile16AndUpdateGraphic(int from_Tile16, int To_Tile16);
    void SetSpinboxesTile8x8sInfo(LevelComponents::Tile8x8* tile8, QSpinBox* spinBoxID, QSpinBox* spinBoxTextureID, QCheckBox* checkBoxHFlip, QCheckBox* checkBoxVFlip);
    void DeleteFGTile8x8(int tile8x8id);
    int PaletteBrushValue() {return paletteBrushVal; }
    void SetTile16PaletteId(int tile16ID);

    int GetSelectedTile8x8() { return SelectedTile8x8; }
    int GetFGTile8x8Num() { return tilesetEditParams->newTileset->GetfgGFXlen() / 32; }*/

    ~CreditsEditDialog();

private slots:/*
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
    void on_pushButton_ExportTile16sCombinationData_clicked();
    void on_pushButton_ImportTile16sCombinationData_clicked();
    void on_pushButton_ExportPalette_clicked();
    void on_pushButton_ImportPalette_clicked();*/

private:
    Ui::CreditsEditDialog *ui;

    // members
    DialogParams::CreditsEditParams* creditsEditParams;
    QWidget* tabs[13];
    QTableView* tables_view[13];

    std::unordered_map<short,char> map= {
        //One-tile high
        {0x4340,'A'},
        {0x4341,'B'},
        {0x4342,'C'},
        {0x4343,'D'},
        {0x4344,'E'},
        {0x4345,'F'},
        {0x4346,'G'},
        {0x4347,'H'},
        {0x4348,'I'},
        {0x4349,'J'},
        {0x434A,'K'},
        {0x434B,'L'},
        {0x434C,'M'},
        {0x434D,'N'},
        {0x434E,'O'},
        {0x434F,'P'},
        {0x4350,'Q'},
        {0x4351,'R'},
        {0x4352,'S'},
        {0x4353,'T'},
        {0x4354,'U'},
        {0x4355,'V'},
        {0x4356,'W'},
        {0x4357,'X'},
        {0x4358,'Y'},
        {0x4359,'Z'},
        {0x435A,'.'},
        {0x435B,','},

        //Two-tile high, upper half
        {0x0360,'A'},
        {0x0361,'B'},
        {0x0362,'C'},
        {0x0363,'D'},
        {0x0364,'E'},
        {0x0365,'F'},
        {0x0366,'G'},
        {0x0367,'H'},
        {0x0368,'I'},
        {0x0369,'J'},
        {0x036A,'K'},
        {0x036B,'L'},
        {0x036C,'M'},
        {0x036D,'N'},
        {0x036E,'O'},
        {0x036F,'P'},
        {0x0370,'Q'},
        {0x0371,'R'},
        {0x0372,'S'},
        {0x0373,'T'},
        {0x0374,'U'},
        {0x0375,'V'},
        {0x0376,'W'},
        {0x0377,'X'},
        {0x0378,'Y'},
        {0x0379,'Z'},

        {0x03A0,'a'},
        {0x03A1,'b'},
        {0x03A2,'c'},
        {0x03A3,'d'},
        {0x03A4,'e'},
        {0x03A5,'f'},
        {0x03A6,'g'},
        {0x03A7,'h'},
        {0x03A8,'i'},
        {0x03A9,'j'},
        {0x03AA,'k'},
        {0x03AB,'l'},
        {0x03AC,'m'},
        {0x03AD,'n'},
        {0x03AE,'o'},
        {0x03AF,'p'},
        {0x03B0,'q'},
        {0x03B1,'r'},
        {0x03B2,'s'},
        {0x03B3,'t'},
        {0x03B4,'u'},
        {0x03B5,'v'},
        {0x03B6,'w'},
        {0x03B7,'x'},
        {0x03B8,'y'},
        //{0x03B9,'z'},

        //Two-tile high, lower half
        {0x0380,'A'},
        {0x0381,'B'},
        {0x0382,'C'},
        {0x0383,'D'},
        {0x0384,'E'},
        {0x0385,'F'},
        {0x0386,'G'},
        {0x0387,'H'},
        {0x0388,'I'},
        {0x0389,'J'},
        {0x038A,'K'},
        {0x038B,'L'},
        {0x038C,'M'},
        {0x038D,'N'},
        {0x038E,'O'},
        {0x038F,'P'},
        {0x0390,'Q'},
        {0x0391,'R'},
        {0x0392,'S'},
        {0x0393,'T'},
        {0x0394,'U'},
        {0x0395,'V'},
        {0x0396,'W'},
        {0x0397,'X'},
        {0x0398,'Y'},
        {0x0399,'Z'},

        {0x03C0,'a'},
        {0x03C1,'b'},
        {0x03C2,'c'},
        {0x03C3,'d'},
        {0x03C4,'e'},
        {0x03C5,'f'},
        {0x03C6,'g'},
        {0x03C7,'h'},
        {0x03C8,'i'},
        {0x03C9,'j'},
        {0x03CA,'k'},
        {0x03CB,'l'},
        {0x03CC,'m'},
        {0x03CD,'n'},
        {0x03CE,'o'},
        {0x03CF,'p'},
        {0x03D0,'q'},
        {0x03D1,'r'},
        {0x03D2,'s'},
        {0x03D3,'t'},
        {0x03D4,'u'},
        {0x03D5,'v'},
        {0x03D6,'w'},
        {0x03D7,'x'},
        {0x03D8,'y'},
        {0x03D9,'.'}
    };
    /*bool HasInitialized = false;
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
    void OverwriteATile8x8InTile8x8MapAndUpdateTile16Map(int posId, unsigned char *tiledata);*/
};

#endif // CREDITSEDITDIALOG_H
