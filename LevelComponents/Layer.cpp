#include "Layer.h"
#include "ROMUtils.h"

#include <cassert>
#include <cstring>
#include <iostream>

namespace LevelComponents
{
    /// <summary>
    /// Construct an instance of Layer.
    /// </summary>
    /// <remarks>
    /// Mapping type is a parameter because that information is not contained in the layer data itself.
    /// </remarks>
    /// <param name="layerDataPtr">
    /// Pointer to the beginning of the layer data.
    /// </param>
    /// <param name="mappingType">
    /// The mapping type for the layer.
    /// </param>
    Layer::Layer(int layerDataPtr, enum LayerMappingType mappingType) :
            MappingType(mappingType), Enabled(mappingType != LayerDisabled), DataPtr(layerDataPtr)
    {
        if (mappingType == LayerDisabled)
        {
            return;
        }

        // Get the layer dimensions
        if (mappingType == LayerMap16)
        {
            Width = ROMUtils::CurrentFile[layerDataPtr];
            Height = ROMUtils::CurrentFile[layerDataPtr + 1];

            // Get the layer data
            LayerData =
                reinterpret_cast<unsigned short *>(ROMUtils::LayerRLEDecompress(layerDataPtr + 2, Width * Height * 2));
        }
        else if (mappingType == LayerTile8x8)
        {
            // Set
            Width = (1 + (ROMUtils::CurrentFile[layerDataPtr] & 1)) << 5;
            Height = (1 + ((ROMUtils::CurrentFile[layerDataPtr] >> 1) & 1)) << 5;

            // Get the layer data
            LayerData =
                reinterpret_cast<unsigned short *>(ROMUtils::LayerRLEDecompress(layerDataPtr + 1, Width * Height * 2));

            // Rearrange tile data for dimension type 1
            //   1 2 3 4 5 6      1 2 3 A B C
            //   7 8 9 A B C  =>  4 5 6 D E F
            //   D E F G H I      7 8 9 G H I
            if (ROMUtils::CurrentFile[layerDataPtr] == 1)
            {
                unsigned short *rearranged = new unsigned short[Width * Height * 2];
                for (int j = 0; j < 32; ++j)
                {
                    for (int k = 0; k < 32; ++k)
                    {
                        rearranged[(j << 6) + k] = LayerData[(j << 5) + k];
                        rearranged[(j << 6) + k + 32] = LayerData[(j << 5) + k + 1024];
                    }
                }
                unsigned short *tmp = LayerData;
                LayerData = rearranged;
                delete[] tmp;
            }
        }

        // Was layer decompression successful?
        if (!LayerData)
            std::cout << "Failed to decompress layer data: " << (layerDataPtr + 1) << std::endl;
    }

    /// <summary>
    /// Deep copy constructor for Layer.
    /// </summary>
    /// <param name="layer">
    /// The Layer object to deep copy from.
    /// </param>
    Layer::Layer(Layer &layer) :
            MappingType(layer.MappingType), Enabled(layer.Enabled), Width(layer.Width), Height(layer.Height),
            LayerPriority(layer.LayerPriority), dirty(layer.dirty), DataPtr(layer.DataPtr)
    {
        int layerDataSize = Width * Height * 2;
        LayerData = (unsigned short *) malloc(layerDataSize);
        memcpy(LayerData, layer.LayerData, layerDataSize);
        if (MappingType == LayerMappingType::LayerTile8x8)
            foreach (Tile *tile, layer.tiles)
                tiles.push_back(new Tile8x8((Tile8x8 *) tile));
        else // Map16 tiles are not deep copied
            foreach (Tile *tile, layer.tiles)
                tiles.push_back(tile);
    }

    /// <summary>
    /// Deconstruct the Layer and clean up its instance objects on the heap.
    /// </summary>
    Layer::~Layer()
    {
        if (!LayerData)
            return;
        DeconstructTiles();
        delete[] LayerData;
    }

    /// <summary>
    /// Reset the Layer data on the heap.
    /// </summary>
    void Layer::ResetData()
    {
        dirty = Enabled = true;
        memset(LayerData, 0, 2 * Width * Height);
        dirty = true;
    }

