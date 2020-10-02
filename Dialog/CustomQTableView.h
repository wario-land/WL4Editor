#ifndef CUSTOMQTABLEVIEW_H
#define CUSTOMQTABLEVIEW_H

#include <QTableView>

class CustomQTableView : public QTableView {
    Q_OBJECT
public:
          CustomQTableView(QWidget *parent = nullptr);
          //~CustomQTableView();

private:
    private slots:
          void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) override;
          void showContextMenu(const QPoint &);
};
    //! [Widget definition]

#endif // CUSTOMQTABLEVIEW_H
