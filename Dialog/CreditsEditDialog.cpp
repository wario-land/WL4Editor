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

#include "WL4EditorWindow.h"

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
                if (j>=30) {
                    newItem->setEditable(false);
                }
                newItem->setText(QString(this->map[data_to_save[k][j+i*32]]));
                model[k]->setItem(i, j, newItem);
            }
        }
    }

    for (int i=0 ;i<13;i++) {
        tabs[i]=new QWidget;
        tables_view[i]=new CustomQTableView(tabs[i]);

        tables_view[i]->setModel(model[i]);

        for (int j=0;j<32;j++) {
            tables_view[i]->setColumnWidth(j,1);
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
/// Deconstructor of CreditEditDialog class.
/// </summary>
CreditsEditDialog::~CreditsEditDialog()
{
    delete ui;
}