    /// <summary>
    /// Deconstruct the layer's tile
    /// </summary>
    void Layer::DeconstructTiles()
    {
        // If this is mapping type tile8x8, then the tiles are heap copies of tileset tiles.
        // If it is map16 type, then they are just pointer copies and should be deconstructed in ~Tileset() only
        if (MappingType == LayerTile8x8)
        {
            foreach (Tile *t, tiles)
            {
                delete t;
            }
        }
    }

    /// <summary>
    /// Render a layer as a QPixmap.
    /// </summary>
    /// <remarks>
    /// The tileset parameter is necessary because tileset information is in the Room object, not layer.
    /// </remarks>
    /// <param name="tileset">
    /// The tileset defining the tiles that will be drawn on the layer graphics.
    /// </param>
    /// <return>
    /// A QPixmap of the fully rendered layer, including transparency.
    /// </return>
    QPixmap Layer::RenderLayer(Tileset *tileset)
    {
        // Set the units we are drawing in (depending on the Tile type)
        int units = -1;
        switch (MappingType)
        {
        case LayerDisabled:
            return QPixmap();
        case LayerMap16:
            units = 16;
            break;
        case LayerTile8x8:
            units = 8;
            break;
        default:
            assert(0 /* Invalid tileset mapping type encountered in Layer::RenderLayer */);
        }

        // Create tiles
        if (MappingType == LayerMap16)
        {
            // Re-initialize tile vector
            if (tiles.size() != 0)
                tiles.clear();
            tiles = std::vector<Tile *>(Width * Height);

            // For 16x16 tiles, just copy the tiles from the map16
            TileMap16 **map16 = tileset->GetMap16arrayPtr();
            for (int i = 0; i < Width * Height; ++i)
            {
                tiles[i] = map16[LayerData[i]];
            }
        }
        else if (MappingType == LayerTile8x8)
        {
            // Re-initialize tile vector
            DeconstructTiles();
            if (tiles.size() != 0)
                tiles.clear();
            tiles = std::vector<Tile *>(Width * Height);

            // For 8x8 tiles, we must use the copy constructor and set each tile's properties
            Tile8x8 **tile8x8 = tileset->GetTile8x8arrayPtr();
            for (int i = 0; i < Width * Height; ++i)
            {
                unsigned short tileData = LayerData[i];
                Tile8x8 *newTile = new Tile8x8(tile8x8[0x200 + (tileData & 0x3FF)]);
                newTile->SetFlipX((tileData & (1 << 10)) != 0);
                newTile->SetFlipY((tileData & (1 << 11)) != 0);
                newTile->SetPaletteIndex((tileData >> 12) & 0xF);
                tiles[i] = newTile;
            }
        }

        // Initialize the QPixmap with transparency
        QPixmap layerPixmap(Width * units, Height * units);
        layerPixmap.fill(Qt::transparent);

        // Draw the tiles to the QPixmap
        for (int i = 0; i < Height; ++i)
        {
            for (int j = 0; j < Width; ++j)
            {
                Tile *t = tiles[j + i * Width];
                t->DrawTile(&layerPixmap, j * units, i * units);
            }
        }

        return layerPixmap;
    }

    /// <summary>
    /// Re-render a single tile's graphics in the tiles instance variable.
    /// </summary>
    /// <remarks>
    /// This is used as a counterpart to Layer::RenderLayer, which only re-renders one tile.
    /// </remarks>
    /// <param name="X">
    /// The X position of the tile to change.
    /// </param>
    /// <param name="Y">
    /// The Y position of the tile to change.
    /// </param>
    /// <param name="TileID">
    /// The tile ID that the destination tile will be changed to.
    /// </param>
    /// <param name="tileset">
    /// The tileset defining the tiles that will be drawn on the layer graphics.
    /// </param>
    void Layer::ReRenderTile(int X, int Y, unsigned short TileID, Tileset *tileset)
    {
        dirty = true;
        int index = X + Y * Width;
        if (MappingType == LayerMap16)
        {
            // If map16 type, then just copy the map16 tile object from the tileset
            tiles[index] = tileset->GetMap16arrayPtr()[TileID];
        }
        else if (MappingType == LayerTile8x8)
        {
            // If tile8x8 type, then new Tile8x8 objects must be constructed from data
            Tile8x8 *newTile = new Tile8x8(tileset->GetTile8x8arrayPtr()[0x200 + (TileID & 0x3FF)]);
            newTile->SetFlipX((TileID & (1 << 10)) != 0);
            newTile->SetFlipY((TileID & (1 << 11)) != 0);
            newTile->SetPaletteIndex((TileID >> 12) & 0xF);
            delete tiles[index];
            tiles[index] = newTile;
        }
        else
            std::cout << "WARNING: Invalid mapping type ecountered in Layer::ChangeTile" << std::endl;
    }

