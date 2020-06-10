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
        PatchEditDialog(parent,
        {
            "C:/Users/Andrew/Desktop/PatchCode/URB.c",
            PatchType::C,
            0x6C75E,
            "034886460148004702E0P67C70608C046C046C046C046C046C046C046C046C046",
            (unsigned int) 10,
            0,
            ""
        }) {} // default UI values
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
