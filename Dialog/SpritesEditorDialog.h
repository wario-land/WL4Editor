#ifndef SPRITESEDITORDIALOG_H
#define SPRITESEDITORDIALOG_H

#include <QDialog>

namespace Ui {
class SpritesEditorDialog;
}

class SpritesEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SpritesEditorDialog(QWidget *parent = nullptr);
    ~SpritesEditorDialog();

private:
    Ui::SpritesEditorDialog *ui;
};

#endif // SPRITESEDITORDIALOG_H
