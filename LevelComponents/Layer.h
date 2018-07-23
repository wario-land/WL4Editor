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
        std::vector<Tile*> tiles;
        int Width, Height;
        unsigned short *LayerData;
        int LayerPriority;

    public:
        Layer(int layerDataPtr, enum LayerMappingType mappingType);
        QPixmap RenderLayer(Tileset *tileset);
        int GetLayerWidth() { return Width; }
        int GetLayerHeight() { return Height; }
        enum LayerMappingType GetMappingType() { return MappingType; }
        unsigned short *GetLayerData() { return LayerData; }
        int GetLayerPriority() { return LayerPriority; }
        void SetLayerPriority(int priority) { LayerPriority = priority; }
        void ReRenderTile(int xpos, int ypos, unsigned short TileID, Tileset *tileset);
        std::vector<Tile*> GetTiles() { return tiles; }
        ~Layer();
    };
}

#endif // LAYER_H
