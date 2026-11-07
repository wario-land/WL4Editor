#include "AssortedGraphicUtils.h"
#include "LevelComponents/Layer.h"
#include "WL4EditorWindow.h"

#define AssortedGraphic_CHUNK_VERSION 1
#define AssortedGraphic_FIELD_COUNT 13

extern WL4EditorWindow *singleton;

/// <summary>
/// Get the raw background Tile8x8 data of a Tileset.
/// </summary>
/// <remarks>
/// The Tile8x8 id used by the mapping data of a Tile8x8 layer is the index of the Tileset Tile8x8
/// array offset by 0x200 (see LevelComponents::Layer::RenderLayer), so the returned data is indexed
/// by the Tile8x8 id itself.
/// </remarks>
/// <param name="tileset">
/// The Tileset which provides the background Tile8x8 data.
/// </param>
/// <param name="result">
/// The raw data of the background Tile8x8 set, in the order of the Tile8x8 ids.
/// </param>
/// <returns>
/// Return true if the Tile8x8 data is obtained.
/// </returns>
bool AssortedGraphicUtils::GetTilesetBGTile8x8RawData(LevelComponents::Tileset *tileset, QByteArray &result)
{
    if (result.size())
    {
        result.clear();
    }
    if (!tileset)
    {
        return false;
    }
    QVector<LevelComponents::Tile8x8 *> tile8x8array = tileset->GetTile8x8arrayPtr();
    int tileNum = tileset->GetbgGFXlen() / 32;
    if ((tileNum <= 0) || (tileNum > 0x3FF) || (0x600 > tile8x8array.size()))
    {
        return false;
    }

    // The result is indexed by the raw Tile8x8 id used by the mapping data. The background
    // Tile8x8 set occupies the raw ids [0x3FF - tileNum, 0x3FE], which correspond to the
    // Tile8x8 array indices [0x5FF - tileNum, 0x5FE].
    const int startId = 0x3FF - tileNum;
    result = QByteArray(0x400 * 32, 0);
    for (int i = 0; i < tileNum; ++i)
    {
        const int tileId = startId + i;
        const int arrayIndex = 0x200 + tileId;
        if ((tileId < 0) || (tileId >= 0x400) || (arrayIndex >= tile8x8array.size()))
        {
            break;
        }
        memcpy(result.data() + tileId * 32,
               tile8x8array[arrayIndex]->GetRawPixelData().constData(), 32);
    }
    return true;
}

/// <summary>
/// Check if a Tile8x8 set uses the same background Tile8x8 data as the one of a Tileset.
/// </summary>
/// <remarks>
/// The background Tile8x8 data of a Tileset is the data the game loads as its background Tile8x8
/// set, and the mapping data of a Tile8x8 layer only references background Tile8x8 ids, palettes
/// and flips. Tile8x8 sets sharing the same data are therefore interchangeable, even when their
/// ROM data pointers differ. Such Tile8x8 sets must not be merged into one ROM data chunk though,
/// because that would turn the chunk into a duplicated reference (see ChunkUtils::DuplicateRef).
/// </remarks>
/// <param name="tileset">
/// The Tileset which provides the background Tile8x8 set to compare with.
/// </param>
/// <param name="tileDataAddr">
/// The ROM data address of the Tile8x8 set to check. 0 when the set has no address in the ROM.
/// </param>
/// <param name="tileDataSizeInByte">
/// The size of the Tile8x8 set to check, in byte.
/// </param>
/// <param name="tileDataRAMOffsetNum">
/// The Tile8x8 id of the first Tile8x8 of the set to check.
/// </param>
/// <param name="tileData">
/// The Tile8x8 set to check.
/// </param>
/// <returns>
/// Return true if the Tile8x8 set is the one used by the Tileset.
/// </returns>
static bool DoesTilesetUseTile8x8Set(LevelComponents::Tileset *tileset, unsigned int tileDataAddr,
                                     unsigned int tileDataSizeInByte, unsigned int tileDataRAMOffsetNum,
                                     const QByteArray &tileData, const QByteArray &tilesetTileData)
{
    if (!tileDataSizeInByte || (tileDataSizeInByte % 32) || ((unsigned int) tileData.size() != tileDataSizeInByte))
    {
        // there is no usable Tile8x8 data to compare with
        return false;
    }

    // The Tileset points at the data of this Tile8x8 set, so the Tileset uses the set no matter
    // what the Tile8x8 data looks like. This also covers the Tile8x8 set which has been detached
    // from its ROM data address (for example by the Graphic Manager).
    if (tileDataAddr && (tileDataAddr == (unsigned int) tileset->GetbgGFXptr()))
    {
        return true;
    }

    // Compare the Tile8x8 data, the background Tile8x8 set of a Tileset has a fixed size
    if ((int) tileDataSizeInByte > tileset->GetbgGFXlen())
    {
        return false;
    }
    int tileNum = tileDataSizeInByte / 32;
    for (int i = 0; i < tileNum; ++i)
    {
        // The mapping data of a Tile8x8 layer uses the Tile8x8 id straight as the index of the
        // Tileset Tile8x8 array (see LevelComponents::Layer::RenderLayer), while a Tile8x8 set
        // stores its data from its own TileDataRAMOffsetNum on, so this is the index to compare
        int tileId = tileDataRAMOffsetNum + i;
        if ((tileId >= 0x400) || (((tileId + 1) * 32) > tilesetTileData.size()))
        {
            return false;
        }
        if (memcmp(tilesetTileData.constData() + tileId * 32, tileData.constData() + i * 32, 32))
        {
            return false;
        }
    }
    return true;
}

/// <summary>
/// Upgrade the format of a assorted graphic list chunk by one version.
/// </summary>
/// <remarks>
/// Version 0 (14 fields per entry):
///   Semicolon-delimited:
///      0: TileDataAddress         (hex string)
///      1: TileDataSizeInByte       (hex string)
///      2: TileDataRAMOffsetNum    (hex string)
///      3: TileDataCompressType    (hex string)
///      4: TileDataName            (ascii string, may not contain a semicolon)
///      5: MappingDataAddress      (hex string)
///      6: MappingDataSizeInByte    (hex string)
///      7: MappingDataCompressType (hex string)
///      8: MappingDataName         (ascii string, may not contain a semicolon)
///      9: optionalPaletteAddress  (hex string)
///     10: PaletteNum              (hex string)
///     11: PaletteRAMOffsetNum     (hex string)
///     12: optionalGraphicWidth    (hex string)
///     13: optionalGraphicHeight   (hex string)
///
/// Version 1 (13 fields per entry):
///   Semicolon-delimited:
///      0: TileDataAddress         (hex string)
///      1: TileDataSizeInByte       (hex string)
///      2: TileDataRAMOffsetNum    (hex string)
///      3: TileDataCompressType    (hex string)
///      4: TileDataName            (ascii string, may not contain a semicolon)
///      5: MappingDataAddress      (hex string)
///      6: MappingDataSizeInByte    (hex string)
///      7: MappingDataCompressType (hex string)
///      8: MappingDataName         (ascii string, may not contain a semicolon)
///      9: optionalPaletteAddress  (hex string)
///     10: PaletteSlotIDs          (comma-separated hex, e.g. "1,3,5")
///     11: optionalGraphicWidth    (hex string)
///     12: optionalGraphicHeight   (hex string)
/// </remarks>
/// <param name="contents">
/// The assorted graphic list chunk contents to upgrade.
/// </param>
/// <param name="version">
/// The version of the data being passed in.
/// </param>
/// <returns>
/// The assorted graphic list chunk contents, upgraded one version.
/// </returns>
static QString UpgradeAssortedGraphicListContents(QString contents, int version)
{
    switch(version)
    {
        case 0:
        {
            // Version 0 -> 1: Convert PaletteNum (field 10) + PaletteRAMOffsetNum (field 11)
            // into PaletteSlotIDs comma-separated hex (new field 10).
            // Fields 12,13 (optionalGraphicWidth, optionalGraphicHeight) shift to 11,12.
            // Total fields per entry goes from 14 to 13.
            QStringList tuples = contents.split(";");
            if (tuples.count() % 14 != 0) return contents; // corrupted, leave as-is

            QStringList newTuples;
            for (int i = 0; i < tuples.count(); i += 14)
            {
                // Fields 0-9 stay the same
                for (int j = 0; j < 10; j++)
                    newTuples.append(tuples[i + j]);

                // Convert field 10 (PaletteNum) and field 11 (PaletteRAMOffsetNum)
                // into comma-separated hex palette slot IDs
                bool ok1, ok2;
                unsigned int palNum = tuples[i + 10].toUInt(&ok1, 16);
                unsigned int palOffset = tuples[i + 11].toUInt(&ok2, 16);
                QString slotIDs;
                if (ok1 && ok2)
                {
                    for (unsigned int k = 0; k < palNum && (k + palOffset) < 16; k++)
                    {
                        if (k > 0) slotIDs += ",";
                        slotIDs += QString::number(k + palOffset, 16);
                    }
                }
                if (slotIDs.isEmpty()) slotIDs = "0";
                newTuples.append(slotIDs);

                // Fields 12,13 become 11,12
                newTuples.append(tuples[i + 12]); // optionalGraphicWidth
                newTuples.append(tuples[i + 13]); // optionalGraphicHeight
            }
            return newTuples.join(";");
        }
    }
    return contents;
}

