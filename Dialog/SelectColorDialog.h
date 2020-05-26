#ifndef SELECTCOLORDIALOG_H
#define SELECTCOLORDIALOG_H

#include <QDialog>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>

namespace Ui {
class SelectColorDialog;
}

class SelectColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectColorDialog(QWidget *parent = nullptr);
    ~SelectColorDialog();
    void SetPalette(QVector<QRgb> palette);
    void SetColor(int colorId);
    int GetSelectedColorId();

private:
    Ui::SelectColorDialog *ui;
    QGraphicsScene *pb = nullptr;
    QGraphicsPixmapItem *PalBar = nullptr;
    QGraphicsPixmapItem *SelectionBox_Color = nullptr;
};

#endif // SELECTCOLORDIALOG_H
