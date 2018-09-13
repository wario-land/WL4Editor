#ifndef ENTITYFILTERTABLEVIEW_H
#define ENTITYFILTERTABLEVIEW_H

#include <QWidget>
#include <QTableView>

namespace Ui {
    class EntityFilterTableView;
}

class EntityFilterTableView : public QTableView
{
    Q_OBJECT

private:
    ~EntityFilterTableView();

public:
    EntityFilterTableView(QWidget *param);
};

#endif // ENTITYFILTERTABLEVIEW_H
