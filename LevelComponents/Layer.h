#ifndef LAYER_H
#define LAYER_H

#include "Tile.h"

namespace LevelComponents
{
    enum LayerMappingType
    {
        LayerDisabled = 0x00,
        LayerMap16    = 0x10,
        LayerTile8x8  = 0x20
    };

    class Layer
    {
    private:
        bool Enabled; // so we can disable a back ground of type 0x00
        bool Visible; // so we can show or hide in the editor
        std::vector<Tile> tiles;
    public:
        Layer(int layerDataPtr);
    };
}

#endif // LAYER_H
