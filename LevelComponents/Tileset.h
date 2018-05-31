#ifndef TILESET_H
#define TILESET_H

#include <QVector>
#include <QColor>

#include "Tile.h"

namespace LevelComponents
{
    class Tileset
    {
    private:
        Tile8x8 *tile8x8data[0x600];
        TileMap16 *map16data[0x300];
        QVector<QRgb> palettes[16];
    public:
        Tileset(int tilesetPtr);
    };
}

#endif // TILESET_H
