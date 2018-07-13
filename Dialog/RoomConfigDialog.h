#ifndef ROOMCONFIGDIALOG_H
#define ROOMCONFIGDIALOG_H

#include <QDialog>

#include <QGraphicsScene>
#include <QPixmap>
#include <QGraphicsPixmapItem>

#include "WL4Constants.h"
#include "ROMUtils.h"
#include "LevelComponents/Layer.h"  //include Tileset.h

namespace DialogParams
{
    struct RoomConfigParams
    {
        int CurrentTilesetIndex;
        bool Layer0Enable;
        bool Layer0Alpha;
        int LayerPriorityAndAlphaAttr;
        int Layer0MappingTypeParam;
        int RoomWidth;
        int RoomHeight;
        bool Layer2Enable;
        bool BackgroundLayerEnable;
        bool BackgroundLayerAutoScrollEnable;
        // TODO
    };

    // Unuse
    /*
    enum LayerPriorityCases
    {
        L0L1L2L3 = 0,
        L1L0L2L3 = 1, // or 2
        L1L2L0L3 = 3,
    };

    enum AlphaBlendAttrCases
    {
        NoAlpha = 0,
        EVA_7_EVB_16 = 8,
        EVA_10_EVB_16 = 16,
        EVA_13_EVB_16 = 24,
        EVA_16_EVB_16 = 32,
        EVA_16_EVB_0 = 40,
        EVA_13_EVB_3 = 48,
        EVA_10_EVB_6 = 56,
        EVA_7_EVB_9 = 64,
        EVA_5_EVB_11 = 72,
        EVA_3_EVB_13 = 80,
        EVA_0_EVB_16 = 88
    };
    */

}

namespace Ui {
class RoomConfigDialog;
}

class RoomConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RoomConfigDialog(QWidget *parent = 0);
    ~RoomConfigDialog();
    InitDialog();
    InitDialog(DialogParams::RoomConfigParams *CurrentRoomParams);

private slots:
    void on_CheckBox_Layer0Enable_stateChanged(int arg1);
    void on_CheckBox_Layer0Alpha_stateChanged(int arg1);
    void on_ComboBox_Layer0MappingType_currentIndexChanged(int index);
    void on_CheckBox_Layer0AutoScroll_stateChanged(int arg1);
    void on_ComboBox_AlphaBlendAttribute_currentIndexChanged(int index);
    void on_ComboBox_TilesetID_currentIndexChanged(int index);
    void on_SpinBox_RoomWidth_valueChanged(int arg1);
    void on_SpinBox_RoomHeight_valueChanged(int arg1);
    void on_CheckBox_Layer2Enable_stateChanged(int arg1);
    void on_CheckBox_BGLayerAutoScroll_stateChanged(int arg1);
    void on_CheckBox_BGLayerEnable_stateChanged(int arg1);
    void on_ComboBox_LayerPriority_currentIndexChanged(int index);

private:
    Ui::RoomConfigDialog *ui;
    DialogParams::RoomConfigParams *currentParams;
    QGraphicsScene *tmpGraphicviewScene = nullptr;
    int BGLayerdataPtrs[0x5C][3] = {0};
    void SetBGLayerdataPtrs();
    void InitComboBoxItems();
    void ShowTilesetDetails();
    void ShowMappingType20LayerDetails(int _layerdataAddr, LevelComponents::Layer *_tmpLayer);
};

#endif // ROOMCONFIGDIALOG_H