/// <summary>
/// Deserialize assorted graphic metadata from the assorted graphic list chunk into a assorted graphic metadata struct.
/// </summary>
/// <param name="assortedgraphicTuples">
/// The string list of the fields for the patch metadata.
/// See the format specified in the documentation for UpgradePatchListContents.
/// </param>
/// <returns>
/// The patch metadata struct.
/// </returns>
static struct AssortedGraphicUtils::AssortedGraphicEntryItem DeserializeAssortedGraphicMetadata(QStringList assortedgraphicTuples)
{
    struct AssortedGraphicUtils::AssortedGraphicEntryItem entry;
    entry.TileDataAddress = assortedgraphicTuples[0].toUInt(Q_NULLPTR, 16); // TileDataAddress
    entry.TileDataSizeInByte = assortedgraphicTuples[1].toUInt(Q_NULLPTR, 16); // TileDataSizeInByte
    entry.TileDataRAMOffsetNum = assortedgraphicTuples[2].toUInt(Q_NULLPTR, 16); // TileDataRAMOffsetNum
    entry.TileDataType = static_cast<enum AssortedGraphicUtils::AssortedGraphicTileDataType>(assortedgraphicTuples[3].toUInt(Q_NULLPTR, 16)); // TileDataCompressType
    entry.TileDataName = assortedgraphicTuples[4]; // TileDataName
    entry.MappingDataAddress = assortedgraphicTuples[5].toUInt(Q_NULLPTR, 16); // MappingDataAddress
    entry.MappingDataSizeAfterCompressionInByte = assortedgraphicTuples[6].toUInt(Q_NULLPTR, 16); // MappingDataSizeInByte
    entry.MappingDataCompressType = static_cast<enum AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType>(assortedgraphicTuples[7].toUInt(Q_NULLPTR, 16)); // MappingDataCompressType
    entry.MappingDataName = assortedgraphicTuples[8]; // MappingDataName
    entry.PaletteAddress = assortedgraphicTuples[9].toUInt(Q_NULLPTR, 16); // optionalPaletteAddress

    // Parse PaletteSlotIDs (comma-separated hex, e.g. "1,3,5")
    QStringList slotIDStrings = assortedgraphicTuples[10].split(",", Qt::SkipEmptyParts);
    for (const QString &s : slotIDStrings)
    {
        bool ok;
        unsigned int slotId = s.toUInt(&ok, 16);
        if (ok && slotId < 16)
            entry.PaletteSlotIDs.push_back(slotId);
    }
    if (entry.PaletteSlotIDs.isEmpty())
        entry.PaletteSlotIDs.push_back(0); // fallback

    entry.optionalGraphicWidth = assortedgraphicTuples[11].toUInt(Q_NULLPTR, 16); // optionalGraphicWidth
    entry.optionalGraphicHeight = assortedgraphicTuples[12].toUInt(Q_NULLPTR, 16); // optionalGraphicHeight
    return entry;
}

/// <summary>
/// Serialize a assorted graphic struct into a metadata string for the assorted graphic list chunk.
/// </summary>
/// <param name="assortedGraphicMetadata">
/// The metadata struct to serialize.
/// See the format specified in the documentation for UpgradeAssortedGraphicListContents.
/// </param>
/// <returns>
/// The metadata string.
/// </returns>
static QString SerializeAssortedGraphicMetadata(const struct AssortedGraphicUtils::AssortedGraphicEntryItem &assortedGraphicMetadata)
{
    QString ret = QString::number(assortedGraphicMetadata.TileDataAddress, 16) + ";";
    ret += QString::number(assortedGraphicMetadata.TileDataSizeInByte, 16) + ";";
    ret += QString::number(assortedGraphicMetadata.TileDataRAMOffsetNum, 16) + ";";
    ret += QString::number(assortedGraphicMetadata.TileDataType, 16) + ";";
    ret += assortedGraphicMetadata.TileDataName + ";";
    ret += QString::number(assortedGraphicMetadata.MappingDataAddress, 16) + ";";
    ret += QString::number(assortedGraphicMetadata.MappingDataSizeAfterCompressionInByte, 16) + ";";
    ret += QString::number(assortedGraphicMetadata.MappingDataCompressType, 16) + ";";
    ret += assortedGraphicMetadata.MappingDataName + ";";
    ret += QString::number(assortedGraphicMetadata.PaletteAddress, 16) + ";";

    // Serialize PaletteSlotIDs as comma-separated hex
    QString slotIDs;
    for (int i = 0; i < assortedGraphicMetadata.PaletteSlotIDs.size(); i++)
    {
        if (i > 0) slotIDs += ",";
        slotIDs += QString::number(assortedGraphicMetadata.PaletteSlotIDs[i], 16);
    }
    if (slotIDs.isEmpty()) slotIDs = "0";
    ret += slotIDs + ";";

    ret += QString::number(assortedGraphicMetadata.optionalGraphicWidth, 16) + ";";
    ret += QString::number(assortedGraphicMetadata.optionalGraphicHeight, 16);

    return ret;
}

/// <summary>
/// Create the data for a assorted Graphic List Chunk.
/// </summary>
/// <param name="entries">
/// The entries used to create the assorted Graphic List Chunk.
/// </param>
/// <returns>
/// The data as an allocated char array.
/// </returns>
static QString CreateAssortedGraphicListChunkData(QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> &entries)
{
    QString contents;
    bool first = true;
    for(const struct AssortedGraphicUtils::AssortedGraphicEntryItem &entry : entries)
    {
        // Delimit entries with semicolon
        if(first) first = false; else contents += ";";

        contents += SerializeAssortedGraphicMetadata(entry);
    }
    return contents;
}

