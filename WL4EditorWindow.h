#ifndef WL4EDITORWINDOW_H
#define WL4EDITORWINDOW_H

#include <QMainWindow>
#include <QLabel>

#include "LevelComponents/Room.h"
#include "Dialog/ChooseLevelDialog.h"
#include "DockWidget/Tile16DockWidget.h"
#include "DockWidget/EditModeDockWidget.h"
#include "Dialog/LevelConfigDialog.h"

namespace Ui {
    class WL4EditorWindow;
}

class WL4EditorWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::WL4EditorWindow *ui;
    QLabel *statusBarLabel;
    Tile16DockWidget *Tile16SelecterWidget;
    EditModeDockWidget *EditModeWidget;

protected:
    void resizeEvent(QResizeEvent *event);

public:
    explicit WL4EditorWindow(QWidget *parent = 0);
    ~WL4EditorWindow();
    void RenderScreenFull();
    void RenderScreenVisibilityChange();
    void SetStatusBarText(char *str);
    void LoadRoomUIUpdate();

private slots:
    void on_actionOpen_ROM_triggered();
    void on_loadLevelButton_clicked();
    void on_roomDecreaseButton_clicked();
    void on_roomIncreaseButton_clicked();
    void on_actionLevel_Config_triggered();
};

#endif // WL4EDITORWINDOW_H
