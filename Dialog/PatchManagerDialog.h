#ifndef PATCHMANAGERDIALOG_H
#define PATCHMANAGERDIALOG_H

#include <QDialog>
#include "PatchManagerTableView.h"
#include "PatchEditDialog.h"

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
    void on_addPatchButton_clicked();
    void on_editPatchButton_clicked();
    void on_removePatchButton_clicked();
    void on_savePatchButton_clicked();

private:
    PatchManagerTableView *PatchTable;
    Ui::PatchManagerDialog *ui;
    bool dirty = false;
};

#endif // PATCHMANAGERDIALOG_H