/// <summary>
/// Obtain the assorted graphic list chunk contents in the current version's format.
/// </summary>
/// <param name="assortedGraphicDataAddr">
/// The address where the assorted graphic data (including header) starts.
/// </param>
/// <returns>
/// The updated assorted graphic list chunk's contents, regardless of the version in the ROM.
/// </returns>
static QString GetUpgradedAssortedGraphicListChunkData(unsigned int assortedGraphicDataAddr)
{

    unsigned short contentSize = ROMUtils::GetChunkDataLength(assortedGraphicDataAddr) - 1;
    int chunkVersion = ROMUtils::ROMFileMetadata->ROMDataPtr[assortedGraphicDataAddr + 12];
    if(chunkVersion > AssortedGraphic_CHUNK_VERSION)
    {
        singleton->GetOutputWidgetPtr()->PrintString(QString(QT_TR_NOOP("assorted graphic list chunk either corrupt or this verison of WL4Editor is old and doesn't support the saved format. Version found: ")) + QString::number(chunkVersion));
        return "";
    }
    QString contents = QString::fromLocal8Bit(reinterpret_cast<const char*>(ROMUtils::ROMFileMetadata->ROMDataPtr + assortedGraphicDataAddr + 13), contentSize);
    while(chunkVersion < AssortedGraphic_CHUNK_VERSION)
    {
        contents = UpgradeAssortedGraphicListContents(contents, chunkVersion++);
    }
    return contents;
}

/// <summary>
/// Generate the TileDataName and MappingDataName of a vanilla assorted graphic entry.
/// </summary>
/// <remarks>
/// The TileDataName lists every Tileset which uses the same background Tile8x8 set with the same
/// palette as the entry, so all the Tileset ids of a graphic shared by several Tilesets are
/// visible in the entry list. The MappingDataName names the vanilla layer the mapping data is
/// used by: the two Layer 0 mapping data stored in WL4Constants are "Layer 0", every other vanilla
/// mapping data is used by Layer 3.
/// </remarks>
/// <param name="entry">
/// The vanilla entry to generate the names for.
/// </param>
static void GenerateVanillaAssortedGraphicEntryNames(struct AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    QStringList tilesetIds;
    for (unsigned int i = 0; i < sizeof(ROMUtils::singletonTilesets) / sizeof(ROMUtils::singletonTilesets[0]); ++i)
    {
        LevelComponents::Tileset *tileset = ROMUtils::singletonTilesets[i];
        if (tileset && ((unsigned int) tileset->GetbgGFXptr() == entry.TileDataAddress)
            && ((unsigned int) tileset->GetPaletteAddr() == entry.PaletteAddress)
            && ((unsigned int) tileset->GetbgGFXlen() == entry.TileDataSizeInByte))
        {
            tilesetIds.append("0x" + QString::number(i, 16).toUpper().rightJustified(2, QLatin1Char('0')));
        }
    }

    entry.TileDataName = tilesetIds.size()
            ? ("vanilla Tileset " + tilesetIds.join(", ") + " bg tiles")
            : QString("vanilla bg tiles");
    entry.MappingDataName = ((entry.MappingDataAddress == WL4Constants::ToxicLandfillDustyLayer0Ptr)
                             || (entry.MappingDataAddress == WL4Constants::FieryCavernDustyLayer0Ptr))
            ? "Layer 0"
            : "Layer 3";
}

/// <summary>
/// Obtain the assorted graphic entries from the currently loaded ROM file.
/// </summary>
/// <remarks>
/// The vanilla entries generated from VanillaAssortedGraphicEntryData are the base of the list.
/// The entries of the assorted graphic list chunk are appended afterwards. A chunk entry which
/// describes one of the vanilla graphics (the same TileData, mapping data and palette addresses)
/// is skipped, so the vanilla entry generated from the data table is the one which is kept.
/// </remarks>
/// <returns>
/// The vanilla and the customized assorted graphic entries of the ROM.
/// </returns>
QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> AssortedGraphicUtils::GetAssortedGraphicsFromROM()
{
    // Start with the complete set of vanilla graphic entries, so the vanilla data table is the
    // base of the list and the entries of the assorted graphic list chunk can be merged into it.
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> assortedGraphicEntries =
            GetVanillaAssortedGraphicsFromROM();
    const int vanillaEntryCount = static_cast<int>(assortedGraphicEntries.size());

    // Obtain the AssortedGraphic list chunk, if it exists
    unsigned int assortedGraphicListAddr = ROMUtils::FindChunkInROM(
        ROMUtils::ROMFileMetadata->ROMDataPtr,
        ROMUtils::ROMFileMetadata->Length,
        WL4Constants::AvailableSpaceBeginningInROM,
        ROMUtils::SaveDataChunkType::AssortedGraphicListChunkType
    );
    if (!assortedGraphicListAddr)
    {
        return assortedGraphicEntries;
    }

    // Obtain the AssortedGraphic list info
    QString contents = GetUpgradedAssortedGraphicListChunkData(assortedGraphicListAddr);
    if (!contents.length())
    {
        singleton->GetOutputWidgetPtr()->PrintString(QT_TR_NOOP("ROM contains an empty AssortedGraphic list chunk (this should not be possible)"));
        return assortedGraphicEntries;
    }
    QStringList assortedGraphicTuples = contents.split(";");
    if (assortedGraphicTuples.count() % AssortedGraphic_FIELD_COUNT)
    {
        singleton->GetOutputWidgetPtr()->PrintString(QT_TR_NOOP("ROM contains a corrupted AssortedGraphic list chunk (field count is not a multiple of ") + QString::number(AssortedGraphic_FIELD_COUNT) + ")");
        return assortedGraphicEntries;
    }
    for (int i = 0; i < assortedGraphicTuples.count(); i += AssortedGraphic_FIELD_COUNT)
    {
        struct AssortedGraphicUtils::AssortedGraphicEntryItem entry =
                DeserializeAssortedGraphicMetadata(assortedGraphicTuples.mid(i, AssortedGraphic_FIELD_COUNT));
        // TODO: add validation check logic for each sub chunk

        // A chunk entry which describes the same graphic as a vanilla entry is skipped, so the
        // vanilla entry generated from the data table is the one which is kept
        bool isVanillaEntry = false;
        for (int j = 0; j < vanillaEntryCount; ++j)
        {
            if ((entry.TileDataAddress == assortedGraphicEntries[j].TileDataAddress)
                && (entry.MappingDataAddress == assortedGraphicEntries[j].MappingDataAddress)
                && (entry.PaletteAddress == assortedGraphicEntries[j].PaletteAddress))
            {
                isVanillaEntry = true;
                break;
            }
        }
        if (isVanillaEntry)
        {
            continue;
        }

        // Extract Data from Infos
        ExtractDataFromEntryInfo_v2(entry);

        assortedGraphicEntries.append(entry);
    }
    return assortedGraphicEntries;
}

