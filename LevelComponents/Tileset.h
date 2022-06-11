﻿#ifndef TILESET_H
#define TILESET_H

#define Tile8x8DefaultNum 0x600
#define Tile16DefaultNum 0x300

#include <QColor>
#include <QVector>

#include "Tile.h"

namespace LevelComponents
{
    class Tileset
    {
    private:
        int tilesetPtr;
        QVector<Tile8x8 *> tile8x8array;
        QVector<TileMap16 *> map16array;
        QVector<QRgb> palettes[16];
        Tile8x8 *blankTile = nullptr;
        int UniversalSpritesTilesPalettePtr = 0;
        unsigned short *AnimatedTileData[2] = {nullptr, nullptr};
        unsigned char *AnimatedTileSwitchTable = nullptr;
        unsigned short *Map16EventTable = nullptr;
        unsigned char *Map16TerrainTypeIDTable = nullptr;
        unsigned short *TilesetPaletteData = nullptr;
        bool hasconstructed = false;
        bool newtileset = false;
        int paletteAddress, fgGFXptr, fgGFXlen, bgGFXptr, bgGFXlen, map16ptr;

    public:
        Tileset(int tilesetPtr, int __TilesetID, bool IsloadFromTmpROM = false);
        Tileset(Tileset *old_tileset, int __TilesetID);
        void SetAnimatedTile(int tile8x8groupId, int tile8x8group2Id, int SwitchId, int startTile8x8Id);
        int getTilesetPtr() { return tilesetPtr; }
        void setTilesetPtr(int pointerValue) { tilesetPtr = pointerValue; }
        QVector<Tile8x8 *> GetTile8x8arrayPtr() { return tile8x8array; }
        QVector<TileMap16 *> GetMap16arrayPtr() { return map16array; }
        QVector<QRgb> *GetPalettes() { return palettes; }
        void SetColor(int paletteId, int colorId, QRgb newcolor) { palettes[paletteId][colorId] = newcolor; }
        void SetTile8x8(Tile8x8 *newtile, int tileId) { tile8x8array[tileId] = newtile; }
        ~Tileset();
        QPixmap RenderAllTile8x8(int paletteId);
        QPixmap RenderAllTile16(int columns);
        QPixmap RenderTile8x8(int tileId, int paletteId);
        int GetUniversalSpritesTilesPalettePtr() { return UniversalSpritesTilesPalettePtr; }

        unsigned char *GetTerrainTypeIDTablePtr() { return Map16TerrainTypeIDTable; }
        unsigned short *GetEventTablePtr() { return Map16EventTable; }
        unsigned short *GetTilesetPaletteDataPtr() { return TilesetPaletteData; }
        unsigned short *GetAnimatedTileData(int id) { return AnimatedTileData[id]; }
        unsigned char *GetAnimatedTileSwitchTable() { return AnimatedTileSwitchTable; }

        int GetfgGFXptr() { return fgGFXptr; }
        void SetfgGFXptr(int new_fgGFXptr) { fgGFXptr = new_fgGFXptr; }
        int GetfgGFXlen() { return fgGFXlen; }
        void SetfgGFXlen(int lenth) { fgGFXlen = lenth; }
        int GetbgGFXptr() { return bgGFXptr; }
        void SetbgGFXptr(int new_bgGFXptr) { bgGFXptr = new_bgGFXptr; }
        int GetbgGFXlen() { return bgGFXlen; }
        void SetbgGFXlen(int lenth) { bgGFXlen = lenth; }
        void Setmap16ptr(int new_map16ptr) { map16ptr = new_map16ptr; }

        int GetPaletteAddr() { return paletteAddress; }
        void SetPaletteAddr(int new_paletteAddress) { paletteAddress = new_paletteAddress; }
        void ReGeneratePaletteData();
        void SetChanged(bool changed) {newtileset = changed;}
        bool IsNewTileset() {return newtileset; }
        Tile8x8 *GetblankTile() { return blankTile; }

        void DelTile8x8(int tile8x8Id);
    };
} // namespace LevelComponents

#endif // TILESET_H
