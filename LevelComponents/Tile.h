#ifndef TILE_H
#define TILE_H

#include <QImage>

namespace LevelComponents
{
    enum TileType
    {
        TileType8x8,
        TileTypeMap16
    };

    class Tile
    {
    private:
        enum TileType Type;
    protected:
        Tile(enum TileType _type) : Type(_type) { }
    public:
        virtual void DrawTile(int x, int y) = 0;
    };

    class Tile8x8 : Tile
    {
    private:
        QImage *ImageData;
        QVector<QRgb> *palettes;
        int paletteIndex = 0;
        bool FlipX = false;
        bool FlipY = false;
    public:
        Tile8x8(QVector<QRgb> *_palettes) : Tile(TileType8x8),
            ImageData(new QImage(8, 8, QImage::Format_Indexed8)),
            palettes(_palettes)
        {
            ImageData->setColorTable(palettes[paletteIndex]);
        }
        Tile8x8(int dataPtr, QVector<QRgb> *_palettes);
        Tile8x8(Tile8x8 &other) : Tile(TileType8x8),
            ImageData(other.ImageData) { }
        void DrawTile(int x, int y);
        static Tile8x8 *CreateBlankTile(QVector<QRgb> *_palettes);
        void SetFlipX(bool _flipX) { FlipX = _flipX; }
        void SetFlipY(bool _flipY) { FlipY = _flipY; }
        void SetPaletteIndex(int index) { paletteIndex = index; }
    };

    class TileMap16 : Tile
    {
    private:
        Tile8x8 *TileData[4];
    public:
        TileMap16() : Tile(TileTypeMap16) { }
        TileMap16(Tile8x8 *t0, Tile8x8 *t1, Tile8x8 *t2, Tile8x8 *t3) : Tile(TileTypeMap16)
        {
            TileData[0] = t0;
            TileData[1] = t1;
            TileData[2] = t2;
            TileData[3] = t3;
        }
        void DrawTile(int x, int y)
        {
            TileData[0]->DrawTile(x, y);
            TileData[1]->DrawTile(x + 8, y);
            TileData[2]->DrawTile(x, y + 8);
            TileData[3]->DrawTile(x + 8, y + 8);
        }
    };
}

#endif // TILE_H
