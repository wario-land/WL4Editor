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
        //Save the ROM pointer into the tileset object
        this->tilesetPtr=tilesetPtr;

        tile8x8array = new Tile8x8* [0x600];
        map16array = new TileMap16* [0x300];
        statictile8x8data = new unsigned char[(1024 - 64 - 2) * 32];
        tile16data = new unsigned char[0x300 * 8];
        memset(tile8x8array, 0, Tile8x8DefaultNum * sizeof(tile8x8array[0]));
        memset(map16array, 0, Tile16DefaultNum * sizeof(map16array[0]));
        memset(statictile8x8data, 0, (1024 - 64 - 2) * 32);
        memset(tile16data, 0, Tile16DefaultNum * 8);

        // Create all 16 color palettes
        palettePtr = ROMUtils::PointerFromData(tilesetPtr + 8);
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
        for (int i = 0; i < Tile8x8DefaultNum; ++i)
        {
            tile8x8array[i] = blankTile;
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
                tile8x8array[i + 4 * v1] = new Tile8x8(tmpAnimatedTilesdataPtr + tmpoffset + i * 32, palettes);
            }
        }

        // Load the 8x8 tile graphics
        fgGFXptr = ROMUtils::PointerFromData(tilesetPtr);
        fgGFXlen = ROMUtils::IntFromData(tilesetPtr + 4);
        bgGFXptr = ROMUtils::PointerFromData(tilesetPtr + 12);
        bgGFXlen = ROMUtils::IntFromData(tilesetPtr + 16);

        memcpy(statictile8x8data, ROMUtils::CurrentFile + fgGFXptr, fgGFXlen);
        memcpy(statictile8x8data + (1023 - 64 - 2) * 32 - bgGFXlen, ROMUtils::CurrentFile + bgGFXptr, bgGFXlen);

        // Foreground
        int fgGFXcount = fgGFXlen / 32;
        for (int i = 0; i < fgGFXcount; ++i)
        {
            tile8x8array[i + 0x41] = new Tile8x8(fgGFXptr + i * 32, palettes);
        }

        // Background
        int bgGFXcount = bgGFXlen / 32;
        for (int i = 0; i < bgGFXcount; ++i)
        {
            tile8x8array[Tile8x8DefaultNum - 1 - bgGFXcount + i] = new Tile8x8(bgGFXptr + i * 32, palettes);
        }

        // Load the map16 data
        map16ptr = ROMUtils::PointerFromData(tilesetPtr + 0x14);
        for (int i = 0; i < Tile16DefaultNum; ++i)
        {
            unsigned short *map16tilePtr = (unsigned short *) (ROMUtils::CurrentFile + map16ptr + i * 8);
            Tile8x8 *tiles[4];
            for (int j = 0; j < 4; ++j)
            {
                int index = map16tilePtr[j] & 0x3FF;
                bool FlipX = (map16tilePtr[j] & (1 << 10)) != 0;
                bool FlipY = (map16tilePtr[j] & (1 << 11)) != 0;
                int paletteIndex = map16tilePtr[j] >> 12;
                tiles[j] = new Tile8x8(tile8x8array[index]);
                tiles[j]->SetIndex(index);
                tiles[j]->SetFlipX(FlipX);
                tiles[j]->SetFlipY(FlipY);
                tiles[j]->SetPaletteIndex(paletteIndex);
            }
            map16array[i] = new TileMap16(tiles[0], tiles[1], tiles[2], tiles[3]);
        }

        memcpy(tile16data, ROMUtils::CurrentFile + map16ptr, Tile16DefaultNum * 8);

        // Get pointer to the map16 event table
        Map16EventTable = new unsigned short[0x300];
        memcpy(Map16EventTable, (unsigned short *) (ROMUtils::CurrentFile + ROMUtils::PointerFromData(tilesetPtr + 28)), Tile16DefaultNum * sizeof(unsigned short));

        // Get pointer to the Map16 Wario Animation Slot ID Table
        Map16TerrainTypeIDTable = new unsigned char[0x300];
        memcpy(Map16TerrainTypeIDTable, (unsigned char *) (ROMUtils::CurrentFile + ROMUtils::PointerFromData(tilesetPtr + 24)), Tile16DefaultNum * sizeof(unsigned char));

        // Get pointer of Universal Sprites tiles Palette
        TilesetPaletteData = new unsigned short[16 * 16];
        memcpy(TilesetPaletteData, (unsigned short *) (ROMUtils::CurrentFile + ROMUtils::PointerFromData(tilesetPtr + 32)), 16 * 16 * sizeof(unsigned short));
    }

    /// <summary>
    /// Copy constructor of Tileset.
    /// </summary>
    /// <remarks>
    /// the new instance should only be used when editing Tileset, it should be delete after this period.
    /// </remarks>
    Tileset::Tileset(Tileset *old_tileset, int __TilesetID) :
        palettePtr(old_tileset->palettePtr),
        fgGFXptr(old_tileset->fgGFXptr),
        fgGFXlen(old_tileset->fgGFXlen),
        bgGFXptr(old_tileset->bgGFXptr),
        bgGFXlen(old_tileset->bgGFXlen),
        map16ptr(old_tileset->map16ptr)
    {
        newtileset = true;
        tile8x8array = new Tile8x8* [0x600];
        map16array = new TileMap16* [0x300];
        statictile8x8data = new unsigned char[(1024 - 64 - 2) * 32];
        tile16data = new unsigned char[0x300 * 8];
        memset(statictile8x8data, 0, (1024 - 64 - 2) * 32);
        memset(tile16data, 0, Tile16DefaultNum * 8);
        memcpy(statictile8x8data, old_tileset->statictile8x8data, (1024 - 64 - 2) * 32);
        memcpy(tile16data, old_tileset->tile16data, Tile16DefaultNum * 8);

        //Save the ROM pointer into the tileset object
        this->tilesetPtr = old_tileset->getTilesetPtr();

        memset(tile8x8array, 0, Tile8x8DefaultNum * sizeof(tile8x8array[0]));
        memset(map16array, 0, Tile16DefaultNum * sizeof(map16array[0]));

        // Create all 16 color palettes
        for (unsigned int i = 0; i < 16; ++i)
        {
            palettes[i] = old_tileset->GetPalettes()[i];
        }

        // Copy all the Tile8x8
        Tile8x8 **oldTile8x8Data = old_tileset->GetTile8x8arrayPtr();
        for (unsigned int i = 0; i < Tile8x8DefaultNum; ++i)
        {
            tile8x8array[i] = new Tile8x8(oldTile8x8Data[i], palettes);
        }

        // Copy all the Tile16
        TileMap16 **oldTileMap16Data = old_tileset->GetMap16arrayPtr();
        for (unsigned int i = 0; i < Tile16DefaultNum; ++i)
        {
            map16array[i] = new TileMap16(oldTileMap16Data[i], palettes);
        }

        // Get pointer to the map16 event table
        Map16EventTable = new unsigned short[0x300];
        memcpy(Map16EventTable, old_tileset->GetEventTablePtr(), Tile16DefaultNum * sizeof(unsigned short));

        // Get pointer to the Map16 Wario Animation Slot ID Table
        Map16TerrainTypeIDTable = new unsigned char[0x300];
        memcpy(Map16TerrainTypeIDTable, old_tileset->GetTerrainTypeIDTablePtr(), Tile16DefaultNum * sizeof(unsigned char));

        // Get pointer of Universal Sprites tiles Palette
        TilesetPaletteData = new unsigned short[16 * 16];
        memcpy(TilesetPaletteData, old_tileset->GetTilesetPaletteDataPtr(), 16 * 16 * sizeof(unsigned short));
    }

    /// <summary>
    /// Deconstruct the Tileset and clean up its instance objects on the heap.
    /// </summary>
    Tileset::~Tileset()
    {
        // Deconstruct tile8x8 data
        for (unsigned int i = 0; i < Tile8x8DefaultNum; ++i)
        {
            // The blank tile entry must be deleted separately
            if (tile8x8array[i] != blankTile)
            {
                delete tile8x8array[i];
            }
        }
        delete blankTile;
        for (unsigned int i = 0; i < Tile16DefaultNum; ++i)
        {
            delete map16array[i];
        }

        for (unsigned int i = 0; i < 16; ++i)
        {
            palettes[i].clear();
        }

        delete statictile8x8data;
        delete tile16data;
        delete Map16EventTable;
        delete Map16TerrainTypeIDTable;
        delete TilesetPaletteData;
    }

    /// <summary>
    /// Render the tileset by Tile8 as a pixmap.
    /// </summary>
    QPixmap Tileset::RenderTile8x8(int paletteId)
    {
        int lineNum = 0x600 / 16;
        QPixmap pixmap(8 * 16, 8 * lineNum);
        pixmap.fill(Qt::transparent);

        // drawing
        for (int i = 0; i < lineNum; ++i)
        {
            for (int j = 0; j < 16; ++j)
            {
                tile8x8array[i * 16 + j]->SetPaletteIndex(paletteId);
                tile8x8array[i * 16 + j]->DrawTile(&pixmap, j * 8, i * 8);
            }
        }
        return pixmap;
    }

    /// <summary>
    /// Render the tileset by Tile16 as a pixmap.
    /// </summary>
    /// <param name="columns">
    /// The number of columns to divide the graphics into.
    /// </param>
    /// <returns>
    /// The tileset rendered at a pixmap.
    /// </returns>
    QPixmap Tileset::RenderTile16(int columns)
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
                    map16array[(c * tileCountY + i) * 8 + j]->DrawTile(&pixmap, (c * 8 + j) * 16, i * 16);
                }
            }
        }
        return pixmap;
    }

    /// <summary>
    /// Reset a tile8x8 mapping data in tile16data.
    /// </summary>
    /// <param name="tile16Id">
    /// Id of tile16 need to change.
    /// </param>
    /// <param name="position">
    /// tile8x8 position in Tile16, from 1 to 3.
    /// </param>
    /// <param name="newtile8x8Id">
    /// Id of new set tile8x8.
    /// </param>
    /// <param name="xflip">
    /// set xflip.
    /// </param>
    /// <param name="yflip">
    /// set yflip.
    /// </param>
    /// <param name="paletteId">
    /// set tile8x8's palette.
    /// </param>
    void Tileset::ResetATile8x8MapDataInTile16Data(int tile16Id, int position, int newtile8x8Id, bool xflip, bool yflip, int paletteId)
    {
        unsigned short tile8x8id = newtile8x8Id & 0x3FF;
        unsigned short *map16tilecurrentAddr = (unsigned short *)tile16data + tile16Id * 4 + position;
        *map16tilecurrentAddr = (unsigned short)(tile8x8id + ((xflip? 1: 0) << 10) + ((yflip? 1: 0) << 11) + ((paletteId & 15) << 12));
    }
} // namespace LevelComponents
