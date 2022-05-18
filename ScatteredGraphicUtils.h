#ifndef SCATTEREDGRAPHICUTILS_H
#define SCATTEREDGRAPHICUTILS_H

#include <QString>

namespace ScatteredGraphicUtils
{
    // structs
    enum ScatteredGraphicTileDataCompressionType
    {
        No_tile_data_comp = 0
    };

    enum ScatteredGraphicMappingDataCompressionType
    {
        No_mapping_data_comp = 0,
        Tileset_RLE_with_sizeheader = 1
    };

    /***
     * Save the whole struct into the ROM by converting
     *
     */
    struct ScatteredGraphicEntryItem
    {
        unsigned int TileDataAddress;
        unsigned int TileDataSize_Byte; // unit: Byte
        unsigned int TileDataRAMOffsetNum = 0; // unit: per Tile8x8
        enum ScatteredGraphicTileDataCompressionType TileDataCompressType;
        QString TileDataName;
        unsigned int MappingDataAddress;
        unsigned int MappingDataSize_Byte; // unit: Byte
        enum ScatteredGraphicMappingDataCompressionType MappingDataCompressType;
        QString MappingDataName;
        unsigned int optionalPaletteAddress = 0; // set 0 to not save or load palette for it
        unsigned int PaletteNum = 1; // when (optionalPaletteAddress + PaletteNum) > 16, we just discard the latter palettes
        unsigned int PaletteRAMOffsetNum = 0; // unit: per palette, 16 color
        unsigned int optionalGraphicWidth = 0; // overwrite size params when the mapping data include size info
        unsigned int optionalGraphicHeight = 0;
    };

    // functions
};

#endif // SCATTEREDGRAPHICUTILS_H
