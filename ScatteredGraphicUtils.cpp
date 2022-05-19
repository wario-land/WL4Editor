#include "ScatteredGraphicUtils.h"
#include "ROMUtils.h"
#include "WL4EditorWindow.h"

#define ScatteredGraphic_CHUNK_VERSION 0
#define ScatteredGraphic_FIELD_COUNT 14

extern WL4EditorWindow *singleton;

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
        static_cast<enum ScatteredGraphicUtils::ScatteredGraphicTileDataType>(scatteredgraphicTuples[3].toUInt(Q_NULLPTR, 16)), // TileDataCompressType
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

/// <summary>
/// Serialize a scattered graphic struct into a metadata string for the scattered graphic list chunk.
/// </summary>
/// <param name="scatteredGraphicMetadata">
/// The metadata struct to serialize.
/// See the format specified in the documentation for UpgradeScatteredGraphicListContents.
/// </param>
/// <returns>
/// The metadata string.
/// </returns>
static QString SerializeScatteredGraphicMetadata(const struct ScatteredGraphicUtils::ScatteredGraphicEntryItem &scatteredGraphicMetadata)
{
    QString ret = QString::number(scatteredGraphicMetadata.TileDataAddress, 16) + ";";
    ret += QString::number(scatteredGraphicMetadata.TileDataSize_Byte, 16) + ";";
    ret += QString::number(scatteredGraphicMetadata.TileDataRAMOffsetNum, 16) + ";";
    ret += QString::number(scatteredGraphicMetadata.TileDataCompressType, 16) + ";";
    ret += scatteredGraphicMetadata.TileDataName + ";";
    ret += QString::number(scatteredGraphicMetadata.MappingDataAddress, 16) + ";";
    ret += QString::number(scatteredGraphicMetadata.MappingDataSize_Byte, 16) + ";";
    ret += QString::number(scatteredGraphicMetadata.MappingDataCompressType, 16) + ";";
    ret += scatteredGraphicMetadata.MappingDataName + ";";
    ret += QString::number(scatteredGraphicMetadata.PaletteAddress, 16) + ";";
    ret += QString::number(scatteredGraphicMetadata.PaletteNum, 16) + ";";
    ret += QString::number(scatteredGraphicMetadata.PaletteRAMOffsetNum, 16) + ";";
    ret += QString::number(scatteredGraphicMetadata.optionalGraphicWidth, 16) + ";";
    ret += QString::number(scatteredGraphicMetadata.optionalGraphicHeight, 16);

    return ret;
}

/// <summary>
/// Obtain the scattered graphic list chunk contents in the current version's format.
/// </summary>
/// <param name="scatteredGraphicDataAddr">
/// The address where the scattered graphic data (including header) starts.
/// </param>
/// <returns>
/// The updated scattered graphic list chunk's contents, regardless of the version in the ROM.
/// </returns>
static QString GetUpgradedScatteredGraphicListChunkData(unsigned int scatteredGraphicDataAddr)
{

    unsigned short contentSize = ROMUtils::GetChunkDataLength(scatteredGraphicDataAddr) - 1;
    int chunkVersion = ROMUtils::ROMFileMetadata->ROMDataPtr[scatteredGraphicDataAddr + 12];
    if(chunkVersion > ScatteredGraphic_CHUNK_VERSION)
    {
        singleton->GetOutputWidgetPtr()->PrintString(QString(QT_TR_NOOP("Scattered graphic list chunk either corrupt or this verison of WL4Editor is old and doesn't support the saved format. Version found: ")) + QString::number(chunkVersion));
        return "";
    }
    QString contents = QString::fromLocal8Bit(reinterpret_cast<const char*>(ROMUtils::ROMFileMetadata->ROMDataPtr + scatteredGraphicDataAddr + 13), contentSize);
    while(chunkVersion < ScatteredGraphic_CHUNK_VERSION)
    {
        contents = UpgradeScatteredGraphicListContents(contents, chunkVersion++);
    }
    return contents;
}

/// <summary>
/// Obtain the ScatteredGraphic entries from the currently loaded ROM file.
/// </summary>
/// <returns>
/// A list of all ScatteredGraphic entries, which is empty if none exist.
/// </returns>
QVector<struct ScatteredGraphicUtils::ScatteredGraphicEntryItem> ScatteredGraphicUtils::GetScatteredGraphicsFromROM()
{
    // Obtain the ScatteredGraphic list chunk, if it exists
    QVector<struct ScatteredGraphicUtils::ScatteredGraphicEntryItem> scatteredGraphicEntries;
    unsigned int scatteredGraphicListAddr = ROMUtils::FindChunkInROM(
        ROMUtils::ROMFileMetadata->ROMDataPtr,
        ROMUtils::ROMFileMetadata->Length,
        WL4Constants::AvailableSpaceBeginningInROM,
        ROMUtils::SaveDataChunkType::ScatteredGraphicListChunkType
    );
    if(scatteredGraphicListAddr)
    {
        // Obtain the ScatteredGraphic list info
        QString contents = GetUpgradedScatteredGraphicListChunkData(scatteredGraphicListAddr);
        if(!contents.length())
        {
            singleton->GetOutputWidgetPtr()->PrintString(QT_TR_NOOP("ROM contains an empty ScatteredGraphic list chunk (this should not be possible)"));
            return scatteredGraphicEntries;
        }
        QStringList scatteredGraphicTuples = contents.split(";");
        if(scatteredGraphicTuples.count() % ScatteredGraphic_FIELD_COUNT)
        {
            singleton->GetOutputWidgetPtr()->PrintString(QT_TR_NOOP("ROM contains a corrupted ScatteredGraphic list chunk (field count is not a multiple of ") + QString::number(ScatteredGraphic_FIELD_COUNT) + ")");
            return scatteredGraphicEntries;
        }
        for(int i = 0; i < scatteredGraphicTuples.count(); i += ScatteredGraphic_FIELD_COUNT)
        {
            struct ScatteredGraphicUtils::ScatteredGraphicEntryItem entry =
                    DeserializeScatteredGraphicMetadata(scatteredGraphicTuples.mid(i, ScatteredGraphic_FIELD_COUNT));
            // TODO: add validation check logic for each sub chunk

            scatteredGraphicEntries.append(entry);
        }
    }
    return scatteredGraphicEntries;
}
