#include "LevelConfigDialog.h"
#include "ui_LevelConfigDialog.h"

LevelConfigDialog::LevelConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LevelConfigDialog)
{
    ui->setupUi(this);
    this->setFixedSize(this->width(), this->height());
    QRegExp regx1("[a-zA-Z0-9\\s]{1, 26}");  //still have problem here, I don't know why this cannot limit what I type in the levelname_textbox (ssp)
    this->LevelnameRegx = new QRegExp(regx1);
    QRegExp regx2("[0-9][:][0-5][0-9]");
    this->TimerRegx = new QRegExp(regx2);
    ui->LevelName_TextBox->setMaxLength(26);
    ui->LevelName_TextBox->setValidator(new QRegExpValidator(*(this->LevelnameRegx), ui->LevelName_TextBox));
    ui->HModeTimer_TextBox->setValidator(new QRegExpValidator(*(this->TimerRegx), ui->HModeTimer_TextBox));
    ui->NModeTimer_TextBox->setValidator(new QRegExpValidator(*(this->TimerRegx), ui->NModeTimer_TextBox));
    ui->SHModeTimer_TextBox->setValidator(new QRegExpValidator(*(this->TimerRegx), ui->SHModeTimer_TextBox));
}

LevelConfigDialog::~LevelConfigDialog()
{
    //delete this->LevelnameRegx;
    //delete this->TimerRegx;  // these 2 lines cause a crash (ssp)
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
