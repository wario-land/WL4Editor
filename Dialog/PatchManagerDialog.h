#ifndef PATCHMANAGERDIALOG_H
#define PATCHMANAGERDIALOG_H

#include <QDialog>
#include "PatchManagerTableView.h"

namespace Ui {
class PatchManagerDialog;
}

class PatchManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PatchManagerDialog(QWidget *parent = nullptr);
    ~PatchManagerDialog();

private:
    PatchManagerTableView PatchTable;
    Ui::PatchManagerDialog *ui;
};

#endif // PATCHMANAGERDIALOG_H
