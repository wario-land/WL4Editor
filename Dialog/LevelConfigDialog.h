#ifndef LEVELCONFIGDIALOG_H
#define LEVELCONFIGDIALOG_H

#include <QDialog>
#include <string>
#include <QString>

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
};

#endif // LEVELCONFIGDIALOG_H
