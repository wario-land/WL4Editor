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
    int SelectedLimitator = -1;
    bool IsSavingData = false;
    void SetCurrentLimitator();
    void SetListviewItemText(int row);
    void PaintListView();
    void ClearListView();

private slots:
    void on_CameraLimitators_listView_clicked(const QModelIndex &index);
    void on_CameraLimitatorTypePicker_comboBox_currentIndexChanged(int index);
    void on_spinBox_x1_valueChanged(int arg1);
    void on_spinBox_y1_valueChanged(int arg1);
    void on_spinBox_width_valueChanged(int arg1);
    void on_spinBox_height_valueChanged(int arg1);
    void on_LimitatorSideOffset_spinBox_valueChanged(int arg1);
    void on_TriggerBlockPositionX_spinBox_valueChanged(int arg1);
    void on_TriggerBlockPositionY_spinBox_valueChanged(int arg1);
    void on_CameraYFixed_radioButton_clicked(bool checked);
    void on_FollowWario_radioButton_clicked(bool checked);
    void on_UseCameraLimitators_radioButton_clicked(bool checked);

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
