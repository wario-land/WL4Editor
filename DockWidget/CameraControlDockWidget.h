#ifndef CAMERACONTROLDOCKWIDGET_H
#define CAMERACONTROLDOCKWIDGET_H

#include <QDockWidget>
#include <QModelIndex>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>

#include <vector>

#include "LevelComponents/Room.h"

namespace Ui
{
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
    int CurrentRoomWidth = 0;
    int CurrentRoomHeight = 0;
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
    void on_AddCameraLimitator_pushButton_clicked();
    void on_DeleteCameraLimitator_pushButton_clicked();
    void on_VerticalSeperate_radioButton_clicked(bool checked);

public:
    explicit CameraControlDockWidget(QWidget *parent = 0);
    ~CameraControlDockWidget();
    void SetCameraControlInfo(LevelComponents::Room *currentroom);
    static void StaticInitialization();

    // clang-format off
    static constexpr const char *CameraLimitatorResetSideTypeNameData[5] =
    {
        "Fixed",
        "0: Can reset left side",
        "1: Can reset right side",
        "2: Can reset upper side",
        "3: Can reset lower side"
    };
    // clang-format on
};

#endif // CAMERACONTROLDOCKWIDGET_H
