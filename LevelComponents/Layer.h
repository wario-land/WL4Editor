#ifndef LAYER_H
#define LAYER_H

#include "Tile.h"
#include "Tileset.h"

#include <QPixmap>

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
        bool Enabled; // Set false to disable backgrounds of type 0x00
        bool Visible; // Set false to hide this layer in the editor
        std::vector<Tile*> tiles;
        int width, height;
        unsigned short * LayerMappingCode;
    public:
        Layer(int layerDataPtr, enum LayerMappingType mappingType, Tileset *tileset);
        QPixmap RenderLayer();
        int GetLayerwidth() {return width;}
        int GetLayerheight() {return height;}
        unsigned short *GetLayerMappingCode() {return this->LayerMappingCode;}
    };
}

#endif // LAYER_H
