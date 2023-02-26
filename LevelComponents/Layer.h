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
        unsigned short *CreateLayerDataCopy()
        {
            if (MappingType != LayerMap16) return nullptr;
            unsigned short *datacopy = new unsigned short[Width * Height];
            memcpy(datacopy, LayerData, 2 * Width * Height);
            return datacopy;
        }
        void SetLayerData(unsigned short *ptr) { LayerData = ptr; }
        void SetTileData(unsigned short id, unsigned char x, unsigned char y)
        {
            if((x + y * Width) < (Width * Height))
                LayerData[x + y * Width] = id;
        }
        unsigned short GetTileData(unsigned char x, unsigned char y)
        {
            if((x + y * Width) < (Width * Height))
                return LayerData[x + y * Width];
            return 0xFFFF; // TODO
        }
        int GetLayerPriority() { return LayerPriority; }
        void SetLayerPriority(int priority) { LayerPriority = priority; }
        void ReRenderTile(int xpos, int ypos, unsigned short TileID, Tileset *tileset);
        std::vector<Tile *> GetTiles() { return tiles; }
        bool IsEnabled() { return Enabled; }
        void SetDisabled();
        void SetWidthHeightData(int layerWidth, int layerHeight, unsigned short *data)
        {
            if (layerWidth < 1 || layerHeight < 1) return;
            SetDisabled();
            MappingType = LayerMap16;
            Enabled = true;
            Width = layerWidth; Height = layerHeight;
            LayerData = new unsigned short[Width * Height];
            if (data != nullptr)
            {
                memcpy(LayerData, data, 2 * Width * Height);
            }
            else
            {
                memset(LayerData, 0, 2 * Width * Height);
            }
        }
        bool IsDirty() { return dirty; }
        void SetDirty(bool _dirty) { dirty = _dirty; }
        unsigned char *GetCompressedLayerData(unsigned int *dataSize);
        ~Layer();
        unsigned int GetDataPtr() { return DataPtr; }
        // don't use it before save level and reset layer pointer
        // just let DataPtr keep the old layer data pointer for now
        void SetDataPtr(unsigned int _dataPtr) { DataPtr = _dataPtr; }
        void ResetData();

        // tools
        static unsigned char *CompressLayerData(QVector<unsigned short> &data, enum LayerMappingType mappingType,
                                 unsigned int width,  unsigned int height, unsigned int *dataSize);
    };
} // namespace LevelComponents

#endif // LAYER_H
