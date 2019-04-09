#include "PatchManagerDialog.h"
#include "ui_PatchManagerDialog.h"
#include <QMessageBox>

/// <summary>
/// Construct an instance of the PatchManagerDialog.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
PatchManagerDialog::PatchManagerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PatchManagerDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("Patch Manager");
    PatchTable = ui->patchManagerTableView;
    // TODO
}

/// <summary>
/// Deconstruct the PatchManagerDialog and clean up its instance objects on the heap.
/// </summary>
PatchManagerDialog::~PatchManagerDialog()
{
    // TODO
    delete ui;
}

/// <summary>
/// Deconstruct the PatchManagerDialog and clean up its instance objects on the heap.
/// </summary>
void PatchManagerDialog::SetButtonsEnabled(bool enable)
{
    ui->removePatchButton->setEnabled(enable);
    ui->editPatchButton->setEnabled(enable);
    ui->savePatchButton->setEnabled(enable);
}

/// <summary>
/// This slot function will be triggered when clicking the TableView.
/// </summary>
void PatchManagerDialog::on_patchManagerTableView_clicked(const QModelIndex &index)
{
    (void) index;
    QItemSelectionModel *select = PatchTable->selectionModel();
    QModelIndexList selectedRows = select->selectedRows();
    bool selected = selectedRows.size();
    SetButtonsEnabled(selected);
}

/// <summary>
/// This slot function will be triggered when clicking the "Add" button.
/// </summary>
void PatchManagerDialog::on_addPatchButton_clicked()
{

}

/// <summary>
/// This slot function will be triggered when clicking the "Edit" button.
/// </summary>
void PatchManagerDialog::on_editPatchButton_clicked()
{
    QItemSelectionModel *select = PatchTable->selectionModel();
    QModelIndexList selectedRows = select->selectedRows();
    if(selectedRows.size() == 1)
    {

    }
    else
    {
        QMessageBox infoPrompt;
        infoPrompt.setWindowTitle(tr("About"));
        infoPrompt.setText(QString("You can only select 1 row at a time to edit"));
        infoPrompt.addButton(tr("Ok"), QMessageBox::NoRole);
        infoPrompt.exec();
    }
}

/// <summary>
/// This slot function will be triggered when clicking the "Remove" button.
/// </summary>
void PatchManagerDialog::on_removePatchButton_clicked()
{
    PatchTable->RemoveSelected();
    ui->removePatchButton->setEnabled(false);
    ui->editPatchButton->setEnabled(false);
}

/// <summary>
/// This slot function will be triggered when clicking the "Save" button.
/// </summary>
void PatchManagerDialog::on_savePatchButton_clicked()
{

}
