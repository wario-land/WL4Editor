#include "LevelConfigDialog.h"
#include "ui_LevelConfigDialog.h"

LevelConfigDialog::LevelConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LevelConfigDialog)
{
    ui->setupUi(this);
    this->setFixedSize(this->width(), this->height());
}

LevelConfigDialog::~LevelConfigDialog()
{
    delete ui;
}

void LevelConfigDialog::InitTextBoxes(std::string _levelname, int HModeTimer, int NModeTimer, int SHModeTimer)
{
    // trim(_levelname) and Show LevelName
    _levelname.erase(0, _levelname.find_first_not_of(" "));
    _levelname.erase(_levelname.find_last_not_of(" ") + 1);
    ui->LevelName_TextBox->setText(QString::fromStdString(_levelname));

    // Parse and Show Timers
    int a, b, c;
    a = HModeTimer / 60; b = (HModeTimer - 60 * a) / 10; c = HModeTimer - 60 * a - 10 * b;
    ui->HModeTimer_TextBox->setText(QString::number(a, 10) + ":" + QString::number(b, 10) + QString::number(c, 10));
    a = NModeTimer / 60; b = (NModeTimer - 60 * a) / 10; c = NModeTimer - 60 * a - 10 * b;
    ui->NModeTimer_TextBox->setText(QString::number(a, 10) + ":" + QString::number(b, 10) + QString::number(c, 10));
    a = SHModeTimer / 60; b = (SHModeTimer - 60 * a) / 10; c = SHModeTimer - 60 * a - 10 * b;
    ui->SHModeTimer_TextBox->setText(QString::number(a, 10) + ":" + QString::number(b, 10) + QString::number(c, 10));
}
