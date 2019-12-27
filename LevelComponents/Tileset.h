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
        unsigned short *AnimatedTileData = nullptr;
        unsigned short *Map16EventTable = nullptr;
        unsigned char *Map16TerrainTypeIDTable = nullptr;
        unsigned short *TilesetPaletteData = nullptr;
        bool hasconstructed = false;
        bool newtileset = false;
        int paletteAddress, fgGFXptr, fgGFXlen, bgGFXptr, bgGFXlen, map16ptr;

    public:
        Tileset(int tilesetPtr, int __TilesetID);
        Tileset(Tileset *old_tileset, int __TilesetID);
        void SetAnimatedTile(int tile8x8groupId, int startTile8x8Id);
        int getTilesetPtr() { return tilesetPtr; }
        Tile8x8 **GetTile8x8arrayPtr() { return tile8x8array; }
        TileMap16 **GetMap16arrayPtr() { return map16array; }
        QVector<QRgb> *GetPalettes() { return palettes; }
        void SetColor(int paletteId, int colorId, QRgb newcolor) { palettes[paletteId][colorId] = newcolor; }
        ~Tileset();
        QPixmap RenderAllTile8x8(int paletteId);
        QPixmap RenderAllTile16(int columns);
        QPixmap RenderTile8x8(int tileId, int paletteId);
        int GetUniversalSpritesTilesPalettePtr() { return UniversalSpritesTilesPalettePtr; }

        unsigned char *GetTerrainTypeIDTablePtr() { return Map16TerrainTypeIDTable; }
        unsigned short *GetEventTablePtr() { return Map16EventTable; }
        unsigned short *GetTilesetPaletteDataPtr() { return TilesetPaletteData; }
        unsigned short *GetAnimatedTileData() { return AnimatedTileData; }

        int GetfgGFXptr() { return fgGFXptr; }
        int GetfgGFXlen() { return fgGFXlen; }
        int GetbgGFXlen() { return bgGFXlen; }
        void SetfgGFXlen(int lenth) { fgGFXlen = lenth; }

        int GetPaletteAddr() { return paletteAddress; }
        void ReGeneratePaletteData();
        void SetChanged(bool changed) {newtileset = changed;}
        bool IsNewTileset() {return newtileset; }
        Tile8x8 *GetblankTile() { return blankTile; }
    };
} // namespace LevelComponents

#endif // TILESET_H
