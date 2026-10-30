#ifndef ASSORTEDGRAPHICUTILS_H
#define ASSORTEDGRAPHICUTILS_H

#include <QString>
#include <QColor>
#include <QVector>
#include <QByteArray>

#include "ROMUtils.h"

namespace LevelComponents
{
    class Tileset;
}

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

    // Vanilla assorted graphics.
    //
    // Every vanilla background graphic is a background Tile8x8 set (TileData) combined with one
    // RLE mapping data (MappingData), drawn with a Tileset palette. The table below lists every
    // such combination of the vanilla ROM, including the two vanilla Layer 0 mapping data which
    // are also stored in WL4Constants (ToxicLandfillDustyLayer0Ptr and FieryCavernDustyLayer0Ptr).
    //
    // The data is stored as one flat unsigned int array, in the style of the other vanilla data
    // tables of WL4Editor. Each entry starts with 9 fixed fields and is followed by the complete
    // palette slot id array of the entry, so the non-contiguous palette usage of the mapping data
    // is preserved:
    //
    //   +0  TileDataAddress
    //   +1  TileDataSizeInByte
    //   +2  TileDataRAMOffsetNum
    //   +3  TileDataType
    //   +4  MappingDataAddress
    //   +5  MappingDataSizeAfterCompressionInByte
    //   +6  MappingDataCompressType
    //   +7  PaletteAddress
    //   +8  PaletteSlotID count (the length of the following array, not a palette range)
    //   +9  PaletteSlotIDs (PaletteSlotID count values, may be non-contiguous)
    //
    // The complete palette slot id array of an entry is stored inline, so a graphic which uses
    // non-contiguous palette slots (for example 5 and 10) is described correctly without a
    // contiguous palette range.
    //
    // The TileDataName and MappingDataName fields of AssortedGraphicEntryItem are not stored,
    // because they are generated by the editor and are not guaranteed to stay stable. All the
    // vanilla entries use Tile8x8_4bpp_no_comp_Tileset_text_bg and RLE_mappingtype_0x20.
    //
    // The table is generated from the vanilla ROM: the TileData fields come from the Tileset
    // table, the MappingData fields come from BGLayerdataPtrsData and the vanilla Layer 0 mapping
    // data addresses of RoomConfigDialog, and the palette fields come from the Tileset palettes
    // plus the palette slots actually referenced by the mapping data. Entries are deduplicated by
    // (TileDataAddress, TileDataSizeInByte, PaletteAddress, MappingDataAddress); the comment of a
    // row names every Tileset which uses that graphic.
    //
    // All the address fields reference the named constants of WL4Constants.h: the Tile8x8 data
    // addresses use Tileset_BGTile_*, the mapping data addresses use BG_* (or the vanilla Layer 0
    // constants), and the palettes use Tileset_Palette_*. Every entry is written as three lines:
    // the TileData fields, the mapping data fields, and the palette and palette slot id fields.
    // clang-format off
    static constexpr const unsigned int VanillaAssortedGraphicEntryData[] =
    {
        WL4Constants::Tileset_BGTile_0x01, 0x3D20, 0x216, 0,
        WL4Constants::BG_0x5FB2CC, 0x60D, 1,
        WL4Constants::Tileset_Palette_0x583E7C, 1, 0xF, // Tileset 0x01
        WL4Constants::Tileset_BGTile_0x02, 0x3660, 0x24C, 0,
        WL4Constants::BG_0x5FB8DC, 0x9F4, 1,
        WL4Constants::Tileset_Palette_0x58407C, 1, 0xE, // Tileset 0x02
        WL4Constants::Tileset_BGTile_0x03, 0x21C0, 0x2F1, 0,
        WL4Constants::BG_0x603064, 0x627, 1,
        WL4Constants::Tileset_Palette_0x586E7C, 1, 0x3, // Tileset 0x03
        WL4Constants::Tileset_BGTile_0x04, 0x21C0, 0x2F1, 0,
        WL4Constants::BG_0x603064, 0x627, 1,
        WL4Constants::Tileset_Palette_0x58707C, 1, 0x3, // Tileset 0x04
        WL4Constants::Tileset_BGTile_0x05, 0x1FA0, 0x302, 0,
        WL4Constants::BG_0x60368C, 0x80C, 1,
        WL4Constants::Tileset_Palette_0x58727C, 1, 0xC, // Tileset 0x05
        WL4Constants::Tileset_BGTile_0x06, 0x5220, 0x16E, 0,
        WL4Constants::BG_0x5FC9A0, 0x6D7, 1,
        WL4Constants::Tileset_Palette_0x58447C, 2, 0xE, 0xF, // Tileset 0x06
        WL4Constants::Tileset_BGTile_0x06, 0x5220, 0x16E, 0,
        WL4Constants::BG_0x5FC2D0, 0x6D0, 1,
        WL4Constants::Tileset_Palette_0x58447C, 2, 0xE, 0xF, // Tileset 0x06
        WL4Constants::Tileset_BGTile_0x07, 0xFE0, 0x380, 0,
        WL4Constants::BG_0x6045C4, 0x508, 1,
        WL4Constants::Tileset_Palette_0x587A7C, 1, 0xF, // Tileset 0x07
        WL4Constants::Tileset_BGTile_0x08, 0x4EE0, 0x188, 0,
        WL4Constants::BG_0x5FFD94, 0x5F1, 1,
        WL4Constants::Tileset_Palette_0x58567C, 2, 0x5, 0xA, // Tileset 0x08, 0x3F
        WL4Constants::Tileset_BGTile_0x0A, 0x400, 0x3DF, 0,
        WL4Constants::BG_0x5FD078, 0x40C, 1,
        WL4Constants::Tileset_Palette_0x58467C, 1, 0x9, // Tileset 0x0A
        WL4Constants::Tileset_BGTile_0x0B, 0x2D20, 0x296, 0,
        WL4Constants::BG_0x5FD484, 0x1FB, 1,
        WL4Constants::Tileset_Palette_0x584A7C, 2, 0x4, 0x6, // Tileset 0x0B
        WL4Constants::Tileset_BGTile_0x0C, 0xFE0, 0x380, 0,
        WL4Constants::BG_0x603E98, 0x72A, 1,
        WL4Constants::Tileset_Palette_0x58767C, 1, 0xF, // Tileset 0x0C
        WL4Constants::Tileset_BGTile_0x0E, 0xF00, 0x387, 0,
        WL4Constants::BG_0x604ACC, 0x7A1, 1,
        WL4Constants::Tileset_Palette_0x587C7C, 1, 0xA, // Tileset 0x0E
        WL4Constants::Tileset_BGTile_0x0F, 0x2C20, 0x29E, 0,
        WL4Constants::BG_0x605A7C, 0x933, 1,
        WL4Constants::Tileset_Palette_0x587E7C, 1, 0xC, // Tileset 0x0F
        WL4Constants::Tileset_BGTile_0x0F, 0x2C20, 0x29E, 0,
        WL4Constants::BG_0x6063B0, 0x943, 1,
        WL4Constants::Tileset_Palette_0x587E7C, 1, 0xC, // Tileset 0x0F
        WL4Constants::Tileset_BGTile_0x0F, 0x2C20, 0x29E, 0,
        WL4Constants::BG_0x606CF4, 0x7CF, 1,
        WL4Constants::Tileset_Palette_0x587E7C, 1, 0xC, // Tileset 0x0F
        WL4Constants::Tileset_BGTile_0x10, 0x1000, 0x37F, 0,
        WL4Constants::BG_0x6074C4, 0x80C, 1,
        WL4Constants::Tileset_Palette_0x58807C, 1, 0xD, // Tileset 0x10
        WL4Constants::Tileset_BGTile_0x11, 0x24A0, 0x2DA, 0,
        WL4Constants::BG_0x5FA6D0, 0xBFB, 1,
        WL4Constants::Tileset_Palette_0x583C7C, 1, 0xF, // Tileset 0x11, 0x47, 0x50
        WL4Constants::Tileset_BGTile_0x12, 0x1BE0, 0x320, 0,
        WL4Constants::BG_0x60A1D8, 0xD, 1,
        WL4Constants::Tileset_Palette_0x58987C, 1, 0x0, // Tileset 0x12
        WL4Constants::Tileset_BGTile_0x13, 0x1B40, 0x325, 0,
        WL4Constants::BG_0x60A1E8, 0x167, 1,
        WL4Constants::Tileset_Palette_0x589A7C, 2, 0xA, 0xB, // Tileset 0x13
        WL4Constants::Tileset_BGTile_0x14, 0x2600, 0x2CF, 0,
        WL4Constants::BG_0x60A4B4, 0x85C, 1,
        WL4Constants::Tileset_Palette_0x58A27C, 1, 0xD, // Tileset 0x14
        WL4Constants::Tileset_BGTile_0x15, 0x4620, 0x1CE, 0,
        WL4Constants::BG_0x6094F4, 0x58D, 1,
        WL4Constants::Tileset_Palette_0x588C7C, 1, 0xF, // Tileset 0x15
        WL4Constants::Tileset_BGTile_0x17, 0x4620, 0x1CE, 0,
        WL4Constants::BG_0x6094F4, 0x58D, 1,
        WL4Constants::Tileset_Palette_0x58907C, 1, 0xF, // Tileset 0x17
        WL4Constants::Tileset_BGTile_0x18, 0x4620, 0x1CE, 0,
        WL4Constants::BG_0x6094F4, 0x58D, 1,
        WL4Constants::Tileset_Palette_0x58927C, 1, 0xF, // Tileset 0x18
        WL4Constants::Tileset_BGTile_0x19, 0x4620, 0x1CE, 0,
        WL4Constants::BG_0x6094F4, 0x58D, 1,
        WL4Constants::Tileset_Palette_0x58947C, 1, 0xF, // Tileset 0x19
        WL4Constants::Tileset_BGTile_0x1A, 0x1000, 0x37F, 0,
        WL4Constants::BG_0x607CD0, 0x80C, 1,
        WL4Constants::Tileset_Palette_0x58827C, 1, 0xE, // Tileset 0x1A
        WL4Constants::Tileset_BGTile_0x1B, 0x1000, 0x37F, 0,
        WL4Constants::BG_0x6084DC, 0x80C, 1,
        WL4Constants::Tileset_Palette_0x58847C, 1, 0xD, // Tileset 0x1B
        WL4Constants::Tileset_BGTile_0x1C, 0x1000, 0x37F, 0,
        WL4Constants::BG_0x608CE8, 0x80C, 1,
        WL4Constants::Tileset_Palette_0x58867C, 1, 0xF, // Tileset 0x1C
        WL4Constants::Tileset_BGTile_0x1D, 0x45C0, 0x1D1, 0,
        WL4Constants::BG_0x600388, 0x33B, 1,
        WL4Constants::Tileset_Palette_0x585A7C, 2, 0x0, 0xF, // Tileset 0x1D
        WL4Constants::Tileset_BGTile_0x1E, 0x5040, 0x17D, 0,
        WL4Constants::BG_0x600EF8, 0x4DB, 1,
        WL4Constants::Tileset_Palette_0x58587C, 1, 0x5, // Tileset 0x1E, 0x40
        WL4Constants::Tileset_BGTile_0x1F, 0x3480, 0x25B, 0,
        WL4Constants::BG_0x5FD680, 0x33A, 1,
        WL4Constants::Tileset_Palette_0x584C7C, 1, 0xF, // Tileset 0x1F
        WL4Constants::Tileset_BGTile_0x1F, 0x3480, 0x25B, 0,
        WL4Constants::BG_0x5FD9BC, 0x64C, 1,
        WL4Constants::Tileset_Palette_0x584C7C, 1, 0xF, // Tileset 0x1F
        WL4Constants::Tileset_BGTile_0x20, 0x4000, 0x1FF, 0,
        WL4Constants::BG_0x6006C4, 0x833, 1,
        WL4Constants::Tileset_Palette_0x585C7C, 1, 0xF, // Tileset 0x20
        WL4Constants::Tileset_BGTile_0x21, 0xF60, 0x384, 0,
        WL4Constants::BG_0x6013D4, 0x47F, 1,
        WL4Constants::Tileset_Palette_0x58647C, 2, 0xE, 0xF, // Tileset 0x21
        WL4Constants::Tileset_BGTile_0x21, 0xF60, 0x384, 0,
        WL4Constants::ToxicLandfillDustyLayer0Ptr, 0x1B8, 1,
        WL4Constants::Tileset_Palette_0x58647C, 1, 0xA, // Tileset 0x21, vanilla Layer 0 mapping data
        WL4Constants::Tileset_BGTile_0x22, 0x1260, 0x36C, 0,
        WL4Constants::BG_0x601A0C, 0x80F, 1,
        WL4Constants::Tileset_Palette_0x58667C, 1, 0xF, // Tileset 0x22
        WL4Constants::Tileset_BGTile_0x26, 0x4140, 0x1F5, 0,
        WL4Constants::BG_0x5FE918, 0x5BF, 1,
        WL4Constants::Tileset_Palette_0x58507C, 1, 0xA, // Tileset 0x26
        WL4Constants::Tileset_BGTile_0x26, 0x4140, 0x1F5, 0,
        WL4Constants::BG_0x5FEED8, 0x389, 1,
        WL4Constants::Tileset_Palette_0x58507C, 1, 0xA, // Tileset 0x26
        WL4Constants::Tileset_BGTile_0x27, 0x1BA0, 0x322, 0,
        WL4Constants::BG_0x60221C, 0x63A, 1,
        WL4Constants::Tileset_Palette_0x58687C, 1, 0xA, // Tileset 0x27
        WL4Constants::Tileset_BGTile_0x28, 0x1920, 0x336, 0,
        WL4Constants::BG_0x5FE540, 0x3D7, 1,
        WL4Constants::Tileset_Palette_0x584C7C, 1, 0x5, // Tileset 0x28
        WL4Constants::Tileset_BGTile_0x29, 0x1840, 0x33D, 0,
        WL4Constants::BG_0x60E860, 0xD, 1,
        WL4Constants::Tileset_Palette_0x58C07C, 1, 0x0, // Tileset 0x29
        WL4Constants::Tileset_BGTile_0x2F, 0x1CA0, 0x31A, 0,
        WL4Constants::BG_0x60AD10, 0x58C, 1,
        WL4Constants::Tileset_Palette_0x58AE7C, 3, 0x6, 0xD, 0xF, // Tileset 0x2F
        WL4Constants::Tileset_BGTile_0x34, 0x3660, 0x24C, 0,
        WL4Constants::BG_0x5FB8DC, 0x9F4, 1,
        WL4Constants::Tileset_Palette_0x58427C, 1, 0xE, // Tileset 0x34
        WL4Constants::Tileset_BGTile_0x35, 0x5000, 0x17F, 0,
        WL4Constants::BG_0x5FF264, 0x41E, 1,
        WL4Constants::Tileset_Palette_0x58507C, 1, 0xF, // Tileset 0x35
        WL4Constants::Tileset_BGTile_0x36, 0x3F80, 0x203, 0,
        WL4Constants::BG_0x5FF960, 0x432, 1,
        WL4Constants::Tileset_Palette_0x58547C, 1, 0x7, // Tileset 0x36
        WL4Constants::Tileset_BGTile_0x37, 0x3920, 0x236, 0,
        WL4Constants::BG_0x60BC54, 0x9A8, 1,
        WL4Constants::Tileset_Palette_0x58B07C, 1, 0xC, // Tileset 0x37
        WL4Constants::Tileset_BGTile_0x37, 0x3920, 0x236, 0,
        WL4Constants::BG_0x60B29C, 0x9B5, 1,
        WL4Constants::Tileset_Palette_0x58B07C, 1, 0xC, // Tileset 0x37
        WL4Constants::Tileset_BGTile_0x38, 0x4BE0, 0x1A0, 0,
        WL4Constants::BG_0x5FF684, 0x2D9, 1,
        WL4Constants::Tileset_Palette_0x58527C, 1, 0x8, // Tileset 0x38
        WL4Constants::Tileset_BGTile_0x39, 0x2220, 0x2EE, 0,
        WL4Constants::BG_0x60ED78, 0x96F, 1,
        WL4Constants::Tileset_Palette_0x58D67C, 3, 0xD, 0xE, 0xF, // Tileset 0x39
        WL4Constants::Tileset_BGTile_0x3E, 0xDE0, 0x390, 0,
        WL4Constants::BG_0x602858, 0x80C, 1,
        WL4Constants::Tileset_Palette_0x586A7C, 1, 0xD, // Tileset 0x3E
        WL4Constants::Tileset_BGTile_0x41, 0x9C0, 0x3B1, 0,
        WL4Constants::BG_0x609A84, 0x753, 1,
        WL4Constants::Tileset_Palette_0x58967C, 1, 0xF, // Tileset 0x41
        WL4Constants::Tileset_BGTile_0x42, 0x1840, 0x33D, 0,
        WL4Constants::BG_0x60E860, 0xD, 1,
        WL4Constants::Tileset_Palette_0x58BE7C, 1, 0x0, // Tileset 0x42
        WL4Constants::Tileset_BGTile_0x43, 0xC00, 0x39F, 0,
        WL4Constants::BG_0x60E96C, 0x40C, 1,
        WL4Constants::Tileset_Palette_0x58CA7C, 1, 0xD, // Tileset 0x43
        WL4Constants::Tileset_BGTile_0x44, 0x1840, 0x33D, 0,
        WL4Constants::BG_0x60E860, 0xD, 1,
        WL4Constants::Tileset_Palette_0x58C47C, 1, 0x0, // Tileset 0x44
        WL4Constants::Tileset_BGTile_0x45, 0x58E0, 0x138, 0,
        WL4Constants::BG_0x60CF98, 0x999, 1,
        WL4Constants::Tileset_Palette_0x58B27C, 1, 0xC, // Tileset 0x45
        WL4Constants::Tileset_BGTile_0x45, 0x58E0, 0x138, 0,
        WL4Constants::BG_0x60C5FC, 0x99C, 1,
        WL4Constants::Tileset_Palette_0x58B27C, 1, 0xC, // Tileset 0x45
        WL4Constants::Tileset_BGTile_0x45, 0x58E0, 0x138, 0,
        WL4Constants::FieryCavernDustyLayer0Ptr, 0x70F, 1,
        WL4Constants::Tileset_Palette_0x58B27C, 1, 0x9, // Tileset 0x45, vanilla Layer 0 mapping data
        WL4Constants::Tileset_BGTile_0x46, 0x5A0, 0x3D2, 0,
        WL4Constants::BG_0x60E044, 0x81A, 1,
        WL4Constants::Tileset_Palette_0x58B47C, 1, 0xD, // Tileset 0x46
        WL4Constants::Tileset_BGTile_0x48, 0x1840, 0x33D, 0,
        WL4Constants::BG_0x60E860, 0xD, 1,
        WL4Constants::Tileset_Palette_0x58C67C, 1, 0x0, // Tileset 0x48
        WL4Constants::Tileset_BGTile_0x49, 0x1840, 0x33D, 0,
        WL4Constants::BG_0x60E860, 0xD, 1,
        WL4Constants::Tileset_Palette_0x58C27C, 1, 0x0, // Tileset 0x49
        WL4Constants::Tileset_BGTile_0x4A, 0xC00, 0x39F, 0,
        WL4Constants::BG_0x60E96C, 0x40C, 1,
        WL4Constants::Tileset_Palette_0x58CC7C, 1, 0xD, // Tileset 0x4A
        WL4Constants::Tileset_BGTile_0x4B, 0xC00, 0x39F, 0,
        WL4Constants::BG_0x60E96C, 0x40C, 1,
        WL4Constants::Tileset_Palette_0x58CE7C, 1, 0xD, // Tileset 0x4B
        WL4Constants::Tileset_BGTile_0x4C, 0xC00, 0x39F, 0,
        WL4Constants::BG_0x60E96C, 0x40C, 1,
        WL4Constants::Tileset_Palette_0x58D07C, 1, 0xD, // Tileset 0x4C
        WL4Constants::Tileset_BGTile_0x4D, 0xC00, 0x39F, 0,
        WL4Constants::BG_0x60E96C, 0x40C, 1,
        WL4Constants::Tileset_Palette_0x58D27C, 1, 0xD, // Tileset 0x4D
        WL4Constants::Tileset_BGTile_0x4E, 0xC00, 0x39F, 0,
        WL4Constants::BG_0x60E96C, 0x40C, 1,
        WL4Constants::Tileset_Palette_0x58D47C, 1, 0xD, // Tileset 0x4E
        WL4Constants::Tileset_BGTile_0x4F, 0x280, 0x3EB, 0,
        WL4Constants::BG_0x60E870, 0xFB, 1,
        WL4Constants::Tileset_Palette_0x58C87C, 1, 0x1, // Tileset 0x4F
        WL4Constants::Tileset_BGTile_0x51, 0x3480, 0x25B, 0,
        WL4Constants::BG_0x5FE008, 0x536, 1,
        WL4Constants::Tileset_Palette_0x584E7C, 1, 0xF, // Tileset 0x51
        WL4Constants::Tileset_BGTile_0x52, 0x2D20, 0x296, 0,
        WL4Constants::BG_0x5FD484, 0x1FB, 1,
        WL4Constants::Tileset_Palette_0x58467C, 2, 0x4, 0x6, // Tileset 0x52
        WL4Constants::Tileset_BGTile_0x53, 0x1B40, 0x325, 0,
        WL4Constants::BG_0x60A350, 0x163, 1,
        WL4Constants::Tileset_Palette_0x589C7C, 2, 0xA, 0xB, // Tileset 0x53
        WL4Constants::Tileset_BGTile_0x54, 0x1B40, 0x325, 0,
        WL4Constants::BG_0x60A1E8, 0x167, 1,
        WL4Constants::Tileset_Palette_0x589E7C, 2, 0xA, 0xB, // Tileset 0x54
        WL4Constants::Tileset_BGTile_0x55, 0x1B40, 0x325, 0,
        WL4Constants::BG_0x60A1E8, 0x167, 1,
        WL4Constants::Tileset_Palette_0x58A07C, 2, 0xA, 0xB, // Tileset 0x55
        WL4Constants::Tileset_BGTile_0x56, 0x880, 0x3BB, 0,
        WL4Constants::BG_0x605270, 0x80C, 1,
        WL4Constants::Tileset_Palette_0x58767C, 1, 0xA, // Tileset 0x56
    };
    // clang-format on

    // functions
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> GetAssortedGraphicsFromROM();

    // Create the vanilla assorted graphic entries from VanillaAssortedGraphicEntryData. The entry
    // names are generated from the current Tilesets, the tile, palette and mapping data are not
    // loaded from the ROM.
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> GetVanillaAssortedGraphicEntries();

    // Create the vanilla assorted graphic entries from VanillaAssortedGraphicEntryData and load
    // their tile, palette and mapping data from the currently opened ROM.
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> GetVanillaAssortedGraphicsFromROM();

    void ExtractDataFromEntryInfo_v2(struct AssortedGraphicUtils::AssortedGraphicEntryItem &entry);
    QString SaveAssortedGraphicsToROM(QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> &entries);

    // Collect the mapping data addresses usable by the background Tile8x8 set of the given Tileset.
    // Any graphic entry whose Tile8x8 set is identical to the Tileset background Tile8x8 set is
    // treated as compatible, even when the entries have different ROM data pointers. This way the
    // duplicated entries generated by the Graphic Manager (one Tile8x8 set, several identical
    // copies, several mapping data chunks) share their mapping data options, so both Layer 0 and
    // Layer 3 of a room can use any of those mapping data chunks.
    QVector<unsigned int> FindCompatibleMappingDataAddresses(LevelComponents::Tileset *tileset);

    // savechunk relative functions
    QVector<unsigned int> GetSaveDataAddresses(AssortedGraphicEntryItem &entry);
    QVector<struct ROMUtils::SaveData> CreateSaveData(AssortedGraphicEntryItem &entry, unsigned int entryId);

    // helper functions
    bool CheckEditability(struct AssortedGraphicUtils::AssortedGraphicEntryItem &entry, unsigned int &find_l, unsigned int &find_r, unsigned int &find_t);
    bool FindbgGFXptrInAllTilesets(unsigned int address, unsigned int *tilesetId_find);
    bool FindLayerptrInAllRooms(unsigned int address, unsigned int *levelId_found, unsigned int *roomId_found);
};

#endif // ASSORTEDGRAPHICUTILS_H
