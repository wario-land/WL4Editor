#ifndef CHUNKEDITORDIALOG_H
#define CHUNKEDITORDIALOG_H

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
    Ui::ChunkEditorDialog *ui;
};

#endif // CHUNKEDITORDIALOG_H