/// <summary>
/// Create the vanilla assorted graphic entries from the constant data table.
/// </summary>
/// <remarks>
/// The entries are stored as a flat array of unsigned ints, see the documentation of
/// VanillaAssortedGraphicEntryData in AssortedGraphicUtils.h for the field layout. Only the info
/// fields are filled in here, the tile, palette and mapping data are not loaded from the ROM.
/// </remarks>
/// <returns>
/// The vanilla assorted graphic entries, which is empty if the data table is corrupted.
/// </returns>
QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> AssortedGraphicUtils::GetVanillaAssortedGraphicEntries()
{
    // Number of the fixed fields at the start of every entry, the palette slot ids follow them
    const unsigned int fixedFieldCount = 9;
    const unsigned int dataCount = sizeof(VanillaAssortedGraphicEntryData) / sizeof(VanillaAssortedGraphicEntryData[0]);
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> entries;
    unsigned int offset = 0;
    while (offset < dataCount)
    {
        if ((dataCount - offset) < fixedFieldCount)
        {
            // the last entry is truncated, the table is corrupt
            entries.clear();
            break;
        }

        struct AssortedGraphicUtils::AssortedGraphicEntryItem entry;
        entry.TileDataAddress = VanillaAssortedGraphicEntryData[offset++];
        entry.TileDataSizeInByte = VanillaAssortedGraphicEntryData[offset++];
        entry.TileDataRAMOffsetNum = VanillaAssortedGraphicEntryData[offset++];
        entry.TileDataType = static_cast<enum AssortedGraphicUtils::AssortedGraphicTileDataType>(
            VanillaAssortedGraphicEntryData[offset++]);
        entry.MappingDataAddress = VanillaAssortedGraphicEntryData[offset++];
        entry.MappingDataSizeAfterCompressionInByte =
            VanillaAssortedGraphicEntryData[offset++];
        entry.MappingDataCompressType =
            static_cast<enum AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType>(
                VanillaAssortedGraphicEntryData[offset++]);
        entry.PaletteAddress = VanillaAssortedGraphicEntryData[offset++];

        // Load the complete palette slot id array of the entry
        unsigned int paletteSlotCount = VanillaAssortedGraphicEntryData[offset++];
        if (paletteSlotCount > (dataCount - offset))
        {
            // the palette slot id array runs past the end of the table, the table is corrupt
            entries.clear();
            break;
        }
        for (unsigned int i = 0; i < paletteSlotCount; ++i)
        {
            entry.PaletteSlotIDs.push_back(VanillaAssortedGraphicEntryData[offset++]);
        }
        if (entry.PaletteSlotIDs.isEmpty())
        {
            entry.PaletteSlotIDs.push_back(0); // fallback, same as the chunk deserialization
        }

        // Generate the names of the vanilla entry, they are not stored in the data table
        GenerateVanillaAssortedGraphicEntryNames(entry);
        entries.push_back(entry);
    }
    return entries;
}

/// <summary>
/// Create the vanilla assorted graphic entries and load their graphic data from the opened ROM.
/// </summary>
/// <returns>
/// The vanilla assorted graphic entries with all the graphic data loaded.
/// </returns>
QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> AssortedGraphicUtils::GetVanillaAssortedGraphicsFromROM()
{
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> entries = GetVanillaAssortedGraphicEntries();
    for (int i = 0; i < entries.size(); ++i)
    {
        ExtractDataFromEntryInfo_v2(entries[i]);
    }
    return entries;
}

/// <summary>
/// Collect the mapping data addresses which can be used by the background Tile8x8 set of a Tileset.
/// </summary>
/// <remarks>
/// The mapping data of a Tile8x8 layer only references background Tile8x8 ids, palettes and flips,
/// so any mapping data built on the same background Tile8x8 set can be used by both Layer 0 and
/// Layer 3 of a room. The options of "Use existing Layer 0" and "Use existing Background Layer"
/// are therefore generated from this single list.
/// </remarks>
/// <param name="tileset">
/// The Tileset which provides the background Tile8x8 set.
/// </param>
/// <returns>
/// The mapping data addresses in use order, without duplicated addresses.
/// </returns>
QVector<unsigned int> AssortedGraphicUtils::FindCompatibleMappingDataAddresses(LevelComponents::Tileset *tileset)
{
    QVector<unsigned int> result;
    if (!tileset)
    {
        return result;
    }

    // the default background mapping data can always be used
    result.push_back(WL4Constants::BGLayerDefaultPtr);

    // add the mapping data of all the graphic entries built on the same Tile8x8 set
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> graphicEntries = GetAssortedGraphicsFromROM();
    QByteArray tilesetTileData;
    bool tilesetTileDataAvailable = GetTilesetBGTile8x8RawData(tileset, tilesetTileData);
    for (int i = 0; i < graphicEntries.size(); ++i)
    {
        if (graphicEntries[i].MappingDataCompressType != RLE_mappingtype_0x20)
        {
            continue; // only the RLE compressed mapping data can be used by a layer
        }
        if (!graphicEntries[i].MappingDataAddress)
        {
            continue; // an entry without a saved mapping data cannot be used by a layer
        }
        if (graphicEntries[i].TileDataType != Tile8x8_4bpp_no_comp_Tileset_text_bg)
        {
            continue; // only this Tile8x8 data type can be the background Tile8x8 set of a Tileset
        }
        // The two vanilla Layer 0 mapping data are handled by the Layer 0 picker of the
        // RoomConfigDialog directly, they must not be offered as a background mapping data.
        if ((graphicEntries[i].MappingDataAddress == WL4Constants::ToxicLandfillDustyLayer0Ptr)
            || (graphicEntries[i].MappingDataAddress == WL4Constants::FieryCavernDustyLayer0Ptr))
        {
            continue;
        }
        if (!DoesTilesetUseTile8x8Set(tileset, graphicEntries[i].TileDataAddress, graphicEntries[i].TileDataSizeInByte,
                                      graphicEntries[i].TileDataRAMOffsetNum, graphicEntries[i].tileData,
                                      tilesetTileDataAvailable ? tilesetTileData : QByteArray()))
        {
            continue;
        }
        if (!result.contains(graphicEntries[i].MappingDataAddress))
        {
            result.push_back(graphicEntries[i].MappingDataAddress);
        }
    }

    return result;
}

/// <summary>
/// Check if an address points into the vanilla part of the ROM.
/// </summary>
/// <param name="address">
/// The address to check.
/// </param>
/// <returns>
/// Return true if the address is a non-zero address below the available space of the ROM.
/// </returns>
bool AssortedGraphicUtils::IsVanillaAddress(unsigned int address)
{
    return (address != 0) && (address < WL4Constants::AvailableSpaceBeginningInROM);
}

/// <summary>
/// Check if all the data of an entry is loaded from vanilla ROM addresses.
/// </summary>
/// <param name="entry">
/// The entry to check.
/// </param>
/// <returns>
/// Return true if all three data parts of the entry reference vanilla ROM addresses.
/// </returns>
bool AssortedGraphicUtils::IsVanillaReferencingEntry(const AssortedGraphicEntryItem &entry)
{
    return IsVanillaAddress(entry.TileDataAddress)
            && IsVanillaAddress(entry.PaletteAddress)
            && IsVanillaAddress(entry.MappingDataAddress);
}

