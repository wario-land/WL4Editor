#ifndef DOORCONFIGDIALOG_H
#define DOORCONFIGDIALOG_H

#include <QDialog>

namespace Ui {
class DoorConfigDialog;
}

class DoorConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DoorConfigDialog(QWidget *parent = 0);
    ~DoorConfigDialog();

private:
    Ui::DoorConfigDialog *ui;
};

#endif // DOORCONFIGDIALOG_H
