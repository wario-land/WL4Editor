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

private slots:
    void on_pushButton_PreviousEntity_clicked();
    void on_pushButton_NextEntity_clicked();

private:
    Ui::EntitySetDockWidget *ui;
    LevelComponents::Room *currentRoom = nullptr;
    int EntityAmount = 0;
    int currentEntityId = 1; // local Entity id, start from 1
    void RenderEntityAndResetInfo();
};

#endif // ENTITYSETDOCKWIDGET_H
