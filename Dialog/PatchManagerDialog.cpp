#include "PatchManagerDialog.h"
#include "ui_PatchManagerDialog.h"

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
/// This slot function will be triggered when clicking the TableView.
/// </summary>
void PatchManagerDialog::on_patchManagerTableView_clicked(const QModelIndex &index)
{
    SelectedLine = index.row();
    ui->removePatchButton->setEnabled(true);
    ui->editPatchButton->setEnabled(true);
}
