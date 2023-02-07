﻿#include "LevelConfigDialog.h"
#include "ui_LevelConfigDialog.h"

#include <QRegularExpressionValidator>

#include "LevelComponents/Level.h"
QRegularExpression TimerRegx("^[0-9]:[0-5][0-9]$");

/// <summary>
/// Enable or disable the OK button depending on the validity of the input fields.
/// All fields must be in the QValidator::State::Acceptable, or else the OK button will be disabled.
/// </summary>
/// <param name="textBoxes">
/// The text boxes to validate.
/// </param>
/// <param name="okButton">
/// The OK button which will be enabled or disabled based on the text boxes' validator states.
/// </param>
void LevelConfigDialog::SetOKButtonEnable(QVector<QLineEdit *> textBoxes, QPushButton *okButton)
{
    bool allValid = true;
    for (auto iter = textBoxes.begin(); iter != textBoxes.end(); ++iter)
    {
        QLineEdit *line = *iter;
        auto variable = line->text();
        int pos = 0;
        if (line->validator()->validate(variable, pos) != QValidator::State::Acceptable)
        {
            allValid = false;
            break;
        }
    }
    if(!allValid)
    {
        okButton->setEnabled(false);
        return;
    }
    QString charTable = LevelComponents::Level::GetAvailableLevelNameChars();
    for (int k = 0; k < 2; k++)
    {
        QString LevelName = k ? ui->LevelNameJ_TextBox->text() : ui->LevelName_TextBox->text();
        for (int i = 0; i < LevelName.size(); i++)
        {
            allValid = false;
            if(charTable.contains(LevelName[i]) || LevelName.at(i) == ' ')
            {
                allValid = true;
            }
            if(!allValid)
            {
                okButton->setEnabled(false);
                return;
            }
        }
    }
    okButton->setEnabled(true);
}

/// <summary>
/// Construct the instance of the LevelConfigDialog.
/// </summary>
/// <remarks>
/// Also set the attributes on the text boxes that can't normally be set from within the UI editor.
/// </remarks>
/// <param name="parent">
/// The parent QWidget.
/// </param>
LevelConfigDialog::LevelConfigDialog(QWidget *parent) : QDialog(parent), ui(new Ui::LevelConfigDialog)
{
    ui->setupUi(this);

    ui->LevelName_TextBox->setAttribute(Qt::WA_InputMethodEnabled, true);
    ui->LevelNameJ_TextBox->setAttribute(Qt::WA_InputMethodEnabled, true);
    ui->HModeTimer_TextBox->setAttribute(Qt::WA_InputMethodEnabled, false);
    ui->HModeTimer_TextBox->setValidator(new QRegularExpressionValidator(TimerRegx, ui->HModeTimer_TextBox));
    ui->NModeTimer_TextBox->setAttribute(Qt::WA_InputMethodEnabled, false);
    ui->NModeTimer_TextBox->setValidator(new QRegularExpressionValidator(TimerRegx, ui->NModeTimer_TextBox));
    ui->SHModeTimer_TextBox->setAttribute(Qt::WA_InputMethodEnabled, false);
    ui->SHModeTimer_TextBox->setValidator(new QRegularExpressionValidator(TimerRegx, ui->SHModeTimer_TextBox));
}

/// <summary>
/// Deconstruct the Level Config Dialog.
/// </summary>
LevelConfigDialog::~LevelConfigDialog() { delete ui; }

/// <summary>
/// Initialize the dialog's text boxes with a level name, and level frog timer values.
/// </summary>
/// <remarks>
/// The prepended and appended space character padding will be trimmed before updating the level name text box.
/// </remarks>
/// <param name="_levelname">
/// The name of the level.
/// </param>
/// <param name="HModeTimer">
/// Number of seconds after pressing the frog switch in hard mode.
/// </param>
/// <param name="NModeTimer">
/// Number of seconds after pressing the frog switch in normal mode.
/// </param>
/// <param name="SHModeTimer">
/// Number of seconds after pressing the frog switch in super hard mode.
/// </param>
void LevelConfigDialog::InitTextBoxes(QString _levelname, QString _levelnameJ, int HModeTimer, int NModeTimer, int SHModeTimer)
{
    ui->LevelName_TextBox->setText(_levelname.trimmed());
    ui->LevelNameJ_TextBox->setText(_levelnameJ.trimmed());

    // Parse and Show Timers
    int a, b, c;
    a = HModeTimer / 60;
    b = (HModeTimer - 60 * a) / 10;
    c = HModeTimer - 60 * a - 10 * b;
    ui->HModeTimer_TextBox->setText(QString::number(a, 10) + ":" + QString::number(b, 10) + QString::number(c, 10));
    a = NModeTimer / 60;
    b = (NModeTimer - 60 * a) / 10;
    c = NModeTimer - 60 * a - 10 * b;
    ui->NModeTimer_TextBox->setText(QString::number(a, 10) + ":" + QString::number(b, 10) + QString::number(c, 10));
    a = SHModeTimer / 60;
    b = (SHModeTimer - 60 * a) / 10;
    c = SHModeTimer - 60 * a - 10 * b;
    ui->SHModeTimer_TextBox->setText(QString::number(a, 10) + ":" + QString::number(b, 10) + QString::number(c, 10));
}

