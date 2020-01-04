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
        memset(tile8x8array, 0, Tile8x8DefaultNum * sizeof(tile8x8array[0]));
        memset(map16array, 0, Tile16DefaultNum * sizeof(map16array[0]));

        // Create all 16 color palettes
        paletteAddress = ROMUtils::PointerFromData(tilesetPtr + 8);
        for (int i = 0; i < 16; ++i)
        {
            int subPalettePtr = paletteAddress + i * 32;
            unsigned short *tmpptr = (unsigned short*) (ROMUtils::CurrentFile + subPalettePtr);
            ROMUtils::LoadPalette(&palettes[i], tmpptr);
        }

        // Initialize the 8x8 tiles by setting all the tiles to blank tiles
        blankTile = Tile8x8::CreateBlankTile(palettes);
        for (int i = 0; i < Tile8x8DefaultNum; ++i)
        {
            tile8x8array[i] = blankTile;
        }

        // Load the animated tiles
        /*
         * the reason why not using this is that it will cause problem and worse in some situation,
         * so before we know what the first case really used for, we should just use the simple one.
         * TODO: find the usage of the first case
         * [0300002E..03000032] are all set to zero at 6B8FA
         * and the arrange just contains all the values the table start from 0x3F8C18 have
         * if(ROMUtils::CurrentFile[0x3F8C18 + __TilesetID * 16 + v1] & 1)
              tmpAnimatedTilesHeaderPtr = 0x3F7828 + (int) (8 * (*(unsigned short*) (ROMUtils::CurrentFile +
                                                                                        __TilesetID * 32 + 2 * v1 + 0x3F91D8)));
         * else
         * tmpAnimatedTilesHeaderPtr = 0x3F7828 + (int) (8 * (*(unsigned short*) (ROMUtils::CurrentFile +
                                                                                        __TilesetID * 32 + 2 * v1 + 0x3F8098)));
         */
        AnimatedTileData = new unsigned short[16];
        memcpy((unsigned char *)AnimatedTileData, ROMUtils::CurrentFile + __TilesetID * 32 + WL4Constants::AnimatedTileIdTableCase2, 32);
        for (int v1 = 0; v1 < 16; ++v1)
        {
            SetAnimatedTile(AnimatedTileData[v1], 4 * v1);
        }

        // Load the 8x8 tile graphics
        fgGFXptr = ROMUtils::PointerFromData(tilesetPtr);
        fgGFXlen = ROMUtils::IntFromData(tilesetPtr + 4);
        bgGFXptr = ROMUtils::PointerFromData(tilesetPtr + 12);
        bgGFXlen = ROMUtils::IntFromData(tilesetPtr + 16);

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

        // Get pointer to the map16 event table
        Map16EventTable = new unsigned short[0x300];
        memcpy(Map16EventTable, (unsigned short *) (ROMUtils::CurrentFile + ROMUtils::PointerFromData(tilesetPtr + 28)), Tile16DefaultNum * sizeof(unsigned short));

        // Get pointer to the Map16 Wario Animation Slot ID Table
        Map16TerrainTypeIDTable = new unsigned char[0x300];
        memcpy(Map16TerrainTypeIDTable, (unsigned char *) (ROMUtils::CurrentFile + ROMUtils::PointerFromData(tilesetPtr + 24)), Tile16DefaultNum * sizeof(unsigned char));

        // Get pointer of Universal Sprites tiles Palette
        TilesetPaletteData = new unsigned short[16 * 16];
        memcpy(TilesetPaletteData, (unsigned short *) (ROMUtils::CurrentFile + ROMUtils::PointerFromData(tilesetPtr + 8)), 16 * 16 * sizeof(unsigned short));

        hasconstructed = true;
    }

    /// <summary>
    /// Copy constructor of Tileset.
    /// </summary>
    /// <remarks>
    /// the new instance should only be used when editing Tileset, it should be delete after this period.
    /// </remarks>
    Tileset::Tileset(Tileset *old_tileset, int __TilesetID) :
        paletteAddress(old_tileset->paletteAddress),
        fgGFXptr(old_tileset->fgGFXptr),
        fgGFXlen(old_tileset->fgGFXlen),
        bgGFXptr(old_tileset->bgGFXptr),
        bgGFXlen(old_tileset->bgGFXlen),
        map16ptr(old_tileset->map16ptr)
    {
        newtileset = true;
        tile8x8array = new Tile8x8* [0x600];
        map16array = new TileMap16* [0x300];

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

        // Get Animated Tile Data
        AnimatedTileData = new unsigned short[16];
        memcpy(AnimatedTileData, old_tileset->GetAnimatedTileData(), 32 * sizeof(unsigned char));

        hasconstructed = true;
    }

    /// <summary>
    /// Set animated tile8x8 group.
    /// </summary>
    /// <param name="tile8x8groupId">
    /// animeated tile8x8 group id.
    /// </param>
    /// <param name="startTile8x8Id">
    /// the first tile8x8 id in Tileset you want to render the animated tile group.
    /// </param>
    void Tileset::SetAnimatedTile(int tile8x8groupId, int startTile8x8Id)
    {
        AnimatedTileData[startTile8x8Id / 4] = tile8x8groupId;
        int tmpAnimatedTilesHeaderPtr = 0x3F7828 + 8 * tile8x8groupId;
        int tmpAnimatedTilesdataPtr = ROMUtils::PointerFromData(tmpAnimatedTilesHeaderPtr + 4);
        int tmpoffset = (int) ROMUtils::CurrentFile[tmpAnimatedTilesHeaderPtr + 2];
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
            if(tile8x8array[i + startTile8x8Id] != blankTile)
                delete tile8x8array[i + startTile8x8Id];
            tile8x8array[i + startTile8x8Id] = new Tile8x8(tmpAnimatedTilesdataPtr + tmpoffset + i * 32, palettes);
        }

        // Update TIle16 data
        if(hasconstructed)
        {
            for (int i = 0; i < Tile16DefaultNum; ++i)
            {
                Tile8x8 *tiles[4];
                for (int j = 0; j < 4; ++j)
                {
                    Tile8x8 *tile = map16array[i]->GetTile8X8(j);
                    if((tile->GetIndex() >> 2) == (startTile8x8Id >> 2))
                    {
                        int index = tile->GetIndex() & 0x3FF;
                        bool FlipX = tile->GetFlipX();
                        bool FlipY = tile->GetFlipY();
                        int paletteIndex = tile->GetPaletteIndex();
                        map16array[i]->ResetTile8x8(tile8x8array[index], j, index, paletteIndex, FlipY, FlipX);
                    }
                }
            }
        }
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

        delete Map16EventTable;
        delete Map16TerrainTypeIDTable;
        delete TilesetPaletteData;
        delete AnimatedTileData;
    }

    /// <summary>
    /// Render the tileset by Tile8 as a pixmap.
    /// </summary>
    QPixmap Tileset::RenderAllTile8x8(int paletteId)
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
    QPixmap Tileset::RenderAllTile16(int columns)
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
    /// Render a Tile8x8 to a pixmap.
    /// </summary>
    /// <param name="tileId">
    /// the Id of the Tile8x8 needs to render.
    /// </param>
    /// <param name="paletteId">
    /// the id of palette to render Tile8x8.
    /// </param>
    /// <returns>
    /// The Tile8x8 rendered at a pixmap.
    /// </returns>
    QPixmap Tileset::RenderTile8x8(int tileId, int paletteId)
    {
        QPixmap pixmap(8, 8);
        pixmap.fill(Qt::transparent);

        // drawing
        tile8x8array[tileId]->SetPaletteIndex(paletteId);
        tile8x8array[tileId]->DrawTile(&pixmap, 0, 0);

        return pixmap;
    }

    /// <summary>
    /// Regenerate Palette data when saving ROM.
    /// </summary>
    void Tileset::ReGeneratePaletteData()
    {
        QColor tmp_color;
        memset(TilesetPaletteData, 0, 16 * 16 * 2);
        for(int i = 0; i < 16; ++i)
        {
            // First color is transparent
            // RGB555 format: bbbbbgggggrrrrr
            for(int j = 1; j < 16; ++j)
            {
                tmp_color.setRgb(palettes[i][j]);
                int b = (tmp_color.blue() >> 3) & 0x1F;
                int g = (tmp_color.green() >> 3) & 0x1F;
                int r = (tmp_color.red() >> 3) & 0x1F;
                TilesetPaletteData[16 * i + j] = (unsigned short) ((b << 10) | (g << 5) | r);
            }
        }
    }
} // namespace LevelComponents
