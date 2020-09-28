#include "SpritesEditorDialog.h"
#include "ui_SpritesEditorDialog.h"

SpritesEditorDialog::SpritesEditorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SpritesEditorDialog)
{
    ui->setupUi(this);
}

SpritesEditorDialog::~SpritesEditorDialog()
{
    delete ui;
}
