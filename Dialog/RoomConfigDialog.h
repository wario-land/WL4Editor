#ifndef ROOMCONFIGDIALOG_H
#define ROOMCONFIGDIALOG_H

#include <QDialog>

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QLabel>
#include <QPixmap>
#include <QScrollBar>
#include <QVector>

#include "LevelComponents/Layer.h"
#include "LevelComponents/Room.h"
#include "LevelComponents/Tileset.h"
#include "ROMUtils.h"
#include "RoomPreviewGraphicsView.h"
#include "WL4Constants.h"

class QComboBox;

namespace DialogParams
{
    struct RoomConfigParams
    {
        int roomID;
        int CurrentTilesetIndex;
        int CurrentEntitySetID;
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
                roomID(room->GetRoomID()),
                CurrentTilesetIndex(room->GetTilesetID()),
                CurrentEntitySetID(room->GetCurrentEntitySetID()),
                Layer0Alpha(room->IsLayer0ColorBlendingEnabled()),
                LayerPriorityAndAlphaAttr(room->GetLayerEffectsParam()),
                Layer0MappingTypeParam(room->GetLayer0MappingParam()),
                Layer0DataPtr((room->GetLayer0MappingParam() & 0x20) ? room->GetLayer(0)->GetDataPtr() : 0),
                RoomWidth(room->GetLayer1Width()), RoomHeight(room->GetLayer1Height()), Layer2MappingTypeParam(room->GetLayer2MappingParam()),
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

    // helper functions
    static unsigned short *ChangeLayerDimensions(int newWidth, int newHeight, int oldWidth, int oldHeight, unsigned short *oldData);
    static bool IsLayer0Tile8x8MappingType(int layer0MappingTypeParam);

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
    void UpdateLayer0PickerAvailability(int tilesetId);

    // One selectable mapping data of the background Tile8x8 set of the current Tileset.
    struct LayerMappingDataOption
    {
        unsigned int Address = 0;
        QString TileDataName;
        QString MappingDataName;
        bool IsVanilla = false;
        int EntryIndex = -1;
    };

    QVector<LayerMappingDataOption> FindLayerMappingDataOptions(int newTilesetId, bool forLayer0);
    void PopulateLayerPickerComboBox(QComboBox *picker,
                                     const QVector<LayerMappingDataOption> &mappingDataOptions,
                                     unsigned int currentAddress);

    // The pointer of the layer data the Room currently uses, so the pickers can keep them selected
    unsigned int CurrentBGLayerPtr = 0;
    unsigned int CurrentLayer0Ptr = 0;

    LevelComponents::Tileset *currentTileset = nullptr;

    // Enumeration of the available tilesets
    // This part is modified from xTibor/steaks
    // Although he uses MIT license in that repository, we think we had better point this out
    // clang-format off
    static constexpr const char *TilesetNamesSetData[0x5C] =
    {
        "00  Debug room",
        "01  Palm Tree Paradise",
        "02  Palm Tree Paradise Cave (yellow)",
        "03  The Big Board (green)",
        "04  The Big Board (yellow)",
        "05  The Big Board (purple)",
        "06  Wildflower Fields",
        "07  Toy Block Tower (cyan)",
        "08  Factory (blue)",
        "09  Wildflower Underground",
        "0A  Wildflower WaterPlace",
        "0B  Wildflower Sunflower",
        "0C  Toy Block Tower (green, purple)",
        "0D  Toy Block Tower (green)",
        "0E  Toy Block Tower (yellow)",
        "0F  Doodle woods",
        "10  Dominoes (green)",
        "11  Hall of Hieroglyphs (water, A button)",
        "12  Haunted House",
        "13  Crescent Moon Village outside",
        "14  Drain",
        "15  Arabian outside",
        "16  Arabian inside",
        "17  Arabian inside grid",
        "18  Arabian (cyen)",
        "19  Arabian (green)",
        "1A  Dominoes (blue)",
        "1B  Dominoes (purple)",
        "1C  Dominoes (teal)",
        "1D  Factory (red)",
        "1E  Factory (green)",
        "1F  Jungle (water current)",
        "20  Factory (scaffold, cyen)",
        "21  Toxic Landfill (gray)",
        "22  Toxic Landfill (yellow)",
        "23  Pinball (spike ball)",
        "24  Pinball (Dinosaur)",
        "25  Pinball (Gorilla)",
        "26  Jungle (light green)",
        "27  40 Below Fridge (machine)",
        "28  Jungle (no water current)",
        "29  Cractus",
        "2A  Hotel (cyen)",
        "2B  Hotel (blue)",
        "2C  Hotel (purple)",
        "2D  Hotel (red)",
        "2E  Hotel (crack wall red)",
        "2F  Hotel (outside)",
        "30  Hotel Unused (Haunted crack wall red)",
        "31  Hotel Unused (Haunted crack wall red)",
        "32  Cardboard (yellow, green)",
        "33  Cardboard (purple)",
        "34  Palm Tree Paradise Caves (brown)",
        "35  Jungle (light green)",
        "36  Wildflower Caves (red)",
        "37  Lava level",
        "38  Jungle Caves",
        "39  Golden Passage (water current)",
        "3A  Hotel (cyan, blue)",
        "3B  Hotel (blue, purple, red)",
        "3C  Hotel (green blue, purple)",
        "3D  Hotel (red purple)",
        "3E  40 Below Fridge (white)",
        "3F  Factory (blue)",
        "40  Factory (green)",
        "41  Arabian (green, dark)",
        "42  Spoiled Rotten",
        "43  Boss corridor (purple)",
        "44  Aerodent",
        "45  Frozen lava",
        "46  Lava and Frozen switch",
        "47  Hall of Hieroglyphs (water, A button revert)",
        "48  Catbat",
        "49  Cuckoo Condor",
        "4A  Boss corridor (green)",
        "4B  Boss corridor (red)",
        "4C  Boss corridor (yellow)",
        "4D  Boss corridor (blue)",
        "4E  Boss corridor (gold)",
        "4F  Golden Diva",
        "50  Hall of Hieroglyphs (frog Tiles)",
        "51  Mystic Lake",
        "52  Wildflower (water current, cloud)",
        "53  Crescent Moon Village (green)",
        "54  Crescent Moon Village (red)",
        "55  Crescent Moon Village (purple)",
        "56  Toy Block Tower (red green yellow)",
        "57  Pinball (Robot)",
        "58  Bonus room (green red switch)",
        "59  Bonus room (green purple switch)",
        "5A  Golden Passage (curtains)",
        "5B  The Big Board goal room"
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

    // clang-format on
};

#endif // ROOMCONFIGDIALOG_H