/// <summary>
/// Collect the graphic entries which can provide a mapping data for the background Tile8x8 set of
/// a Tileset.
/// </summary>
/// <remarks>
/// The result is the single source of the mapping data options of the RoomConfigDialog. The
/// default background mapping data is always included. The two vanilla Layer 0 mapping data are
/// only included when the caller asks for the Layer 0 options.
/// </remarks>
/// <param name="tileset">
/// The Tileset which provides the background Tile8x8 set.
/// </param>
/// <param name="forLayer0">
/// True to include the vanilla Layer 0 mapping data.
/// </param>
/// <returns>
/// The compatible entries, deduplicated by mapping data address.
/// </returns>
QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> AssortedGraphicUtils::FindCompatibleGraphicEntries(
        LevelComponents::Tileset *tileset, bool forLayer0)
{
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> result;
    if (!tileset)
    {
        return result;
    }

    // The default background mapping data can always be used. It has no own entry in the vanilla
    // data table, so a synthetic entry is created for the picker.
    struct AssortedGraphicUtils::AssortedGraphicEntryItem defaultEntry;
    defaultEntry.TileDataAddress = static_cast<unsigned int>(tileset->GetbgGFXptr());
    defaultEntry.TileDataSizeInByte = static_cast<unsigned int>(tileset->GetbgGFXlen());
    const unsigned int defaultTileCount = defaultEntry.TileDataSizeInByte / 32;
    defaultEntry.TileDataRAMOffsetNum = (defaultTileCount > 0x3FF) ? 0 : (0x3FF - defaultTileCount);
    defaultEntry.TileDataType = Tile8x8_4bpp_no_comp_Tileset_text_bg;
    defaultEntry.TileDataName = "default background tiles";
    defaultEntry.MappingDataAddress = WL4Constants::BGLayerDefaultPtr;
    defaultEntry.MappingDataCompressType = RLE_mappingtype_0x20;
    defaultEntry.MappingDataName = "default background";
    defaultEntry.PaletteAddress = static_cast<unsigned int>(tileset->GetPaletteAddr());
    defaultEntry.PaletteSlotIDs.push_back(0);
    result.push_back(defaultEntry);

    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> graphicEntries = GetAssortedGraphicsFromROM();
    QByteArray tilesetTileData;
    bool tilesetTileDataAvailable = GetTilesetBGTile8x8RawData(tileset, tilesetTileData);
    for (int i = 0; i < graphicEntries.size(); ++i)
    {
        if (graphicEntries[i].MappingDataCompressType != RLE_mappingtype_0x20)
        {
            continue; // only the RLE compressed mapping data can be used by a layer
        }
        if (!graphicEntries[i].MappingDataAddress)
        {
            continue; // an entry without a saved mapping data cannot be used by a layer
        }
        if (graphicEntries[i].TileDataType != Tile8x8_4bpp_no_comp_Tileset_text_bg)
        {
            continue; // only this Tile8x8 data type can be the background Tile8x8 set of a Tileset
        }
        bool isVanillaLayer0Data =
                (graphicEntries[i].MappingDataAddress == WL4Constants::ToxicLandfillDustyLayer0Ptr)
                || (graphicEntries[i].MappingDataAddress == WL4Constants::FieryCavernDustyLayer0Ptr);
        if (isVanillaLayer0Data && !forLayer0)
        {
            continue;
        }
        if (!DoesTilesetUseTile8x8Set(tileset, graphicEntries[i].TileDataAddress,
                                      graphicEntries[i].TileDataSizeInByte,
                                      graphicEntries[i].TileDataRAMOffsetNum, graphicEntries[i].tileData,
                                      tilesetTileDataAvailable ? tilesetTileData : QByteArray()))
        {
            continue;
        }

        bool alreadyAdded = false;
        for (const struct AssortedGraphicUtils::AssortedGraphicEntryItem &existing : result)
        {
            if (existing.MappingDataAddress == graphicEntries[i].MappingDataAddress)
            {
                alreadyAdded = true;
                break;
            }
        }
        if (!alreadyAdded)
        {
            result.push_back(graphicEntries[i]);
        }
    }

    return result;
}

/// <summary>
/// Extract tiles, palette and mapping data from entry's info.
/// </summary>
/// <param name="entry">
/// The struct data saves the info of a graphic.
/// </param>
void AssortedGraphicUtils::ExtractDataFromEntryInfo_v2(AssortedGraphicEntryItem &entry)
{
    // palettes
    for (int i = 0; i < 16; ++i)
    {
        if (entry.palettes[i].size()) // clean up if need
        {
            entry.palettes[i].clear();
        }
        for (int j = 0; j < 16; ++j) // (re-)initialization
            entry.palettes[i].push_back(QColor(0, 0, 0, 0xFF).rgba());
    }
    for (int i = 0; i < entry.PaletteSlotIDs.size(); i++)
    { // set palette(s) by palette data
        unsigned int tmpPalId = entry.PaletteSlotIDs[i];
        if (tmpPalId >= 16) continue;
        // An address of 0 means the entry has no palette data saved yet, so there is nothing
        // to load and the palette keeps the transparent colors set above
        if (!entry.PaletteAddress)
        {
            continue;
        }
        if (entry.palettes[tmpPalId].size())
        {
            entry.palettes[tmpPalId].clear();
        }
        // For vanilla ROM addresses (below the available space), palettes are at fixed
        // slot-based offsets. For saved chunks (in the available space), palettes are
        // stored contiguously in PaletteSlotIDs order.
        int subPalettePtr;
        if (entry.PaletteAddress >= WL4Constants::AvailableSpaceBeginningInROM)
            subPalettePtr = entry.PaletteAddress + i * 32;              // saved chunk: contiguous
        else
            subPalettePtr = entry.PaletteAddress + tmpPalId * 32;       // vanilla ROM: slot-based offset
        unsigned short *tmpptr = (unsigned short*) (ROMUtils::ROMFileMetadata->ROMDataPtr + subPalettePtr);
        // All the 16 colors are loaded, the palette color 0 is real data as well
        ROMUtils::LoadPalette(&(entry.palettes[tmpPalId]), tmpptr, true);
    }

    // tiles data
    entry.tileData.resize(entry.TileDataSizeInByte);
    for (int j = 0; j < entry.TileDataSizeInByte; ++j)
    {
        entry.tileData[j] = *(ROMUtils::ROMFileMetadata->ROMDataPtr + entry.TileDataAddress + j);
    }

    // mapping data
    switch (static_cast<int>(entry.MappingDataCompressType))
    {
        case AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType::No_mapping_data_comp:
        { // this case never work atm
            for (int i = 0; i < entry.optionalGraphicWidth * entry.optionalGraphicHeight; ++i)
            {
                unsigned short *data = (unsigned short *)(ROMUtils::ROMFileMetadata->ROMDataPtr + entry.MappingDataAddress);
                entry.mappingData.push_back(data[i]);
            }
            break;
        }
        case AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType::RLE_mappingtype_0x20:
        {
            LevelComponents::Layer BGlayer(entry.MappingDataAddress, LevelComponents::LayerTile8x8);
            entry.optionalGraphicHeight = BGlayer.GetLayerHeight();
            entry.optionalGraphicWidth = BGlayer.GetLayerWidth();
            unsigned short *layerdata = BGlayer.GetLayerData();
            for (int i = 0; i < entry.optionalGraphicWidth * entry.optionalGraphicHeight; ++i)
            {
                entry.mappingData.push_back(layerdata[i]);
            }
            break;
        }
    }
}

