#ifndef WL4EDITORWINDOW_H
#define WL4EDITORWINDOW_H

#include <QMainWindow>
#include <QLabel>

#include "LevelComponents/Room.h"
#include "Dialog/ChooseLevelDialog.h"
#include "DockWidget/Tile16DockWidget.h"
#include "Dialog/LevelConfigDialog.h"

namespace Ui {
    class WL4EditorWindow;
}

class WL4EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit WL4EditorWindow(QWidget *parent = 0);
    ~WL4EditorWindow();
    void RenderScreen(LevelComponents::Room *room);
    void SetStatusBarText(char *str);
    int LoadRoom(); //return TilesetID

private slots:
    void on_actionOpen_ROM_triggered();
    void on_loadLevelButton_clicked();
    void on_roomDecreaseButton_clicked();
    void on_roomIncreaseButton_clicked();
    void on_actionLevel_Config_triggered();

private:
    Ui::WL4EditorWindow *ui;
    QLabel *statusBarLabel;
    Tile16DockWidget *Tile16SelecterDockWidget;
};

#endif // WL4EDITORWINDOW_H
