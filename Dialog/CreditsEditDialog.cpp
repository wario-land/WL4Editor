#include "Dialog/CreditsEditDialog.h"
#include "ROMUtils.h"
#include "ui_CreditsEditDialog.h"
#include "WL4Constants.h"

#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QFileDevice>
#include <QMessageBox>
#include <QTableView>
#include "Dialog/CreditEditor_TableView.h"
#include <QHeaderView>
#include <QStyledItemDelegate>

constexpr const short CreditsEditDialog::CreditTileMapData[0x108];
std::map< std::pair<std::string,std::string>, short> CreditsEditDialog::CreditTileReverseMap;
std::map<short,char> CreditsEditDialog::CreditTileMap;

/// <summary>
/// Construct an instance of the credit dialog.
/// It contains 13 tabs containing one table each.
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

        memcpy(dataToSave[k],&ROMUtils::CurrentFile[WL4Constants::CreditsTiles+k*1280],1280); //1280 = 0x500 size of one credit screen

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

                    //One-tile high -> blue
                    if (mapId >= 0x4340 && mapId <= 0x435B)
                    {
                        newItem->setData(QColor(BLUECOLOR), Qt::BackgroundRole);
                    //Two-tile high, upper half -> red
                    } else if ((mapId >= 0x0360 && mapId <=0x0379) || (mapId >= 0x03A0 && mapId <= 0x03B9))
                    {
                        newItem->setData(QColor(REDCOLOR), Qt::BackgroundRole);
                    //Two-tile high, lower half -> green
                    } else if ((mapId >= 0x0380 && mapId <=0x0399) || (mapId >= 0x03C0 && mapId <= 0x03D9))
                    {
                        newItem->setData(QColor(GREENCOLOR), Qt::BackgroundRole);
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

    //One-tile high
    std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.,";
    short creditHexCode=0x4340;
    for(unsigned int i=0;i<alphabet.length();i++)
    {
          std::string oneLetter(1,alphabet.at(i));
          CreditTileReverseMap[std::make_pair(oneLetter,BLUECOLOR)]=creditHexCode;
          creditHexCode++;
    }

    //Two-tile high, upper half
    std::string alphabet2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    short creditHexCode2=0x0360;
    for(unsigned int i=0;i<alphabet2.length();i++)
    {
          std::string oneLetter(1,alphabet2.at(i));
          CreditTileReverseMap[std::make_pair(oneLetter,REDCOLOR)]=creditHexCode2;
          creditHexCode2++;
    }

    //Two-tile high, upper half 2
    std::string alphabet3 = "abcdefghijklmnopqrstuvwxyz";
    short creditHexCode3=0x03A0;
    for(unsigned int i=0;i<alphabet3.length();i++)
    {
          std::string oneLetter(1,alphabet3.at(i));
          CreditTileReverseMap[std::make_pair(oneLetter,REDCOLOR)]=creditHexCode3;
          creditHexCode3++;
    }

    //Two-tile high, lower half
    short creditHexCode4=0x0380;
    for(unsigned int i=0;i<alphabet2.length();i++)
    {
          std::string oneLetter(1,alphabet2.at(i));
          CreditTileReverseMap[std::make_pair(oneLetter,GREENCOLOR)]=creditHexCode4;
          creditHexCode4++;
    }

    //Two-tile high, lower half 2
    std::string alphabet4 = "abcdefghijklmnopqrstuvwxyz.";
    short creditHexCode5=0x03C0;
    for(unsigned int i=0;i<alphabet4.length();i++)
    {
          std::string oneLetter(1,alphabet4.at(i));
          CreditTileReverseMap[std::make_pair(oneLetter,GREENCOLOR)]=creditHexCode5;
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
            for (int j=0;j<32;j=j++)
            {
                QModelIndex modelIndex=tablesView[k]->model()->index(i,j);
                QString first_letter=modelIndex.data(Qt::DisplayRole).value<QString>();
                QString role=modelIndex.data(Qt::BackgroundRole).value<QColor>().name();

                std::string string = first_letter.toUtf8().constData();

                if (string.size() > 0)
                {
                    std::string roleString = role.toUtf8().constData();
                    std::string firstLetterString(1,string.at(0));

                    dataToSave[k][j+i*32]=CreditTileReverseMap[std::make_pair(firstLetterString,roleString)];
                } else
                {
                    dataToSave[k][j+i*32]=0x0000;
                }
            }
        }
    }
    for (int k=0;k<NUMBEROFCREDITSSCREEN;k++)
    {
        memcpy(&ROMUtils::CurrentFile[WL4Constants::CreditsTiles+k*1280],dataToSave[k],1280);
    }
}
