#ifndef ROOMCONFIGDIALOG_H
#define ROOMCONFIGDIALOG_H

#include <QDialog>

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QLabel>
#include <QPixmap>
#include <QScrollBar>

#include "LevelComponents/Layer.h"
#include "LevelComponents/Room.h"
#include "LevelComponents/Tileset.h"
#include "ROMUtils.h"
#include "RoomPreviewGraphicsView.h"
#include "WL4Constants.h"

namespace DialogParams
{
    struct RoomConfigParams
    {
        int CurrentTilesetIndex;
        bool Layer0Alpha;
        int LayerPriorityAndAlphaAttr;
        int Layer0MappingTypeParam;
        int Layer0DataPtr;
        int Layer0Width;
        int Layer0Height;
        int RoomWidth;
        int RoomHeight;
        int Layer2MappingTypeParam;

        bool BackgroundLayerEnable;
        unsigned char BGLayerScrollFlag;
        int BackgroundLayerDataPtr;
        unsigned short *LayerData[3]; //always copy the layerdata, don't pass the pointer directly
        unsigned char RasterType;
        unsigned char Water;
        unsigned short BGMVolume;

        // Default constructor
        RoomConfigParams() { memset(this, 0, sizeof(struct RoomConfigParams)); }

        // Construct this param struct using a Room object
        RoomConfigParams(LevelComponents::Room *room) :
                CurrentTilesetIndex(room->GetTilesetID()),
                Layer0Alpha(room->IsLayer0ColorBlendingEnabled()),
                LayerPriorityAndAlphaAttr(room->GetLayerEffectsParam()),
                Layer0MappingTypeParam(room->GetLayer0MappingParam()),
                Layer0DataPtr((room->GetLayer0MappingParam() & 0x20) ? room->GetLayer(0)->GetDataPtr() : 0),
                RoomWidth(room->GetWidth()), RoomHeight(room->GetHeight()), Layer2MappingTypeParam(room->GetLayer2MappingParam()),
                BackgroundLayerEnable(room->IsBGLayerEnabled()), RasterType(room->GetLayerGFXEffect01()),
                Water(room->GetLayerGFXEffect02()), BGMVolume(room->GetBgmvolume())
        {
            if((Layer0MappingTypeParam & 0x10) == LevelComponents::LayerMap16){
                Layer0Width = room->GetLayer(0)->GetLayerWidth(); Layer0Height = room->GetLayer(0)->GetLayerHeight();
            } else {
                Layer0Width = 0; Layer0Height = 0;
            }
            for (int i = 0; i < 3; i++) {
                if (room->GetLayer(i)->GetMappingType() == LevelComponents::LayerMap16) {
                    LayerData[i] = room->GetLayer(i)->CreateLayerDataCopy();
                } else {
                    LayerData[i] = nullptr;
                }
            }
            if (BackgroundLayerEnable) {
                BackgroundLayerDataPtr = room->GetLayer(3)->GetDataPtr();
            } else {
                BackgroundLayerDataPtr = WL4Constants::BGLayerDefaultPtr;
            }
            BGLayerScrollFlag = room->GetBGLayerScrollFlag();
        }

        ~RoomConfigParams()
        {
            // new and delete by myself.
            for (int i = 0; i < 3; i++)
            {
                if (LayerData[i]) delete[] LayerData[i];
            }
        }
    };
} // namespace DialogParams

namespace Ui
{
    class RoomConfigDialog;
}

class RoomConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RoomConfigDialog(QWidget *parent, DialogParams::RoomConfigParams *CurrentRoomParams);
    ~RoomConfigDialog();
    static void StaticComboBoxesInitialization();
    DialogParams::RoomConfigParams *GetConfigParams(DialogParams::RoomConfigParams *prevRoomParams);

