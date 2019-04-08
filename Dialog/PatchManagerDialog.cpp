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
    ui(new Ui::PatchManagerDialog),
    PatchTable(this)
{
    ui->setupUi(this);

    this->setWindowTitle("Patch Manager");
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
