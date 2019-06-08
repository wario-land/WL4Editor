#include "Tileset.h"
#include "ROMUtils.h"

#include <iostream>

#include <QPixmap>

namespace LevelComponents
{
    /// <summary>
    /// Construct an instance of Tileset.
    /// </summary>
    /// <remarks>
    /// Mapping type is a parameter because that information is not contained in the layer data itself.
    /// </remarks>
    /// <param name="tilesetPtr">
    /// Pointer to the beginning of the tileset data.
    /// </param>
    /// <param name="__TilesetID">
    /// The index for the tileset in the ROM.
    /// </param>
    Tileset::Tileset(int tilesetPtr, int __TilesetID)
    {
        memset(tile8x8data, 0, sizeof(tile8x8data) / sizeof(tile8x8data[0]));

        // Create all 16 color palettes
        int palettePtr = ROMUtils::PointerFromData(tilesetPtr + 8);
        for (int i = 0; i < 16; ++i)
        {
            // First color is transparent
            palettes[i].push_back(0);
            int subPalettePtr = palettePtr + i * 32;
            for (int j = 1; j < 16; ++j)
            {
                unsigned short color555 = *(unsigned short *) (ROMUtils::CurrentFile + subPalettePtr + j * 2);
                int r = ((color555 << 3) & 0xF8) | ((color555 >> 2) & 3);
                int g = ((color555 >> 2) & 0xF8) | ((color555 >> 7) & 3);
                int b = ((color555 >> 7) & 0xF8) | ((color555 >> 13) & 3);
                int a = 0xFF;
                palettes[i].push_back(QColor(r, g, b, a).rgba());
            }
        }

        // Initialize the 8x8 tiles by setting all the tiles to blank tiles
        blankTile = Tile8x8::CreateBlankTile(palettes);
        int tile8x8size = sizeof(tile8x8data) / sizeof(tile8x8data[0]);
        for (int i = 0; i < tile8x8size; ++i)
        {
            tile8x8data[i] = blankTile;
        }

        // Load the animated tiles
        int tmpAnimatedTilesHeaderPtr;
        int tmpoffset;
        int tmpAnimatedTilesdataPtr;
        for (int v1 = 0; v1 < 16; ++v1)
        {
            /*
             * the reason why not using this is that it will cause problem and worse in some situation,
             * so before we know what the first case really used for, we should just use the simple one.
             * TODO: find the usage of the first case
            //[0300002E..03000032] are all set to zero at 6B8FA and the arrange just contains all the values the table
            start from 0x3F8C18 have if(ROMUtils::CurrentFile[0x3F8C18 + __TilesetID * 16 + v1] & 1)
                tmpAnimatedTilesHeaderPtr = 0x3F7828 + (int) (8 * (*(unsigned short*) (ROMUtils::CurrentFile +
            __TilesetID * 32 + 2 * v1 + 0x3F91D8))); else tmpAnimatedTilesHeaderPtr = 0x3F7828 + (int) (8 * (*(unsigned
            short*) (ROMUtils::CurrentFile + __TilesetID * 32 + 2 * v1 + 0x3F8098)));
            */
            tmpAnimatedTilesHeaderPtr =
                0x3F7828 +
                (int) (8 * (*(unsigned short *) (ROMUtils::CurrentFile + __TilesetID * 32 + 2 * v1 + 0x3F8098)));
            tmpAnimatedTilesdataPtr = ROMUtils::PointerFromData(tmpAnimatedTilesHeaderPtr + 4);
            tmpoffset = (int) ROMUtils::CurrentFile[tmpAnimatedTilesHeaderPtr + 2];
            if ((ROMUtils::CurrentFile[tmpAnimatedTilesHeaderPtr] == '\x03') ||
                (ROMUtils::CurrentFile[tmpAnimatedTilesHeaderPtr] == '\x06'))
            {
                tmpoffset -= 1;
            }
            else
            {
                tmpoffset = 0;
            }
            tmpoffset *= 128;
            for (int i = 0; i < 4; ++i)
            {
                tile8x8data[i + 4 * v1] = new Tile8x8(tmpAnimatedTilesdataPtr + tmpoffset + i * 32, palettes);
            }
        }

        // Load the 8x8 tile graphics
        int fgGFXptr = ROMUtils::PointerFromData(tilesetPtr);
        int fgGFXlen = ROMUtils::IntFromData(tilesetPtr + 4);
        int bgGFXptr = ROMUtils::PointerFromData(tilesetPtr + 12);
        int bgGFXlen = ROMUtils::IntFromData(tilesetPtr + 16);

        // Foreground
        int fgGFXcount = fgGFXlen / 32;
        for (int i = 0; i < fgGFXcount; ++i)
        {
            tile8x8data[i + 0x41] = new Tile8x8(fgGFXptr + i * 32, palettes);
        }

        // Background
        int bgGFXcount = bgGFXlen / 32;
        for (int i = 0; i < bgGFXcount; ++i)
        {
            tile8x8data[tile8x8size - 1 - bgGFXcount + i] = new Tile8x8(bgGFXptr + i * 32, palettes);
        }

        // Load the map16 data
        int map16ptr = ROMUtils::PointerFromData(tilesetPtr + 0x14);
        int map16size = sizeof(map16data) / sizeof(map16data[0]);
        for (int i = 0; i < map16size; ++i)
        {
            unsigned short *map16tilePtr = (unsigned short *) (ROMUtils::CurrentFile + map16ptr + i * 8);
            Tile8x8 *tiles[4];
            for (int j = 0; j < 4; ++j)
            {
                int index = map16tilePtr[j] & 0x3FF;
                bool FlipX = (map16tilePtr[j] & (1 << 10)) != 0;
                bool FlipY = (map16tilePtr[j] & (1 << 11)) != 0;
                int paletteIndex = map16tilePtr[j] >> 12;
                tiles[j] = new Tile8x8(tile8x8data[index]);
                tiles[j]->SetFlipX(FlipX);
                tiles[j]->SetFlipY(FlipY);
                tiles[j]->SetPaletteIndex(paletteIndex);
            }
            map16data[i] = new TileMap16(tiles[0], tiles[1], tiles[2], tiles[3]);
        }

        // Get pointer to the map16 event table
        Map16EventTable = (unsigned short *) (ROMUtils::CurrentFile + ROMUtils::PointerFromData(tilesetPtr + 28));

        // Get pointer to the Map16 Wario Animation Slot ID Table
        Map16TerrainTypeIDTable =
            (unsigned char *) (ROMUtils::CurrentFile + ROMUtils::PointerFromData(tilesetPtr + 24));

        // Get pointer of Universal Sprites tiles Palette
        UniversalSpritesTilesPalettePtr = ROMUtils::PointerFromData(tilesetPtr + 32);
    }

