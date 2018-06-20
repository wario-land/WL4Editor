#ifndef WL4EDITORWINDOW_H
#define WL4EDITORWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "LevelComponents/Room.h"
#include "Dialog/ChooseLevelDialog.h"
#include <DockWidget/tile16dockwidget.h>

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
    void LoadRoom();

private slots:
    void on_actionOpen_ROM_triggered();
    void on_loadLevelButton_clicked();
    void on_roomDecreaseButton_clicked();
    void on_roomIncreaseButton_clicked();

private:
    Ui::WL4EditorWindow *ui;
    QLabel *statusBarLabel;
    Tile16DockWidget *Tile16SelecterDockWidget;
};

#endif // WL4EDITORWINDOW_H
