#ifndef PATCHEDITDIALOG_H
#define PATCHEDITDIALOG_H

#include <QDialog>

namespace Ui {
class PatchEditDialog;
}

class PatchEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PatchEditDialog(QWidget *parent = nullptr);
    ~PatchEditDialog();

private:
    Ui::PatchEditDialog *ui;
};

#endif // PATCHEDITDIALOG_H
