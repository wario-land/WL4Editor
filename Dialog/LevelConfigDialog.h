#ifndef LEVELCONFIGDIALOG_H
#define LEVELCONFIGDIALOG_H

#include <QDialog>
#include <string>
#include <QString>
#include <QValidator> // include <QRegExp> and <QRegExpValidator>

namespace Ui {
class LevelConfigDialog;
}

class LevelConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LevelConfigDialog(QWidget *parent = 0);
    ~LevelConfigDialog();
    void InitTextBoxes(std::string _levelname, int HModeTimer, int NModeTimer, int SHModeTimer);

private:
    Ui::LevelConfigDialog *ui;
    QRegExp *LevelnameRegx;
    QRegExp *TimerRegx;

};

#endif // LEVELCONFIGDIALOG_H
