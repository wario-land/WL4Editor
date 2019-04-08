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

private slots:
    void on_patchManagerTableView_clicked(const QModelIndex &index);

private:
    PatchManagerTableView *PatchTable;
    Ui::PatchManagerDialog *ui;
    int SelectedLine = -1;
};

#endif // PATCHMANAGERDIALOG_H
