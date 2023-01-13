#ifndef TILEUTILS_H
#define TILEUTILS_H

#include <QObject>
#include "LevelComponents/Tile.h"

namespace PCG
{
    namespace GFXUtils
    {
        class TileUtils : public QObject
        {
            Q_OBJECT
        public:
            explicit TileUtils(QObject *parent = nullptr);

            // Tile functions can be used in js script
            Q_INVOKABLE int GetFitness_CurTilesetJoinTile16_UL(unsigned int upper_tile16_id, unsigned int lower_tile16_id);
            Q_INVOKABLE int GetFitness_CurTilesetJoinTile16_LR(unsigned int left_tile16_id, unsigned int right_tile16_id);
            Q_INVOKABLE bool IsBlankTile_CurTilesetTile16(unsigned int tile16_id);

            // helper functions for Tile stuff
            int GetFitnessJoinTile16_UL(LevelComponents::TileMap16 *Upper_tile16, LevelComponents::TileMap16 *Lower_tile16);
            int GetFitnessJoinTile16_LR(LevelComponents::TileMap16 *Left_tile16, LevelComponents::TileMap16 *Right_tile16);

        signals:
            // nothings here

        private:
            static const int max_Tile16_border_pixels_color_channel_diff_value = 0xFF * 16 * 3;

        };
    }
}

#endif // TILEUTILS_H
