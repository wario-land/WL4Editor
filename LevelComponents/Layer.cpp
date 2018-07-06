#include "Layer.h"
#include "ROMUtils.h"
#include <iostream>

namespace LevelComponents
{
    Layer::Layer(int layerDataPtr, enum LayerMappingType mappingType, Tileset *tileset) :
        Enabled(mappingType != LayerDisabled), MappingType(mappingType)
    {
        if(mappingType == LayerDisabled)
        {
            return;
        }

        // Get the layer dimensions
        if(mappingType == LayerMap16)
        {
            Width = ROMUtils::CurrentFile[layerDataPtr];
            Height = ROMUtils::CurrentFile[layerDataPtr + 1];

            // Get the layer data
            LayerData = (unsigned short *) ROMUtils::LayerRLEDecompress(layerDataPtr + 2, Width * Height * 2);
        }
        else if(mappingType == LayerTile8x8)
        {
            // Set
            switch(ROMUtils::CurrentFile[layerDataPtr])
            {
            case 0:
                Width = Height = 32;
                break;
            case 1:
                Width = 64;
                Height = 32;
                break;
            case 2:
                Width = 32;
                Height = 64;
                break;
            default:
                // TODO error handling. This line should not be reached for valid layer data!
                return;
            }

            // Get the layer data
            LayerData = (unsigned short *) ROMUtils::LayerRLEDecompress(layerDataPtr + 1, Width * Height * 2);

            // Rearrange tile data for dimension type 1
            //   1 2 3 4 5 6    1 2 3 A B C
            //   7 8 9 A B C => 4 5 6 D E F
            //   D E F G H I    7 8 9 G H I
            if(ROMUtils::CurrentFile[layerDataPtr] == 1)
            {
                unsigned short *rearranged = new unsigned short[Width * Height * 2];
                for(int j = 0; j < 32; ++j) {
                    for(int k = 0; k < 32; ++k) {
                        rearranged[(j << 6) + k] = LayerData[(j << 5) + k];
                        rearranged[(j << 6) + k + 32] = LayerData[(j << 5) + k + 1024];
                    }
                }
                void *tmp = LayerData;
                LayerData = rearranged;
                delete[] tmp;
            }
        }

        // Was layer decompression successful?
        if(LayerData == nullptr)
            std::cout << "Failed to decomporess layer data: " << (layerDataPtr + 1) << std::endl;

        // Create tiles
        tiles = std::vector<Tile*>(Width * Height);
        if(mappingType == LayerMap16)
        {
            // For 16x16 tiles, just copy the tiles from the map16
            TileMap16 **map16 = tileset->GetMap16Data();
            for(int i = 0; i < Width * Height; ++i)
            {
                tiles[i] = map16[LayerData[i]];
            }
        }
        else if(mappingType == LayerTile8x8)
        {
            // For 8x8 tiles, we must use the copy constructor and set each tile's properties
            Tile8x8 **tile8x8 = tileset->GetTile8x8Data();
            for(int i = 0; i < Width * Height; ++i)
            {
                unsigned short tileData = LayerData[i];
                Tile8x8 *newTile = new Tile8x8(tile8x8[0x200 + (tileData & 0x3FF)]);
                newTile->SetFlipX((tileData & (1 << 10)) != 0);
                newTile->SetFlipY((tileData & (1 << 11)) != 0);
                newTile->SetPaletteIndex((tileData >> 12) & 0xF);
                tiles[i] = newTile;
            }
        }
    }

    QPixmap Layer::RenderLayer()
    {
        // Set the units we are drawing in (depending on the Tile type)
        int units;
        switch(MappingType)
        {
            case LayerDisabled:
                return QPixmap();
            case LayerMap16:
                units = 16;
                break;
            case LayerTile8x8:
                units = 8;
        }

        // Initialize the QPixmap with transparency
        QPixmap layerPixmap(Width * units, Height * units);
        layerPixmap.fill(Qt::transparent);

        // Draw the tiles to the QPixmap
        for(int i = 0; i < Height; ++i)
        {
            for(int j = 0; j < Width; ++j)
            {
                tiles[j + i * Width]->DrawTile(&layerPixmap, j * units, i * units);
            }
        }

        return layerPixmap;
    }

    void Layer::ChangeTile(int xpos, int ypos, unsigned short TileID)
    {
        this->LayerData[ypos * this->Height + xpos] = TileID;
    }
}
