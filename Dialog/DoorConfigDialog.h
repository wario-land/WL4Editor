#ifndef DOORCONFIGDIALOG_H
#define DOORCONFIGDIALOG_H

#include <QDialog>

#include "LevelComponents/Room.h" // Include "Door.h" inside

namespace Ui {
class DoorConfigDialog;
}

class DoorConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DoorConfigDialog(QWidget *parent, LevelComponents::Room *currentroom, int doorID, std::vector<LevelComponents::Room*> _levelrooms);
    ~DoorConfigDialog();
    static void StaticInitialization();

private:
    Ui::DoorConfigDialog *ui;
    std::vector<LevelComponents::Room*> Levelrooms;
    LevelComponents::Room *CurrentRoom = nullptr;
    int DoorID = -1;
    void InitRenderGraphicsView_Preview();
    void InitRenderGraphicsView_DestinationDoor(LevelComponents::Room *currentRoom, int doorIDinRoom);

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
