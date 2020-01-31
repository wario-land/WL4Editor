#include "Tile.h"
#include "ROMUtils.h"

#include <cassert>

#include <QGraphicsPixmapItem>
#include <QPoint>
#include <QTransform>

#include <iostream>

//#define NOCACHE

namespace LevelComponents
{
    QHash<QImageW *, int> Tile8x8::ImageDataCache;

    /// <summary>
    /// Construct an instance of Tile8x8 with uninitialized data. (private constructor)
    /// </summary>
    /// <param name="_palettes">
    /// Entire palette for the tileset this tile is a part of.
    /// </param>
    Tile8x8::Tile8x8(QVector<QRgb> *_palettes) :
            Tile(TileType8x8), palettes(_palettes), ImageData(new QImageW(8, 8, QImage::Format_Indexed8))
    {
        ImageData->setColorTable(palettes[paletteIndex]);
    }

    /// <summary>
    /// Copy constructor for Tile8x8, used only in current Tileset
    /// </summary>
    /// <param name="other">
    /// Another Tile8x8 to copy image data from.
    /// </param>
    Tile8x8::Tile8x8(Tile8x8 *other) :
        Tile(TileType8x8), palettes(other->palettes),
#ifndef NOCACHE
        ImageData(GetCachedImageData(other->ImageData)),
#else
        ImageData(new QImageW(other->ImageData->copy())),
#endif
        index(other->index), paletteIndex(other->paletteIndex), FlipX(other->FlipX), FlipY(other->FlipY)
    {}

    /// <summary>
    /// Deep Copy constructor for Tile8x8, use this to create new Tile8x8 for new Tileset
    /// </summary>
    /// <param name="other">
    /// Another Tile8x8 to copy image data from.
    /// </param>
    Tile8x8::Tile8x8(Tile8x8 *other, QVector<QRgb> *_palettes) :
        Tile(TileType8x8), palettes(_palettes),
#ifndef NOCACHE
        ImageData(GetCachedImageData(other->ImageData)),
#else
        ImageData(new QImageW(other->ImageData->copy())),
#endif
        index(other->index), paletteIndex(other->paletteIndex), FlipX(other->FlipX), FlipY(other->FlipY)
    {}

    /// <summary>
    /// Construct an instance of Tile8x8.
    /// </summary>
    /// <remarks>
    /// This constructor will attempt to match the image data to a cached QImage
    /// </remarks>
    /// <param name="dataPtr">
    /// Pointer to the beginning of the tile graphic data.
    /// </param>
    /// <param name="_palettes">
    /// Entire palette for the tileset this tile is a part of.
    /// </param>
    Tile8x8::Tile8x8(int dataPtr, QVector<QRgb> *_palettes) : Tile8x8(_palettes)
    {
        // Initialize the QImage data from ROM
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                unsigned char val = ROMUtils::CurrentFile[dataPtr + i * 4 + j];
                ImageData->setPixel(j * 2, i, (unsigned char) (val & 0xF));
                ImageData->setPixel(j * 2 + 1, i, (unsigned char) ((val >> 4) & 0xF));
            }
        }

#ifndef NOCACHE
        // Cache the QImage
        QImageW *cached = GetCachedImageData(ImageData);
        if (cached != ImageData)
        {
            delete ImageData;
            ImageData = cached;
        }
#endif
    }

    /// <summary>
    /// Construct an instance of Tile8x8.
    /// </summary>
    /// <remarks>
    /// This constructor will attempt to match the image data to a cached QImage
    /// </remarks>
    /// <param name="data">
    /// Pointer to the beginning of the tile graphic data.
    /// </param>
    /// <param name="_palettes">
    /// Entire palette for the tileset this tile is a part of.
    /// </param>
    Tile8x8::Tile8x8(unsigned char *data, QVector<QRgb> *_palettes) : Tile8x8(_palettes)
    {
        // Initialize the QImage data from data
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                unsigned char val = data[i * 4 + j];
                ImageData->setPixel(j * 2, i, (unsigned char) (val & 0xF));
                ImageData->setPixel(j * 2 + 1, i, (unsigned char) ((val >> 4) & 0xF));
            }
        }

#ifndef NOCACHE
        // Cache the QImage
        QImageW *cached = GetCachedImageData(ImageData);
        if (cached != ImageData)
        {
            delete ImageData;
            ImageData = cached;
        }
#endif
    }

    /// <summary>
    /// Deconstruct the Tile8x8 and clean up its instance objects on the heap.
    /// </summary>
    Tile8x8::~Tile8x8()
    {
#ifndef NOCACHE
        DeleteCachedImageData(ImageData);
#else
        delete ImageData;
#endif
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
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                t->ImageData->setPixel(i, j, 0);
            }
        }

#ifndef NOCACHE
        // Cache the QImage
        QImageW *cached = GetCachedImageData(t->ImageData);
        if (cached != t->ImageData)
        {
            delete t->ImageData;
            t->ImageData = cached;
        }
