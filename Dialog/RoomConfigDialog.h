#ifndef ROOMCONFIGDIALOG_H
#define ROOMCONFIGDIALOG_H

#include <QDialog>

#include <QGraphicsScene>
#include <QPixmap>
#include <QGraphicsPixmapItem>

#include "WL4Constants.h"
#include "ROMUtils.h"
#include "LevelComponents/Layer.h"  //include Tileset.h

namespace DialogParams
{
    struct RoomConfigParams
    {
        int CurrentTilesetIndex;
        // TODO
    };

}

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
    DialogParams::RoomConfigParams currentParams;
    void ShowTilesetDetails();
    void ShowMappingType20LayerDetails(int _layerdataAddr, LevelComponents::Layer *_tmpLayer);
};

#endif // ROOMCONFIGDIALOG_H
