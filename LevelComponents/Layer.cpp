#include "Layer.h"
#include "ROMUtils.h"
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
        MappingType(mappingType), Enabled(mappingType != LayerDisabled)
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
                unsigned short *tmp = LayerData;
                LayerData = rearranged;
                delete[] tmp;
            }
        }

        // Was layer decompression successful?
        if(LayerData == nullptr)
            std::cout << "Failed to decompress layer data: " << (layerDataPtr + 1) << std::endl;
    }

    /// <summary>
    /// Deconstruct the Layer and clean up its instance objects on the heap.
    /// </summary>
    Layer::~Layer()
    {
        if(LayerData == nullptr) return;

        // If this is mapping type tile8x8, then the tiles are heap copies of tileset tiles.
        if(MappingType == LayerTile8x8)
        {
            for(auto iter = tiles.begin(); iter != tiles.end(); ++iter)
            {
                delete(*iter);
            }
        }

        // If it is map16 type, then they are just pointer copies and should be deconstructed in ~Tileset() only
        delete[] LayerData;
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

        // Create tiles
        if(tiles.size() != 0) tiles.clear();
        tiles = std::vector<Tile*>(Width * Height);
        if(MappingType == LayerMap16)
        {
            // For 16x16 tiles, just copy the tiles from the map16
            TileMap16 **map16 = tileset->GetMap16Data();
            for(int i = 0; i < Width * Height; ++i)
            {
                tiles[i] = map16[LayerData[i]];
            }
        }
        else if(MappingType == LayerTile8x8)
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
        int index = X + Y * Width;
        if(MappingType == LayerMap16)
        {
            // If map16 type, then just copy the map16 tile object from the tileset
            tiles[index] = tileset->GetMap16Data()[TileID];
        }
        else if(MappingType == LayerTile8x8)
        {
            // If tile8x8 type, then new Tile8x8 objects must be constructed from data
            Tile8x8 *newTile = new Tile8x8(tileset->GetTile8x8Data()[0x200 + (TileID & 0x3FF)]);
            newTile->SetFlipX((TileID & (1 << 10)) != 0);
            newTile->SetFlipY((TileID & (1 << 11)) != 0);
            newTile->SetPaletteIndex((TileID >> 12) & 0xF);
            delete tiles[index];
            tiles[index] = newTile;
        }
        else std::cout << "WARNING: Invalid mapping type ecountered in Layer::ChangeTile" << std::endl;
    }

    /// <summary>
    /// Disable a Layer and free all of the memory it use but the instance is still exist
    /// </summary>
    void Layer::SetDisabled()
    {
        if(LayerData == nullptr) return;
        if(MappingType == LayerTile8x8) // If this is mapping type tile8x8, then the tiles are heap copies of tileset tiles.
        {
            for(auto iter = tiles.begin(); iter != tiles.end(); ++iter)
            {
                delete(*iter);
            }
        }else{ // If it is map16 type, then they are just pointer copies so only free the vector
            tiles.clear();
        }

        delete[] LayerData; LayerData = nullptr;
        MappingType = LayerDisabled;
        Enabled = false; Width = 0; Height = 0;
        NewLayer = false;
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
        NewLayer = true; Enabled = true;
        MappingType = LayerMap16;
        if(LayerData != nullptr)
            delete[] LayerData;
        LayerData = new unsigned short[layerWidth * layerHeight];
        memset(LayerData, '\0', sizeof(char) * 2 * layerWidth * layerHeight);
    }

    /// <summary>
    /// Use this function to add rows to the existing layer.
    /// </summary>
    /// <param name="NumberOfNewRows">
    /// Number of new rows you wan to add to the layer.
    /// </param>
    /// <param name="StartFrom">
    /// the id of the specific row which we add new blank rows before it.
    /// </param>
    void Layer::AddRows(int NumberOfNewRows, int StartFrom)
    {
        Height += NumberOfNewRows;
        unsigned short *tmpLayerData = new unsigned short[Width * Height];
        int k = 0;
        for(int j = 0; j < Height; j++)
        {
            for(int i = 0; i < Width; i++)
            {
                if((StartFrom >= j) && (k < NumberOfNewRows))
                    tmpLayerData[i + j * Width] = (unsigned short) 0;
                else
                    tmpLayerData[i + j * Width] = LayerData[i + (j - k) * Width];
            }
            if((StartFrom >= j) && (k < NumberOfNewRows)) k++;
        }
        delete[] LayerData;
        LayerData = tmpLayerData;
    }

    /// <summary>
    /// Use this function to add columns to the existing layer.
    /// </summary>
    /// <param name="NumberOfNewColumns">
    /// Number of new columns you wan to add to the layer.
    /// </param>
    /// <param name="StartFrom">
    /// the id of the specific column which we add new blank columns before it.
    /// </param>
    void Layer::AddColumns(int NumberOfNewColumns, int StartFrom)
    {
        Width += NumberOfNewColumns;
        unsigned short *tmpLayerData = new unsigned short[Width * Height];
        int k = 0;
        for(int j = 0; j < Height; j++)
        {
            for(int i = 0; i < Width; i++)
            {
                if((StartFrom >= i) && (k < NumberOfNewColumns))
                {
                    tmpLayerData[i + j * Width] = (unsigned short) 0;
                    k++;
                }
                else
                {
                    tmpLayerData[i + j * Width] = LayerData[i - k + j * (Width - NumberOfNewColumns)];
                }
            }
            k = 0;
        }
        delete[] LayerData;
        LayerData = tmpLayerData;
    }

    /// <summary>
    /// Use this function to delete rows from the existing layer.
    /// </summary>
    /// <param name="NumberOfWillBeDeletedRows">
    /// Number of rows you wan to delete form the layer.
    /// </param>
    /// <param name="StartFrom">
    /// the id of the specific row which we start delet rows.
    /// </param>
    void Layer::DeleteRows(int NumberOfWillBeDeletedRows, int StartFrom)
    {
        Height -= NumberOfWillBeDeletedRows;
        unsigned short *tmpLayerData = new unsigned short[Width * Height];
        for(int j = 0; j < Height; j++)
        {
            for(int i = 0; i < Width; i++)
            {
                if(StartFrom >= j)
                    tmpLayerData[i + j * Width] = LayerData[i + (j + NumberOfWillBeDeletedRows) * Width];
                else
                    tmpLayerData[i + j * Width] = LayerData[i + j * Width];
            }
        }
        delete[] LayerData;
        LayerData = tmpLayerData;
    }

    /// <summary>
    /// Use this function to delete columns from the existing layer.
    /// </summary>
    /// <param name="NumberOfWillBeDeletedColumns">
    /// Number of columns you wan to delete form the layer.
    /// </param>
    /// <param name="StartFrom">
    /// the id of the specific column which we start delet columns.
    /// </param>
    void Layer::DeleteColumns(int NumberOfWillBeDeletedColumns, int StartFrom)
    {
        Width -= NumberOfWillBeDeletedColumns;
        unsigned short *tmpLayerData = new unsigned short[Width * Height];
        for(int j = 0; j < Height; j++)
        {
            for(int i = 0; i < Width; i++)
            {
                if(StartFrom >= i)
                    tmpLayerData[i + j * Width] = LayerData[i + j * (Width + NumberOfWillBeDeletedColumns) + NumberOfWillBeDeletedColumns];
                else
                    tmpLayerData[i + j * Width] = LayerData[i + j * (Width + NumberOfWillBeDeletedColumns)];
            }
        }
        delete[] LayerData;
        LayerData = tmpLayerData;
    }
}
