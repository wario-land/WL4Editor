#ifndef DOORCONFIGDIALOG_H
#define DOORCONFIGDIALOG_H

#include <QDialog>
#include <QMessageBox>

#include "LevelComponents/Level.h"

namespace Ui {
class DoorConfigDialog;
}

class DoorConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DoorConfigDialog(QWidget *parent, LevelComponents::Room *currentroom, int doorID, LevelComponents::Level *_level);
    ~DoorConfigDialog();
    static void StaticComboBoxesInitialization();
    static void StaticEntitySetsInitialization();

private slots:
    void on_ComboBox_DoorDestinationPicker_currentIndexChanged(int index);
    void on_SpinBox_DoorX_valueChanged(int arg1);
    void on_SpinBox_DoorY_valueChanged(int arg1);
    void on_SpinBox_DoorWidth_valueChanged(int arg1);
    void on_SpinBox_DoorHeight_valueChanged(int arg1);
    void on_ComboBox_DoorType_currentIndexChanged(int index);
    void on_SpinBox_WarioX_valueChanged(int arg1);
    void on_SpinBox_WarioY_valueChanged(int arg1);
    void on_SpinBox_BGM_ID_valueChanged(int arg1);

private:
    Ui::DoorConfigDialog *ui;
    LevelComponents::Level *_currentLevel;
    LevelComponents::Room *tmpCurrentRoom = nullptr;
    LevelComponents::Room *tmpDestinationRoom = nullptr;
    int DoorID = -1;
    bool IsInitialized = false;
    static LevelComponents::EntitySet *entitiessets[90];
    static LevelComponents::Entity *entities[129];
    void RenderGraphicsView_Preview();
    void RenderGraphicsView_DestinationDoor(int doorIDinRoom);
    void ResetDoorRect();
    void UpdateDoorLayerGraphicsView_Preview();
    void UpdateDoorLayerGraphicsView_DestinationDoor();

    // Enumeration of Door type
    static constexpr const char *DoortypeSetData[5] =
    {
        "1: Portal",
        "2: Room Edge",
        "3: Door or Pipe",
        "4: Item Shop Door",
        "5: Boss Door"
    };

    // Enumeration of Entity name
    static constexpr const char *EntitynameSetData[128] =
    {
        "0x01 Box with top-right quadrant",
        "0x02 Box with bottom-right quadrant",
        "0x03 Box with bottom-left quadrant",
        "0x04 Box with top-left quadrant",
        "0x05 CD box",
        "0x06 Health box",
        "0x07 Large diamond",
        "0x08 Frog switch",
        "0x09 Keyzer",
        "0x0A nothing",
        "0x0B nothing",
        "0x0C nothing",
        "0x0D nothing",
        "0x0E nothing",
        "0x0F nothing",
        "0x10 cat",
        "0x11 yellow Spear-Mask",
        "0x12 blue Spear-Mask",
        "0x13 red Spear-Mask",
        "0x14 Rotate Platforms",
        "0x15 Rock",
        "0x16 Skeleton Bird",
        "0x17 Kaentsubo",
        "0x18 Boss: Cuckoo Condor",
        "0x19 Totsumen",
        "0x1A Pig Head Statue",
        "0x1B Moguramen",
        "0x1B Moguramen",
        "0x1C Harimen (100 points)",
        "0x1D Harimenzetto",
        "0x1E Bobble",
        "0x1F Togerobo",
        "0x20 falling snow",
        "0x21 Spiked Head",
        "0x22 ball lightning",
        "0x23 drive-by-wire Platform",
        "0x24 vertical moving platform go down first",
        "0x25 horizontal moving platform go right first",
        "0x26 Magic Carpets",
        "0x27 Bow Balloon",
        "0x28 chance wheel",
        "0x29 Vortex",
        "0x2A purple Marumen",
        "0x2B red Marumen",
        "0x2C Boss: Spoiled Rotten",
        "0x2D nothing",
        "0x2E trigger push down 9 o'clock CW",
        "0x2F trigger push up 3 o'clock CCW",
        "0x30 4 dominoes go right",
        "0x31 4 dominoes go left",
        "0x32 dominoes stares go right & down",
        "0x33 dominoes stares go right & up",
        "0x34 go right & up dominoes stares",
        "0x35 go left & up dominoes stares",
        "0x36 dominoes go right & trigger down",
        "0x37 dominoes go left & trigger down",
        "0x38 dominoes go right & trigger up",
        "0x39 dominoes go left & trigger up",
        "0x3A Domino terminal flag point",
        "0x3B dise block",
        "0x3C Mayu Bird",
        "0x3D Minicula",
        "0x3E falling Spikeball",
        "0x3F Yūrei",
        "0x40 Beezley",
        "0x41 Money Flowers",
        "0x42 Imomushi",
        "0x43 Triangle Toy Block",
        "0x44 nothing",
        "0x45 Rectangle Toy Block",
        "0x46 Rolling Toy Block",
        "0x47 Toy Door",
        "0x48 Menhanmā",
        "0x49 Men'ono",
        "0x4A Goggley-Blade 100 points",
        "0x4B Goggley-Blade 200 points",
        "0x4C Tobawani",
        "0x4D Shieragucchi",
        "0x4E Ringosukī",
        "0x4F Steam Hammer",
        "0x50 Steam Hammer 2rd",
        "0x51 Boss: Catbat",
        "0x52 Vortex, duplicated",
        "0x53 Swinging Platform",
        "0x54 Swinging Platform 2rd",
        "0x55 Auto-rotate Platform CW",
        "0x56 Auto-rotate Platform CCW",
        "0x57 Pinball",
        "0x58 Pinball block upside open",
        "0x59 Pinball block downside open",
        "0x5A Pinball block leftside open",
        "0x5B Pinball block rightside open",
        "0x5C Existing Pinball counter",
        "0x5D final chance wheel",
        "0x5E Spikes emitter (without body)",
        "0x5F money sunflower disk",
        "0x60 Yeti",
        "0x61 Iwao",
        "0x62 spouting lava",
        "0x63 boss door 1",
        "0x64 boss door 2",
        "0x65 boss door 3",
        "0x66 boss door 4",
        "0x67 boss door 5",
        "0x68 boss door 6",
        "0x69 Boss: Aerodent",
        "0x6A Hoggus",
        "0x6B Denden",
        "0x6C Butatabi",
        "0x6E folding door",
        "0x6F perple pencil",
        "0x70 blue pencil",
        "0x71 red pencil",
        "0x72 Robo Bird",
        "0x73 Utsuboankō",
        "0x74 Togenobi",
        "0x75 falling icicle",
        "0x76 Cractus",
        "0x77 Dice",
        "0x78 Toy Car 10 points",
        "0x79 Onomī",
        "0x7A cat in bass corrider",
        "0x7B PET Bottom",
        "0x7C Pearl Bird",
        "0x7D Golden Diva",
        "0x7E falling Chandelier",
        "0x7F unsteady step",
        "0x80 Arewo Shitain-hakase"
    };
};

#endif // DOORCONFIGDIALOG_H
