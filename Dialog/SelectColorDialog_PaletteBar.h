#ifndef SELECTCOLORDIALOG_PALETTEBAR_H
#define SELECTCOLORDIALOG_PALETTEBAR_H

#include <QGraphicsView>
#include <QMouseEvent>
#include <QScrollBar>
#include "Dialog/SelectColorDialog.h"

class SelectColorDialog_PaletteBar : public QGraphicsView
{
    Q_OBJECT

public:
    SelectColorDialog_PaletteBar(QWidget *param) : QGraphicsView(param) {}
    void mouseReleaseEvent(QMouseEvent *event);
    int GetSelectedColorId() { return SelectedColorId; }
    void SetFather(SelectColorDialog *parent) {pdialog = parent; }

private:
    SelectColorDialog *pdialog = nullptr;
    int SelectedColorId = 0;
    int mouseX_Release = 0;
    int mouseY_Release = 0;
};

#endif // SELECTCOLORDIALOG_PALETTEBAR_H
