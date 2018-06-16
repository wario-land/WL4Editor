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
        enum LayerMappingType MappingType;
        bool Enabled; // Set false to disable backgrounds of type 0x00
        bool Visible; // Set false to hide this layer in the editor
        std::vector<Tile*> tiles;
        int Width, Height;
        unsigned short *LayerData;
        int LayerPriority;
    public:
        Layer(int layerDataPtr, enum LayerMappingType mappingType, Tileset *tileset);
        QPixmap RenderLayer();
        int GetLayerwidth() { return Width; }
        int GetLayerheight() { return Height; }
        enum LayerMappingType GetMappingType() { return MappingType; }
        unsigned short *GetLayerData() { return LayerData; }
        int GetLayerPriority() { return LayerPriority; }
        void SetLayerPriority(int priority) { LayerPriority = priority; }
        bool IsVisible() { return Visible; }
    };
}

#endif // LAYER_H
