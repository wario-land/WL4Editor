#ifndef ANIMATEDTILE8X8GROUP_H
#define ANIMATEDTILE8X8GROUP_H

#include <QColor>
#include <QVector>
#include <QPixmap>

namespace LevelComponents
{
    enum TileAnimationType
    {
        NoAnimation      = 0,
        Loop             = 1,
        StopAtMin        = 2,
        MinToMaxThenStop = 3,
        BackAndForth     = 4,
        MaxToMinThenStop = 5,
        ReverseLoop      = 6
    };

    class AnimatedTile8x8Group
    {
    public:
        AnimatedTile8x8Group(unsigned int animatedTilegroupHeaderPtr);

        // functions
        QPixmap RenderWholeAnimatedTileGroup(QVector<QRgb> *palettes, unsigned int paletteId);

        // getters
        TileAnimationType GetAnimationType() { return animationtype; }
        unsigned char GetCountPerFrame() { return countPerFrame; }
        int GetTotalFrameCount() { return tile8x8Numcount / 4; }

        // setters
        void SetAnimationType(unsigned int value) { animationtype = static_cast<enum TileAnimationType>(value); }
        void SetCountPerFrame(unsigned char _countPerFrame) { countPerFrame = _countPerFrame; }
        void SetTileData(QByteArray _tiledata) { if (!(_tiledata.size() % (32 * 4))) {tileData = _tiledata; tile8x8Numcount = _tiledata.size() / (32 * 4);} }

    private:
        /**
         * struct in ROM:
         *
         * u8  type;
         * u8  countPerFrame;
         * u8  TotalFrameCount; // always use 4 Tile8x8s per frame, so the total tile data size is: 32 Bytes x 4 x TotalFrameCount
         * u8  unused
         * u32 tiledataptr;
         */
        TileAnimationType animationtype;
        unsigned char countPerFrame;
        int tile8x8Numcount = 0; // tile8x8Numcount = TotalFrameCount * 4
        QByteArray tileData;
    };
}


#endif // ANIMATEDTILE8X8GROUP_H
