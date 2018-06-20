#ifndef TILE16DOCKWIDGET_H
#define TILE16DOCKWIDGET_H

#include <QDockWidget>
#include <WL4Constants.h>
#include <ROMUtils.h>
#include <LevelComponents/Tileset.h>
#include <QGraphicsScene>
#include <QPixmap>

namespace Ui {
class Tile16DockWidget;
}

class Tile16DockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit Tile16DockWidget(QWidget *parent = 0);
    ~Tile16DockWidget();

private:
    Ui::Tile16DockWidget *ui;

public:
    int SetTileset(int _tilesetIndex);
};

#endif // TILE16DOCKWIDGET_H
