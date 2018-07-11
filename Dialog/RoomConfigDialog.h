#ifndef ROOMCONFIGDIALOG_H
#define ROOMCONFIGDIALOG_H

#include <QDialog>

namespace Ui {
class RoomConfigDialog;
}

class RoomConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RoomConfigDialog(QWidget *parent = 0);
    ~RoomConfigDialog();

private:
    Ui::RoomConfigDialog *ui;
};

#endif // ROOMCONFIGDIALOG_H
