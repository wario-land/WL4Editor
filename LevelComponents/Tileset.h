#ifndef TILESET_H
#define TILESET_H

#include <QColor>
#include <QVector>

#include "Tile.h"

namespace LevelComponents
{
    class Tileset
    {
    private:
        Tile8x8 *tile8x8data[0x600];
        TileMap16 *map16data[0x300];
        QVector<QRgb> palettes[16];
        Tile8x8 *blankTile = nullptr;
        int UniversalSpritesTilesPalettePtr = 0;

    public:
        Tileset(int tilesetPtr, int __TilesetID);
        Tile8x8 **GetTile8x8Data() { return tile8x8data; }
        TileMap16 **GetMap16Data() { return map16data; }
        QVector<QRgb> *GetPalettes() { return palettes; }
        unsigned short *Map16EventTable;
        unsigned char *Map16TerrainTypeIDTable;
        ~Tileset();
        QPixmap Render(int columns);
        int GetUniversalSpritesTilesPalettePtr() { return UniversalSpritesTilesPalettePtr; }
        unsigned char *GetTerrainTypeIDTablePtr() { return Map16TerrainTypeIDTable; }
        unsigned short *GetEventTablePtr() { return Map16EventTable; }
    };
} // namespace LevelComponents

#endif // TILESET_H
