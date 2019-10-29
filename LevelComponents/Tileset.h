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
        Tile8x8 **tile8x8data = nullptr;
        TileMap16 **map16data = nullptr;
        QVector<QRgb> palettes[16];
        Tile8x8 *blankTile = nullptr;
        int UniversalSpritesTilesPalettePtr = 0;
        unsigned short *Map16EventTable = nullptr;
        unsigned char *Map16TerrainTypeIDTable = nullptr;
        unsigned short *TilesetPaletteData = nullptr;
        bool HasChanged = false;
        bool Isbackup = false; // Set this when this is a deep-copy constructed Tileset and being edited
        bool DeleteEverythingOnDeconstruction = true; // Set this to false if changes need to be saved

    public:
        Tileset(int tilesetPtr, int __TilesetID);
        Tileset(Tileset *existing_tileset, int __TilesetID);
        int getTilesetPtr() { return tilesetPtr; }
        Tile8x8 **GetTile8x8Data() { return tile8x8data; }
        TileMap16 **GetMap16Data() { return map16data; }
        QVector<QRgb> *GetPalettes() { return palettes; }
        ~Tileset();
        QPixmap RenderTile8x8();
        QPixmap RenderTile16(int columns);
        int GetUniversalSpritesTilesPalettePtr() { return UniversalSpritesTilesPalettePtr; }
        unsigned char *GetTerrainTypeIDTablePtr() { return Map16TerrainTypeIDTable; }
        unsigned short *GetEventTablePtr() { return Map16EventTable; }
        unsigned short *GetTilesetPaletteDataPtr() { return TilesetPaletteData; }
        void SetChanged() {HasChanged = true;}
        void DisableDeleteOnDeconstruction() { DeleteEverythingOnDeconstruction = false; }
        int GettilesetPtr() {return tilesetPtr; }
        Tile8x8 *GetblankTile() { return blankTile; }
        void OverwriteAllData(Tileset *backup_tileset); // this function should be used by the original Tileset which accepts chanegs
    };
} // namespace LevelComponents

#endif // TILESET_H