    /// <summary>
    /// Disable a Layer and free all of the memory it use but the instance is still exist
    /// </summary>
    void Layer::SetDisabled()
    {
        if (!LayerData)
            return;
        if (MappingType ==
            LayerTile8x8) // If this is mapping type tile8x8, then the tiles are heap copies of tileset tiles.
        {
            for (auto iter = tiles.begin(); iter != tiles.end(); ++iter)
            {
                delete (*iter);
            }
        }
        else
        {
            // If it is map16 type, then they are just pointer copies so only free the vector
            tiles.clear();
        }

        delete[] LayerData;
        LayerData = nullptr;
        MappingType = LayerDisabled;
        Enabled = false;
        dirty = true;
        Width = Height = 0;
    }

    /// <summary>
    /// Use this function to take the place of one existing layer in a room.
    /// </summary>
    /// <param name="layerWidth">
    /// New layer width.
    /// </param>
    /// <param name="layerHeight">
    /// New layer height.
    /// </param>
    void Layer::CreateNewLayer_type0x10(int layerWidth, int layerHeight)
    {
        Width = layerWidth;
        Height = layerHeight;
        dirty = Enabled = true;
        MappingType = LayerMap16;
        if (LayerData)
        {
            delete[] LayerData;
        }
        LayerData = new unsigned short[layerWidth * layerHeight];
        memset(LayerData, 0, 2 * layerWidth * layerHeight);
    }

    /// <summary>
    /// Change the size of the Layer.
    /// </summary>
    /// <remarks>
    /// This function will initialize new tiles to 0x40
    /// </remarks>
    /// <param name="newWidth">
    /// New layer width.
    /// </param>
    /// <param name="newHeight">
    /// New layer height.
    /// </param>
    void Layer::ChangeDimensions(int newWidth, int newHeight)
    {
        unsigned short *tmpLayerData = new unsigned short[newWidth * newHeight];
        int boundX = qMin(Width, newWidth), boundY = qMin(Height, newHeight);
        unsigned short defaultValue = 0x0000;
        for (int i = 0; i < boundY; ++i)
        {
            memcpy(tmpLayerData + i * newWidth, LayerData + i * Width, boundX * sizeof(short));
            for (int j = boundX; j < newWidth; ++j)
            {
                tmpLayerData[i * newWidth + j] = defaultValue;
            }
        }
        for (int i = boundY * newWidth; i < newWidth * newHeight; ++i)
        {
            tmpLayerData[i] = defaultValue;
        }
        Width = newWidth;
        Height = newHeight;
        delete LayerData;
        LayerData = tmpLayerData;
        dirty = true;
    }

    /// <summary>
    /// Create and returned compressed layer data (on the heap)
    /// </summary>
    /// <param name="dataSize">
    /// The int to write the data size to after the compressed data array is created.
    /// </param>
    /// <returns>
    /// Pointer to the compressed data.
    /// </returns>
    unsigned char *Layer::GetCompressedLayerData(unsigned int *dataSize)
    {
        unsigned char *dataBuffer;
        unsigned int compressedSize = ROMUtils::LayerRLECompress(Width * Height, LayerData, &dataBuffer);
        unsigned int sizeInfoLen = MappingType == LayerMap16 ? 2 : 1;
        unsigned char *dataChunk = new unsigned char[sizeInfoLen + compressedSize];
        memcpy(dataChunk + sizeInfoLen, dataBuffer, compressedSize);
        delete[] dataBuffer;
        if (MappingType == LayerMap16)
        {
            dataChunk[0] = (unsigned char) Width;
            dataChunk[1] = (unsigned char) Height;
        }
        else
        {
            dataChunk[0] = (unsigned char) ((Width >> 6) | ((Height >> 6) << 1));
        }
        *dataSize = sizeInfoLen + compressedSize;
        return dataChunk;
    }
} // namespace LevelComponents
