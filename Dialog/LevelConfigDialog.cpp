#include "LevelConfigDialog.h"
#include "ui_LevelConfigDialog.h"

#include <QPushButton>

QRegExp LevelnameRegx("^[A-Za-z0-9\\s]+$");
QRegExp TimerRegx("^[0-9]:[0-5][0-9]$");

LevelConfigDialog::LevelConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LevelConfigDialog)
{
    ui->setupUi(this);

    ui->LevelName_TextBox->setAttribute(Qt::WA_InputMethodEnabled, false);
    ui->LevelName_TextBox->setValidator(new QRegExpValidator(LevelnameRegx, ui->LevelName_TextBox));
    ui->HModeTimer_TextBox->setAttribute(Qt::WA_InputMethodEnabled, false);
    ui->HModeTimer_TextBox->setValidator(new QRegExpValidator(TimerRegx, ui->HModeTimer_TextBox));
    ui->NModeTimer_TextBox->setAttribute(Qt::WA_InputMethodEnabled, false);
    ui->NModeTimer_TextBox->setValidator(new QRegExpValidator(TimerRegx, ui->NModeTimer_TextBox));
    ui->SHModeTimer_TextBox->setAttribute(Qt::WA_InputMethodEnabled, false);
    ui->SHModeTimer_TextBox->setValidator(new QRegExpValidator(TimerRegx, ui->SHModeTimer_TextBox));
}

LevelConfigDialog::~LevelConfigDialog()
{
    //delete this->LevelnameRegx;
    //delete this->TimerRegx;  // these 2 lines cause a crash (ssp)
    delete ui;
}

void LevelConfigDialog::InitTextBoxes(std::string _levelname, int HModeTimer, int NModeTimer, int SHModeTimer)
{
    // trimmed(_levelname) and Show LevelName
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

std::string LevelConfigDialog::GetNewLevelName()
{
    QString tmplevelname = ui->LevelName_TextBox->text();
    int a, b;
    a = (26 - tmplevelname.length()) / 2;
    b = (26 - tmplevelname.length()) / 2 + (26 - tmplevelname.length()) % 2;
    QString stra, strb;
    stra.fill(' ', a);
    strb.fill(' ', b);
    stra = stra + tmplevelname + strb;
    return stra.toStdString();
}

int LevelConfigDialog::GetHModeTimer()
{
    int a, b, c;
    a = (int)(ui->HModeTimer_TextBox->text().at(0).unicode()) - 48;
    b = (int)(ui->HModeTimer_TextBox->text().at(2).unicode()) - 48;
    c = (int)(ui->HModeTimer_TextBox->text().at(3).unicode()) - 48;
    return (60 * a + 10 * b + c);
}

int LevelConfigDialog::GetNModeTimer()
{
    int a, b, c;
    a = (int)(ui->NModeTimer_TextBox->text().at(0).unicode()) - 48;
    b = (int)(ui->NModeTimer_TextBox->text().at(2).unicode()) - 48;
    c = (int)(ui->NModeTimer_TextBox->text().at(3).unicode()) - 48;
    return (60 * a + 10 * b + c);
}

int LevelConfigDialog::GetSHModeTimer()
{
    int a, b, c;
    a = (int)(ui->SHModeTimer_TextBox->text().at(0).unicode()) - 48;
    b = (int)(ui->SHModeTimer_TextBox->text().at(2).unicode()) - 48;
    c = (int)(ui->SHModeTimer_TextBox->text().at(3).unicode()) - 48;
    return (60 * a + 10 * b + c);
}

void SetOKButtonEnable(QVector<QLineEdit*> textBoxes, QPushButton *okButton)
{
    bool allValid = true;
    for(auto iter = textBoxes.begin(); iter != textBoxes.end(); ++iter)
    {
        QLineEdit *line = *iter;
        auto variable = line->text();
        int pos = 0;
        if(line->validator()->validate(variable, pos) != QValidator::State::Acceptable)
        {
            allValid = false;
            break;
        }
    }
    okButton->setEnabled(allValid);
}

void LevelConfigDialog::on_SHModeTimer_TextBox_textChanged(const QString &arg1)
{
    QVector<QLineEdit*> textBoxes;
    textBoxes.append(ui->LevelName_TextBox);
    textBoxes.append(ui->NModeTimer_TextBox);
    textBoxes.append(ui->HModeTimer_TextBox);
    textBoxes.append(ui->SHModeTimer_TextBox);
    SetOKButtonEnable(textBoxes, ui->buttonBox->button(QDialogButtonBox::Ok));
}

void LevelConfigDialog::on_NModeTimer_TextBox_textChanged(const QString &arg1)
{
    QVector<QLineEdit*> textBoxes;
    textBoxes.append(ui->LevelName_TextBox);
    textBoxes.append(ui->NModeTimer_TextBox);
    textBoxes.append(ui->HModeTimer_TextBox);
    textBoxes.append(ui->SHModeTimer_TextBox);
    SetOKButtonEnable(textBoxes, ui->buttonBox->button(QDialogButtonBox::Ok));
}

void LevelConfigDialog::on_HModeTimer_TextBox_textChanged(const QString &arg1)
{
    QVector<QLineEdit*> textBoxes;
    textBoxes.append(ui->LevelName_TextBox);
    textBoxes.append(ui->NModeTimer_TextBox);
    textBoxes.append(ui->HModeTimer_TextBox);
    textBoxes.append(ui->SHModeTimer_TextBox);
    SetOKButtonEnable(textBoxes, ui->buttonBox->button(QDialogButtonBox::Ok));
}

void LevelConfigDialog::on_LevelName_TextBox_textChanged(const QString &arg1)
{
    QVector<QLineEdit*> textBoxes;
    textBoxes.append(ui->LevelName_TextBox);
    textBoxes.append(ui->NModeTimer_TextBox);
    textBoxes.append(ui->HModeTimer_TextBox);
    textBoxes.append(ui->SHModeTimer_TextBox);
    SetOKButtonEnable(textBoxes, ui->buttonBox->button(QDialogButtonBox::Ok));
}
