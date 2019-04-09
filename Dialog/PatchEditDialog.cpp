#include "PatchEditDialog.h"
#include "ui_PatchEditDialog.h"

PatchEditDialog::PatchEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PatchEditDialog)
{
    ui->setupUi(this);

    // Initialize the items in comboBox_PatchType
    QStringList PatchTypeNameSet;
    PatchTypeNameSet <<
        "Binary" << "Assembly" << "C";
    ui->comboBox_PatchType->addItems(PatchTypeNameSet);
    ui->comboBox_PatchType->setCurrentIndex(0);
}

PatchEditDialog::~PatchEditDialog()
{
    delete ui;
}
