#ifndef SPRITESEDITORDIALOG_H
#define SPRITESEDITORDIALOG_H

#include "LevelComponents/Entity.h"
#include "LevelComponents/EntitySet.h"
#include <QVector>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QDialog>

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
    void SetSelectedEntityColorId(int colorID);
    void SetSelectedEntityPaletteId(int paletteID);
    void SetColor(int paletteId, int colorId);
    void SetSelectedSpriteTile(const int tileID);

private slots:
    void on_spinBox_GlobalSpriteId_valueChanged(int arg1);
    void on_spinBox_SpritesetID_valueChanged(int arg1);
    void on_spinBox_SpritesetPaletteID_valueChanged(int arg1);
    void on_pushButton_ResetLoadTable_clicked();

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
    QGraphicsScene *SpritesetTileMAPScene = nullptr; // no need selestionbox for spriteset graphicview
    QGraphicsPixmapItem *SpritesetTilemapping = nullptr;
    QGraphicsScene *PaletteBarScene = nullptr;
    QGraphicsPixmapItem *SelectionBox_Color = nullptr;
    QGraphicsPixmapItem *Palettemapping = nullptr;

    // Functions
    void RenderSpritesTileMap();
    void RenderSpritesPalette();
    void RenderSpritesetTileMapAndResetLoadTable();
    LevelComponents::Entity *FindCurEntity();
    LevelComponents::EntitySet *FindCurEntitySet();
};

#endif // SPRITESEDITORDIALOG_H
