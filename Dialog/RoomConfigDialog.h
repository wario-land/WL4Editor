#ifndef ROOMCONFIGDIALOG_H
#define ROOMCONFIGDIALOG_H

#include <QDialog>

#include <QGraphicsScene>
#include <QPixmap>
#include <QGraphicsPixmapItem>

#include "WL4Constants.h"
#include "ROMUtils.h"
#include "LevelComponents/Tileset.h"

namespace Ui {
class RoomConfigDialog;
}

class RoomConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RoomConfigDialog(QWidget *parent = 0);
    ~RoomConfigDialog();

private:
    Ui::RoomConfigDialog *ui;
    int CurrentTilesetIndex;  // TODO: put this into a struct
    void ShowTilesetDetails(int _tilesetIndex);
    void ShowMappingType20LayerDetails(int _layerdataAddr);
};

#endif // ROOMCONFIGDIALOG_H
