#ifndef CHUNKEDITORDIALOG_H
#define CHUNKEDITORDIALOG_H

#include "ChunkManagerTreeView.h"
#include "ChunkManagerInfoGroupBox.h"

#include <QDialog>

namespace Ui {
    class ChunkManagerDialog;
}

class ChunkManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChunkManagerDialog(QWidget *parent = nullptr);
    ~ChunkManagerDialog();

private slots:
    void on_pushButton_SelectAllOrphanChunks_clicked();
    void on_pushButton_DeselectAll_clicked();

private:
    ChunkManagerTreeView *TreeView;
    ChunkManagerInfoGroupBox *Info;
    Ui::ChunkManagerDialog *ui;
};

#endif // CHUNKEDITORDIALOG_H
