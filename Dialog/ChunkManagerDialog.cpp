#include "ChunkManagerDialog.h"
#include "ui_ChunkManagerDialog.h"
#include <QDialogButtonBox>

ChunkManagerDialog::ChunkManagerDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ChunkManagerDialog)
{
    ui->setupUi(this);
}

ChunkManagerDialog::~ChunkManagerDialog() { delete ui; }

void ChunkManagerDialog::on_pushButton_Refresh_clicked()
{
    // TODO
}


void ChunkManagerDialog::on_pushButton_InvalidateCurrentOrphanedChunk_clicked()
{
    // TODO
}


void ChunkManagerDialog::on_pushButton_FixBrokenHeader_clicked()
{
    // TODO
}


void ChunkManagerDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    // Retrieve the standard button type
    QDialogButtonBox::StandardButton btnType = ui->buttonBox->standardButton(button);

    switch (btnType) {
    case QDialogButtonBox::Save:
        // TODO: Execute your save logic here
        accept(); // Closes the dialog with a QDialog::Accepted status
        break;

    case QDialogButtonBox::Ignore:
        // TODO: Execute your ignore logic here
        accept(); // Or use done(QDialog::Accepted);
        break;

    case QDialogButtonBox::Abort:
        // TODO: Execute rollback or abort logic
        reject(); // Closes the dialog with a QDialog::Rejected status
        break;

    default:
        break;
    }
}


void ChunkManagerDialog::on_treeView_clicked(const QModelIndex &index)
{
    // TODO
}

