#ifndef LEVELCONFIGDIALOG_H
#define LEVELCONFIGDIALOG_H

#include <QDialog>
#include <QString>
#include <QValidator> // include <QRegExp> and <QRegExpValidator>
#include <string>

namespace Ui
{
    class LevelConfigDialog;
}

class LevelConfigDialog : public QDialog
{
    Q_OBJECT

private:
    Ui::LevelConfigDialog *ui;

public:
    explicit LevelConfigDialog(QWidget *parent = 0);
    ~LevelConfigDialog();
    void InitTextBoxes(std::string _levelname, int HModeTimer, int NModeTimer, int SHModeTimer);
    std::string GetPaddedLevelName();
    int GetHModeTimer();
    int GetNModeTimer();
    int GetSHModeTimer();

private slots:
    void on_NModeTimer_TextBox_textChanged(const QString &arg1);
    void on_SHModeTimer_TextBox_textChanged(const QString &arg1);
    void on_HModeTimer_TextBox_textChanged(const QString &arg1);
    void on_LevelName_TextBox_textChanged(const QString &arg1);
};

#endif // LEVELCONFIGDIALOG_H
