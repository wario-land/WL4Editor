﻿#include "Dialog/CreditsEditDialog.h"
#include "ROMUtils.h"
#include "ui_CreditsEditDialog.h"
#include "WL4Constants.h"

constexpr const short CreditsEditDialog::CreditTileMapData[(0x1C + 0x1C + 0x34 + 0x34) * 2];
std::map<std::string, short> CreditsEditDialog::CreditTileReverseMap;
std::map<short,char> CreditsEditDialog::CreditTileMap;

/// <summary>
/// Construct an instance of the credit dialog.
/// It contains 14 tabs containing one table each.
/// Each table allow to modify the corresponding credit screen.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
/// <param name="creditsEditParam">
/// An instance of creditsEditParam
/// </param>
CreditsEditDialog::CreditsEditDialog(QWidget *parent, DialogParams::CreditsEditParams *creditsEditParam) :
    QDialog(parent),
    ui(new Ui::CreditsEditDialog)
{
    ui->setupUi(this);

    QStandardItemModel* model[NUMBEROFCREDITSSCREEN];

    for (int k=0;k<NUMBEROFCREDITSSCREEN;k++)
    {
        model[k]=new QStandardItemModel();

        memcpy(dataToSave[k],&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::CreditsTiles+k*1280],1280); //1280 = 0x500 size of one credit screen

        for (int i=0;i<20;i++) {
            for (int j=0;j<32;j++) {
                QStandardItem *newItem = new QStandardItem();

                //The two last column never appeared in game
                if (j>=30)
                {
                    newItem->setEditable(false);
                    newItem->setData(QColor(Qt::gray), Qt::BackgroundRole);
                } else {
                    short mapId=dataToSave[k][j+i*32];
                    newItem->setText(QString(this->CreditTileMap[mapId]));

                    if (mapId >= 0x4340 && mapId <= 0x435B)
                    {  //One-tile high -> blue
                        newItem->setData(QColor(BLUECOLOR), Qt::BackgroundRole);
                    } else if (mapId >= 0x5340 && mapId <= 0x535B)
                    {  //One-tile high -> orange
                        newItem->setData(QColor(ORANGECOLOR), Qt::BackgroundRole);
                    } else if ((mapId >= 0x0360 && mapId <=0x0379) || (mapId >= 0x03A0 && mapId <= 0x03B9))
                    {  //Two-tile high, upper half -> light green
                        newItem->setData(QColor(GREENCOLOR_U), Qt::BackgroundRole);
                    } else if ((mapId >= 0x0380 && mapId <=0x0399) || (mapId >= 0x03C0 && mapId <= 0x03D9))
                    {  //Two-tile high, lower half -> dark green
                        newItem->setData(QColor(GREENCOLOR_D), Qt::BackgroundRole);
                    }
                }
                model[k]->setItem(i, j, newItem);
            }
        }
    }

    for (int i=0 ;i<NUMBEROFCREDITSSCREEN;i++)
    {
        tabs[i]=new QWidget;
        tablesView[i]=new CreditEditor_TableView(tabs[i]);

        tablesView[i]->setModel(model[i]);

        //Needed in ordrer to have the smallest cells
        for (int j=0;j<32;j++)
        {
            tablesView[i]->setColumnWidth(j,1);
        }

        for (int j=0;j<20;j++)
        {
            tablesView[i]->setRowHeight(j,1);
        }

        tablesView[i]->setObjectName(QString::number(i+1));

        //With this grid Layout the CustomQTableView takes all available size in the tab
        QGridLayout* layout = new QGridLayout;
        layout->addWidget(tablesView[i], 0,0);
        tabs[i]->setLayout(layout);
        ui->tabWidget->addTab(tabs[i],QString::number(i+1));
    }
}

/// <summary>
/// Deconstructor of CreditsEditDialog class.
/// </summary>
CreditsEditDialog::~CreditsEditDialog()
{
    delete ui;
}

