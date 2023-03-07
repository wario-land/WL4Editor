﻿#ifndef DOORCONFIGDIALOG_H
#define DOORCONFIGDIALOG_H

#include <QAbstractTableModel>
#include <QDialog>
#include <QStandardItemModel>
#include <QString>
#include <QScrollBar>
#include <vector>

#include "LevelComponents/Level.h"
#include "LevelComponents/LevelDoorVector.h"

namespace Ui
{
    class DoorConfigDialog;
}

struct TableEntityItem
{
    LevelComponents::Entity *entity; // pointer to entity
    QString entityName;              // name of entity
    QImage entityImage;              // image of entity
    bool visible;                    // unused
};

class EntityFilterTableModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit EntityFilterTableModel(QWidget *_parent);
    ~EntityFilterTableModel();
    void AddEntity(LevelComponents::Entity *entity);
    QList<TableEntityItem> entities;

private:
    QWidget *parent;
};

struct EntitySetItem
{
    int id;       // id of entity set
    bool visible; // visible in ComboBox
};

class DoorConfigDialog : public QDialog
{
    Q_OBJECT

private slots:
    void on_TableView_Checkbox_stateChanged(QStandardItem *item);
    void on_ComboBox_DoorDestinationPicker_currentIndexChanged(int index);
    void on_SpinBox_DoorX_valueChanged(int arg1);
    void on_SpinBox_DoorY_valueChanged(int arg1);
    void on_SpinBox_DoorWidth_valueChanged(int arg1);
    void on_SpinBox_DoorHeight_valueChanged(int arg1);
    void on_ComboBox_DoorType_currentIndexChanged(int index);
    void on_SpinBox_WarioX_valueChanged(int arg1);
    void on_SpinBox_WarioY_valueChanged(int arg1);
    void on_SpinBox_BGM_ID_valueChanged(int arg1);
    void on_ComboBox_EntitySetID_currentIndexChanged(int index);
    void on_pushButton_DeselectAll_clicked();

private:
    Ui::DoorConfigDialog *ui;
    LevelComponents::Level *_currentLevel;           // Use this to reset tmpDestinationRoom
    LevelComponents::LevelDoorVector tmpDoorVec;     // Use this to save the tmp door vec data and do render job, all the changes should be done here first
                                                     // only when user confirms the changes, we save the door data back to the Level instance
    LevelComponents::Room *tmpCurrentRoom = nullptr;      // Use this to render Door preview
    LevelComponents::Room *tmpDestinationRoom = nullptr;  // Use this to render destination Door view
    int LocalDoorID = -1;  // local Door id
    bool IsInitialized = false;
    EntityFilterTableModel *EntityFilterTable;

    void RenderGraphicsView_Preview();
    void RenderGraphicsView_DestinationDoor(int doorIDinRoom);
    void ResetDoorRect();
    void UpdateDoorLayerGraphicsView_Preview();
    void UpdateDoorLayerGraphicsView_DestinationDoor();
    void PopulateTable(LevelComponents::EntitySet entitySet);
    int GetSelectedComboBoxEntitySetID();

    // EntitySet
    void UpdateComboBoxEntitySet();
    // TableView
    void UpdateTableView();

    // visible in EntitySet ComboBox
    QList<EntitySetItem> comboboxEntitySet;

public:
    explicit DoorConfigDialog(QWidget *parent, LevelComponents::Room *currentroom, int localDoorID,
                              LevelComponents::Level *_level);
    ~DoorConfigDialog();
    LevelComponents::LevelDoorVector &GetChangedDoorVectorResult();
    static void StaticInitialization();

    // Enumeration of Door type
    // clang-format off
    static constexpr const char *DoortypeSetData[5] =
    {
        "1: Portal & Door",
        "2: Warp",
        "3: Pipe",
        "4: Boss Door",
        "5: Item Shop Door"
    };

