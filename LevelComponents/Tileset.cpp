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
    /// <param name="IsloadFromTmpROM">
    /// Ture when load from a temp ROM.
    /// </param>
    Tileset::Tileset(int tilesetPtr, int __TilesetID, bool IsloadFromTmpROM)
    {
        //Save the ROM pointer into the tileset object
        this->tilesetPtr = tilesetPtr;
        unsigned char *curFilePtr = ROMUtils::ROMFileMetaData->ROMDataPtr;

        // Create all 16 color palettes
        paletteAddress = ROMUtils::PointerFromData(tilesetPtr + 8);
        for (int i = 0; i < 16; ++i)
        {
            int subPalettePtr = paletteAddress + i * 32;
            unsigned short *tmpptr = (unsigned short*) (curFilePtr + subPalettePtr);
            ROMUtils::LoadPalette(&palettes[i], tmpptr);
        }

        // Initialize the 8x8 tiles by setting all the tiles to blank tiles
        blankTile = Tile8x8::CreateBlankTile(palettes);
        for (int i = 0; i < Tile8x8DefaultNum; ++i)
        {
            tile8x8array.push_back(blankTile);
        }

        // Load the animated tiles
        /*
         * bgAnimated_dat2_pack = 0x3F8C18
         * bgAnimated_dat3_pack = 0x3F91D8
         * bgAnimated_dat1_pack = 0x3F8098
         * BgIntAnimated_PackNum_tbl = 0x3F7828
         * if ( MapSw[bgAnimated_dat2_pack[16 * CurrentRoomHeader_TilesetId + v1]] & 1 ) // only when some switch flag is set to 1
               AnimatedTilesHeaderPtr = (char *)&bgAnimated_dat3_pack + 32 * CurrentRoomHeader_TilesetId + v2;
         * else // All the MapSw(MapSwitch) are all set to zero at rom:6B8FA, this is the case when loading Level
               AnimatedTilesHeaderPtr = (char *)&bgAnimated_dat1_pack + 32 * CurrentRoomHeader_TilesetId + v2;
         */
        AnimatedTileData[0] = new unsigned short[16];
        memcpy((unsigned char *)AnimatedTileData[0], curFilePtr + __TilesetID * 32 + WL4Constants::AnimatedTileIdTableSwitchOff, 32);
        AnimatedTileSwitchTable = new unsigned char[16];
        memcpy(AnimatedTileSwitchTable, curFilePtr + __TilesetID * 16 + WL4Constants::AnimatedTileSwitchInfoTable, 16);
        AnimatedTileData[1] = new unsigned short[16];
        memcpy((unsigned char *)AnimatedTileData[1], curFilePtr + __TilesetID * 32 + WL4Constants::AnimatedTileIdTableSwitchOn, 32);
        for (int v1 = 0; v1 < 16; ++v1)
        {
            SetAnimatedTile(AnimatedTileData[0][v1], AnimatedTileData[1][v1], AnimatedTileSwitchTable[v1], 4 * v1);
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
            unsigned short *map16tilePtr = (unsigned short *) (curFilePtr + map16ptr + i * 8);
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
            map16array.push_back(new TileMap16(tiles[0], tiles[1], tiles[2], tiles[3]));
        }

        // Get pointer to the map16 event table
        Map16EventTable = new unsigned short[Tile16DefaultNum];
        memcpy(Map16EventTable, (unsigned short *) (curFilePtr + ROMUtils::PointerFromData(tilesetPtr + 28)), Tile16DefaultNum * sizeof(unsigned short));

        // Get pointer to the Map16 Wario Animation Slot ID Table
        Map16TerrainTypeIDTable = new unsigned char[Tile16DefaultNum];
        memcpy(Map16TerrainTypeIDTable, (unsigned char *) (curFilePtr + ROMUtils::PointerFromData(tilesetPtr + 24)), Tile16DefaultNum * sizeof(unsigned char));

        // Get pointer of Universal Sprites tiles Palette
        TilesetPaletteData = new unsigned short[16 * 16];
        memcpy(TilesetPaletteData, (unsigned short *) (curFilePtr + ROMUtils::PointerFromData(tilesetPtr + 8)), 16 * 16 * sizeof(unsigned short));

        // Reset pointer variables if the data comes from a temp ROM
        if (IsloadFromTmpROM)
        {
            this->fgGFXptr = 0;
            this->map16ptr = 0;
            this->paletteAddress = 0;
        }
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
        (void) __TilesetID;
        newtileset = true;

        //Save the ROM pointer into the tileset object
        this->tilesetPtr = old_tileset->getTilesetPtr();

        // Create all 16 color palettes
        for (unsigned int i = 0; i < 16; ++i)
        {
            palettes[i] = old_tileset->GetPalettes()[i];
        }

        // Initialize the 8x8 tiles by setting all the tiles to blank tiles
        blankTile = Tile8x8::CreateBlankTile(palettes);
        for (int i = 0; i < Tile8x8DefaultNum; ++i)
        {
            tile8x8array.push_back(blankTile);
        }

        // Get pointer to the map16 event table
        Map16EventTable = new unsigned short[Tile16DefaultNum];
        memcpy(Map16EventTable, old_tileset->GetEventTablePtr(), Tile16DefaultNum * sizeof(unsigned short));

        // Get pointer to the Map16 Wario Animation Slot ID Table
        Map16TerrainTypeIDTable = new unsigned char[Tile16DefaultNum];
        memcpy(Map16TerrainTypeIDTable, old_tileset->GetTerrainTypeIDTablePtr(), Tile16DefaultNum * sizeof(unsigned char));

        // Get pointer of Universal Sprites tiles Palette
        TilesetPaletteData = new unsigned short[16 * 16];
        memcpy(TilesetPaletteData, old_tileset->GetTilesetPaletteDataPtr(), 16 * 16 * sizeof(unsigned short));

        // Get Animated Tile Data
        AnimatedTileData[0] = new unsigned short[16];
        memcpy(AnimatedTileData[0], old_tileset->GetAnimatedTileData(0), 32 * sizeof(unsigned char));
        AnimatedTileData[1] = new unsigned short[16];
        memcpy(AnimatedTileData[1], old_tileset->GetAnimatedTileData(1), 32 * sizeof(unsigned char));
        AnimatedTileSwitchTable = new unsigned char[16];
        memcpy(AnimatedTileSwitchTable, old_tileset->GetAnimatedTileSwitchTable(), 16 * sizeof(unsigned char));

        // Copy all the Tile8x8
        for (int v1 = 0; v1 < 16; ++v1)
        {
            SetAnimatedTile(AnimatedTileData[0][v1], AnimatedTileData[1][v1], AnimatedTileSwitchTable[v1], 4 * v1);
        }
        QVector<Tile8x8 *> oldTile8x8Data = old_tileset->GetTile8x8arrayPtr();
        int fgGFXcount = fgGFXlen / 32;
        for (int i = 0; i < fgGFXcount; ++i)
        {
            tile8x8array[i + 0x41] = new Tile8x8(oldTile8x8Data[i + 0x41], palettes);
        }
        int bgGFXcount = bgGFXlen / 32;
        for (int i = 0; i < bgGFXcount; ++i)
        {
            tile8x8array[Tile8x8DefaultNum - 1 - bgGFXcount + i] = new Tile8x8(oldTile8x8Data[Tile8x8DefaultNum - 1 - bgGFXcount + i], palettes);
        }

        // Copy all the Tile16
        QVector<TileMap16 *> oldTileMap16Data = old_tileset->GetMap16arrayPtr();
        for (int i = 0; i < Tile16DefaultNum; ++i)
        {
            Tile8x8 *tiles[4];
            for (int j = 0; j < 4; ++j)
            {
                int index = oldTileMap16Data[i]->GetTile8X8(j)->GetIndex();
                tiles[j] = new Tile8x8(tile8x8array[index]);
                tiles[j]->SetIndex(index);
                tiles[j]->SetFlipX(oldTileMap16Data[i]->GetTile8X8(j)->GetFlipX());
                tiles[j]->SetFlipY(oldTileMap16Data[i]->GetTile8X8(j)->GetFlipY());
                tiles[j]->SetPaletteIndex(oldTileMap16Data[i]->GetTile8X8(j)->GetPaletteIndex());
            }
            map16array.push_back(new TileMap16(tiles[0], tiles[1], tiles[2], tiles[3]));
        }

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
    void Tileset::SetAnimatedTile(int tile8x8groupId, int tile8x8group2Id, int SwitchId, int startTile8x8Id)
    {
        // TODO: Only save graphics from animated tile table 0 for now, need to load tiles from table 1 in the future. --ssp
        AnimatedTileData[0][startTile8x8Id >> 2] = tile8x8groupId;
        AnimatedTileData[1][startTile8x8Id >> 2] = tile8x8group2Id;
        AnimatedTileSwitchTable[startTile8x8Id >> 2] = SwitchId;
        int tmpAnimatedTilesHeaderPtr = 0x3F7828 + 8 * tile8x8groupId;
        int tmpAnimatedTilesdataPtr = ROMUtils::PointerFromData(tmpAnimatedTilesHeaderPtr + 4);
        int tmpoffset = (int) ROMUtils::ROMFileMetaData->ROMDataPtr[tmpAnimatedTilesHeaderPtr + 2];
        if ((ROMUtils::ROMFileMetaData->ROMDataPtr[tmpAnimatedTilesHeaderPtr] == '\x03') ||
            (ROMUtils::ROMFileMetaData->ROMDataPtr[tmpAnimatedTilesHeaderPtr] == '\x06'))
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

        // Update Tile16 data
        if(hasconstructed)
        {
            for (int i = 0; i < Tile16DefaultNum; ++i)
            {
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

        delete[] Map16EventTable;
        delete[] Map16TerrainTypeIDTable;
        delete[] TilesetPaletteData;
        delete[] AnimatedTileData[0];
        delete[] AnimatedTileData[1];
        delete[] AnimatedTileSwitchTable;
    }

    /// <summary>
    /// Render the tileset by Tile8 as a pixmap.
    /// </summary>
    QPixmap Tileset::RenderAllTile8x8(int paletteId)
    {
        int lineNum = Tile8x8DefaultNum / 16;
        QPixmap pixmap(8 * 16, 8 * lineNum);
        pixmap.fill(Qt::transparent);

        // drawing
        for (int i = 0; i < lineNum; ++i)
        {
            for (int j = 0; j < 16; ++j)
            {
                if (tile8x8array[i * 16 + j] == blankTile) continue;
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

    /// <summary>
    /// Delete a Tile8x8 from the Tileset and reset the Tile16 map.
    /// </summary>
    /// <param name="tile8x8Id">
    /// the Id of the Tile8x8 needs to be deleted.
    /// </param>
    void Tileset::DelTile8x8(int tile8x8Id)
    {
        LevelComponents::Tile8x8* tile = tile8x8array[tile8x8Id];
        if(tile == blankTile)
            return;
        delete tile;
        for(int i = tile8x8Id; i < (0x40 + fgGFXlen / 32); i++)
        {
            tile8x8array[i] = tile8x8array[i + 1];
        }
        tile8x8array[0x40 + fgGFXlen / 32] = blankTile;
        fgGFXlen -= 32;

        // update Tile16 map
        for(int i = 0; i < Tile16DefaultNum; ++i)
        {
            LevelComponents::TileMap16* tile16 = map16array[i];
            for(int j = 0; j < 4; ++j)
            {
                LevelComponents::Tile8x8* tmptile = tile16->GetTile8X8(j);
                int oldid = tmptile->GetIndex();
                int pal = tmptile->GetPaletteIndex();
                bool xflip = tmptile->GetFlipX();
                bool yflip = tmptile->GetFlipY();
                if(oldid > tile8x8Id)
                {
                    tile16->ResetTile8x8(tile8x8array[oldid - 1], j, oldid - 1, pal, xflip, yflip);
                }
                else if(oldid == tile8x8Id)
                {
                    tile16->ResetTile8x8(tile8x8array[0x40], j, 0x40, 0, false, false);
                }
            }
        }
    }
} // namespace LevelComponents
