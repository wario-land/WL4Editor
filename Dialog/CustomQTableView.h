#ifndef CUSTOMQTABLEVIEW_H
#define CUSTOMQTABLEVIEW_H

#include <QTableView>

class CustomQTableView : public QTableView {
    Q_OBJECT
public:
          CustomQTableView(QWidget *parent = nullptr);
          //~CustomQTableView();
private:
      QMenu* menu;
      QAction* action_one_tile;
      QAction* action_upper_tile;
      QAction* action_lower_tile;
      QAction* action_delete;

    private slots:
          void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) override;
          void showContextMenu(const QPoint &pos);
          void doAction(QAction *);
          void keyPressEvent(QKeyEvent *event) override;
};
    //! [Widget definition]

#endif // CUSTOMQTABLEVIEW_H
