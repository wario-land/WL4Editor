#ifndef PATCHEDITDIALOG_H
#define PATCHEDITDIALOG_H

#include <QDialog>
#include "PatchManagerTableView.h"

namespace Ui {
class PatchEditDialog;
}

class PatchEditDialog : public QDialog
{
    Q_OBJECT

public:
    PatchEditDialog(QWidget *parent) :
        PatchEditDialog(parent, { "", PatchType::C, 0, "", (unsigned int) -1, 0, "", "" }) {} // default UI values
    PatchEditDialog(QWidget *parent, struct PatchEntryItem patchEntry);
    ~PatchEditDialog();
    struct PatchEntryItem CreatePatchEntry();
    static void StaticComboBoxesInitialization();

private slots:
    void on_pushButton_Browse_clicked();

private:
    Ui::PatchEditDialog *ui;
    void InitializeComponents(struct PatchEntryItem patchEntry);
    QRegExpValidator *addressvalidator;
};

#endif // PATCHEDITDIALOG_H
