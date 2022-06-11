#include "ScatteredGraphicUtils.h"
#include "LevelComponents/Layer.h"
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
    ret += QString::number(scatteredGraphicMetadata.TileDataType, 16) + ";";
    ret += scatteredGraphicMetadata.TileDataName + ";";
    ret += QString::number(scatteredGraphicMetadata.MappingDataAddress, 16) + ";";
    ret += QString::number(scatteredGraphicMetadata.MappingDataSizeAfterCompression_Byte, 16) + ";";
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

            // Extract Data from Infos
            ExtractDataFromEntryInfo_v1(entry);

            scatteredGraphicEntries.append(entry);
        }
    }
    return scatteredGraphicEntries;
}

/// <summary>
/// Extract tiles, palette and mapping data from entry's info.
/// </summary>
/// <param name="entry">
/// The struct data saves the info of a graphic.
/// </param>
void ScatteredGraphicUtils::ExtractDataFromEntryInfo_v1(ScatteredGraphicEntryItem &entry)
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
    for (int i = entry.PaletteRAMOffsetNum; i < qMin((entry.PaletteRAMOffsetNum + entry.PaletteNum), (unsigned int)16); ++i)
    { // set palette(s) by palette data
        if (entry.palettes[i].size())
        {
            entry.palettes[i].clear();
        }
        int subPalettePtr = entry.PaletteAddress + i * 32;
        unsigned short *tmpptr = (unsigned short*) (ROMUtils::ROMFileMetadata->ROMDataPtr + subPalettePtr);
        ROMUtils::LoadPalette(&(entry.palettes[i]), tmpptr);
    }

    // tiles data
    entry.tileData.resize(entry.TileDataSize_Byte);
    for (int j = 0; j < entry.TileDataSize_Byte; ++j)
    {
        entry.tileData[j] = *(ROMUtils::ROMFileMetadata->ROMDataPtr + entry.TileDataAddress + j);
    }

    // mapping data
    switch (static_cast<int>(entry.MappingDataCompressType))
    {
        case ScatteredGraphicUtils::ScatteredGraphicMappingDataCompressionType::No_mapping_data_comp:
        { // this case never work atm
            for (int i = 0; i < entry.optionalGraphicWidth * entry.optionalGraphicHeight; ++i)
            {
                unsigned short *data = (unsigned short *)(ROMUtils::ROMFileMetadata->ROMDataPtr + entry.MappingDataAddress);
                entry.mappingData.push_back(data[i]);
            }
            break;
        }
        case ScatteredGraphicUtils::ScatteredGraphicMappingDataCompressionType::RLE_mappingtype_0x20:
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
/// The array of struct data saves the info of all scattered graphic.
/// </param>
/// <returns>
/// Error infos.
/// </returns>
QString ScatteredGraphicUtils::SaveScatteredGraphicsToROM(QVector<ScatteredGraphicEntryItem> entries)
{
    ROMUtils::SaveDataIndex = 1;
    QVector<struct ROMUtils::SaveData> chunks;
    entry_datatype_chunk_tuple.clear();

    // logic to find changed graphics is so complicated, so we just remove all them.
    QVector<struct ScatteredGraphicUtils::ScatteredGraphicEntryItem> removeGraphics = GetScatteredGraphicsFromROM();

    // Populate the chunk list with data to add to the ROM
    for(int i = 0; i < entries.size(); i++)
    {
        QVector<struct ROMUtils::SaveData> savedata = CreateSaveData(entries[i], i);
        chunks.append(savedata);
    }

    // Populate the chunk list with invalidation chunks for data to be removed from the ROM
    QVector<unsigned int> invalidationChunks;
    if (removeGraphics.size())
    {
        for(struct ScatteredGraphicUtils::ScatteredGraphicEntryItem &graphicEntry : removeGraphics)
        {
            QVector<unsigned int> chunkaddrs = GetSaveDataAddresses(graphicEntry);
            invalidationChunks.append(chunkaddrs);
        }
    }

    // We must invalidate the old patch list chunk (if it exists)
    unsigned int patchListChunkAddr = ROMUtils::FindChunkInROM(
        ROMUtils::ROMFileMetadata->ROMDataPtr,
        ROMUtils::ROMFileMetadata->Length,
        WL4Constants::AvailableSpaceBeginningInROM,
        ROMUtils::SaveDataChunkType::ScatteredGraphicListChunkType
    );
    if(patchListChunkAddr)
    {
        invalidationChunks.append(patchListChunkAddr + 12);
    }

    return QString("");
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
QVector<unsigned int> ScatteredGraphicUtils::GetSaveDataAddresses(ScatteredGraphicEntryItem &entry)
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
/// <returns>
/// An array of new SaveData.
/// </returns>
QVector<ROMUtils::SaveData> ScatteredGraphicUtils::CreateSaveData(ScatteredGraphicEntryItem &entry, unsigned int entryId)
{
    QVector<ROMUtils::SaveData> result;
    if (entry.PaletteAddress >= WL4Constants::AvailableSpaceBeginningInROM || !(entry.PaletteAddress))
    {
        unsigned int datasize = entry.PaletteNum * 16 * 2; // 16 color, 2 bytes per color
        unsigned char *data = new unsigned char[datasize];
        memset(data, 0, datasize);
        for(int i = 0; i < entry.PaletteNum; ++i)
        {
            // The first color is transparent
            for(int j = 1; j < 16; ++j)
            {
                data[16 * i + j] = ROMUtils::QRgbToData(entry.palettes[i + entry.PaletteRAMOffsetNum][j]);
            }
        }

        // Create the palette data save chunk
        result.append({0,
                       datasize,
                       data,
                       ROMUtils::SaveDataIndex++,
                       true,
                       0,
                       0,
                       ROMUtils::SaveDataChunkType::ScatteredGraphicPaletteChunkType});
        entry_datatype_chunk_tuple.append({entryId, graphicPalette, result.last().index});
    }
    if (entry.TileDataAddress >= WL4Constants::AvailableSpaceBeginningInROM || !(entry.TileDataAddress))
    {
        switch (entry.TileDataType)
        {
            case Tile8x8_4bpp_no_comp_Tileset_text_bg:
            {
                unsigned int datasize = entry.TileDataSize_Byte;
                unsigned char *data = new unsigned char[datasize];
                memcpy(&data[0], entry.tileData.constData(), 32);

                // Create the tile data save chunk
                result.append({0,
                               datasize,
                               data,
                               ROMUtils::SaveDataIndex++,
                               true,
                               0,
                               0,
                               ROMUtils::SaveDataChunkType::ScatteredGraphicTile8x8DataChunkType});
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
    if (entry.MappingDataAddress >= WL4Constants::AvailableSpaceBeginningInROM || !(entry.MappingDataAddress))
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

                // Create the tile data save chunk
                result.append({0,
                               datasize,
                               data,
                               ROMUtils::SaveDataIndex++,
                               false, // if this caused problem some day, dig into it, since there is some logic in layer data saving use true here -- ssp
                               0,
                               0,
                               ROMUtils::SaveDataChunkType::ScatteredGraphicmappingChunkType});
                entry_datatype_chunk_tuple.append({entryId, graphicmappingdata, result.last().index});
                break;
            }
        }
    }
    return result;
}
