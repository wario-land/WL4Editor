#ifndef CAMERACONTROLDOCKWIDGET_H
#define CAMERACONTROLDOCKWIDGET_H

#include <QString>
#include <QDockWidget>

#include "LevelComponents/Room.h"

namespace Ui {
class CameraControlDockWidget;
}

class CameraControlDockWidget : public QDockWidget
{
    Q_OBJECT

private:
    Ui::CameraControlDockWidget *ui;
    LevelComponents::Room *currentRoom = nullptr;

public:
    explicit CameraControlDockWidget(QWidget *parent = 0);
    ~CameraControlDockWidget();
    void ResetCameraControlInfo(LevelComponents::Room *currentroom);
    static void StaticInitialization();

    static constexpr const char *CameraLimitatorResetSideTypeNameData[5] =
    {
        "Fixed",
        "0: Can reset upper side",
        "1: Can reset lower side",
        "2: Can reset left side",
        "3: Can reset right side"
    };
};

#endif // CAMERACONTROLDOCKWIDGET_H
