#ifndef CREDITSEDITOR_TABLEVIEW_H
#define CREDITSEDITOR_TABLEVIEW_H

#include <QTableView>

#include "SettingsUtils.h"

class CreditEditor_TableView : public QTableView
{
    Q_OBJECT
public:
          CreditEditor_TableView(QWidget *parent = nullptr);
private:
      QMenu* menu;
      QAction* action_one_tile;
      QAction* action_upper_tile;
      QAction* action_lower_tile;
      QAction* action_delete;

      void deleteFunction(QModelIndex indexToDelete);
protected:
      void keyPressEvent(QKeyEvent *event) override;

    private slots:
          void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) override;
          void showContextMenu(const QPoint &pos);
          void doAction(QAction *);

};
#endif // CREDITSEDITOR_TABLEVIEW_H
