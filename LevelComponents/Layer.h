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
        LayerMap16 = 0x10,
        LayerTile8x8 = 0x20
    };

    class Layer
    {
    private:
        enum LayerMappingType MappingType;
        bool Enabled = false;
        std::vector<Tile *> tiles;
        int Width = 0, Height = 0;
        unsigned short *LayerData = nullptr;
        int LayerPriority = 0;
        bool dirty = false;
        unsigned int DataPtr; // this pointer does not include the 0x8000000 bit
        void DeconstructTiles();

    public:
        Layer(int layerDataPtr, enum LayerMappingType mappingType);
        Layer(Layer &layer);
        QPixmap RenderLayer(Tileset *tileset);
        int GetLayerWidth() { return Width; }
        int GetLayerHeight() { return Height; }
        enum LayerMappingType GetMappingType() { return MappingType; }
        unsigned short *GetLayerData() { return LayerData; }
        void SetLayerData(unsigned short *ptr) { LayerData = ptr; }
        int GetLayerPriority() { return LayerPriority; }
        void SetLayerPriority(int priority) { LayerPriority = priority; }
        void ReRenderTile(int xpos, int ypos, unsigned short TileID, Tileset *tileset);
        std::vector<Tile *> GetTiles() { return tiles; }
        bool IsEnabled() { return Enabled; }
        void SetDisabled();
        void CreateNewLayer_type0x10(int layerWidth, int layerHeight);
        void ChangeDimensions(int newWidth, int newHeight);
        bool IsDirty() { return dirty; }
        void SetDirty(bool _dirty) { dirty = _dirty; }
        unsigned char *GetCompressedLayerData(unsigned int *dataSize);
        ~Layer();
        unsigned int GetDataPtr() { return DataPtr; }
        void SetDataPtr(unsigned int _dataPtr) { DataPtr = _dataPtr; }
        void ResetData();
    };
} // namespace LevelComponents

#endif // LAYER_H
