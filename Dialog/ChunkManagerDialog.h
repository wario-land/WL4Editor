#ifndef CHUNKEDITORDIALOG_H
#define CHUNKEDITORDIALOG_H

#include "ChunkManagerTreeView.h"

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

private:
    ChunkManagerTreeView *TreeView;
    Ui::ChunkManagerDialog *ui;
};

#endif // CHUNKEDITORDIALOG_H
