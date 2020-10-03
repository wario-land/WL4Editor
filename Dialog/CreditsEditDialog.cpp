#include "Dialog/CreditsEditDialog.h"
#include "ROMUtils.h"
#include "ui_CreditsEditDialog.h"

#include <iostream>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QFileDevice>
#include <QMessageBox>
#include <QTableView>
#include "Dialog/CustomQTableView.h"
#include <QHeaderView>
#include <QStyledItemDelegate>

constexpr const short CreditsEditDialog::CreditTileMapData[0x108];
std::map< std::pair<std::string,std::string>, short> CreditsEditDialog::CreditTileReverseMap;
std::map<short,char> CreditsEditDialog::CreditTileMap;


CreditsEditDialog::CreditsEditDialog(QWidget *parent, DialogParams::CreditsEditParams *creditsEditParam) :
    QDialog(parent),
    ui(new Ui::CreditsEditDialog)
{
    ui->setupUi(this);

    QStandardItemModel* model[13];

    for (int k=0;k<13;k++) {
        model[k]=new QStandardItemModel();

        memcpy(data_to_save[k],&ROMUtils::CurrentFile[0x789FCC+k*1280],1280); //1280 = 0x500 size of one credit screen

        for (int i=0;i<20;i++) {
            for (int j=0;j<32;j=j+1) {
                QStandardItem *newItem = new QStandardItem();

                //The two last column never appeared in game
                if (j>=30) {
                    newItem->setEditable(false);
                    newItem->setData(QColor(Qt::gray), Qt::BackgroundRole);
                } else {
                    short mapId=data_to_save[k][j+i*32];
                    newItem->setText(QString(this->CreditTileMap[mapId]));

                    //One-tile high -> blue
                    if (mapId >= 0x4340 && mapId <= 0x435B) {
                        newItem->setData(QColor("#8080FF"), Qt::BackgroundRole);
                    //Two-tile high, upper half -> red
                    } else if ((mapId >= 0x0360 && mapId <=0x0379) || (mapId >= 0x03A0 && mapId <= 0x03B9)) {
                        newItem->setData(QColor("#FF8080"), Qt::BackgroundRole);
                    //Two-tile high, lower half -> green
                    } else if ((mapId >= 0x0380 && mapId <=0x0399) || (mapId >= 0x03C0 && mapId <= 0x03D9)) {
                        newItem->setData(QColor("#80FF80"), Qt::BackgroundRole);
                    }

                }
                model[k]->setItem(i, j, newItem);
            }
        }
    }

    for (int i=0 ;i<13;i++) {
        tabs[i]=new QWidget;
        tables_view[i]=new CustomQTableView(tabs[i]);

        tables_view[i]->setModel(model[i]);

        //Needed in ordrer to have the smallest cells
        for (int j=0;j<32;j++) {
            tables_view[i]->setColumnWidth(j,1);
        }

        for (int j=0;j<20;j++) {
            tables_view[i]->setRowHeight(j,1);
        }

        tables_view[i]->setObjectName(QString::number(i+1));

        //With this grid Layout the CustomQTableView takes all available size in the tab
        QGridLayout* layout = new QGridLayout;
        layout->addWidget(tables_view[i], 0,0);
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
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("A","#8080ff"),0x4340));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("B","#8080ff"),0x4341));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("C","#8080ff"),0x4342));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("D","#8080ff"),0x4343));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("E","#8080ff"),0x4344));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("F","#8080ff"),0x4345));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("G","#8080ff"),0x4346));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("H","#8080ff"),0x4347));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("I","#8080ff"),0x4348));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("J","#8080ff"),0x4349));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("K","#8080ff"),0x434A));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("L","#8080ff"),0x434B));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("M","#8080ff"),0x434C));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("N","#8080ff"),0x434D));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("O","#8080ff"),0x434E));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("P","#8080ff"),0x434F));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("Q","#8080ff"),0x4350));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("R","#8080ff"),0x4351));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("S","#8080ff"),0x4352));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("T","#8080ff"),0x4353));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("U","#8080ff"),0x4354));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("V","#8080ff"),0x4355));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("W","#8080ff"),0x4356));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("X","#8080ff"),0x4357));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("Y","#8080ff"),0x4358));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("Z","#8080ff"),0x4359));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair(".","#8080ff"),0x435A));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair(",","#8080ff"),0x435B));

    //Two-tile high, upper half
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("A","#ff8080"),0x0360));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("B","#ff8080"),0x0361));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("C","#ff8080"),0x0362));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("D","#ff8080"),0x0363));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("E","#ff8080"),0x0364));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("F","#ff8080"),0x0365));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("G","#ff8080"),0x0366));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("H","#ff8080"),0x0367));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("I","#ff8080"),0x0368));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("J","#ff8080"),0x0369));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("K","#ff8080"),0x036A));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("L","#ff8080"),0x036B));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("M","#ff8080"),0x036C));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("N","#ff8080"),0x036D));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("O","#ff8080"),0x036E));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("P","#ff8080"),0x036F));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("Q","#ff8080"),0x0370));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("R","#ff8080"),0x0371));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("S","#ff8080"),0x0372));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("T","#ff8080"),0x0373));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("U","#ff8080"),0x0374));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("V","#ff8080"),0x0375));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("W","#ff8080"),0x0376));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("X","#ff8080"),0x0377));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("Y","#ff8080"),0x0378));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("Z","#ff8080"),0x0379));

    CreditTileReverseMap.insert(std::make_pair(std::make_pair("a","#ff8080"),0x03A0));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("b","#ff8080"),0x03A1));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("c","#ff8080"),0x03A2));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("d","#ff8080"),0x03A3));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("e","#ff8080"),0x03A4));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("f","#ff8080"),0x03A5));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("g","#ff8080"),0x03A6));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("h","#ff8080"),0x03A7));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("i","#ff8080"),0x03A8));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("j","#ff8080"),0x03A9));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("k","#ff8080"),0x03AA));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("l","#ff8080"),0x03AB));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("m","#ff8080"),0x03AC));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("n","#ff8080"),0x03AD));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("o","#ff8080"),0x03AE));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("p","#ff8080"),0x03AF));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("q","#ff8080"),0x03B0));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("r","#ff8080"),0x03B1));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("s","#ff8080"),0x03B2));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("t","#ff8080"),0x03B3));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("u","#ff8080"),0x03B4));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("v","#ff8080"),0x03B5));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("w","#ff8080"),0x03B6));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("x","#ff8080"),0x03B7));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("y","#ff8080"),0x03B8));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("z","#ff8080"),0x03B9));

    //Two-tile high, lower half
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("A","#80ff80"),0x0380));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("B","#80ff80"),0x0381));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("C","#80ff80"),0x0382));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("D","#80ff80"),0x0383));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("E","#80ff80"),0x0384));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("F","#80ff80"),0x0385));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("G","#80ff80"),0x0386));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("H","#80ff80"),0x0387));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("I","#80ff80"),0x0388));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("J","#80ff80"),0x0389));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("K","#80ff80"),0x038A));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("L","#80ff80"),0x038B));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("M","#80ff80"),0x038C));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("N","#80ff80"),0x038D));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("O","#80ff80"),0x038E));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("P","#80ff80"),0x038F));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("Q","#80ff80"),0x0390));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("R","#80ff80"),0x0391));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("S","#80ff80"),0x0392));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("T","#80ff80"),0x0393));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("U","#80ff80"),0x0394));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("V","#80ff80"),0x0395));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("W","#80ff80"),0x0396));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("X","#80ff80"),0x0397));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("Y","#80ff80"),0x0398));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("Z","#80ff80"),0x0399));

    CreditTileReverseMap.insert(std::make_pair(std::make_pair("a","#80ff80"),0x03C0));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("b","#80ff80"),0x03C1));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("c","#80ff80"),0x03C2));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("d","#80ff80"),0x03C3));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("e","#80ff80"),0x03C4));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("f","#80ff80"),0x03C5));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("g","#80ff80"),0x03C6));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("h","#80ff80"),0x03C7));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("i","#80ff80"),0x03C8));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("j","#80ff80"),0x03C9));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("k","#80ff80"),0x03CA));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("l","#80ff80"),0x03CB));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("m","#80ff80"),0x03CC));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("n","#80ff80"),0x03CD));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("o","#80ff80"),0x03CE));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("p","#80ff80"),0x03CF));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("q","#80ff80"),0x03D0));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("r","#80ff80"),0x03D1));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("s","#80ff80"),0x03D2));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("t","#80ff80"),0x03D3));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("u","#80ff80"),0x03D4));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("v","#80ff80"),0x03D5));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("w","#80ff80"),0x03D6));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("x","#80ff80"),0x03D7));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair("y","#80ff80"),0x03D8));
    CreditTileReverseMap.insert(std::make_pair(std::make_pair(".","#80ff80"),0x03D9));

}

/// <summary>
/// When the OK button is pressed all data in the model is stored into data_to_save and then in the temporary ROM
/// </summary>
void CreditsEditDialog::on_buttonBox_accepted() {
    for (int k=0;k<13;k++) {
        for (int i=0;i<20;i++) {
            for (int j=0;j<32;j=j+1) {
                QModelIndex model_index=tables_view[k]->model()->index(i,j);
                QString first_letter=model_index.data(Qt::DisplayRole).value<QString>();
                QString role=model_index.data(Qt::BackgroundRole).value<QColor>().name();

                std::string string = first_letter.toUtf8().constData();

                if (string.size() > 0) {
                    std::string role_string = role.toUtf8().constData();
                    std::string first_letter_string(1,string.at(0));

                    data_to_save[k][j+i*32]=CreditTileReverseMap[std::make_pair(first_letter_string,role_string)];
                } else {
                    data_to_save[k][j+i*32]=0x0000;
                }
            }
        }
    }
    for (int k=0;k<13;k++) {
        memcpy(&ROMUtils::CurrentFile[0x789FCC+k*1280],data_to_save[k],1280);
    }
}
