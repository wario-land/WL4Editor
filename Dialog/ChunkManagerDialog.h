#ifndef CHUNKMANAGERDIALOG_H
#define CHUNKMANAGERDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui
{
    class ChunkManagerDialog;
}

class ChunkManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChunkManagerDialog(QWidget *parent = nullptr);
    ~ChunkManagerDialog();

private slots:
    void on_pushButton_Refresh_clicked();
    void on_pushButton_InvalidateCurrentOrphanedChunk_clicked();
    void on_pushButton_FixBrokenHeader_clicked();
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_treeView_clicked(const QModelIndex &index);

private:
    Ui::ChunkManagerDialog *ui;
};

#endif // CHUNKMANAGERDIALOG_H
