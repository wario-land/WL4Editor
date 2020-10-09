#ifndef CUSTOMQTABLEVIEW_H
#define CUSTOMQTABLEVIEW_H

#include <QTableView>

class CreditEditor_TableView : public QTableView {
    Q_OBJECT
public:
          CreditEditor_TableView(QWidget *parent = nullptr);
          //~CustomQTableView();
private:
      QMenu* menu;
      QAction* action_one_tile;
      QAction* action_upper_tile;
      QAction* action_lower_tile;
      QAction* action_delete;

protected:
      void keyPressEvent(QKeyEvent *event) override;

    private slots:
          void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) override;
          void showContextMenu(const QPoint &pos);
          void doAction(QAction *);

};
    //! [Widget definition]

#endif // CUSTOMQTABLEVIEW_H
