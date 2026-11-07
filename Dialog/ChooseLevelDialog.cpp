#include "ChooseLevelDialog.h"
#include "ui_ChooseLevelDialog.h"

#include <QVariant>

ChooseLevelDialog::ChooseLevelDialog(struct DialogParams::PassageAndLevelIndex currentSelection, QWidget *parent) :
        QDialog(parent), ui(new Ui::ChooseLevelDialog)
{
    ui->setupUi(this);

    QStringList PassageNameSet; // TODO: read the names directly from the ROM, then we can change the names.
    PassageNameSet << "Entry Passage"
                   << "Emerald Passage"
                   << "Ruby Passage"
                   << "Topaz Passage"
                   << "Sapphire Passage"
                   << "Golden Passage";
    ui->comboBox_Passage->addItems(PassageNameSet);
    ui->comboBox_Passage->setCurrentIndex(currentSelection._PassageIndex);
    // The combo boxes store the internal level index (the __stage value) as item data. Entry
    // Passage uses 0 (HoH), 2 (Debug Level) and 4 (Boss Level), so the item index is not the same
    // as the level index there.
    ConfigureLevelComboBox(currentSelection._LevelIndex);
}

ChooseLevelDialog::~ChooseLevelDialog() { delete ui; }

void ChooseLevelDialog::ConfigureLevelComboBox(int levelIndex)
{
    ui->comboBox_Level->clear();
    const int passageIndex = ui->comboBox_Passage->currentIndex();
    if (passageIndex == 5)
    {
        // Golden Passage only has its first level and the boss level.
        ui->comboBox_Level->addItem("First Level", 0);
        ui->comboBox_Level->addItem("Boss Level", 4);
    }
    else if (passageIndex == 0)
    {
        // Entry Passage skips the second, third and fourth stages.
        ui->comboBox_Level->addItem("First Level", 0);
        ui->comboBox_Level->addItem("Debug Level", 2);
        ui->comboBox_Level->addItem("Boss Level", 4);
    }
    else
    {
        ui->comboBox_Level->addItem("First Level", 0);
        ui->comboBox_Level->addItem("Second Level", 1);
        ui->comboBox_Level->addItem("Third Level", 2);
        ui->comboBox_Level->addItem("Fourth Level", 3);
        ui->comboBox_Level->addItem("Boss Level", 4);
    }

    // Select the item which carries the requested internal level index. This keeps the current
    // level selected when the dialog is reopened, including the Entry Passage boss level.
    const int comboIndex = ui->comboBox_Level->findData(levelIndex);
    ui->comboBox_Level->setCurrentIndex(comboIndex >= 0 ? comboIndex : 0);
}

void ChooseLevelDialog::on_comboBox_Passage_currentTextChanged(const QString &arg1)
{
    (void) arg1;
    ConfigureLevelComboBox(0);
}

DialogParams::PassageAndLevelIndex ChooseLevelDialog::GetResult()
{
    DialogParams::PassageAndLevelIndex tmpRetStruct;
    tmpRetStruct._PassageIndex = ui->comboBox_Passage->currentIndex();
    const QVariant levelData = ui->comboBox_Level->currentData();
    tmpRetStruct._LevelIndex = levelData.isValid() ? levelData.toInt() : 0;
    return tmpRetStruct;
}
