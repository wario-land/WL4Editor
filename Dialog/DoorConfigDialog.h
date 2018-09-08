#ifndef DOORCONFIGDIALOG_H
#define DOORCONFIGDIALOG_H

#include <QDialog>

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
    static void StaticInitialization();

private:
    Ui::DoorConfigDialog *ui;
    LevelComponents::Level *_currentLevel;
    LevelComponents::Room *tmpCurrentRoom = nullptr;
    LevelComponents::Room *tmpDestinationRoom = nullptr;
    int DoorID = -1;
    void RenderGraphicsView_Preview();
    void RenderGraphicsView_DestinationDoor(int doorIDinRoom);

    // Enumeration of Door type
    static constexpr const char *DoortypeSetData[5] =
    {
        "1. Portal",
        "2, Room Edge",
        "3. Door or Tube",
        "4. Item Shop Door",
        "5. Boss Door"
    };
};

#endif // DOORCONFIGDIALOG_H
