#ifndef TILE16DOCKWIDGET_H
#define TILE16DOCKWIDGET_H

#include <QDockWidget>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QPixmap>

#include "LevelComponents/Tileset.h"
#include "ROMUtils.h"
#include "WL4Constants.h"

namespace Ui
{
    class Tile16DockWidget;
}

class Tile16DockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit Tile16DockWidget(QWidget *parent = 0);
    ~Tile16DockWidget();
    void FocusInEvent(QFocusEvent *e);

private:
    Ui::Tile16DockWidget *ui;
    int scalerate = 1;
    LevelComponents::Tileset *SelectedTileset = nullptr;
    QGraphicsScene *Tile16MAPScene = nullptr;

    // Selection box
    const QColor highlightColor = QColor(0xFF, 0, 0, 0x7F);
    QGraphicsPixmapItem *SelectionBox = nullptr;
    unsigned short SelectedTile = 0;
    int rw = 1; int rh = 1;

    // Function
    void SetTileInfoText(QString str);

public:
    int SetTileset(int _tilesetIndex);
    LevelComponents::Tileset *GetSelectedTileset() { return SelectedTileset; }
    void SetSelectedTile(unsigned short tile, bool resetscrollbar);
    void RectSelectTiles(int rect_width, int rect_height);
    unsigned short GetSelectedTile() { return SelectedTile; }
    int getrw() {return rw;}
    int getrh() {return rh;}
};

#endif // TILE16DOCKWIDGET_H
