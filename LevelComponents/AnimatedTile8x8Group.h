#ifndef ANIMATEDTILE8X8GROUP_H
#define ANIMATEDTILE8X8GROUP_H

#include <QColor>
#include <QVector>
#include <QPixmap>

#include "Tile.h"

namespace LevelComponents
{
    enum TileAnimationType
    {
        NoAnimation      = 0,
        Loop             = 1,
        StopAtMin        = 2, // TODO: need to find what's the difference between 2 and 5
        MinToMaxThenStop = 3,
        BackAndForth     = 4,
        MaxToMinThenStop = 5,
        ReverseLoop      = 6
    };

    class AnimatedTile8x8Group
    {
    public:
        AnimatedTile8x8Group(unsigned int animatedTilegroupHeaderPtr, unsigned short _globalId);
        AnimatedTile8x8Group(AnimatedTile8x8Group &animatedtilegroup) :
            globalId(animatedtilegroup.globalId),
            animationtype(animatedtilegroup.animationtype),
            countPerFrame(animatedtilegroup.countPerFrame),
            tile8x8Numcount(animatedtilegroup.tile8x8Numcount)
        {
            tileData.resize(animatedtilegroup.tileData.size());
            for (int i = 0; i < animatedtilegroup.tileData.size(); i++)
            {
                tileData[i] = animatedtilegroup.tileData[i];
            }
        }

        // functions
        QPixmap RenderWholeAnimatedTileGroup(QVector<QRgb> *palettes, unsigned int paletteId);

        // getters
        unsigned short GetGlobalID() { return globalId; }
        unsigned char GetAnimationType() { return animationtype; }
        unsigned char GetCountPerFrame() { return countPerFrame; }
        int GetTotalFrameCount() { return tile8x8Numcount / 4; }
        QVector<Tile8x8 *> GetRenderTile8x8s(bool switchIsOn, QVector<QRgb> *palettes);

        // setters
        void SetAnimationType(unsigned int value) { animationtype = static_cast<enum TileAnimationType>(value); }
        void SetCountPerFrame(unsigned char _countPerFrame) { countPerFrame = _countPerFrame; }
        void SetTileData(QByteArray _tiledata) { if (!(_tiledata.size() % (32 * 4))) {tileData = _tiledata; tile8x8Numcount = _tiledata.size() / (32 * 4);} }

    private:
        unsigned short globalId = -1;
        /**
         * struct in ROM:
         *
         * u8  type;
         * u8  countPerFrame;
         * u8  TotalFrameCount; // always use 4 Tile8x8s per frame, so the total tile data size is: 32 Bytes x 4 x TotalFrameCount
         * u8  unused
         * u32 tiledataptr;
         */
        unsigned char animationtype;
        unsigned char countPerFrame;
        int tile8x8Numcount = 0; // tile8x8Numcount = TotalFrameCount * 4
        QByteArray tileData;
    };
}


#endif // ANIMATEDTILE8X8GROUP_H
