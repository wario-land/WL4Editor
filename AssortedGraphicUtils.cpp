#include "AssortedGraphicUtils.h"
#include "LevelComponents/Layer.h"
#include "WL4EditorWindow.h"

#define AssortedGraphic_CHUNK_VERSION 0
#define AssortedGraphic_FIELD_COUNT 14

extern WL4EditorWindow *singleton;

/// <summary>
/// Upgrade the format of a assorted graphic list chunk by one version.
/// </summary>
/// <remarks>
/// Version 0:
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
            // TODO implement when there are new versions
            break;
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
    struct AssortedGraphicUtils::AssortedGraphicEntryItem entry
    {
        assortedgraphicTuples[0].toUInt(Q_NULLPTR, 16), // TileDataAddress
        assortedgraphicTuples[1].toUInt(Q_NULLPTR, 16), // TileDataSizeInByte
        assortedgraphicTuples[2].toUInt(Q_NULLPTR, 16), // TileDataRAMOffsetNum
        static_cast<enum AssortedGraphicUtils::AssortedGraphicTileDataType>(assortedgraphicTuples[3].toUInt(Q_NULLPTR, 16)), // TileDataCompressType
        assortedgraphicTuples[4], // TileDataName
        assortedgraphicTuples[5].toUInt(Q_NULLPTR, 16), // MappingDataAddress
        assortedgraphicTuples[6].toUInt(Q_NULLPTR, 16), // MappingDataSizeInByte
        static_cast<enum AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType>(assortedgraphicTuples[7].toUInt(Q_NULLPTR, 16)), // MappingDataCompressType
        assortedgraphicTuples[8], // MappingDataName
        assortedgraphicTuples[9].toUInt(Q_NULLPTR, 16), // optionalPaletteAddress
        assortedgraphicTuples[10].toUInt(Q_NULLPTR, 16), // PaletteNum
        assortedgraphicTuples[11].toUInt(Q_NULLPTR, 16), // PaletteRAMOffsetNum
        assortedgraphicTuples[12].toUInt(Q_NULLPTR, 16), // optionalGraphicWidth
        assortedgraphicTuples[13].toUInt(Q_NULLPTR, 16), // optionalGraphicHeight
    };
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
    ret += QString::number(assortedGraphicMetadata.PaletteNum, 16) + ";";
    ret += QString::number(assortedGraphicMetadata.PaletteRAMOffsetNum, 16) + ";";
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
/// Obtain the AssortedGraphic entries from the currently loaded ROM file.
/// </summary>
/// <returns>
/// A list of all AssortedGraphic entries, which is empty if none exist.
/// </returns>
QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> AssortedGraphicUtils::GetAssortedGraphicsFromROM()
{
    // Obtain the AssortedGraphic list chunk, if it exists
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> assortedGraphicEntries;
    unsigned int assortedGraphicListAddr = ROMUtils::FindChunkInROM(
        ROMUtils::ROMFileMetadata->ROMDataPtr,
        ROMUtils::ROMFileMetadata->Length,
        WL4Constants::AvailableSpaceBeginningInROM,
        ROMUtils::SaveDataChunkType::AssortedGraphicListChunkType
    );
    if(assortedGraphicListAddr)
    {
        // Obtain the AssortedGraphic list info
        QString contents = GetUpgradedAssortedGraphicListChunkData(assortedGraphicListAddr);
        if(!contents.length())
        {
            singleton->GetOutputWidgetPtr()->PrintString(QT_TR_NOOP("ROM contains an empty AssortedGraphic list chunk (this should not be possible)"));
            return assortedGraphicEntries;
        }
        QStringList assortedGraphicTuples = contents.split(";");
        if(assortedGraphicTuples.count() % AssortedGraphic_FIELD_COUNT)
        {
            singleton->GetOutputWidgetPtr()->PrintString(QT_TR_NOOP("ROM contains a corrupted AssortedGraphic list chunk (field count is not a multiple of ") + QString::number(AssortedGraphic_FIELD_COUNT) + ")");
            return assortedGraphicEntries;
        }
        for(int i = 0; i < assortedGraphicTuples.count(); i += AssortedGraphic_FIELD_COUNT)
        {
            struct AssortedGraphicUtils::AssortedGraphicEntryItem entry =
                    DeserializeAssortedGraphicMetadata(assortedGraphicTuples.mid(i, AssortedGraphic_FIELD_COUNT));
            // TODO: add validation check logic for each sub chunk

            // Extract Data from Infos
            ExtractDataFromEntryInfo_v1(entry);

            assortedGraphicEntries.append(entry);
        }
    }
    return assortedGraphicEntries;
}

/// <summary>
/// Extract tiles, palette and mapping data from entry's info.
/// </summary>
/// <param name="entry">
/// The struct data saves the info of a graphic.
/// </param>
void AssortedGraphicUtils::ExtractDataFromEntryInfo_v1(AssortedGraphicEntryItem &entry)
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
    for (int i = 0; ((i < entry.PaletteNum) && ((i + entry.PaletteRAMOffsetNum) < (unsigned int)16)); i++)
    { // set palette(s) by palette data
        unsigned int tmpPalId = i + entry.PaletteRAMOffsetNum;
        if (entry.palettes[tmpPalId].size())
        {
            entry.palettes[tmpPalId].clear();
        }
        int subPalettePtr = entry.PaletteAddress + i * 32;
        unsigned short *tmpptr = (unsigned short*) (ROMUtils::ROMFileMetadata->ROMDataPtr + subPalettePtr);
        ROMUtils::LoadPalette(&(entry.palettes[tmpPalId]), tmpptr);
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
    if (removeGraphics.size())
    {
        for(int i = 0; i < removeGraphics.size(); i++)
        {
            // skip entries which has been used in Rooms and Tilesets
            unsigned int dummy_uint;
            if (!CheckEditability(removeGraphics[i], dummy_uint, dummy_uint, dummy_uint))
            {
                continue;
            }

            QVector<unsigned int> chunkaddrs = GetSaveDataAddresses(removeGraphics[i]);
            for (auto &j: chunkaddrs)
            {
                // since people can use the same chunk in multiple entries
                if (!invalidationChunks.contains(j))
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
        (unsigned char *TempFile, struct ROMUtils::FreeSpaceRegion freeSpace, struct ROMUtils::SaveData *sd, bool resetchunkIndex)
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
/// <returns>
/// An array of new SaveData.
/// </returns>
QVector<ROMUtils::SaveData> AssortedGraphicUtils::CreateSaveData(AssortedGraphicEntryItem &entry, unsigned int entryId)
{
    QVector<ROMUtils::SaveData> result;
    if (entry.PaletteAddress >= WL4Constants::AvailableSpaceBeginningInROM || !(entry.PaletteAddress))
    {
        unsigned int datasize = entry.PaletteNum * 16 * 2; // 16 color, 2 bytes per color
        unsigned short *data = new unsigned short[datasize];
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
                       (unsigned char*)data,
                       ROMUtils::SaveDataIndex++,
                       true,
                       0,
                       0,
                       ROMUtils::SaveDataChunkType::AssortedGraphicPaletteChunkType});
        entry_datatype_chunk_tuple.append({entryId, graphicPalette, result.last().index});
    }
    if (entry.TileDataAddress >= WL4Constants::AvailableSpaceBeginningInROM || !(entry.TileDataAddress))
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
/// Cehck if the current input entry can be changed or deleted.
/// </summary>
/// <param name="entry">
/// The struct data of the graphic entry.
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
