#ifndef CHUNKEDITORDIALOG_H
#define CHUNKEDITORDIALOG_H

#include "ChunkEditorTreeView.h"

#include <QDialog>

namespace Ui {
class ChunkEditorDialog;
}

class ChunkEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChunkEditorDialog(QWidget *parent = nullptr);
    ~ChunkEditorDialog();

private:
    ChunkEditorTreeView *TreeView;
    Ui::ChunkEditorDialog *ui;
};

#endif // CHUNKEDITORDIALOG_H
