#include "AnimatedTile8x8Group.h"

#include "Tile.h"
#include "ROMUtils.h"

namespace LevelComponents
{
    /// <summary>
    /// Construct an instance of AnimatedTile8x8Group.
    /// </summary>
    /// <param name="animatedTilegroupHeaderPtr">
    /// Pointer to the beginning of an animated tile group's tile8x8 data.
    /// </param>
    AnimatedTile8x8Group::AnimatedTile8x8Group(unsigned int animatedTilegroupHeaderPtr)
    {
        unsigned char *curFilePtr = ROMUtils::ROMFileMetadata->ROMDataPtr;
        unsigned char *tmpptr = curFilePtr + animatedTilegroupHeaderPtr;

        // set properties
        animationtype = static_cast<enum TileAnimationType>(tmpptr[0]);
        countPerFrame = tmpptr[1];
        tile8x8Numcount = tmpptr[2] * 4;

        // load tiles data
        tileData.resize(tile8x8Numcount * 32);
        for (int i = 0; i < tileData.size(); ++i)
        {
            tileData[i] = *(tmpptr + i);
        }
    }

    /// <summary>
    /// Render the while animated tile group onto a pixmap.
    /// </summary>
    /// <param name="palettes">
    /// The temp palettes is needed from the current Tileset to render animated tile group correctly.
    /// </param>
    /// <param name="paletteId">
    /// The id of palette is needed to render animated tile group correctly.
    /// </param>
    /// <returns>
    /// The animated tile group rendered at a pixmap.
    /// </returns>
    QPixmap AnimatedTile8x8Group::RenderWholeAnimatedTileGroup(QVector<QRgb> *palettes, unsigned int paletteId)
    {
        // only use Tile8x8 instances during the rendering
        // create Tile8x8 instances
        std::vector<Tile8x8 *> tile8x8array;
        unsigned char *tiledata = (unsigned char *) tileData.data();
        for (int i = 0; i < tile8x8Numcount; ++i)
        {
            tile8x8array.push_back(new Tile8x8(tiledata + i * 32, palettes));
        }

        // drawing
        QPixmap pixmap(8 * tile8x8Numcount, 8);
        pixmap.fill(Qt::transparent);
        for (int i = 0; i < 16; ++i)
        {
            tile8x8array[i]->SetPaletteIndex(paletteId);
            tile8x8array[i]->DrawTile(&pixmap, i * 8, 0);
        }

        // delete Tile8x8 instances
        for (auto &tileptr: tile8x8array)
        {
            delete tileptr;
        }
        tile8x8array.clear();

        return pixmap;
    }
}

