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
        int tilesetPtr;
        int Tile8x8DefaultNum = 0x600;
        int Tile16DefaultNum = 0x300;
        Tile8x8 **tile8x8array = nullptr;
        TileMap16 **map16array = nullptr;
        QVector<QRgb> palettes[16];
        Tile8x8 *blankTile = nullptr;
        int UniversalSpritesTilesPalettePtr = 0;
        unsigned short *Map16EventTable = nullptr;
        unsigned char *Map16TerrainTypeIDTable = nullptr;
        unsigned short *TilesetPaletteData = nullptr;
        bool IsNewTileset = false;
        int palettePtr, fgGFXptr, fgGFXlen, bgGFXptr, bgGFXlen, map16ptr;
        unsigned char *statictile8x8data = nullptr;
        unsigned char *tile16data = nullptr;

    public:
        Tileset(int tilesetPtr, int __TilesetID);
        Tileset(Tileset *old_tileset, int __TilesetID);
        int getTilesetPtr() { return tilesetPtr; }
        Tile8x8 **GetTile8x8arrayPtr() { return tile8x8array; }
        TileMap16 **GetMap16arrayPtr() { return map16array; }
        QVector<QRgb> *GetPalettes() { return palettes; }
        ~Tileset();
        QPixmap RenderTile8x8(int paletteId);
        QPixmap RenderTile16(int columns);
        int GetUniversalSpritesTilesPalettePtr() { return UniversalSpritesTilesPalettePtr; }
        unsigned char *GetTerrainTypeIDTablePtr() { return Map16TerrainTypeIDTable; }
        unsigned short *GetEventTablePtr() { return Map16EventTable; }
        unsigned short *GetTilesetPaletteDataPtr() { return TilesetPaletteData; }
        void SetChanged() {IsNewTileset = true;}
        int GettilesetPtr() {return tilesetPtr; }
        Tile8x8 *GetblankTile() { return blankTile; }
        void ResetATile8x8MapDataInTile16Data(int tile16Id, int position, int newtile8x8Id, bool xflip, bool yflip, int paletteId);
    };
} // namespace LevelComponents

#endif // TILESET_H
