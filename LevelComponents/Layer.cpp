#include "Layer.h"
#include "ROMUtils.h"
#include <iostream>

namespace LevelComponents
{
    Layer::Layer(int layerDataPtr, enum LayerMappingType mappingType, Tileset *tileset) :
        Enabled(mappingType != LayerDisabled), Visible(true)
    {
        if(mappingType == LayerDisabled)
        {
            return;
        }

        // Get the layer dimensions
        int outputSizeMultiplier = 2;
        unsigned short *layerData;
        if(mappingType == LayerMap16)
        {
            width = ROMUtils::CurrentFile[layerDataPtr];
            height = ROMUtils::CurrentFile[layerDataPtr + 1];
            outputSizeMultiplier = 2;

            // Get the layer data
            layerData = (unsigned short *) ROMUtils::RLEDecompress(layerDataPtr + 2, width * height * outputSizeMultiplier);
        }
        else if(mappingType == LayerTile8x8)
        {
            switch(ROMUtils::CurrentFile[layerDataPtr])
            {
            case 0:
                width = height = 32;
                break;
            case 1:
                width = 64;
                height = 32;
                break;
            case 2:
                width = 32;
                height = 64;
                break;
            default:
                // TODO error handling. This line should not be reached for valid layer data!
                return;
            }

            // Get the layer data
            layerData = (unsigned short *) ROMUtils::RLEDecompress(layerDataPtr + 1, width * height * outputSizeMultiplier);
        }

        if(layerData = nullptr)
            std::cout<<"cannot decompress layer data"<<std::endl;

        // Create tiles
        tiles = std::vector<Tile*>(width * height);
        int index = 0;
        if(mappingType == LayerMap16)
        {
            while(index < width * height)
            {
                TileMap16 **map16 = tileset->GetMap16Data();
                tiles[index] = map16[index++];
            }
        }
        else if(mappingType == LayerTile8x8)
        {
            while(index < width * height)
            {
                Tile8x8 **tile8x8 = tileset->GetTile8x8Data();
                tiles[index] = tile8x8[index++];
            }
        }

        this->LayerMappingCode = layerData;
    }

    QPixmap Layer::RenderLayer()
    {
        // TODO
        return QPixmap();
    }
}