/// <summary>
/// Save the list of graphic entries to the ROM.
/// </summary>
/// <param name="entries">
/// The array of struct data saves the info of all assorted graphic.
/// </param>
/// <returns>
/// Error infos.
/// </returns>
QVector<AssortedGraphicUtils::entry_datatype_chunk> entry_datatype_chunk_tuple;
QString AssortedGraphicUtils::SaveAssortedGraphicsToROM(QVector<AssortedGraphicEntryItem> &entries)
{
    ROMUtils::SaveDataIndex = 1;
    QVector<struct ROMUtils::SaveData> chunks;
    entry_datatype_chunk_tuple.clear();

    // logic to find changed graphics is so complicated, so we just remove all them.
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> removeGraphics = GetAssortedGraphicsFromROM();

    // Populate the chunk list with data to add to the ROM
    for(int i = 0; i < entries.size(); i++)
    {
        // skip entries which has been used in Rooms and Tilesets
        unsigned int dummy_uint;
        if (!CheckEditability(entries[i], dummy_uint, dummy_uint, dummy_uint))
        {
            // we assume all the entries not used in Rooms and Tilesets are edited, and need to be re-processed
            continue;
        }

        // chunks does not contain the things from the entries which should not be processed
        QVector<struct ROMUtils::SaveData> savedata = CreateSaveData(entries[i], i);
        chunks.append(savedata);
    }

    // Populate the chunk list with invalidation chunks for data to be removed from the ROM
    QVector<unsigned int> invalidationChunks;
    QVector<unsigned int> protectedChunks;
    if (removeGraphics.size())
    {
        for(int i = 0; i < removeGraphics.size(); i++)
        {
            // skip entries which has been used in Rooms and Tilesets
            unsigned int dummy_uint;
            if (!CheckEditability(removeGraphics[i], dummy_uint, dummy_uint, dummy_uint))
            {
                // The chunks of an entry which is still in use must stay valid even when another
                // editable entry shares them.
                QVector<unsigned int> chunkaddrs = GetSaveDataAddresses(removeGraphics[i]);
                for (unsigned int chunkaddr : chunkaddrs)
                {
                    if (!protectedChunks.contains(chunkaddr))
                    {
                        protectedChunks.append(chunkaddr);
                    }
                }
                continue;
            }

            QVector<unsigned int> chunkaddrs = GetSaveDataAddresses(removeGraphics[i]);
            for (auto &j: chunkaddrs)
            {
                // since people can use the same chunk in multiple entries
                if (!protectedChunks.contains(j) && !invalidationChunks.contains(j))
                {
                    invalidationChunks.append(j);
                }
            }
        }
    }

    // We must invalidate the old patch list chunk (if it exists)
    unsigned int patchListChunkAddr = ROMUtils::FindChunkInROM(
        ROMUtils::ROMFileMetadata->ROMDataPtr,
        ROMUtils::ROMFileMetadata->Length,
        WL4Constants::AvailableSpaceBeginningInROM,
        ROMUtils::SaveDataChunkType::AssortedGraphicListChunkType
    );
    if(patchListChunkAddr)
    {
        invalidationChunks.append(patchListChunkAddr + 12);
    }

    // cleanup duplicated elements in invalidationChunks
    std::sort(invalidationChunks.begin(), invalidationChunks.end());
    auto it = std::unique(invalidationChunks.begin(), invalidationChunks.end());
    invalidationChunks.erase(it, invalidationChunks.end());

    // Allocate and save the chunks to the ROM
    unsigned int currentChunkId = 0;
    bool hasDoneListChunk = false;
    bool ret = ROMUtils::SaveFile(ROMUtils::ROMFileMetadata->FilePath, invalidationChunks,

        // ChunkAllocator

        [&entries, &currentChunkId, &chunks, &hasDoneListChunk]
        (unsigned char *TempFile, struct ROMUtils::FreeSpaceRegion freeSpace, struct ROMUtils::SaveData *sd, bool resetchunkIndex, int *require_size)
        {
            (void) TempFile;

            // This part of code will be triggered when rom size needs to be expanded
            // So all the chunks will be reallocated
            if(resetchunkIndex)
            {
                currentChunkId = 0;
                hasDoneListChunk = false;
            }

            // Create save data for all the entries in the iterator
            if(currentChunkId < chunks.size())
            {
                // Get the size of the space that would be needed at this address depending on alignment
                unsigned int alignOffset = 0;
                if(chunks[currentChunkId].alignment)
                {
                    unsigned int startAddr = (freeSpace.addr + 3) & ~3;
                    alignOffset = startAddr - freeSpace.addr;
                }

                // Check if there is space for the chunk in the offered area
                // required_size > (freespace.size - alignment - 12 bytes (for header))
                if(chunks[currentChunkId].size > freeSpace.size - alignOffset - 12)
                {
                    // This will request a larger free area
                    *require_size = chunks[currentChunkId].size;
                    return ROMUtils::ChunkAllocationStatus::InsufficientSpace;
                }
                else
                {
                    // Accept the offered free area for this save chunk
                    *sd = chunks[currentChunkId];

                    // Reset address and other info for different type of chunks
                    unsigned int chunkid = currentChunkId; // = entry_datatype_chunk_tuple[currentChunkId].chunkID;
                    unsigned int entryid = entry_datatype_chunk_tuple[currentChunkId].entryID;
                    enum chunkSaveDataType type = entry_datatype_chunk_tuple[currentChunkId].datatype;
                    unsigned int tmpaddr = freeSpace.addr + alignOffset + 12;
                    switch (type)
                    {
                        case graphicPalette:
                        {
                            entries[entryid].PaletteAddress = tmpaddr;
                            break;
                        }
                        case graphictiles:
                        {
                            entries[entryid].TileDataAddress = tmpaddr;
                            break;
                        }
                        case graphicmappingdata:
                        {
                            entries[entryid].MappingDataAddress = tmpaddr;
                            break;
                        }
                    }

                    currentChunkId++;
                    return ROMUtils::ChunkAllocationStatus::Success;
                }
            }
            else
            {
                // Create AssortedGraphicListChunk after all entries have been processed
                if (!hasDoneListChunk && entries.size())
                {
                    // Create patch list chunk contents, make sure there is sufficient space
                    QString assortedGraphicListChunkContents = CreateAssortedGraphicListChunkData(entries);
                    // To see if the data will fit, we must include the text contents, size of the RATS header, and one byte for versioning
                    if((unsigned int)assortedGraphicListChunkContents.length() + 13 > freeSpace.size)
                    {
                            *require_size = assortedGraphicListChunkContents.length() + 13;
                            return ROMUtils::ChunkAllocationStatus::InsufficientSpace;
                    }

                    // Create the save chunk data
                    unsigned char *data = (unsigned char *) malloc(assortedGraphicListChunkContents.length() + 1);
                    memcpy(data + 1, assortedGraphicListChunkContents.toLocal8Bit().constData(), assortedGraphicListChunkContents.length());
                    data[0] = AssortedGraphic_CHUNK_VERSION;
                    struct ROMUtils::SaveData assortedGraphicListChunk =
                    {
                        0,
                        static_cast<unsigned int>(assortedGraphicListChunkContents.length() + 1),
                        data,
                        ROMUtils::SaveDataIndex++,
                        false,
                        0,
                        0,
                        ROMUtils::SaveDataChunkType::AssortedGraphicListChunkType
                    };
                    *sd = assortedGraphicListChunk;
                    hasDoneListChunk = true;

                    return ROMUtils::ChunkAllocationStatus::Success;
                }

                return ROMUtils::ChunkAllocationStatus::NoMoreChunks;
            }
        },

        // PostProcessingCallback

        []
        (unsigned char *TempFile, std::map<int, int> indexToChunkPtr)
        {
            (void)indexToChunkPtr;
            return QString("");
        }
    );

    return ret ? QString("") : QString("Failed to save graphics data");
}

/// <summary>
/// Get the addresses of chunk(s) if the input entry uses some chunks to save palette, tiles or mapping data.
/// </summary>
/// <param name="entry">
/// The struct data saves the info of the graphic entry.
/// </param>
/// <returns>
/// An array of chunks addresses.
/// </returns>
QVector<unsigned int> AssortedGraphicUtils::GetSaveDataAddresses(AssortedGraphicEntryItem &entry)
{
    QVector<unsigned int> result;
    if (entry.PaletteAddress >= WL4Constants::AvailableSpaceBeginningInROM)
    {
        result.append(entry.PaletteAddress);
    }
    if (entry.TileDataAddress >= WL4Constants::AvailableSpaceBeginningInROM)
    {
        result.append(entry.TileDataAddress);
    }
    if (entry.MappingDataAddress >= WL4Constants::AvailableSpaceBeginningInROM)
    {
        result.append(entry.MappingDataAddress);
    }
    return result;
}

