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
#include <QHeaderView>

#include "WL4EditorWindow.h"

CreditsEditDialog::CreditsEditDialog(QWidget *parent, DialogParams::CreditsEditParams *creditsEditParam) :
    QDialog(parent),
    ui(new Ui::CreditsEditDialog)
{
    ui->setupUi(this);

    QStandardItemModel* model[13];

    for (int k=0;k<13;k++) {
        model[k]=new QStandardItemModel();

        for (int i=0;i<20;i++) {
            for (int j=0;j<32;j=j+2) {
                QStandardItem *newItem = new QStandardItem();
                QStandardItem *newItem2 = new QStandardItem();
                unsigned int twoShort=ROMUtils::IntFromData(0x789FCC+j*2+i*64+k*0x500); //0x789FCC Maybe I should put this in constants

                short mapchar1=(short)(0x0000FFFF&twoShort);
                short mapchar2=(short)((0xFFFF0000&twoShort)>>16);
                //std::cout << std::hex << mapchar1 << " " << mapchar2 << std::endl;

                newItem->setText(QString(this->map[mapchar1]));
                newItem2->setText(QString(this->map[mapchar2]));
                model[k]->setItem(i, j, newItem);
                model[k]->setItem(i, j+1, newItem2);
            }
        }
    }

    for (int i=0 ;i<13;i++) {
        tabs[i]=new QWidget;
        tables_view[i]=new QTableView(tabs[i]);


        tables_view[i]->setModel(model[i]);

        for (int j=0;j<32;j++) {
            tables_view[i]->setColumnWidth(j,1);
            /*if (j >= 30) {
                //tables_view[i]->setP
                tables_view[i]->setStyleSheet("QTableView::item#nameEdit { background: red; }");
                //model[i]->takeItem(0,0)->setBackground(QBrush(Qt::gray));
            }*/
        }

        for (int j=0;j<20;j++) {
            tables_view[i]->setRowHeight(j,1);
        }

        tables_view[i]->setObjectName(QString::number(i+1));

        QGridLayout* layout = new QGridLayout;
        layout->addWidget(tables_view[i], 0,0);
        tabs[i]->setLayout(layout);
        ui->tabWidget->addTab(tabs[i],QString::number(i+1));
    }

}

/// <summary>
/// Deconstructor of TilesetEditDialog class.
/// </summary>
CreditsEditDialog::~CreditsEditDialog()
{
    delete ui;
}