    /// <summary>
    /// Deconstruct the Tileset and clean up its instance objects on the heap.
    /// </summary>
    Tileset::~Tileset()
    {
        // Deconstruct tile8x8 data
        for (unsigned int i = 0; i < sizeof(tile8x8data) / sizeof(tile8x8data[0]); ++i)
        {
            // The blank tile entry must be deleted separately
            if (tile8x8data[i] != blankTile)
            {
                delete tile8x8data[i];
            }
        }
        delete blankTile;
        for (unsigned int i = 0; i < sizeof(map16data) / sizeof(map16data[0]); ++i)
        {
            delete map16data[i];
        }

        for (unsigned int i = 0; i < 16; ++i)
        {
            palettes[i].clear();
        }
    }

    /// <summary>
    /// Render the tileset as a pixmap.
    /// </summary>
    /// <param name="columns">
    /// The number of columns to divide the graphics into.
    /// </param>
    /// <returns>
    /// The tileset rendered at a pixmap.
    /// </returns>
    QPixmap Tileset::Render(int columns)
    {
        // Initialize the pixmap with transparency
        int tileCountY = 96 / columns;
        QPixmap pixmap(8 * 16 * columns, 16 * tileCountY);
        pixmap.fill(Qt::transparent);

        // Iterate by 8-tile wide column, then row, then tile horizontally within column
        for (int c = 0; c < columns; ++c)
        {
            for (int i = 0; i < tileCountY; ++i)
            {
                for (int j = 0; j < 8; ++j)
                {
                    map16data[(c * tileCountY + i) * 8 + j]->DrawTile(&pixmap, (c * 8 + j) * 16, i * 16);
                }
            }
        }
        return pixmap;
    }
} // namespace LevelComponents
