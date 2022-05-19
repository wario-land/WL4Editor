#ifndef SCATTEREDGRAPHICUTILS_H
#define SCATTEREDGRAPHICUTILS_H

#include <QString>
#include <QColor>
#include <QVector>
#include "LevelComponents\Tile.h"

namespace ScatteredGraphicUtils
{
    // structs
    enum ScatteredGraphicTileDataType
    {
        Tileset_text_bg_4bpp_no_comp = 0
    };

    enum ScatteredGraphicMappingDataCompressionType
    {
        No_mapping_data_comp = 0,      // reserved for some shit things, and perhaps non-text bg mode
        RLE16_with_sizeheader = 1      // RLE for u16, for mapping Tile8x8 directly
    };

    /***
     * Save the whole struct into the ROM by converting
     *
     */
    struct ScatteredGraphicEntryItem
    {
        // info params need to saved into the ScatteredGraphicListChunk
        unsigned int TileDataAddress;
        unsigned int TileDataSize_Byte; // unit: Byte
        unsigned int TileDataRAMOffsetNum = 0; // unit: per Tile8x8
        enum ScatteredGraphicTileDataType TileDataType;
        QString TileDataName;
        unsigned int MappingDataAddress;
        unsigned int MappingDataSize_Byte; // unit: Byte
        enum ScatteredGraphicMappingDataCompressionType MappingDataCompressType;
        QString MappingDataName;
        unsigned int PaletteAddress = 0;
        unsigned int PaletteNum = 1; // when (optionalPaletteAddress + PaletteNum) > 16, we just discard the latter palettes
        unsigned int PaletteRAMOffsetNum = 0; // unit: per palette, 16 color
        unsigned int optionalGraphicWidth = 0; // overwrite size params when the mapping data include size info
        unsigned int optionalGraphicHeight = 0;

        // things not saved in the ScatteredGraphicListChunk
        bool changed = false;
        QVector<LevelComponents::Tile8x8 *> tile8x8array;
        LevelComponents::Tile8x8 *blankTile = nullptr;
        QVector<QRgb> palettes[16];
        QVector<unsigned short> mappingData;
    };

    // functions
    QVector<struct ScatteredGraphicUtils::ScatteredGraphicEntryItem> GetScatteredGraphicsFromROM();
};

#endif // SCATTEREDGRAPHICUTILS_H
