#ifndef WALLPAINTEDITORDIALOG_H
#define WALLPAINTEDITORDIALOG_H

#include <QDialog>
#include <QPixmap>

namespace Ui {
class WallPaintEditorDialog;
}

class WallPaintEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WallPaintEditorDialog(QWidget *parent = nullptr);
    ~WallPaintEditorDialog();

    void AcceptChanges();

private slots:
    void on_spinBox_PassageID_valueChanged(int arg1);
    void on_spinBox_LocalLevelID_valueChanged(int arg1);
    void on_spinBox_CurrentPalette_valueChanged(int arg1);
    void on_pushButton_ImportColoredPalette_clicked();
    void on_pushButton_ImportGraphics_clicked();
    void on_pushButton_ExportColoredPalette_clicked();
    void on_pushButton_ExportWallPaintPNG_clicked();

private:
    Ui::WallPaintEditorDialog *ui;

    unsigned char gfxdata[1024 * 5 * 6];
    unsigned char pal_passage_color[32 * 5 * 6]; // 1 palette
    unsigned char pal_passage_gray[32 * 5 * 6]; // 1 palette
    unsigned char pal_startlevel_color[(32 * 8) * 4 * 6]; // 8 palette but only works for regular levels, not boss level

    // functions
    void RenderCurrentWallPaintAndPalette();

    // helper functions
    unsigned int GetGradPalStartAddr(int passage_Id, int local_level_id);
    QPixmap GetWallPaint(int passage_Id, int local_level_id, QVector<QRgb> *palettes, int palette_id);
};

#endif // WALLPAINTEDITORDIALOG_H