/// <summary>
/// Pad a level name with spaces so that it is 26 characters long.
/// </summary>
/// <remarks>
/// Half of the spaces will be prepended to the level name, and the other half will be appended.
/// </remarks>
/// <return>
/// The level name, padded to 26 characters with spaces.
/// </return>
QString LevelConfigDialog::GetPaddedLevelName(int levelnameid)
{
    QString tmplevelname;
    if(levelnameid)
        tmplevelname = ui->LevelNameJ_TextBox->text();
    else
        tmplevelname = ui->LevelName_TextBox->text();
    int a, b;
    a = (26 - tmplevelname.size()) / 2;
    b = (26 - tmplevelname.size()) / 2 + (26 - tmplevelname.size()) % 2;
    QString stra, strb;
    stra.fill(' ', a);
    strb.fill(' ', b);
    stra = stra + tmplevelname + strb;
    return stra;
}

/// <summary>
/// Parse the hard mode timer text box into seconds.
/// </summary>
/// <return>
/// The number of secconds for the hard mode frog timer.
/// </return>
int LevelConfigDialog::GetHModeTimer()
{
    int a, b, c;
    a = (int) (ui->HModeTimer_TextBox->text().at(0).unicode()) - 48;
    b = (int) (ui->HModeTimer_TextBox->text().at(2).unicode()) - 48;
    c = (int) (ui->HModeTimer_TextBox->text().at(3).unicode()) - 48;
    return (60 * a + 10 * b + c);
}

/// <summary>
/// Parse the normal mode timer text box into seconds.
/// </summary>
/// <return>
/// The number of secconds for the normal mode frog timer.
/// </return>
int LevelConfigDialog::GetNModeTimer()
{
    int a, b, c;
    a = (int) (ui->NModeTimer_TextBox->text().at(0).unicode()) - 48;
    b = (int) (ui->NModeTimer_TextBox->text().at(2).unicode()) - 48;
    c = (int) (ui->NModeTimer_TextBox->text().at(3).unicode()) - 48;
    return (60 * a + 10 * b + c);
}

/// <summary>
/// Parse the super hard mode timer text box into seconds.
/// </summary>
/// <return>
/// The number of secconds for the super hard mode frog timer.
/// </return>
int LevelConfigDialog::GetSHModeTimer()
{
    int a, b, c;
    a = (int) (ui->SHModeTimer_TextBox->text().at(0).unicode()) - 48;
    b = (int) (ui->SHModeTimer_TextBox->text().at(2).unicode()) - 48;
    c = (int) (ui->SHModeTimer_TextBox->text().at(3).unicode()) - 48;
    return (60 * a + 10 * b + c);
}

/// <summary>
/// Enable or disable the OK button depending on the validity of the input fields, when the super hard timer text box is
/// changed.
/// </summary>
/// <param name="arg1">
/// Unused.
/// </param>
void LevelConfigDialog::on_SHModeTimer_TextBox_textChanged(const QString &arg1)
{
    (void) arg1;
    QVector<QLineEdit *> textBoxes;
    textBoxes.append(ui->NModeTimer_TextBox);
    textBoxes.append(ui->HModeTimer_TextBox);
    textBoxes.append(ui->SHModeTimer_TextBox);
    SetOKButtonEnable(textBoxes, ui->buttonBox->button(QDialogButtonBox::Ok));
}

/// <summary>
/// Enable or disable the OK button depending on the validity of the input fields, when the normal timer text box is
/// changed.
/// </summary>
/// <param name="arg1">
/// Unused.
/// </param>
void LevelConfigDialog::on_NModeTimer_TextBox_textChanged(const QString &arg1)
{
    (void) arg1;
    QVector<QLineEdit *> textBoxes;
    textBoxes.append(ui->NModeTimer_TextBox);
    textBoxes.append(ui->HModeTimer_TextBox);
    textBoxes.append(ui->SHModeTimer_TextBox);
    SetOKButtonEnable(textBoxes, ui->buttonBox->button(QDialogButtonBox::Ok));
}

/// <summary>
/// Enable or disable the OK button depending on the validity of the input fields, when the hard timer text box is
/// changed.
/// </summary>
/// <param name="arg1">
/// Unused.
/// </param>
void LevelConfigDialog::on_HModeTimer_TextBox_textChanged(const QString &arg1)
{
    (void) arg1;
    QVector<QLineEdit *> textBoxes;
    textBoxes.append(ui->NModeTimer_TextBox);
    textBoxes.append(ui->HModeTimer_TextBox);
    textBoxes.append(ui->SHModeTimer_TextBox);
    SetOKButtonEnable(textBoxes, ui->buttonBox->button(QDialogButtonBox::Ok));
}

/// <summary>
/// Enable or disable the OK button depending on the validity of the input fields, when the level name text box is
/// changed.
/// </summary>
/// <param name="arg1">
/// Unused.
/// </param>
void LevelConfigDialog::on_LevelName_TextBox_textChanged(const QString &arg1)
{
    (void) arg1;
    QVector<QLineEdit *> textBoxes;
    textBoxes.append(ui->NModeTimer_TextBox);
    textBoxes.append(ui->HModeTimer_TextBox);
    textBoxes.append(ui->SHModeTimer_TextBox);
    SetOKButtonEnable(textBoxes, ui->buttonBox->button(QDialogButtonBox::Ok));
}

/// <summary>
/// Enable or disable the OK button depending on the validity of the input fields, when the Japanese level name text box
/// is changed.
/// </summary>
/// <param name="arg1">
/// Unused.
/// </param>
void LevelConfigDialog::on_LevelNameJ_TextBox_textChanged(const QString &arg1)
{
    (void) arg1;
    QVector<QLineEdit *> textBoxes;
    textBoxes.append(ui->NModeTimer_TextBox);
    textBoxes.append(ui->HModeTimer_TextBox);
    textBoxes.append(ui->SHModeTimer_TextBox);
    SetOKButtonEnable(textBoxes, ui->buttonBox->button(QDialogButtonBox::Ok));
}
