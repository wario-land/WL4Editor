#include "ScatteredGraphicUtils.h"
#include "ROMUtils.h"

#define ScatteredGraphic_CHUNK_VERSION 0
#define ScatteredGraphic_FIELD_COUNT 14

/// <summary>
/// Upgrade the format of a scattered graphic list chunk by one version.
/// </summary>
/// <remarks>
/// Version 0:
///   Semicolon-delimited:
///      0: TileDataAddress         (hex string)
///      1: TileDataSize_Byte       (hex string)
///      2: TileDataRAMOffsetNum    (hex string)
///      3: TileDataCompressType    (hex string)
///      4: TileDataName            (ascii string, may not contain a semicolon)
///      5: MappingDataAddress      (hex string)
///      6: MappingDataSize_Byte    (hex string)
///      7: MappingDataCompressType (hex string)
///      8: MappingDataName         (ascii string, may not contain a semicolon)
///      9: optionalPaletteAddress  (hex string)
///     10: PaletteNum              (hex string)
///     11: PaletteRAMOffsetNum     (hex string)
///     12: optionalGraphicWidth    (hex string)
///     13: optionalGraphicHeight   (hex string)
/// </remarks>
/// <param name="contents">
/// The scattered graphic list chunk contents to upgrade.
/// </param>
/// <param name="version">
/// The version of the data being passed in.
/// </param>
/// <returns>
/// The scattered graphic list chunk contents, upgraded one version.
/// </returns>
static QString UpgradeScatteredGraphicListContents(QString contents, int version)
{
    switch(version)
    {
        case 0:
            // TODO implement when there are new versions
            break;
    }
    return contents;
}

/// <summary>
/// Deserialize scattered graphic metadata from the scattered graphic list chunk into a scattered graphic metadata struct.
/// </summary>
/// <param name="scatteredgraphicTuples">
/// The string list of the fields for the patch metadata.
/// See the format specified in the documentation for UpgradePatchListContents.
/// </param>
/// <returns>
/// The patch metadata struct.
/// </returns>
static struct ScatteredGraphicUtils::ScatteredGraphicEntryItem DeserializeScatteredGraphicMetadata(QStringList scatteredgraphicTuples)
{
    struct ScatteredGraphicUtils::ScatteredGraphicEntryItem entry
    {
        scatteredgraphicTuples[0].toUInt(Q_NULLPTR, 16), // TileDataAddress
        scatteredgraphicTuples[1].toUInt(Q_NULLPTR, 16), // TileDataSize_Byte
        scatteredgraphicTuples[2].toUInt(Q_NULLPTR, 16), // TileDataRAMOffsetNum
        static_cast<enum ScatteredGraphicUtils::ScatteredGraphicTileDataCompressionType>(scatteredgraphicTuples[3].toUInt(Q_NULLPTR, 16)), // TileDataCompressType
        scatteredgraphicTuples[4], // TileDataName
        scatteredgraphicTuples[5].toUInt(Q_NULLPTR, 16), // MappingDataAddress
        scatteredgraphicTuples[6].toUInt(Q_NULLPTR, 16), // MappingDataSize_Byte
        static_cast<enum ScatteredGraphicUtils::ScatteredGraphicMappingDataCompressionType>(scatteredgraphicTuples[7].toUInt(Q_NULLPTR, 16)), // MappingDataCompressType
        scatteredgraphicTuples[8], // MappingDataName
        scatteredgraphicTuples[9].toUInt(Q_NULLPTR, 16), // optionalPaletteAddress
        scatteredgraphicTuples[10].toUInt(Q_NULLPTR, 16), // PaletteNum
        scatteredgraphicTuples[11].toUInt(Q_NULLPTR, 16), // PaletteRAMOffsetNum
        scatteredgraphicTuples[12].toUInt(Q_NULLPTR, 16), // optionalGraphicWidth
        scatteredgraphicTuples[13].toUInt(Q_NULLPTR, 16), // optionalGraphicHeight
    };
    return entry;
}
