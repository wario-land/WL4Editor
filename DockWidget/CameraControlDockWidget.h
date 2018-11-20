#ifndef CAMERACONTROLDOCKWIDGET_H
#define CAMERACONTROLDOCKWIDGET_H

#include <QString>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QDockWidget>

#include <vector>

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
    QStandardItemModel *ListViewItemModel = nullptr;
    void ClearCurrentLimitatorSetting();

private slots:
    void on_CameraLimitators_listView_clicked(const QModelIndex &index);

public:
    explicit CameraControlDockWidget(QWidget *parent = 0);
    ~CameraControlDockWidget();
    void SetCameraControlInfo(LevelComponents::Room *currentroom);
    static void StaticInitialization();

    static constexpr const char *CameraLimitatorResetSideTypeNameData[5] =
    {
        "Fixed",
        "0: Can reset left side",
        "1: Can reset right side",
        "2: Can reset upper side",
        "3: Can reset lower side"
    };
};

#endif // CAMERACONTROLDOCKWIDGET_H
