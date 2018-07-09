#include "Tile.h"
#include "ROMUtils.h"

#include <QGraphicsPixmapItem>
#include <QTransform>
#include <QPainter>
#include <QPoint>

#include <iostream>

namespace LevelComponents
{
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

    void Tile8x8::DrawTile(QPixmap *layerPixmap, int x, int y)
    {
        QPainter painter(layerPixmap);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        QPixmap tilePixmap = QPixmap::fromImage(ImageData->mirrored(FlipX, FlipY));
        QPoint drawDestination(x, y);
        painter.drawImage(drawDestination, tilePixmap.toImage());
    }
}
