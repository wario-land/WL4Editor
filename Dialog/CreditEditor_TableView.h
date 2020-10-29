#ifndef CREDITEDITOR_TABLEVIEW_H
#define CREDITEDITOR_TABLEVIEW_H

#include <QTableView>

#include "SettingsUtils.h"

class CreditEditor_TableView : public QTableView
{
    Q_OBJECT
public:
          CreditEditor_TableView(QWidget *parent = nullptr);
private:
      QMenu* menu;
      QAction* actionOneTile;
      QAction* actionUpperTile;
      QAction* actionLowerTile;
      QAction* actionDelete;

      void deleteFunction(QModelIndex indexToDelete);
protected:
      void keyPressEvent(QKeyEvent *event) override;

    private slots:
          void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) override;
          void showContextMenu(const QPoint &pos);
          void doAction(QAction *);

};
#endif // CREDITEDITOR_TABLEVIEW_H