#endif

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
        ImageData->setColorTable(palettes[paletteIndex]);
        QImage tileImage = ImageData->mirrored(FlipX, FlipY);
        QPoint drawDestination(x, y);
        painter.drawImage(drawDestination, tileImage);
    }

    /// <summary>
    /// Set the index for this tile within its palette group
    /// </summary>
    /// <param name="index">
    /// The index (0 - 15) of which palette to use for the image.
    /// </param>
    void Tile8x8::SetPaletteIndex(int index)
    {
        paletteIndex = index;
        QImageW *newImage = new QImageW(*ImageData);
        newImage->setColorTable(palettes[paletteIndex]);
        ImageData = newImage;
#ifndef NOCACHE
        QImageW *cached = GetCachedImageData(ImageData);
        if (cached != ImageData)
        {
            delete ImageData;
            ImageData = cached;
        }
#endif
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
    /// Deep copy constructor for TileMap16.
    /// </summary>
    /// <param name="other">
    /// The source map16 tile.
    /// </param>
    TileMap16::TileMap16(TileMap16 *other, QVector<QRgb> *newpalettes) : TileMap16(
        new Tile8x8(other->GetTile8X8(TILE8_TOPLEFT), newpalettes),
        new Tile8x8(other->GetTile8X8(TILE8_TOPRIGHT), newpalettes),
        new Tile8x8(other->GetTile8X8(TILE8_BOTTOMLEFT), newpalettes),
        new Tile8x8(other->GetTile8X8(TILE8_BOTTOMRIGHT), newpalettes))
    { }

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

    /// <summary>
    /// Get a pointer to a Tile8x8 at current position
    /// <param name="position">
    /// The position (TileMap16::TILE8_TOPLEFT : 0, TileMap16::TILE8_TOPLEFT : 1, TileMap16::TILE8_BOTTOMLEFT : 2, TileMap16::TILE8_BOTTOMRIGHT : 3)
    /// </param>
    Tile8x8* TileMap16::GetTile8X8(int position)
    {
        return TileData[position & 3];
    }

    /// <summary>
    /// Change one of the TIle8x8 in TileMap16
    /// </summary>
    /// <param name="other">
    /// an Tile8x8 used as copy referance
    /// </param>
    /// <param name="position">
    /// The position (TileMap16::TILE8_TOPLEFT : 0, TileMap16::TILE8_TOPLEFT : 1, TileMap16::TILE8_BOTTOMLEFT : 2, TileMap16::TILE8_BOTTOMRIGHT : 3)
    /// </param>
    /// <param name="new_index">
    /// new index of tile8x8
    /// </param>
    /// <param name="new_paletteIndex">
    /// set a new palette index
    /// </param>
    /// <param name="xflip">
    /// set xflip bit
    /// </param>
    /// <param name="yflip">
    /// set yflip bit
    /// </param>
    void TileMap16::ResetTile8x8(Tile8x8 *other, int position, int new_index, int new_paletteIndex, bool xflip, bool yflip)
    {
        int pos = position & 3;
        delete TileData[pos];
        TileData[pos] = new Tile8x8(other);
        TileData[pos]->SetIndex(new_index);
        TileData[pos]->SetFlipX(xflip);
        TileData[pos]->SetFlipY(yflip);
        TileData[pos]->SetPaletteIndex(new_paletteIndex);
    }

    /// <summary>
    /// Cache and/or return the cached copy of existing image data in the global cache.
    /// </summary>
    /// <param name="image">
    /// The image data to attempt to obtain a cached copy for.
    /// </param>
    /// <returns>
    /// The original image if not cached yet, or the already cached image.
    /// </returns>
    QImageW *Tile8x8::GetCachedImageData(QImageW *image)
    {
        if (ImageDataCache.value(image, 0))
        {
            ++ImageDataCache[image];
            return ImageDataCache.find(image).key();
        }
        else
        {
            ImageDataCache[image] = 1;
            return image;
        }
    }

    /// <summary>
    /// Delete an image reference from the cache.
    /// If there is more than 1 reference in the cache, only decrement the reference count.
    /// If there is only one copy in the cache, actually remove the data from it as well.
    /// </summary>
    /// <param name="image">
    /// The image data to remove from the cache.
    /// </param>
    void Tile8x8::DeleteCachedImageData(QImageW *image)
    {
        int references = ImageDataCache.value(image, 0);
        if (references > 1)
        {
            --ImageDataCache[image];
        }
        else if (references == 1)
        {
            ImageDataCache.take(image);
            delete image;
        }
        else
        {
            assert(0 /* Cached QImage with 0 references */);
        }
    }

    /// <summary>
    /// Get the two byte corresponding to the tile8 in ROM.
    /// </summary>
    unsigned short Tile8x8::GetValue()
    {
        // First set the 10 first byte that represent the index of the tile8
        unsigned short tile8Data=this->GetIndex() & 0x3FF;

        // Set the flipX and flipY bit if present (11th and 12th bits)
        if (this->GetFlipX()) {
            tile8Data |= 1 << 10;
        }

        if (this->GetFlipY()) {
            tile8Data |= 1 << 11;
        }

        // Set the last 4 bits to the palette bits.
        tile8Data |= this->GetPaletteIndex() << 12;

        return tile8Data;
    }

    /// <summary>
    /// Turn a tile into graphics data.
    /// </summary>
    /// <returns>
    /// The 32 bytes of graphics data in uncompressed GBA format.
    /// </returns>
    QByteArray Tile8x8::CreateGraphicsData()
    {
        QByteArray arr(32, 0);
        for(int i = 0; i < 8; ++i)
        {
            for(int j = 0; j < 4; ++j)
            {
                char val = static_cast<char>(ImageData->pixelIndex(j << 1, i));
                val |= ImageData->pixelIndex((j << 1) + 1, i) << 4;
                arr[(i << 2) + j] = val;
            }
        }
        return arr;
    }

} // namespace LevelComponents
