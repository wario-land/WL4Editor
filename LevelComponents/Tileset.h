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
        Tileset(int tilesetPtr, int __TilesetID);
        Tile8x8 **GetTile8x8Data() { return tile8x8data; }
        TileMap16 **GetMap16Data() { return map16data; }
        QVector<QRgb> *GetPalettes() { return palettes; }
        unsigned short *Map16EventTable;
        unsigned char *Map16WarioAnimationSlotIDTable;
    };
}

#endif // TILESET_H
