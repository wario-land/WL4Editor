#ifndef ASSORTEDGRAPHICUTILS_H
#define ASSORTEDGRAPHICUTILS_H

#include <QString>
#include <QColor>
#include <QVector>
#include <QByteArray>

#include "ROMUtils.h"

namespace AssortedGraphicUtils
{
    // structs
    enum AssortedGraphicTileDataType
    {
        Tile8x8_4bpp_no_comp_Tileset_text_bg = 0,
        Tile8x8_4bpp_no_comp                 = 1   // user-made graphic for their own use, should not work atm
    };

    enum AssortedGraphicMappingDataCompressionType
    {
        No_mapping_data_comp = 0,      // reserved for some shit things, and perhaps non-text bg mode, should not work atm
        RLE_mappingtype_0x20 = 1      // RLE for mapping Tile8x8 directly
    };

    /***
     * Save the whole struct into the ROM by converting
     *
     */
    struct AssortedGraphicEntryItem
    {
        // info params need to saved into the AssortedGraphicListChunk
        unsigned int TileDataAddress;
        unsigned int TileDataSize_Byte; // unit: Byte
        unsigned int TileDataRAMOffsetNum = 0; // unit: per Tile8x8
        enum AssortedGraphicTileDataType TileDataType;
        QString TileDataName;
        unsigned int MappingDataAddress;
        unsigned int MappingDataSizeAfterCompression_Byte; // unit: Byte
        enum AssortedGraphicMappingDataCompressionType MappingDataCompressType;
        QString MappingDataName;
        unsigned int PaletteAddress = 0;
        unsigned int PaletteNum = 1; // when (optionalPaletteAddress + PaletteNum) > 16, we just discard the latter palettes
        unsigned int PaletteRAMOffsetNum = 0; // unit: per palette, 16 color
        unsigned int optionalGraphicWidth = 0; // overwrite size params when the mapping data include size info
        unsigned int optionalGraphicHeight = 0;

        // things not saved in the AssortedGraphicListChunk
        QByteArray tileData;
        QVector<QRgb> palettes[16];
        QVector<unsigned short> mappingData;

        AssortedGraphicEntryItem &operator = (const AssortedGraphicEntryItem &entry)
        {
            this->TileDataAddress = entry.TileDataAddress;
            this->TileDataSize_Byte = entry.TileDataSize_Byte;
            this->TileDataRAMOffsetNum = entry.TileDataRAMOffsetNum;
            this->TileDataType = entry.TileDataType;
            this->TileDataName = entry.TileDataName;
            this->MappingDataAddress = entry.MappingDataAddress;
            this->MappingDataSizeAfterCompression_Byte = entry.MappingDataSizeAfterCompression_Byte;
            this->MappingDataCompressType = entry.MappingDataCompressType;
            this->MappingDataName = entry.MappingDataName;
            this->PaletteAddress = entry.PaletteAddress;
            this->PaletteNum = entry.PaletteNum;
            this->PaletteRAMOffsetNum = entry.PaletteRAMOffsetNum;
            this->optionalGraphicWidth = entry.optionalGraphicWidth;
            this->optionalGraphicHeight = entry.optionalGraphicHeight;
            this->tileData = entry.tileData;
            for(int i = 0; i < 16; i++)
            {
                this->palettes[i] = entry.palettes[i];
            }
            this->mappingData = entry.mappingData;
            return *this;
        }

        // used in palette import lambda functions
        void SetColor(int paletteId, int colorId, QRgb newcolor) { palettes[paletteId][colorId] = newcolor; }
    };

    // used for reset AssortedGraphicListChunk only
    enum chunkSaveDataType
    {
        graphicPalette = 0,
        graphictiles = 1,
        graphicmappingdata = 2
    };
    struct entry_datatype_chunk
    {
        unsigned int entryID = 0;
        enum chunkSaveDataType datatype = graphicPalette;
        unsigned int chunkID = 0;
    };

    // functions
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> GetAssortedGraphicsFromROM();
    void ExtractDataFromEntryInfo_v1(struct AssortedGraphicUtils::AssortedGraphicEntryItem &entry);
    QString SaveAssortedGraphicsToROM(QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> &entries);

    // savechunk relative functions
    QVector<unsigned int> GetSaveDataAddresses(AssortedGraphicEntryItem &entry);
    QVector<struct ROMUtils::SaveData> CreateSaveData(AssortedGraphicEntryItem &entry, unsigned int entryId);
};

#endif // ASSORTEDGRAPHICUTILS_H
