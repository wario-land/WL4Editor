#include "Tile.h"
#include "ROMUtils.h"

#include <cassert>

#include <QGraphicsPixmapItem>
#include <QTransform>
#include <QPainter>
#include <QPoint>

#include <iostream>

#define NOCACHE

namespace LevelComponents
{
    QMap<QImage*, int> Tile8x8::ImageDataCache;

    /// <summary>
    /// Construct an instance of Tile8x8 with uninitialized data. (private constructor)
    /// </summary>
    /// <param name="_palettes">
    /// Entire palette for the tileset this tile is a part of.
    /// </param>
    Tile8x8::Tile8x8(QVector<QRgb> *_palettes) : Tile(TileType8x8),
        ImageData(new QImage(8, 8, QImage::Format_Indexed8)),
        palettes(_palettes)
    {
        ImageData->setColorTable(palettes[paletteIndex]);
    }

    /// <summary>
    /// Copy constructor for Tile8x8
    /// </summary>
    /// <param name="other">
    /// Another Tile8x8 to copy image data from.
    /// </param>
    Tile8x8::Tile8x8(Tile8x8 *other) : Tile(TileType8x8),
        palettes(other->palettes)
    {
        ImageData = GetCachedImageData(other->ImageData);
    }

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
        for(int i = 0; i < 8; ++i) {
            for(int j = 0; j < 4; ++j) {
                unsigned char val = ROMUtils::CurrentFile[dataPtr + i * 4 + j];
                ImageData->setPixel(j * 2, i, (unsigned char) (val & 0xF));
                ImageData->setPixel(j * 2 + 1, i, (unsigned char) ((val >> 4) & 0xF));
            }
        }

#ifndef NOCACHE
        // Cache the QImage
        QImage *cached = GetCachedImageData(ImageData);
        if(cached != ImageData)
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
    /// Set the index for this tile within its palette group
    /// </summary>
    /// <param name="index">
    /// The index (0 - 15) of which palette to use for the image.
    /// </param>
    void Tile8x8::SetPaletteIndex(int index)
    {
        paletteIndex = index;
        QImage *newImage = new QImage(*ImageData);
        newImage->setColorTable(palettes[paletteIndex]);
        ImageData = newImage;
#ifndef NOCACHE
        QImage *cached = GetCachedImageData(ImageData);
        if(cached != ImageData)
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
    /// Cache and/or return the cached copy of existing image data in the global cache.
    /// </summary>
    /// <param name="image">
    /// The image data to attempt to obtain a cached copy for.
    /// </param>
    /// <returns>
    /// The original image if not cached yet, or the already cached image.
    /// </returns>
    QImage *Tile8x8::GetCachedImageData(QImage *image)
    {
        int references = ImageDataCache.value(image, 0);
        if(references)
        {
            ++ImageDataCache[image];
            for(QImage *img : ImageDataCache.keys())
            {
                if(!CompareImages(image, img))
                {
                    return img;
                }
            }
            // This area should never be reached (there is a reference in the map, but no matching QImage)
            assert(0);
        }
        else
        {
            ImageDataCache[image] = 1;
            return image;
        }
    }

    void Tile8x8::DeleteCachedImageData(QImage *image)
    {
        int references = ImageDataCache.value(image, 0);
        if(references > 1)
        {
            --ImageDataCache[image];
        }
        else if(references == 1)
        {
            ImageDataCache.take(image);
            delete image;
        }
        else
        {
            // This area should never be reached (there cannot be a QImage in the map with 0 references)
            assert(0);
        }
    }

    int Tile8x8::CompareImages(QImage *img1, QImage *img2)
    {
        QVector<QRgb> pal1 = img1->colorTable();
        QVector<QRgb> pal2 = img2->colorTable();
        if(pal1.size() != pal2.size())
        {
            return pal1.size() - pal2.size();
        }
        for(int i = 0; i < pal1.size(); ++i)
        {
            if(pal1[i] != pal2[i])
            {
                return pal1[i] - pal2[i];
            }
        }
        int size1 = img1->width() * img1->height();
        int size2 = img2->width() * img2->height();
        if(size1 != size2)
        {
            return size1 - size2;
        }
        return memcmp(img1->data_ptr(), img2->data_ptr(), size1);
    }
}
