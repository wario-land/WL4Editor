#include "Tileset.h"
#include "ROMUtils.h"

namespace LevelComponents
{
    Tileset::Tileset(int tilesetPtr)
    {
        // Create all 16 color palettes
        int palettePtr = ROMUtils::PointerFromData(ROMUtils::CurrentFile, tilesetPtr + 8);
        for(int i = 0; i < 16; ++i)
        {
            // First color is transparent
            palettes[i].push_back(0);
            int subPalettePtr = palettePtr + i * 32;
            for(int j = 1; j < 16; ++j)
            {
                unsigned short color555 = *(unsigned short*) (ROMUtils::CurrentFile + subPalettePtr + j * 2);
                int r = (color555 & 0x1F) << 3;
                int g = (color555 & 0x3E0) >> 2;
                int b = (color555 & 0x7C00) >> 7;
                int a = 0xFF;
                palettes[i].push_back(QColor(r, g, b, a).rgba());
            }
        }

        // TODO Load the animated tiles


        // Load the 8x8 tile graphics
        int fgGFXptr = ROMUtils::PointerFromData(ROMUtils::CurrentFile, tilesetPtr);
        int fgGFXlen = ROMUtils::IntFromData(ROMUtils::CurrentFile, tilesetPtr + 4);
        int bgGFXptr = ROMUtils::PointerFromData(ROMUtils::CurrentFile, tilesetPtr + 12);
        int bgGFXlen = ROMUtils::IntFromData(ROMUtils::CurrentFile, tilesetPtr + 16);
        // Foreground
        tile8x8data[0x40] = Tile8x8::CreateBlankTile(palettes);
        for(int i = 0; i < fgGFXlen / 32; ++i)
        {
            tile8x8data[i + 0x41] = new Tile8x8(fgGFXptr + i * 32, palettes);
        }
        // Background
        int bgGFXcount = bgGFXlen / 32;
        for(int i = 0; i < bgGFXcount; ++i)
        {
            tile8x8data[bgGFXlen - 1 + i - bgGFXcount] = new Tile8x8(bgGFXptr + i * 32, palettes);
        }

        // Load the map16 data
        int map16ptr = ROMUtils::PointerFromData(ROMUtils::CurrentFile, tilesetPtr + 0x14);
        int map16size = sizeof(map16data) / sizeof(map16data[0]);
        for(int i = 0; i < map16size; ++i) // TODO crashes on i = 6
        {
            unsigned short *map16tilePtr = (unsigned short*) (ROMUtils::CurrentFile + map16ptr + i * 8);
            Tile8x8 *tiles[4];
            for(int j = 0; j < 4; ++j)
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
    }
}
