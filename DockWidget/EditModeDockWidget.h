#ifndef EDITMODEDOCKWIDGET_H
#define EDITMODEDOCKWIDGET_H

#include <QDockWidget>

namespace Ui {
class EditModeDockWidget;
}

class EditModeDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit EditModeDockWidget(QWidget *parent = 0);
    ~EditModeDockWidget();

private:
    Ui::EditModeDockWidget *ui;
};

#endif // EDITMODEDOCKWIDGET_H
