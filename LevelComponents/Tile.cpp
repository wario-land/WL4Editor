#include "Tile.h"
#include "ROMUtils.h"

#include <QGraphicsPixmapItem>
#include <QTransform>
#include <QPainter>
#include <QPoint>

#include <iostream>

namespace LevelComponents
{
    /// <summary>
    /// Construct an instance of Tile8x8.
    /// </summary>
    /// <param name="dataPtr">
    /// Pointer to the beginning of the tile graphic data.
    /// </param>
    /// <param name="_palettes">
    /// Entire palette for the tileset this tile is a part of.
    /// </param>
    Tile8x8::Tile8x8(int dataPtr, QVector<QRgb> *_palettes) : Tile8x8(_palettes)
    {
        for(int i = 0; i < 8; ++i) {
            for(int j = 0; j < 4; ++j) {
                unsigned char val = ROMUtils::CurrentFile[dataPtr + i * 4 + j];
                ImageData->setPixel(j * 2, i, (unsigned char) (val & 0xF));
                ImageData->setPixel(j * 2 + 1, i, (unsigned char) ((val >> 4) & 0xF));
            }
        }
    }

    /// <summary>
    /// Deconstruct the Tile8x8 and clean up its instance objects on the heap.
    /// </summary>
    Tile8x8::~Tile8x8()
    {
        delete ImageData;
    }

    /// <summary>
    /// This is a static helper function to create the transparent blank tile object for the Tileset class.
    /// </summary>
    /// <param name="_palettes">
    /// Entire palette for the tileset this tile is a part of.
    /// </param>
    /// <return>
    /// A QPixmap of the transparent 8x8 blank tile object.
    /// </return>
    Tile8x8 *Tile8x8::CreateBlankTile(QVector<QRgb> *_palettes)
    {
        Tile8x8 *t = new Tile8x8(_palettes);
        for(int i = 0; i < 8; ++i)
        {
            for(int j  = 0; j < 8; ++j)
            {
                t->ImageData->setPixel(i, j, 0);
            }
        }
        return t;
    }

    /// <summary>
    /// Draw a Tile8x8 object onto a pixmap.
    /// </summary>
    /// <remarks>
    /// The units for X and Y are pixels.
    /// </remarks>
    /// <param name="layerPixmap">
    /// The pixmap object onto which the tile will be drawn.
    /// </param>
    /// <param name="x">
    /// The X position to draw the tile to.
    /// </param>
    /// <param name="y">
    /// The Y position to draw the tile to.
    /// </param>
    void Tile8x8::DrawTile(QPixmap *layerPixmap, int x, int y)
    {
        QPainter painter(layerPixmap);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        QPixmap tilePixmap = QPixmap::fromImage(ImageData->mirrored(FlipX, FlipY));
        QPoint drawDestination(x, y);
        painter.drawImage(drawDestination, tilePixmap.toImage());
    }

    /// <summary>
    /// Construct an instance of TileMap16.
    /// </summary>
    /// <param name="t0">
    /// The top left 8x8 tile.
    /// </param>
    /// <param name="t1">
    /// The top right 8x8 tile.
    /// </param>
    /// <param name="t2">
    /// The bottom left 8x8 tile.
    /// </param>
    /// <param name="t3">
    /// The bottom right 8x8 tile.
    /// </param>
    TileMap16::TileMap16(Tile8x8 *t0, Tile8x8 *t1, Tile8x8 *t2, Tile8x8 *t3) : Tile(TileTypeMap16)
    {
        TileData[0] = t0;
        TileData[1] = t1;
        TileData[2] = t2;
        TileData[3] = t3;
    }

    /// <summary>
    /// Deconstruct the TileMap16 and clean up its instance objects on the heap.
    /// </summary>
    TileMap16::~TileMap16()
    {
        delete TileData[0];
        delete TileData[1];
        delete TileData[2];
        delete TileData[3];
    }

    /// <summary>
    /// Draw a TileMap16 object onto a pixmap.
    /// </summary>
    /// <remarks>
    /// The units for X and Y are pixels.
    /// </remarks>
    /// <param name="layerPixmap">
    /// The pixmap object onto which the tile will be drawn.
    /// </param>
    /// <param name="x">
    /// The X position to draw the tile to.
    /// </param>
    /// <param name="y">
    /// The Y position to draw the tile to.
    /// </param>
    void TileMap16::DrawTile(QPixmap *layerPixmap, int x, int y)
    {
        TileData[0]->DrawTile(layerPixmap, x, y);
        TileData[1]->DrawTile(layerPixmap, x + 8, y);
        TileData[2]->DrawTile(layerPixmap, x, y + 8);
        TileData[3]->DrawTile(layerPixmap, x + 8, y + 8);
    }
}