    // Enumeration of Entity names
    static constexpr const char *EntitynameSetData[129] =
    {
        "0x00 Gem box (top-right piece)",
        "0x01 Gem box (bottom-right piece)",
        "0x02 Gem box (bottom-left piece)",
        "0x03 Gem box (top-left piece)",
        "0x04 CD box",
        "0x05 Full health box",
        "0x06 Large diamond",
        "0x07 Frog switch",
        "0x08 Keyzer",
        "0x09 nothing",
        "0x0A nothing",
        "0x0B nothing",
        "0x0C nothing",
        "0x0D nothing",
        "0x0E nothing",
        "0x0F nothing",
        "0x10 Treasure box utility entity",
        "0x11 Yellow Spear-Mask",
        "0x12 Blue Spear-Mask",
        "0x13 Red Spear-Mask",
        "0x14 Rotate Platforms",
        "0x15 Rock",
        "0x16 Skeleton Bird",
        "0x17 Kaentsubo",
        "0x18 Boss: Cuckoo Condor",
        "0x19 Totsumen",
        "0x1A Pig Head Statue",
        "0x1B Moguramen",
        "0x1C Harimen (100 points)",
        "0x1D Harimenzetto",
        "0x1E Bubble",
        "0x1F Togerobo",
        "0x20 Falling Snow",
        "0x21 Spiked Head",
        "0x22 Ball Lightning",
        "0x23 Platform on Track",
        "0x24 Vertical Moving Platform (down first)",
        "0x25 Horizontal Moving Platform (right first)",
        "0x26 Magic Carpets",
        "0x27 Bow Balloon",
        "0x28 Chance Wheel",
        "0x29 Vortex",
        "0x2A Purple Marumen",
        "0x2B Red Marumen",
        "0x2C Boss: Spoiled Rotten",
        "0x2D Domino trigger",
        "0x2E Trigger Push Down 9 o'clock CW",
        "0x2F Trigger Push Up 3 o'clock CCW",
        "0x30 4 Dominoes go Right",
        "0x31 4 Dominoes go Left",
        "0x32 Domino Stairs go Right & Down",
        "0x33 Domino Stairs go Right & Up",
        "0x34 Go Right & Up Domino Stairs",
        "0x35 Go Left & Up Domino Stairs",
        "0x36 Dominoes go Right & Trigger Down",
        "0x37 Dominoes go Left & Trigger Down",
        "0x38 Dominoes go Right & Trigger Up",
        "0x39 Dominoes go Left & Trigger Up",
        "0x3A Domino flag",
        "0x3B Domino Block",
        "0x3C Mayu Bird",
        "0x3D Bat, koumori",
        "0x3E Falling Spikeball",
        "0x3F Ghost, Yurei",
        "0x40 Beezley",
        "0x41 Money Flowers",
        "0x42 Hornworm, Imomushi",
        "0x43 Triangle Toy Block",
        "0x44 Triangle Toy Block Receptor",
        "0x45 Rectangle Toy Block",
        "0x46 Rolling Toy Block",
        "0x47 Toy Door",
        "0x48 Face hammer, Menhanma",
        "0x49 Face axe, Men'ono",
        "0x4A Goggley-Blade CCW horizontal 1x3(rxc)",
        "0x4B Goggley-Blade CCW vertical 3x1(rxc)",
        "0x4C Flying crocodile, Tobawani",
        "0x4D Shieragucchi",
        "0x4E Ringosuki",
        "0x4F Flattening Hammer slow",
        "0x50 Flattening Hammer fast",
        "0x51 Boss: Catbat, doramori",
        "0x52 Vortex (duplicate)",
        "0x53 Swinging Platform long",
        "0x54 Swinging Platform short",
        "0x55 Auto-rotate Platform CW",
        "0x56 Auto-rotate Platform CCW",
        "0x57 Pinball",
        "0x58 Pinball Block (up)",
        "0x59 Pinball Block (down)",
        "0x5A Pinball Block (left)",
        "0x5B Pinball Block (right)",
        "0x5C Pinball-Left Counter",
        "0x5D Final Chance Wheel",
        "0x5E Spikes Emitter (without body)",
        "0x5F Money Sunflower Disk",
        "0x60 snowman, Yeti",
        "0x61 rockman, Iwao",
        "0x62 Lava Spout",
        "0x63 Boss Door 1",
        "0x64 Boss Door 2",
        "0x65 Boss Door 3",
        "0x66 Boss Door 4",
        "0x67 Boss Door 5",
        "0x68 Boss Door 6",
        "0x69 Boss: Aerodent",
        "0x6A Hoggus, Kiyobu",
        "0x6B Denden",
        "0x6C Butatabi",
        "0x6D Deburina",
        "0x6E Folding Door, jyabara",
        "0x6F Purple Pencil",
        "0x70 Blue Pencil",
        "0x71 Red Pencil",
        "0x72 Robo Bird",
        "0x73 Utsuboanko",
        "0x74 Togenobi",
        "0x75 Falling Icicle",
        "0x76 Boss: Cractus, furawana",
        "0x77 Dice",
        "0x78 Toy Car",
        "0x79 Onomi",
        "0x7A Cat in boss corridor",
        "0x7B PET Bottle",
        "0x7C mouja",
        "0x7D Boss: Golden Diva, yoki",
        "0x7E Falling Chandelier",
        "0x7F Crumbling Platform",
        "0x80 Arewo Shitain-Hakase"
    };
    // clang-format on
};

#endif // DOORCONFIGDIALOG_H