/// <summary>
/// Create SaveData if the current input entry need some chunks to save palette, tiles or mapping data.
/// </summary>
/// <param name="entry">
/// The struct data saves the info of the graphic entry.
/// </param>
/// <param name="entryId">
/// The index of the entry in the entry list which owns the created chunks.
/// </param>
/// <param name="onlyDetachedData">
/// True to save only the data parts which have no ROM address yet. This is used when new entries
/// are appended to the ROM: an address which is already set must be kept as it is, even when it
/// points into the available space of the ROM.
/// </param>
/// <returns>
/// An array of new SaveData.
/// </returns>
QVector<ROMUtils::SaveData> AssortedGraphicUtils::CreateSaveData(AssortedGraphicEntryItem &entry, unsigned int entryId,
                                                                 bool onlyDetachedData)
{
    QVector<ROMUtils::SaveData> result;
    // Skip the data types which contain no data at all, they need no chunk to be saved
    bool paletteNeedsSave = onlyDetachedData
            ? (!entry.PaletteAddress && entry.PaletteSlotIDs.size())
            : ((entry.PaletteAddress >= WL4Constants::AvailableSpaceBeginningInROM || !entry.PaletteAddress)
               && entry.PaletteSlotIDs.size());
    if (paletteNeedsSave)
    {
        unsigned int palCount = entry.PaletteSlotIDs.size();
        unsigned int datasize = palCount * 16 * 2; // 16 color, 2 bytes per color
        unsigned short *data = new unsigned short[datasize];
        memset(data, 0, datasize);
        for(unsigned int i = 0; i < palCount; ++i)
        {
            unsigned int slotId = entry.PaletteSlotIDs[i];
            // All the 16 colors are saved, the palette color 0 is real data as well
            for(int j = 0; j < 16; ++j)
            {
                data[16 * i + j] = ROMUtils::QRgbToData(entry.palettes[slotId][j]);
            }
        }

        // Create the palette data save chunk
        result.append({0,
                       datasize,
                       (unsigned char*)data,
                       ROMUtils::SaveDataIndex++,
                       true,
                       0,
                       0,
                       ROMUtils::SaveDataChunkType::AssortedGraphicPaletteChunkType});
        entry_datatype_chunk_tuple.append({entryId, graphicPalette, result.last().index});
    }
    bool tileDataNeedsSave = onlyDetachedData
            ? (!entry.TileDataAddress && entry.TileDataSizeInByte)
            : ((entry.TileDataAddress >= WL4Constants::AvailableSpaceBeginningInROM || !entry.TileDataAddress)
               && entry.TileDataSizeInByte);
    if (tileDataNeedsSave && (entry.tileData.size() >= (int) entry.TileDataSizeInByte))
    {
        switch (entry.TileDataType)
        {
            case Tile8x8_4bpp_no_comp_Tileset_text_bg:
            {
                unsigned int datasize = entry.TileDataSizeInByte;
                unsigned char *data = new unsigned char[datasize];
                memcpy(&data[0], entry.tileData.constData(), datasize);

                // Create the tile data save chunk
                result.append({0,
                               datasize,
                               data,
                               ROMUtils::SaveDataIndex++,
                               true,
                               0,
                               0,
                               ROMUtils::SaveDataChunkType::AssortedGraphicTile8x8DataChunkType});
                entry_datatype_chunk_tuple.append({entryId, graphictiles, result.last().index});
                break;
            }
            case Tile8x8_4bpp_no_comp:
            {
                // TODO
                // not supported yet
                break;
            }
        }
    }
    bool mappingDataNeedsSave = onlyDetachedData
            ? !entry.MappingDataAddress
            : (entry.MappingDataAddress >= WL4Constants::AvailableSpaceBeginningInROM || !entry.MappingDataAddress);
    if (mappingDataNeedsSave && entry.optionalGraphicWidth && entry.optionalGraphicHeight
            && (entry.mappingData.size() == (int) (entry.optionalGraphicWidth * entry.optionalGraphicHeight)))
    {
        switch (entry.MappingDataCompressType)
        {
            case No_mapping_data_comp:
            {
                // TODO
                // not supported yet
                break;
            }
            case RLE_mappingtype_0x20:
            {
                unsigned int datasize = 0;
                unsigned char *data = LevelComponents::Layer::CompressLayerData(entry.mappingData, LevelComponents::LayerTile8x8,
                                                                                entry.optionalGraphicWidth, entry.optionalGraphicHeight, &datasize);

                // set entry info
                entry.MappingDataSizeAfterCompressionInByte = datasize;

                // Create the tile data save chunk
                result.append({0,
                               datasize,
                               data,
                               ROMUtils::SaveDataIndex++,
                               false, // if this caused problem some day, dig into it, since there is some logic in layer data saving use true here -- ssp
                               0,
                               0,
                               ROMUtils::SaveDataChunkType::AssortedGraphicmappingChunkType});
                entry_datatype_chunk_tuple.append({entryId, graphicmappingdata, result.last().index});
                break;
            }
        }
    }
    return result;
}

