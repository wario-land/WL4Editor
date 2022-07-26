#ifndef ANIMATEDTILEGROUPEDITORDIALOG_H
#define ANIMATEDTILEGROUPEDITORDIALOG_H

#include <QDialog>

#include "LevelComponents/AnimatedTile8x8Group.h"
#include "LevelComponents/Tileset.h"

namespace DialogParams
{
    struct AnimatedTileGroupsEditParams
    {
        QVector<LevelComponents::AnimatedTile8x8Group*> animatedTileGroups;
        ~AnimatedTileGroupsEditParams() {}
    };
}

namespace Ui {
class AnimatedTileGroupEditorDialog;
}

class AnimatedTileGroupEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AnimatedTileGroupEditorDialog(QWidget *parent, LevelComponents::Tileset *_currentTileset, DialogParams::AnimatedTileGroupsEditParams *param);
    ~AnimatedTileGroupEditorDialog();

private slots:
    void on_spinBox_AnimationType_valueChanged(int arg1);
    void on_spinBox_GlobalID_valueChanged(int arg1);
    void on_spinBox_paletteId_valueChanged(int arg1);

private:
    Ui::AnimatedTileGroupEditorDialog *ui;
    LevelComponents::Tileset *currentTileset = nullptr;
    DialogParams::AnimatedTileGroupsEditParams *animatedTileGroupsEditParam = nullptr;

    // helper functions
    void ExtractAnimatedTileGroupInfoToUI(unsigned int _animatedTileGroup_globalId);
    void UpdatePaletteGraphicView();
    void UpdateTileArrayGraphicView(unsigned int paletteId);
    LevelComponents::AnimatedTile8x8Group *GetAnimatedTileGroupPtr(bool createNewAnimatedTileGroup = false);
};

#endif // ANIMATEDTILEGROUPEDITORDIALOG_H
