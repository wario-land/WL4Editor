#ifndef ENTITYSETDOCKWIDGET_H
#define ENTITYSETDOCKWIDGET_H

#include <QDockWidget>

#include "LevelComponents/Room.h"

namespace Ui
{
    class EntitySetDockWidget;
}

class EntitySetDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit EntitySetDockWidget(QWidget *parent = 0);
    ~EntitySetDockWidget();
    void ResetEntitySet(LevelComponents::Room *currentroom);
    int GetCurrentEntityLocalId() { return currentEntityId; }
    void SetCurrentEntity(int entityindex);

private slots:
    void on_pushButton_PreviousEntity_clicked();
    void on_pushButton_NextEntity_clicked();
    void on_pushButton_firstEntity_clicked();
    void on_pushButton_lastEntity_clicked();
    void on_pushButton_0x10Entity_clicked();

private:
    Ui::EntitySetDockWidget *ui;
    LevelComponents::Room *currentRoom = nullptr;
    int EntityMaxNum = 0;
    int currentEntityId = 0; // local Entity id
    void RenderEntityAndResetInfo();
};

#endif // ENTITYSETDOCKWIDGET_H