/// <summary>
/// Append new entries to the assorted graphic list chunk of the ROM.
/// </summary>
/// <remarks>
/// The chunks of the entries which are already in the ROM are not reallocated and not invalidated.
/// Only the data parts of the new entries which have no ROM address yet are written as new chunks.
/// The old list chunk is replaced by a new one which describes both the old and the new entries.
/// </remarks>
/// <param name="newEntries">
/// The entries to append. Their addresses are updated to the resolved ROM addresses.
/// </param>
/// <returns>
/// An error message, or an empty string on success.
/// </returns>
QString AssortedGraphicUtils::AppendAssortedGraphicsToROM(
        QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> &newEntries)
{
    if (newEntries.isEmpty())
    {
        return QString();
    }

    // Load the entries which are already described by the ROM. They are kept in the list chunk
    // without touching their data chunks. When the ROM has no list chunk yet, this returns the
    // vanilla entries generated from the constant data table.
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> currentEntries =
            GetAssortedGraphicsFromROM();

    ROMUtils::SaveDataIndex = 1;
    entry_datatype_chunk_tuple.clear();

    QVector<struct ROMUtils::SaveData> chunks;
    for (int i = 0; i < newEntries.size(); ++i)
    {
        chunks.append(CreateSaveData(newEntries[i], static_cast<unsigned int>(i), true));
    }

    // Only the old list chunk is invalidated. The data chunks of the entries which are not part of
    // this import stay valid.
    QVector<unsigned int> invalidationChunks;
    unsigned int oldListChunkAddr = ROMUtils::FindChunkInROM(
        ROMUtils::ROMFileMetadata->ROMDataPtr,
        ROMUtils::ROMFileMetadata->Length,
        WL4Constants::AvailableSpaceBeginningInROM,
        ROMUtils::SaveDataChunkType::AssortedGraphicListChunkType
    );
    if (oldListChunkAddr)
    {
        invalidationChunks.append(oldListChunkAddr + 12);
    }

    unsigned int currentChunkId = 0;
    bool hasDoneListChunk = false;

    bool ret = ROMUtils::SaveFile(ROMUtils::ROMFileMetadata->FilePath, invalidationChunks,

        // ChunkAllocator
        [&newEntries, &currentEntries, &currentChunkId, &chunks, &hasDoneListChunk]
        (unsigned char *TempFile, struct ROMUtils::FreeSpaceRegion freeSpace, struct ROMUtils::SaveData *sd,
         bool resetchunkIndex, int *require_size)
        {
            (void) TempFile;

            if (resetchunkIndex)
            {
                currentChunkId = 0;
                hasDoneListChunk = false;
            }

            if (currentChunkId < static_cast<unsigned int>(chunks.size()))
            {
                unsigned int alignOffset = 0;
                if (chunks[currentChunkId].alignment)
                {
                    unsigned int startAddr = (freeSpace.addr + 3) & ~3;
                    alignOffset = startAddr - freeSpace.addr;
                }

                if (chunks[currentChunkId].size > freeSpace.size - alignOffset - 12)
                {
                    *require_size = chunks[currentChunkId].size;
                    return ROMUtils::ChunkAllocationStatus::InsufficientSpace;
                }
                else
                {
                    *sd = chunks[currentChunkId];

                    unsigned int chunkid = currentChunkId;
                    unsigned int entryid = entry_datatype_chunk_tuple[chunkid].entryID;
                    enum chunkSaveDataType type = entry_datatype_chunk_tuple[chunkid].datatype;
                    unsigned int tmpaddr = freeSpace.addr + alignOffset + 12;
                    switch (type)
                    {
                        case graphicPalette:
                        {
                            newEntries[entryid].PaletteAddress = tmpaddr;
                            break;
                        }
                        case graphictiles:
                        {
                            newEntries[entryid].TileDataAddress = tmpaddr;
                            break;
                        }
                        case graphicmappingdata:
                        {
                            newEntries[entryid].MappingDataAddress = tmpaddr;
                            break;
                        }
                    }

                    currentChunkId++;
                    return ROMUtils::ChunkAllocationStatus::Success;
                }
            }
            else
            {
                if (!hasDoneListChunk)
                {
                    // Build the list chunk from the old entries and the new entries with their
                    // resolved addresses. currentEntries is a private copy and is not modified by
                    // the save process.
                    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> allEntries = currentEntries;
                    allEntries += newEntries;
                    QString contents = CreateAssortedGraphicListChunkData(allEntries);
                    if (static_cast<unsigned int>(contents.length()) + 13 > freeSpace.size)
                    {
                        *require_size = contents.length() + 13;
                        return ROMUtils::ChunkAllocationStatus::InsufficientSpace;
                    }

                    unsigned char *data = (unsigned char *) malloc(contents.length() + 1);
                    memcpy(data + 1, contents.toLocal8Bit().constData(), contents.length());
                    data[0] = AssortedGraphic_CHUNK_VERSION;
                    struct ROMUtils::SaveData listChunk =
                    {
                        0,
                        static_cast<unsigned int>(contents.length() + 1),
                        data,
                        ROMUtils::SaveDataIndex++,
                        false,
                        0,
                        0,
                        ROMUtils::SaveDataChunkType::AssortedGraphicListChunkType
                    };
                    *sd = listChunk;
                    hasDoneListChunk = true;

                    return ROMUtils::ChunkAllocationStatus::Success;
                }

                return ROMUtils::ChunkAllocationStatus::NoMoreChunks;
            }
        },

        // PostProcessingCallback
        []
        (unsigned char *TempFile, std::map<int, int> indexToChunkPtr)
        {
            (void) TempFile;
            (void) indexToChunkPtr;
            return QString("");
        }
    );

    return ret ? QString() : QString("Failed to append graphics data to the ROM");
}

/// <summary>
/// Cehck if the current input entry can be changed or deleted.
/// </summary>
/// <param name="entry">
/// The struct data of the graphic entry.
/// </param>
/// <param name="find_l">
/// if the entry is found be used in a Room, then set the level id of that Room to find_l so the caller can get the value.
/// </param>
/// <param name="find_r">
/// if the entry is found be used in a Room, then set the Room id to find_r so the caller can get the value.
/// </param>
/// <param name="find_t">
/// if the entry is found be used in a Tileset, then set the Tileset id to find_t so the caller can get the value.
/// </param>
/// <returns>
/// return true if the entry is not used in any Tilesets or Rooms.
/// </returns>
bool AssortedGraphicUtils::CheckEditability(AssortedGraphicEntryItem &entry, unsigned int &find_l, unsigned int &find_r, unsigned int &find_t)
{
    int tiledataaddr = entry.TileDataAddress;
    find_t = -1;

    // check if the entry is used in any Room
    find_l = -1;
    find_r = -1;
    unsigned int mappingdataaddr = entry.MappingDataAddress;

    if (mappingdataaddr >= WL4Constants::AvailableSpaceBeginningInROM && FindLayerptrInAllRooms(mappingdataaddr, &find_l, &find_r))
    {
        return false;
    }

    // check if the entry is used in any Tileset
    if (tiledataaddr >= WL4Constants::AvailableSpaceBeginningInROM && FindbgGFXptrInAllTilesets(tiledataaddr, &find_t))
    {
        return false;
    }
    return true;
}

/// <summary>
/// Find if a tile data chunk is used in any Tileset.
/// </summary>
/// <param name="address">
/// The address of the tile data.
/// </param>
/// <param name="tilesetId_find">
/// output the id of the tileset in which the bgGFXptr is found.
/// </param>
/// <returns>
/// Return true if the data is found be used.
/// </returns>
bool AssortedGraphicUtils::FindbgGFXptrInAllTilesets(unsigned int address, unsigned int *tilesetId_find)
{
    // in case the return value is not initialzed in the caller
    *tilesetId_find = -1;

    // Go through all the Tilesets to see if tile data is used in any Tileset
    for(int i = 0; i < (sizeof(ROMUtils::singletonTilesets) / sizeof(ROMUtils::singletonTilesets[0])); i++)
    {
        unsigned int addr = ROMUtils::singletonTilesets[i]->GetbgGFXptr();
        if (addr == address)
        {
            *tilesetId_find = i;
            return true;
        }
    }
    return false;
}

/// <summary>
/// Find if a mapping data chunk is used in any Tileset.
/// </summary>
/// <param name="address">
/// The address of the mapping data.
/// </param>
/// <param name="levelId">
/// output the id of the Level in which the mapping data is found.
/// </param>
/// <param name="roomId">
/// output the id of the Room in which the mapping data is found.
/// </param>
/// <returns>
/// Return true if the data is found be used.
/// </returns>
bool AssortedGraphicUtils::FindLayerptrInAllRooms(unsigned int address, unsigned int *levelId_found, unsigned int *roomId_found)
{
    // in case the return value is not initialzed in the caller
    *levelId_found = -1;
    *roomId_found = -1;

    // loop throough all the Rooms
    QVector<unsigned int> levelid_array = {0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5};
    QVector<unsigned int> roomid_array = {0, 2, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 4};
    for (int i = 0; i < levelid_array.size(); i++)
    {
        LevelComponents::Level *tmpLevel = new LevelComponents::Level(static_cast<LevelComponents::__passage>(levelid_array[i]),
                                                                      static_cast<LevelComponents::__stage>(roomid_array[i]));
        for (int j = 0; j < tmpLevel->GetRooms().size(); j++)
        {
            LevelComponents::__RoomHeader header = tmpLevel->GetRooms()[j]->GetRoomHeader();
            if ((header.Layer0MappingType & 0x30) == 0x20)
            {
                if ((header.Layer0Data & 0x7FFFFFF) == address)
                {
                    *levelId_found = levelid_array[i];
                    *roomId_found = roomid_array[j];
                    return true;
                }
            }
            if ((header.Layer3MappingType & 0x30) == 0x20)
            {
                if ((header.Layer3Data & 0x7FFFFFF) == address)
                {
                    *levelId_found = levelid_array[i];
                    *roomId_found = roomid_array[j];
                    return true;
                }
            }
        }

        delete tmpLevel;
    }

    return false;
}
