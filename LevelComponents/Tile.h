#ifndef TILE_H
#define TILE_H

#include <QPainter>

#define ROT(X) (((X) << 13) | ((X) >> 19))

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
        Tile(enum TileType _type) : Type(_type) {}

    public:
        virtual void DrawTile(QPixmap *layerPixmap, int x, int y) = 0;
        virtual ~Tile() {}
    };

    class QImageW : public QImage
    {
    public:
        QImageW(int W, int H, QImage::Format F) : QImage(W, H, F) {}
        QImageW(const QImageW &img) : QImage(img) {}
        QImageW(QImage img) : QImage(img) {}
        ~QImageW() {}

        /// <summary>
        /// Inline equality comparison functionality to provide O(1) performance for QHash
        /// </summary>
        /// <param name="other">
        /// The first image to compare.
        /// </param>
        /// <returns>
        /// True if the image data of this image and the other one are equal.
        /// </returns>
        inline bool operator==(const QImageW &other)
        {
            QVector<QRgb> pal1 = colorTable();
            QVector<QRgb> pal2 = other.colorTable();
            if (pal1.size() != pal2.size())
            {
                return false;
            }
            for (int i = 0; i < pal1.size(); ++i)
            {
                if (pal1[i] != pal2[i])
                {
                    return false;
                }
            }
            int size1 = width() * height();
            int size2 = other.width() * other.height();
            if (size1 != size2)
            {
                return false;
            }
            QImageW *ptr = (QImageW *) &other;
            return !memcmp(data_ptr(), ptr->data_ptr(), size1);
        }

        /// <summary>
        /// Inline hashing functionality to provide O(1) performance for QHash
        /// </summary>
        /// <param name="img">
        /// The image to hash.
        /// </param>
        /// <param name="seed">
        /// The seed value for hashing the image.
        /// </param>
        /// <returns>
        /// A hash value for the QImage.
        /// </returns>
        inline uint qHash(const QImageW *img, uint seed)
        {
            for (QRgb rgb : img->colorTable())
            {
                seed = ROT(seed ^ (uint) rgb);
            }
            const int bytes = img->width() * img->height() * img->depth() / 32;
            unsigned int *data = (unsigned int *) img->constBits();
            for (int i = 0; i < bytes; ++i)
            {
                seed = ROT(seed ^ data[i]);
            }
            return seed;
        }
    };

    class Tile8x8 : public Tile
    {
    private:
        Tile8x8(QVector<QRgb> *_palettes);
        QVector<QRgb> *palettes;
        QImageW *ImageData;
        int paletteIndex = 0;
        bool FlipX = false;
        bool FlipY = false;

        static QImageW *GetCachedImageData(QImageW *image);
        static QHash<QImageW *, int> ImageDataCache;
        static void DeleteCachedImageData(QImageW *image);

    public:
        Tile8x8(int dataPtr, QVector<QRgb> *_palettes);
        Tile8x8(Tile8x8 *other);
        void DrawTile(QPixmap *layerPixmap, int x, int y);
        static Tile8x8 *CreateBlankTile(QVector<QRgb> *_palettes);
        void SetFlipX(bool _flipX) { FlipX = _flipX; }
        void SetFlipY(bool _flipY) { FlipY = _flipY; }
        void SetPaletteIndex(int index);
        ~Tile8x8();
    };

    class TileMap16 : public Tile
    {
    private:
        Tile8x8 *TileData[4];

    public:
        TileMap16() : Tile(TileTypeMap16) {}
        TileMap16(Tile8x8 *t0, Tile8x8 *t1, Tile8x8 *t2, Tile8x8 *t3);
        void DrawTile(QPixmap *layerPixmap, int x, int y);
        ~TileMap16();
    };
} // namespace LevelComponents

#endif // TILE_H