private slots:
    void on_CheckBox_Layer0Alpha_stateChanged(int state);
    void on_ComboBox_TilesetID_currentIndexChanged(int index);
    void on_CheckBox_BGLayerEnable_stateChanged(int state);
    void on_ComboBox_BGLayerPicker_currentIndexChanged(int index);
    void on_ComboBox_Layer0Picker_currentIndexChanged(int index);
    void on_ComboBox_LayerPriority_currentIndexChanged(int index);
    void on_SpinBox_RoomWidth_valueChanged(int arg1);
    void on_SpinBox_RoomHeight_valueChanged(int arg1);
    void on_spinBox_Layer0Width_valueChanged(int arg1);
    void on_spinBox_Layer0Height_valueChanged(int arg1);
    void on_spinBox_BGLayerScrollingFlag_valueChanged(int arg1);
    void on_spinBox_Layer0MappingType_valueChanged(int arg1);
    void on_spinBox_RasterType_valueChanged(int arg1);
    void on_spinBox_Layer2MappingType_valueChanged(int arg1);

private:
    bool ComboBoxInitialized = false;
    Ui::RoomConfigDialog *ui;
    void ShowTilesetDetails(int tilesetIndex);
    void ShowMappingType20LayerDetails(int _layerdataAddr, LevelComponents::Layer *_tmpLayer);

    void ResetBGLayerPickerComboBox(int newTilesetId);

    LevelComponents::Tileset *currentTileset = nullptr;

    // Enumeration of the available tilesets
    // This part is modified from xTibor/steaks
    // Although he uses MIT license in that repository, we think we had better point this out
    // clang-format off
    static constexpr const char *TilesetNamesSetData[0x5C] =
    {
        "00  Debug room",
        "01  Palm Tree Paradise",
        "02  Caves",
        "03  The Big Board",
        "04  The Big Board",
        "05  The Big Board (indoor)",
        "06  Wildflower Fields",
        "07  Toy Block Tower",
        "08  Factory",
        "09  Wildflower Underground",
        "0A  Wildflower WaterPlace",
        "0B  Wildflower Sunflower",
        "0C  Toy Block Tower",
        "0D  Toy Block Tower",
        "0E  Toy Block Tower",
        "0F  Doodle woods",
        "10  Dominoes",
        "11  Hall of Hieroglyphs",
        "12  Haunted House",
        "13  Crescent Moon Village outside",
        "14  Drain",
        "15  Arabian outside",
        "16  Arabian inside",
        "17  Arabian",
        "18  Arabian",
        "19  Arabian",
        "1A  Dominoes (blue)",
        "1B  Dominoes (purple)",
        "1C  Dominoes (teal)",
        "1D  Factory",
        "1E  Factory",
        "1F  Jungle",
        "20  Factory",
        "21  Toxic Landfill",
        "22  Toxic Landfill",
        "23  Pinball",
        "24  Pinball",
        "25  Pinball (with Gorilla)",
        "26  Jungle",
        "27  40 Below Fridge",
        "28  Jungle",
        "29  Cractus",
        "2A  Hotel",
        "2B  Hotel",
        "2C  Hotel",
        "2D  Hotel",
        "2E  Hotel",
        "2F  Hotel (outside)",
        "30  Unused in-game (Haunted House)",
        "31  Unused in-game (Haunted House)",
        "32  Cardboard",
        "33  Cardboard",
        "34  Caves",
        "35  Jungle",
        "36  Caves",
        "37  Lava level",
        "38  Caves",
        "39  Golden Passage",
        "3A  Hotel",
        "3B  Hotel",
        "3C  Hotel",
        "3D  Hotel",
        "3E  40 Below Fridge",
        "3F  Factory",
        "40  Factory",
        "41  Arabian",
        "42  Spoiled Rotten",
        "43  Boss corridor",
        "44  Aerodent",
        "45  Frozen lava level",
        "46  Lava level",
        "47  Hall of Hieroglyphs",
        "48  Catbat",
        "49  Cuckoo Condor",
        "4A  Boss corridor",
        "4B  Boss corridor",
        "4C  Boss corridor",
        "4D  Boss corridor",
        "4E  Boss corridor",
        "4F  Golden Diva",
        "50  Hall of Hieroglyphs",
        "51  Mystic Lake",
        "52  Wildflower",
        "53  Crescent Moon Village",
        "54  Crescent Moon Village",
        "55  Crescent Moon Village",
        "56  Toy Block Tower",
        "57  Pinball",
        "58  Bonus room",
        "59  Bonus room",
        "5A  Golden Passage",
        "5B  The Big Board end"
    };

    // Enumeration of the available layer priority settings
    static constexpr const char *LayerPrioritySetData[4] =
    {
        "L0 (Top) > L1 > L2 > L3 (Bottom)",
        "L1 (Top) > L0 > L2 > L3 (Bottom)",
        "L1 (Top) > L0 > L2 > L3 (Bottom) SP+",
        "L1 (Top) > L2 > L0 > L3 (Bottom) SP+"
    };

    // Enumerations of the available alpha blend settings
    static constexpr const char *AlphaBlendAttrsSetData[12] =
    {
        "No Alpha Blending",
        "EVA 44%,  EVB 100%",
        "EVA 63%,  EVB 100%",
        "EVA 81%,  EVB 100%",
        "EVA 100%, EVB 100%",
        "EVA 100%, EVB 0%",
        "EVA 81%,  EVB 19%",
        "EVA 63%,  EVB 37%",
        "EVA 44%,  EVB 56%",
        "EVA 31%,  EVB 68%",
        "EVA 19%,  EVB 81%",
        "EVA 00%,  EVB 100%"
    };

    // Enumeration of the available BGs per tileset (RLE format)
    static constexpr const unsigned int BGLayerdataPtrsData[166] =
    {
        0,                            // Tileset 0x00
        1, WL4Constants::BG_0x5FB2CC,
        1, WL4Constants::BG_0x5FB8DC,
        1, WL4Constants::BG_0x603064,
        1, WL4Constants::BG_0x603064, // 0x04
        1, WL4Constants::BG_0x60368C,
        2, WL4Constants::BG_0x5FC9A0, WL4Constants::BG_0x5FC2D0,
        1, WL4Constants::BG_0x6045C4,
        1, WL4Constants::BG_0x5FFD94, // 0x08
        0,
        1, WL4Constants::BG_0x5FD078,
        1, WL4Constants::BG_0x5FD484,
        1, WL4Constants::BG_0x603E98, // 0x0C
        0,
        1, WL4Constants::BG_0x604ACC,
        3, WL4Constants::BG_0x605A7C, WL4Constants::BG_0x6063B0, WL4Constants::BG_0x606CF4,
        1, WL4Constants::BG_0x6074C4, // 0x10
        1, WL4Constants::BG_0x5FA6D0,
        1, WL4Constants::BG_0x60A1D8,
        1, WL4Constants::BG_0x60A1E8,
        1, WL4Constants::BG_0x60A4B4, // 0x14
        1, WL4Constants::BG_0x6094F4,
        0,
        1, WL4Constants::BG_0x6094F4,
        1, WL4Constants::BG_0x6094F4, // 0x18
        1, WL4Constants::BG_0x6094F4,
        1, WL4Constants::BG_0x607CD0,
        1, WL4Constants::BG_0x6084DC,
        1, WL4Constants::BG_0x608CE8, // 0x1C
        1, WL4Constants::BG_0x600388,
        1, WL4Constants::BG_0x600EF8,
        2, WL4Constants::BG_0x5FD680, WL4Constants::BG_0x5FD9BC,
        1, WL4Constants::BG_0x6006C4, // 0x20
        1, WL4Constants::BG_0x6013D4,
        1, WL4Constants::BG_0x601A0C,
        0,
        0,                            // 0x24
        0,
        2, WL4Constants::BG_0x5FE918, WL4Constants::BG_0x5FEED8,
        1, WL4Constants::BG_0x60221C,
        1, WL4Constants::BG_0x5FE540, // 0x28
        1, WL4Constants::BG_0x60E860,
        0,
        0,
        0,                            // 0x2C
        0,
        0,
        1, WL4Constants::BG_0x60AD10,
        0,                            // 0x30
        0,
        0,
        0,
        1, WL4Constants::BG_0x5FB8DC, // 0x34
        1, WL4Constants::BG_0x5FF264,
        1, WL4Constants::BG_0x5FF960,
        2, WL4Constants::BG_0x60BC54, WL4Constants::BG_0x60B29C,
        1, WL4Constants::BG_0x5FF684, // 0x38
        1, WL4Constants::BG_0x60ED78,
        0,
        0,
        0,                            // 0x3C
        0,
        1, WL4Constants::BG_0x602858,
        1, WL4Constants::BG_0x5FFD94,
        1, WL4Constants::BG_0x600EF8, // 0x40
        1, WL4Constants::BG_0x609A84,
        1, WL4Constants::BG_0x60E860,
        1, WL4Constants::BG_0x60E96C,
        1, WL4Constants::BG_0x60E860, // 0x44
        2, WL4Constants::BG_0x60CF98, WL4Constants::BG_0x60C5FC,
        1, WL4Constants::BG_0x60E044,
        1, WL4Constants::BG_0x5FA6D0,
        1, WL4Constants::BG_0x60E860, // 0x48
        1, WL4Constants::BG_0x60E860,
        1, WL4Constants::BG_0x60E96C,
        1, WL4Constants::BG_0x60E96C,
        1, WL4Constants::BG_0x60E96C, // 0x4C
        1, WL4Constants::BG_0x60E96C,
        1, WL4Constants::BG_0x60E96C,
        1, WL4Constants::BG_0x60E870,
        1, WL4Constants::BG_0x5FA6D0, // 0x50
        1, WL4Constants::BG_0x5FE008,
        1, WL4Constants::BG_0x5FD484,
        1, WL4Constants::BG_0x60A350,
        1, WL4Constants::BG_0x60A1E8, // 0x54
        1, WL4Constants::BG_0x60A1E8,
        1, WL4Constants::BG_0x605270,
        0,
        0,                            // 0x58
        0,
        0,
        0
    };

    // Enumeration of Tileset BG tile8x8 set data address (RLE format)
    static constexpr const unsigned int VanillaTilesetBGTilesDataAddr[0x5C] =
    {
        WL4Constants::Tileset_BGTile_0x00,
        WL4Constants::Tileset_BGTile_0x01,
        WL4Constants::Tileset_BGTile_0x02,
        WL4Constants::Tileset_BGTile_0x03,
        WL4Constants::Tileset_BGTile_0x04,
        WL4Constants::Tileset_BGTile_0x05,
        WL4Constants::Tileset_BGTile_0x06,
        WL4Constants::Tileset_BGTile_0x07,
        WL4Constants::Tileset_BGTile_0x08,
        WL4Constants::Tileset_BGTile_0x09,
        WL4Constants::Tileset_BGTile_0x0A,
        WL4Constants::Tileset_BGTile_0x0B,
        WL4Constants::Tileset_BGTile_0x0C,
        WL4Constants::Tileset_BGTile_0x0D,
        WL4Constants::Tileset_BGTile_0x0E,
        WL4Constants::Tileset_BGTile_0x0F,
        WL4Constants::Tileset_BGTile_0x10,
        WL4Constants::Tileset_BGTile_0x11,
        WL4Constants::Tileset_BGTile_0x12,
        WL4Constants::Tileset_BGTile_0x13,
        WL4Constants::Tileset_BGTile_0x14,
        WL4Constants::Tileset_BGTile_0x15,
        WL4Constants::Tileset_BGTile_0x16,
        WL4Constants::Tileset_BGTile_0x17,
        WL4Constants::Tileset_BGTile_0x18,
        WL4Constants::Tileset_BGTile_0x19,
        WL4Constants::Tileset_BGTile_0x1A,
        WL4Constants::Tileset_BGTile_0x1B,
        WL4Constants::Tileset_BGTile_0x1C,
        WL4Constants::Tileset_BGTile_0x1D,
        WL4Constants::Tileset_BGTile_0x1E,
        WL4Constants::Tileset_BGTile_0x1F,
        WL4Constants::Tileset_BGTile_0x20,
        WL4Constants::Tileset_BGTile_0x21,
        WL4Constants::Tileset_BGTile_0x22,
        WL4Constants::Tileset_BGTile_0x23,
        WL4Constants::Tileset_BGTile_0x24,
        WL4Constants::Tileset_BGTile_0x25,
        WL4Constants::Tileset_BGTile_0x26,
        WL4Constants::Tileset_BGTile_0x27,
        WL4Constants::Tileset_BGTile_0x28,
        WL4Constants::Tileset_BGTile_0x29,
        WL4Constants::Tileset_BGTile_0x2A,
        WL4Constants::Tileset_BGTile_0x2B,
        WL4Constants::Tileset_BGTile_0x2C,
        WL4Constants::Tileset_BGTile_0x2D,
        WL4Constants::Tileset_BGTile_0x2E,
        WL4Constants::Tileset_BGTile_0x2F,
        WL4Constants::Tileset_BGTile_0x30,
        WL4Constants::Tileset_BGTile_0x31,
        WL4Constants::Tileset_BGTile_0x32,
        WL4Constants::Tileset_BGTile_0x33,
        WL4Constants::Tileset_BGTile_0x34,
        WL4Constants::Tileset_BGTile_0x35,
        WL4Constants::Tileset_BGTile_0x36,
        WL4Constants::Tileset_BGTile_0x37,
        WL4Constants::Tileset_BGTile_0x38,
        WL4Constants::Tileset_BGTile_0x39,
        WL4Constants::Tileset_BGTile_0x3A,
        WL4Constants::Tileset_BGTile_0x3B,
        WL4Constants::Tileset_BGTile_0x3C,
        WL4Constants::Tileset_BGTile_0x3D,
        WL4Constants::Tileset_BGTile_0x3E,
        WL4Constants::Tileset_BGTile_0x3F,
        WL4Constants::Tileset_BGTile_0x40,
        WL4Constants::Tileset_BGTile_0x41,
        WL4Constants::Tileset_BGTile_0x42,
        WL4Constants::Tileset_BGTile_0x43,
        WL4Constants::Tileset_BGTile_0x44,
        WL4Constants::Tileset_BGTile_0x45,
        WL4Constants::Tileset_BGTile_0x46,
        WL4Constants::Tileset_BGTile_0x47,
        WL4Constants::Tileset_BGTile_0x48,
        WL4Constants::Tileset_BGTile_0x49,
        WL4Constants::Tileset_BGTile_0x4A,
        WL4Constants::Tileset_BGTile_0x4B,
        WL4Constants::Tileset_BGTile_0x4C,
        WL4Constants::Tileset_BGTile_0x4D,
        WL4Constants::Tileset_BGTile_0x4E,
        WL4Constants::Tileset_BGTile_0x4F,
        WL4Constants::Tileset_BGTile_0x50,
        WL4Constants::Tileset_BGTile_0x51,
        WL4Constants::Tileset_BGTile_0x52,
        WL4Constants::Tileset_BGTile_0x53,
        WL4Constants::Tileset_BGTile_0x54,
        WL4Constants::Tileset_BGTile_0x55,
        WL4Constants::Tileset_BGTile_0x56,
        WL4Constants::Tileset_BGTile_0x57,
        WL4Constants::Tileset_BGTile_0x58,
        WL4Constants::Tileset_BGTile_0x59,
        WL4Constants::Tileset_BGTile_0x5A,
        WL4Constants::Tileset_BGTile_0x5B
    };
    // clang-format on
};

#endif // ROOMCONFIGDIALOG_H
