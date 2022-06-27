#ifndef SPRITESEDITORDIALOG_H
#define SPRITESEDITORDIALOG_H

#include "LevelComponents/Entity.h"
#include "LevelComponents/EntitySet.h"
#include <QVector>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QDialog>
#include <QModelIndex>
#include <QStandardItem>
#include <QStandardItemModel>

namespace DialogParams
{
    struct EntitiesAndEntitySetsEditParams
    {
        QVector<LevelComponents::Entity*> entities;
        QVector<LevelComponents::EntitySet*> entitySets;
        ~EntitiesAndEntitySetsEditParams() {}
    };
}

namespace Ui {
class SpritesEditorDialog;
}

class SpritesEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SpritesEditorDialog(QWidget *parent, DialogParams::EntitiesAndEntitySetsEditParams *entitiesAndEntitySetsEditParams);
    ~SpritesEditorDialog();
    static void StaticInitialization();

    void SetSelectedEntityColorId(int colorID);
    void SetSelectedEntityPaletteId(int paletteID);
    void SetColor(int paletteId, int colorId);
    void SetSelectedSpriteTile(const int tileID);
    int GetcurEntityId() { return currentEntityID; }

private slots:
    void on_spinBox_GlobalSpriteId_valueChanged(int arg1);
    void on_spinBox_SpritesetID_valueChanged(int arg1);
    void on_spinBox_SpritesetPaletteID_valueChanged(int arg1);
    void on_pushButton_ResetLoadTable_clicked();
    void on_pushButton_SpriteTilesExport_clicked();
    void on_pushButton_SpriteTilesImport_clicked();
    void on_pushButton_SpritePaletteImport_clicked();
    void on_pushButton_SpritePaletteExport_clicked();
    void on_pushButton_DeletePal_clicked();
    void on_pushButton_AddPal_clicked();
    void on_pushButton_SwapPal_clicked();
    void on_pushButton_ResetAllOamData_clicked();
    void on_listView_OamDataList_clicked(const QModelIndex &index);
    void on_pushButton_AddOAM_clicked();
    void on_pushButton_ResetCurOAM_clicked();
    void on_pushButton_ExportOAMData_clicked();
    void on_pushButton_DeleteCurOam_clicked();

private:
    Ui::SpritesEditorDialog *ui;
    DialogParams::EntitiesAndEntitySetsEditParams *entitiesAndEntitySetsEditParam = nullptr;

    int currentEntityID = 0;
    int currentEntitySetID = 0;
    int curEntityPalId = 0;
    int curEntityColorIdInPalette = 0;
    int curEntityTileId = 0;

    QGraphicsScene *SpriteTileMAPScene = nullptr;
    QGraphicsPixmapItem *SelectionBox_Sprite = nullptr;
    QGraphicsPixmapItem *SpriteTilemapping = nullptr;
    QGraphicsScene *SpritesetTileMAPScene = nullptr; // selectionbox no need for spriteset graphicview
    QGraphicsPixmapItem *SpritesetTilemapping = nullptr;
    QGraphicsScene *PaletteBarScene = nullptr;
    QGraphicsPixmapItem *SelectionBox_Color = nullptr;
    QGraphicsPixmapItem *Palettemapping = nullptr;

    // OAM Designer staff
    QGraphicsScene *OAMDesignerMAPScene = nullptr;
    QGraphicsPixmapItem *OAMmapping = nullptr;
    QStandardItemModel *ListViewItemModel = nullptr;
    int SelectedRow_ListView = -1;

    // Functions
    void RenderSpritesTileMap();
    void RenderSpritesPalette();
    void RenderSpritesetTileMapAndResetLoadTable();
    LevelComponents::Entity *GetCurEntityPtr(bool createNewEntity = false);
    LevelComponents::EntitySet *GetCurEntitySetPtr(bool createNewEntitySet = false);
    void RenderOamSet(int selectrow = 0);
    QString GenerateOAMString();
    QString GetOAMArray();
    void handleSelectionChanged();

    // clang-format off
    // shape (using 2 bit) | size (using 2 bit)
    static constexpr const char *OAMShapeTypeNameData[12] =
    {
        "8 x 8",
        "16 x 16",
        "32 x 32",
        "64 x 64",
        "16 x 8",
        "32 x 8",
        "32 x 16",
        "64 x 32",
        "8 x 16",
        "8 x 32",
        "16 x 32",
        "32 x 64"
    };
    // clang-format on
};

#endif // SPRITESEDITORDIALOG_H
