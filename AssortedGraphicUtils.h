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
        unsigned int TileDataSizeInByte; // unit: Byte
        unsigned int TileDataRAMOffsetNum = 0; // unit: per Tile8x8
        enum AssortedGraphicTileDataType TileDataType;
        QString TileDataName;
        unsigned int MappingDataAddress;
        unsigned int MappingDataSizeAfterCompressionInByte; // unit: Byte
        enum AssortedGraphicMappingDataCompressionType MappingDataCompressType;
        QString MappingDataName;
        unsigned int PaletteAddress = 0;
        QVector<unsigned int> PaletteSlotIDs; // individual palette slot IDs, e.g. [1, 3, 5] instead of a start+length pair
        unsigned int optionalGraphicWidth = 0; // overwrite size params when the mapping data include size info
        unsigned int optionalGraphicHeight = 0;

        // things not saved in the AssortedGraphicListChunk
        QByteArray tileData;
        QVector<QRgb> palettes[16];
        QVector<unsigned short> mappingData;

        AssortedGraphicEntryItem &operator = (const AssortedGraphicEntryItem &entry)
        {
            this->TileDataAddress = entry.TileDataAddress;
            this->TileDataSizeInByte = entry.TileDataSizeInByte;
            this->TileDataRAMOffsetNum = entry.TileDataRAMOffsetNum;
            this->TileDataType = entry.TileDataType;
            this->TileDataName = entry.TileDataName;
            this->MappingDataAddress = entry.MappingDataAddress;
            this->MappingDataSizeAfterCompressionInByte = entry.MappingDataSizeAfterCompressionInByte;
            this->MappingDataCompressType = entry.MappingDataCompressType;
            this->MappingDataName = entry.MappingDataName;
            this->PaletteAddress = entry.PaletteAddress;
            this->PaletteSlotIDs = entry.PaletteSlotIDs;
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

    // helper functions
    bool CheckEditability(struct AssortedGraphicUtils::AssortedGraphicEntryItem &entry, unsigned int &find_l, unsigned int &find_r, unsigned int &find_t);
    bool FindbgGFXptrInAllTilesets(unsigned int address, unsigned int *tilesetId_find);
    bool FindLayerptrInAllRooms(unsigned int address, unsigned int *levelId_found, unsigned int *roomId_found);
};

#endif // ASSORTEDGRAPHICUTILS_H