/// <summary>
/// Perform static initialization of constant data structures for the dialog.
/// </summary>
void CreditsEditDialog::StaticInitialization()
{
    // Initialize the selections for the Door type
    for (unsigned int i = 0; i < sizeof(CreditTileMapData) / sizeof(CreditTileMapData[0]); i+=2)
    {
        CreditTileMap[CreditTileMapData[i]] = static_cast<char>(CreditTileMapData[i + 1]);
    }

    //One-tile high type 1
    std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.,";
    short creditHexCode=0x4340;
    for(unsigned int i=0;i<alphabet.length();i++)
    {
          std::string oneLetter(1,alphabet.at(i));
          CreditTileReverseMap[std::string(oneLetter + BLUECOLOR)] = creditHexCode;
          creditHexCode++;
    }

    //One-tile high type 2
    creditHexCode=0x5340;
    for(unsigned int i=0;i<alphabet.length();i++)
    {
          std::string oneLetter(1,alphabet.at(i));
          CreditTileReverseMap[std::string(oneLetter + ORANGECOLOR)] = creditHexCode;
          creditHexCode++;
    }

    //Two-tile high, upper half
    std::string alphabet2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    short creditHexCode2=0x0360;
    for(unsigned int i=0;i<alphabet2.length();i++)
    {
          std::string oneLetter(1,alphabet2.at(i));
          CreditTileReverseMap[std::string(oneLetter + GREENCOLOR_U)] = creditHexCode2;
          creditHexCode2++;
    }

    //Two-tile high, upper half 2
    std::string alphabet3 = "abcdefghijklmnopqrstuvwxy";
    short creditHexCode3=0x03A0;
    for(unsigned int i=0;i<alphabet3.length();i++)
    {
          std::string oneLetter(1,alphabet3.at(i));
          CreditTileReverseMap[std::string(oneLetter + GREENCOLOR_U)] = creditHexCode3;
          creditHexCode3++;
    }

    //Two-tile high, lower half
    short creditHexCode4=0x0380;
    for(unsigned int i=0;i<alphabet2.length();i++)
    {
          std::string oneLetter(1,alphabet2.at(i));
          CreditTileReverseMap[std::string(oneLetter + GREENCOLOR_D)] = creditHexCode4;
          creditHexCode4++;
    }

    //Two-tile high, lower half 2
    std::string alphabet4 = "abcdefghijklmnopqrstuvwxy.";
    short creditHexCode5=0x03C0;
    for(unsigned int i=0;i<alphabet4.length();i++)
    {
          std::string oneLetter(1,alphabet4.at(i));
          CreditTileReverseMap[std::string(oneLetter + GREENCOLOR_D)] = creditHexCode5;
          creditHexCode5++;
    }

}
/// <summary>
/// When the OK button is pressed all data in the model is stored into data_to_save and then in the temporary ROM
/// </summary>
void CreditsEditDialog::on_buttonBox_accepted()
    {
    for (int k=0;k<NUMBEROFCREDITSSCREEN;k++)
    {
        for (int i=0;i<20;i++)
        {
            for (int j=0;j<32;j++)
            {
                QModelIndex modelIndex=tablesView[k]->model()->index(i,j);
                QString first_letter=modelIndex.data(Qt::DisplayRole).value<QString>();
                QString role=modelIndex.data(Qt::BackgroundRole).value<QColor>().name();

                std::string string = first_letter.toUtf8().constData();

                if (string.size() > 0)
                {
                    std::string roleString = role.toUtf8().constData();
                    std::string firstLetterString(1,string.at(0));

                    dataToSave[k][j+i*32] = CreditTileReverseMap[std::string(firstLetterString + roleString)];
                } else
                {
                    dataToSave[k][j+i*32] = 0x0000;
                }
            }
        }
    }
    for (int k=0;k<NUMBEROFCREDITSSCREEN;k++)
    {
        memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::CreditsTiles+k*1280],dataToSave[k],1280);
    }
}
