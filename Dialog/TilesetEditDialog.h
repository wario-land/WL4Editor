#ifndef TILESETEDITDIALOG_H
#define TILESETEDITDIALOG_H

#include <QDialog>

namespace Ui {
class TilesetEditDialog;
}

class TilesetEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TilesetEditDialog(QWidget *parent = nullptr);
    ~TilesetEditDialog();

private:
    Ui::TilesetEditDialog *ui;
};

#endif // TILESETEDITDIALOG_H
