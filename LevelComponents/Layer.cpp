#include "Layer.h"
#include "ROMUtils.h"

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
        if(mappingType == LayerMap16)
        {
            width = ROMUtils::CurrentFile[layerDataPtr];
            height = ROMUtils::CurrentFile[layerDataPtr + 1];
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
        }

        // Get the layer data
        unsigned short *layerData = (unsigned short *) ROMUtils::RLEDecompress(layerDataPtr + 2, width * height);

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

        // Clean up
        delete[] layerData;
    }

    QPixmap Layer::RenderLayer()
    {
        // TODO
        return QPixmap();
    }
}
