#include "ROMUtils.h"
#include "Compress.h"
#include "Operation.h"
#include <QFile>
#include <QTranslator>
#include "WL4EditorWindow.h"
#include "PatchUtils.h"

#include <cassert>
#include <iostream>
#include <QtDebug>

extern WL4EditorWindow *singleton;

// Helper function to find the number of characters matched in ptr to a pattern
static inline int StrMatch(unsigned char *ptr, const char *pattern)
{
    int matched = 0;
    do
    {
        if (ptr[matched] != *pattern)
            break;
        ++matched;
    } while (*++pattern);
    return matched;
}

// Helper function to validate RATS at an address
static inline bool ValidRATS(unsigned char *ptr)
{
    if (strncmp(reinterpret_cast<const char *>(ptr), "STAR", 4))
        return false;
    short chunkLen = *reinterpret_cast<short *>(ptr + 4);
    short chunkComp = *reinterpret_cast<short *>(ptr + 6);
    return chunkLen == ~chunkComp;
}

namespace ROMUtils
{
    unsigned char *CurrentFile;
    unsigned int CurrentFileSize;
    QString ROMFilePath;
    unsigned char *tmpCurrentFile;
    unsigned int tmpCurrentFileSize;
    QString tmpROMFilePath;

    struct ROMFileMetadata CurrentROMMetadata;
    struct ROMFileMetadata TempROMMetadata;
    struct ROMFileMetadata *ROMFileMetadata;

    unsigned int SaveDataIndex;
    LevelComponents::Tileset *singletonTilesets[92];
    LevelComponents::EntitySet *entitiessets[90];
    LevelComponents::Entity *entities[129];

    const char *ChunkTypeString[CHUNK_TYPE_COUNT] = {
        "InvalidationChunk",
        "RoomHeaderChunkType",
        "DoorChunkType",
        "LayerChunkType",
        "LevelNameChunkType",
        "EntityListChunk",
        "CameraPointerTableType",
        "CameraBoundaryChunkType",
        "PatchListChunk",
        "PatchChunk",
        "TilesetForegroundTile8x8DataChunkType",
        "TilesetMap16EventTableChunkType",
        "TilesetMap16TerrainChunkType",
        "TilesetMap16DataChunkType",
        "TilesetPaletteDataChunkType",
        "EntityTile8x8DataChunkType",
        "EntityPaletteDataChunkType",
        "EntitySetLoadTableChunkType"
    };

    void StaticInitialization()
    {
        CurrentFile = tmpCurrentFile = nullptr;
        CurrentROMMetadata = {CurrentFileSize, ROMFilePath, CurrentFile};
        TempROMMetadata = {tmpCurrentFileSize, tmpROMFilePath, tmpCurrentFile};
        ROMFileMetadata = &CurrentROMMetadata;
    }

    /// <summary>
    /// Get a 4-byte, little-endian integer from ROM data.
    /// </summary>
    /// <param name="data">
    /// The ROM data to read from.
    /// </param>
    /// <param name="address">
    /// The address to get the integer from.
    /// </param>
    unsigned int IntFromData(int address)
    {
        return *reinterpret_cast<unsigned int *>(ROMFileMetadata->ROMDataPtr + address);
    }

    /// <summary>
    /// Get a pointer value from ROM data.
    /// </summary>
    /// <remarks>
    /// The pointer which is returned does not include the upper byte, which is only necessary for the GBA memory map.
    /// The returned int value can be used to index the ROM data.
    /// </remarks>
    /// <param name="data">
    /// The ROM data to read from.
    /// </param>
    /// <param name="address">
    /// The address to get the pointer from.
    /// </param>
    /// <param name="loadFromTmpROM">
    /// Ture when load from a temp ROM.
    /// </param>
    unsigned int PointerFromData(int address)
    {
        unsigned int ret = IntFromData(address) & 0x7FFFFFF;
        if(ret >= ROMFileMetadata->Length)
        {
            singleton->GetOutputWidgetPtr()->PrintString(QT_TR_NOOP("Internal or corruption error: Attempted to read a pointer which is larger than the ROM's file size"));
        }
        return ret;
    }

    /// <summary>
    /// Reverse the endianness of an integer.
    /// </summary>
    /// <param name="n">
    /// The integer to reverse.
    /// </param>
    /// <return>
    /// 08 FF A1 C9 -> C9 A1 FF 08
    /// </return>
    uint32_t EndianReverse(uint32_t n)
    {
        return (n << 24) | ((n & 0xFF00) << 8) | ((n & 0xFF0000) >> 8) | ((n >> 24) & 0xFF);
    }

    /// <summary>
    /// Decompress ROM data that was compressed with run-length encoding.
    /// </summary>
    /// <remarks>
    /// The <paramref name="outputSize"/> parameter specifies the predicted output size in bytes.
    /// The return unsigned char * is on the heap, delete it after using.
    /// </remarks>
    /// <param name="data">
    /// A pointer into the ROM data to start reading from.
    /// </param>
    /// <param name="outputSize">
    /// The predicted size of the output data.(unit: Byte)
    /// </param>
    /// <return>A pointer to decompressed data.</return>
    unsigned char *LayerRLEDecompress(int address, size_t outputSize)
    {
        unsigned char *OutputLayerData = new unsigned char[outputSize];
        int runData;

        for (int i = 0; i < 2; i++)
        {
            unsigned char *dst = OutputLayerData + i;
            if (ROMFileMetadata->ROMDataPtr[address++] == 1)
            {
                while (1)
                {
                    int ctrl = ROMFileMetadata->ROMDataPtr[address++];
                    if (!ctrl)
                    {
                        break;
                    }

                    size_t temp = dst - OutputLayerData;
                    if (temp > outputSize)
                    {
                        delete[] OutputLayerData;
                        return nullptr;
                    }

                    else if (ctrl & 0x80)
                    {
                        runData = ctrl & 0x7F;
                        for (int j = 0; j < runData; j++)
                        {
                            dst[2 * j] = ROMFileMetadata->ROMDataPtr[address];
                        }
                        address++;
                    }
                    else
                    {
                        runData = ctrl;
                        for (int j = 0; j < runData; j++)
                        {
                            dst[2 * j] = ROMFileMetadata->ROMDataPtr[address + j];
                        }
                        address += runData;
                    }

                    dst += 2 * runData;
                }
            }
            else // RLE16
            {
                while (1)
                {
                    int ctrl = (static_cast<int>(ROMFileMetadata->ROMDataPtr[address]) << 8) | ROMFileMetadata->ROMDataPtr[address + 1];
                    address += 2; // offset + 2
                    if (!ctrl)
                    {
                        break;
                    }

                    size_t temp = dst - OutputLayerData;
                    if (temp > outputSize)
                    {
                        delete[] OutputLayerData;
                        return nullptr;
                    }

                    if (ctrl & 0x8000)
                    {
                        runData = ctrl & 0x7FFF;
                        for (int j = 0; j < runData; j++)
                        {
                            dst[2 * j] = ROMFileMetadata->ROMDataPtr[address];
                        }
                        address++;
                    }
                    else
                    {
                        runData = ctrl;
                        for (int j = 0; j < runData; j++)
                        {
                            dst[2 * j] = ROMFileMetadata->ROMDataPtr[address + j];
                        }
                        address += runData;
                    }

                    dst += 2 * runData;
                }
            }
        }
        return OutputLayerData;
    }

    /// <summary>
    /// compress Layer data by run-length encoding.
    /// </summary>
    /// <remarks>
    /// the first and second byte as the layer width and height information will not be generated in the function
    /// you have to add them by yourself when saving compressed data.
    /// </remarks>
    /// <param name="_layersize">
    /// the size of the layer, the value equal to (layerwidth * layerheight).
    /// </param>
    /// <param name="LayerData">
    /// unsigned char pointer to the uncompressed layer data.
    /// </param>
    /// <param name="OutputCompressedData">
    /// unsigned char pointer to the compressed layer data.
    /// </param>
    /// <return>the length of compressed data.</return>
    unsigned int LayerRLECompress(unsigned int _layersize, unsigned short *LayerData,
                                  unsigned char **OutputCompressedData)
    {
        // Separate short data into char arrays
        unsigned char *separatedBytes = new unsigned char[_layersize * 2];
        for (unsigned int i = 0; i < _layersize; ++i)
        {
            unsigned short s = LayerData[i];
            separatedBytes[i] = static_cast<unsigned char>(s);
            separatedBytes[i + _layersize] = static_cast<unsigned char>(s >> 8);
        }

        // Decide on 8 or 16 bit compression for the arrays
        RLEMetadata8Bit Lower8Bit(separatedBytes, _layersize);
        RLEMetadata16Bit Lower16Bit(separatedBytes, _layersize);
        RLEMetadata8Bit Upper8Bit(separatedBytes + _layersize, _layersize);
        RLEMetadata16Bit Upper16Bit(separatedBytes + _layersize, _layersize);
        RLEMetadata *Lower = Lower8Bit.GetCompressedLength() < Lower16Bit.GetCompressedLength()
                                 ? (RLEMetadata *) &Lower8Bit
                                 : (RLEMetadata *) &Lower16Bit;
        RLEMetadata *Upper = Upper8Bit.GetCompressedLength() < Upper16Bit.GetCompressedLength()
                                 ? (RLEMetadata *) &Upper8Bit
                                 : (RLEMetadata *) &Upper16Bit;

        // Create the data to return
        unsigned int lowerLength = Lower->GetCompressedLength(), upperLength = Upper->GetCompressedLength();
        unsigned int size = lowerLength + upperLength + 1;
        *OutputCompressedData = new unsigned char[size];
        void *lowerData = Lower->GetCompressedData();
        void *upperData = Upper->GetCompressedData();
        memcpy(*OutputCompressedData, lowerData, lowerLength);
        memcpy(*OutputCompressedData + lowerLength, upperData, upperLength);
        (*OutputCompressedData)[lowerLength + upperLength] = '\0';

        // Clean up
        delete[] separatedBytes;
        return size;
    }

    /// <summary>
    /// Get the savedata chunks from a Tileset.
    /// </summary>
    /// <param name="TilesetId">
    /// Select a Tileset by its Id.
    /// </param>
    /// <param name="chunks">
    /// Push new chunks to it.
    /// </param>
    void GenerateTilesetSaveChunks(int TilesetId, QVector<SaveData> &chunks)
    {
        int tilesetPtr = singletonTilesets[TilesetId]->getTilesetPtr();
        // Create Map16EventTable chunk
        struct ROMUtils::SaveData Map16EventTablechunk = { static_cast<unsigned int>(tilesetPtr + 28),
                                                         0x600,
                                                         (unsigned char *) malloc(0x600),
                                                         ROMUtils::SaveDataIndex++,
                                                         true,
                                                         0,
                                                         ROMUtils::PointerFromData(tilesetPtr + 28),
                                                         ROMUtils::SaveDataChunkType::TilesetMap16EventTableChunkType };
        memcpy(Map16EventTablechunk.data, singletonTilesets[TilesetId]->GetEventTablePtr(), 0x600);
        chunks.append(Map16EventTablechunk);

        // Create FGTile8x8GraphicData chunk
        int FGTileGfxDataLen = singletonTilesets[TilesetId]->GetfgGFXlen();
        unsigned char FGmap8x8tiledata[(1024 - 65) * 32];
        QVector<LevelComponents::Tile8x8 *> tile8x8array = singletonTilesets[TilesetId]->GetTile8x8arrayPtr();
        for (int j = 0; j < (FGTileGfxDataLen / 32); ++j)
        {
            memcpy(&FGmap8x8tiledata[32 * j], tile8x8array[j + 0x41]->CreateGraphicsData().data(), 32);
        }
        struct ROMUtils::SaveData FGTile8x8GraphicDataChunk = { static_cast<unsigned int>(tilesetPtr),
                                                         static_cast<unsigned int>(FGTileGfxDataLen),
                                                         (unsigned char *) malloc(FGTileGfxDataLen),
                                                         ROMUtils::SaveDataIndex++,
                                                         true,
                                                         0,
                                                         ROMUtils::PointerFromData(tilesetPtr),
                                                         ROMUtils::SaveDataChunkType::TilesetForegroundTile8x8DataChunkType };
        memcpy(FGTile8x8GraphicDataChunk.data, FGmap8x8tiledata, FGTileGfxDataLen);
        chunks.append(FGTile8x8GraphicDataChunk);

        // Create Map16TerrainType chunk
        struct ROMUtils::SaveData Map16TerrainTypechunk = { static_cast<unsigned int>(tilesetPtr + 24),
                                                         0x300,
                                                         (unsigned char *) malloc(0x300),
                                                         ROMUtils::SaveDataIndex++,
                                                         true,
                                                         0,
                                                         ROMUtils::PointerFromData(tilesetPtr + 24),
                                                         ROMUtils::SaveDataChunkType::TilesetMap16TerrainChunkType };
        memcpy(Map16TerrainTypechunk.data, singletonTilesets[TilesetId]->GetTerrainTypeIDTablePtr(), 0x300);
        chunks.append(Map16TerrainTypechunk);

        // Save palettes
        singletonTilesets[TilesetId]->ReGeneratePaletteData();
        struct ROMUtils::SaveData TilesetPalettechunk = { static_cast<unsigned int>(tilesetPtr + 8),
                                                         16 * 16 * 2,
                                                         (unsigned char *) malloc(16 * 16 * 2),
                                                         ROMUtils::SaveDataIndex++,
                                                         true,
                                                         0,
                                                         ROMUtils::PointerFromData(tilesetPtr + 8),
                                                         ROMUtils::SaveDataChunkType::TilesetPaletteDataChunkType };
        memcpy(TilesetPalettechunk.data, singletonTilesets[TilesetId]->GetTilesetPaletteDataPtr(), 16 * 16 * 2);
        chunks.append(TilesetPalettechunk);

        // Create Map16Data chunk
        QVector<LevelComponents::TileMap16 *> map16data = singletonTilesets[TilesetId]->GetMap16arrayPtr();
        struct ROMUtils::SaveData Map16Datachunk = { static_cast<unsigned int>(tilesetPtr + 20),
                                                         0x300 * 8,
                                                         (unsigned char *) malloc(0x300 * 8),
                                                         ROMUtils::SaveDataIndex++,
                                                         true,
                                                         0,
                                                         ROMUtils::PointerFromData(tilesetPtr + 20),
                                                         ROMUtils::SaveDataChunkType::TilesetMap16DataChunkType };
        unsigned short map16tilePtr[0x300 * 4];
        for (int j = 0; j < 0x300; ++j)
        {
            map16tilePtr[j * 4] = map16data[j]->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPLEFT)->GetValue();
            map16tilePtr[j * 4 + 1] = map16data[j]->GetTile8X8(LevelComponents::TileMap16::TILE8_TOPRIGHT)->GetValue();
            map16tilePtr[j * 4 + 2] = map16data[j]->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMLEFT)->GetValue();
            map16tilePtr[j * 4 + 3] = map16data[j]->GetTile8X8(LevelComponents::TileMap16::TILE8_BOTTOMRIGHT)->GetValue();
        }
        memcpy(Map16Datachunk.data, (unsigned char*)map16tilePtr, 0x300 * 8);
        chunks.append(Map16Datachunk);
    }

    /// <summary>
    /// Find the next chunk of a specific type.
    /// </summary>
    /// <param name="ROMData">
    /// The pointer to the ROM data being processed.
    /// </param>
    /// <param name="ROMLength">
    /// The length of the ROM data.
    /// </param>
    /// <param name="startAddr">
    /// The start address to search from.
    /// </param>
    /// <param name="chunkType">
    /// The chunk type to search for in the ROM.
    /// </param>
    /// <param name="anyChunk">
    /// If true, then return any chunk instead of specific types specified by <paramref name="chunkType"/>.
    /// </param>
    /// <returns>
    /// The next chunk of a specific type, or 0 if none exists.
    /// </returns>
    unsigned int FindChunkInROM(unsigned char *ROMData, unsigned int ROMLength, unsigned int startAddr, enum SaveDataChunkType chunkType, bool anyChunk)
    {
        if(startAddr >= ROMLength) return 0; // fail if not enough room in ROM
        while(startAddr < ROMLength)
        {
            // Optimize search by incrementing more with partial matches
            int STARmatch = StrMatch(ROMData + startAddr, "STAR");
            if(STARmatch < 4)
            {
                // STAR not found at current address
                startAddr += qMax(STARmatch, 1);
            }
            else
            {
                // STAR found at current address: validate the RATS checksum and chunk type
                if(ValidRATS(ROMData + startAddr) && (anyChunk || ROMData[startAddr + 8] == chunkType))
                {
                    return startAddr;
                }
                else
                {
                    // Invalid RATS or chunk type not found: advance
                    startAddr += 4;
                }
            }
        }
        return 0;
    }

    /// <summary>
    /// Find all chunks of a specific type.
    /// </summary>
    /// <param name="ROMData">
    /// The pointer to the ROM data being processed.
    /// </param>
    /// <param name="ROMLength">
    /// The length of the ROM data.
    /// </param>
    /// <param name="startAddr">
    /// The start address to search from.
    /// </param>
    /// <param name="chunkType">
    /// The chunk type to search for in the ROM.
    /// </param>
    /// <param name="anyChunk">
    /// If true, then return any chunk instead of specific types specified by <paramref name="chunkType"/>.
    /// </param>
    /// <returns>
    /// A list of all chunks of a specific type.
    /// </returns>
    QVector<unsigned int> FindAllChunksInROM(unsigned char *ROMData, unsigned int ROMLength, unsigned int startAddr, enum SaveDataChunkType chunkType, bool anyChunk)
    {
        QVector<unsigned int> chunks;
        while(startAddr < ROMLength)
        {
            unsigned int chunkAddr = FindChunkInROM(ROMData, ROMLength, startAddr, chunkType, anyChunk);
            if(chunkAddr)
            {
                chunks.append(chunkAddr);
                unsigned int chunkLen = *reinterpret_cast<unsigned short*>(ROMData + chunkAddr + 4);
                unsigned int extLen = (unsigned int) *reinterpret_cast<unsigned char*>(ROMData + chunkAddr + 9) << 16;
                startAddr = chunkAddr + chunkLen + extLen + 12;
            }
            else break;
        }
        return chunks;
    }

    /// <summary>
    /// Find all free space regions in the ROM.
    /// </summary>
    /// <param name="ROMData">
    /// The pointer to the ROM data being processed.
    /// </param>
    /// <param name="ROMLength">
    /// The length of the ROM data.
    /// </param>
    /// <returns>
    /// A list of all free space regions.
    /// </returns>
    QVector<struct FreeSpaceRegion> FindAllFreeSpaceInROM(unsigned char *ROMData, unsigned int ROMLength)
    {
        QVector<struct FreeSpaceRegion> freeSpace;
        unsigned int startAddr = WL4Constants::AvailableSpaceBeginningInROM;
        unsigned int freeSpaceStart = startAddr;

        // Search through ROM for chunks. The space between them is free space
        while(startAddr < ROMLength)
        {
            // Optimize search by incrementing more with partial matches
            int STARmatch = StrMatch(ROMData + startAddr, "STAR");
            if(STARmatch < 4)
            {
                // STAR not found at current address
                startAddr += qMax(STARmatch, 1);
            }
            else
            {
                // STAR found at current address: validate the RATS checksum and chunk type
                if(ValidRATS(ROMData + startAddr))
                {
                    // Chunk found. The space up to this point is free space
                    if(startAddr > freeSpaceStart)
                    {
                        freeSpace.append({freeSpaceStart, startAddr - freeSpaceStart});
                    }

                    // Continue search after this chunk
                    unsigned int chunkLen = *reinterpret_cast<unsigned short*>(ROMData + startAddr + 4);
                    unsigned int extLen = (unsigned int) *reinterpret_cast<unsigned char*>(ROMData + startAddr + 9) << 16;
                    startAddr += chunkLen + extLen + 12;
                    freeSpaceStart = startAddr;
                }
                else
                {
                    // Invalid RATS or chunk type not found: advance
                    startAddr += 4;
                }
            }
        }

        // The last space in the ROM is a free space region
        if(startAddr > freeSpaceStart)
        {
            freeSpace.append({freeSpaceStart, startAddr - freeSpaceStart});
        }
        return freeSpace;
    }

    /// <summary>
    /// Save a list of chunks to the ROM file.
    /// </summary>
    /// <param name="filePath">
    /// The file name to use when saving the ROM.
    /// </param>
    /// <param name="invalidationChunks">
    /// Addresses of chunks to invalidate.
    /// </param>
    /// <param name="ChunkAllocator">
    /// Callback function that allocates chunks.
    /// The SaveFile function will offer potential free areas to the allocator, which will then
    /// accept or reject the free area depending on how much space is actually needed.
    /// </param>
    /// <param name="PostProcessingCallback">
    /// Post-processing to perform after writing the save chunks, but before saving the file itself.
    /// This function returns an error string if unsuccessful, or an empty string if successful.
    /// </param>
    /// <returns>
    /// True if the save was successful.
    /// </returns>
    bool SaveFile(QString filePath, QVector<unsigned int> invalidationChunks,
        std::function<ChunkAllocationStatus (unsigned char*, struct FreeSpaceRegion, struct SaveData*, bool)> ChunkAllocator,
        std::function<QString (unsigned char*, std::map<int, int>)> PostProcessingCallback)
    {
        // Finding space for the chunks can be done faster if the chunks are ordered by size
        unsigned char *TempFile = (unsigned char *) malloc(ROMFileMetadata->Length);
        unsigned int TempLength = ROMFileMetadata->Length;
        memcpy(TempFile, ROMFileMetadata->ROMDataPtr, ROMFileMetadata->Length);
        std::map<int, int> chunkIDtoIndex;

        // Invalidate old chunk data
        for(unsigned int invalidationChunk : invalidationChunks)
        {
            // Sanity check
            if(invalidationChunk > ROMFileMetadata->Length)
            {
                singleton->GetOutputWidgetPtr()->PrintString(QString(QT_TR_NOOP("Internal error while saving changes to ROM: Invalidation chunk out of range of entire ROM. Address: %1"))
                        .arg("0x" + QString::number(invalidationChunk - 12, 16).toUpper()));
            }

            // Chunks can only be invalidated within valid chunk area
            if (invalidationChunk > WL4Constants::AvailableSpaceBeginningInROM)
            {
                unsigned char *RATSaddr = TempFile + invalidationChunk - 12;
                if (ValidRATS(RATSaddr)) // old_chunk_addr should point to the start of the chunk data, not the RATS tag
                {
                    strncpy((char *) RATSaddr, "STAR_INV", 8);
                }
                else
                {
                    singleton->GetOutputWidgetPtr()->PrintString(QString(QT_TR_NOOP("Internal error while saving changes to ROM: Invalidation chunk references an invalid RATS identifier for existing chunk. Address: %1. Changes not saved."))
                        .arg("0x" + QString::number(invalidationChunk - 12, 16).toUpper()));
                    return false;
                }
            }
        }

        // Find free space in the ROM and attempt to offer the free regions to the chunk allocator
        QVector<struct FreeSpaceRegion> freeSpaceRegions;
        QVector<struct SaveData> chunksToAdd;
        std::map<int, int> indexToChunkPtr;
        bool success = false;
        bool resizerom = false; // act as a trigger to reset index in ChunkAllocator

resized:freeSpaceRegions.clear();
        chunksToAdd.clear();
        indexToChunkPtr.clear();
        freeSpaceRegions = FindAllFreeSpaceInROM(TempFile, TempLength);

        do
        {
            // Order free space regions by increasing size
            std::sort(freeSpaceRegions.begin(), freeSpaceRegions.end(),
                [](const struct FreeSpaceRegion &a, const struct FreeSpaceRegion &b)
                {return a.size < b.size;});

            // Offer free space to chunk allocator (starting with size of 12)
            unsigned int lastSize = 11, newSize;
            struct SaveData sd;
            int i;
            for(i = 0; i < freeSpaceRegions.size(); ++i)
            {
                if(freeSpaceRegions[i].size <= lastSize) continue;
                ChunkAllocationStatus status = ChunkAllocator(TempFile, freeSpaceRegions[i], &sd, resizerom);
                resizerom = false;
                switch(status)
                {
                case Success:
                    goto spaceFound;
                case NoMoreChunks:
                    goto allocationComplete;
                case ProcessingError:
                    goto error;
                case InsufficientSpace:
                    lastSize = freeSpaceRegions[i].size;
                    continue;
                }
            }

            // No free space regions capable of accommodating chunk. Expand ROM
            newSize = (TempLength << 1) & ~0x7FFFFF;
            if(newSize <= 0x2000000)
            {
                unsigned char *newTempFile = (unsigned char*) realloc(TempFile, newSize);
                if(!newTempFile)
                {
                    // Realloc failed due to system memory constraints
                    QMessageBox::warning(
                        singleton,
                        QT_TR_NOOP("Out of memory"),
                        QT_TR_NOOP("Unable to save changes because your computer is out of memory."),
                        QMessageBox::Ok,
                        QMessageBox::Ok
                    );
                    goto error;
                }
                TempFile = newTempFile;
                memset(TempFile + TempLength, 0xFF, newSize - TempLength);
                TempLength = newSize;
                resizerom = true;
                goto resized;
            }
            else
            {
                // ROM size cannot exceed 32MB
                QMessageBox::warning(
                    singleton,
                    QT_TR_NOOP("ROM too large"),
                    QString(QT_TR_NOOP("Unable to save changes because there is not enough free space, and the ROM file cannot be expanded larger than 32MB.")),
                    QMessageBox::Ok,
                    QMessageBox::Ok
                );
                goto error;
            }

spaceFound:
            // Split the free space region
            struct FreeSpaceRegion freeSpace = freeSpaceRegions[i];
            freeSpaceRegions.remove(i);

            // Determine where the chunk starts if alignment would modify it
            unsigned int alignedAddr = freeSpace.addr;
            if(sd.alignment)
            {
                alignedAddr = (alignedAddr + 3) & ~3;
            }
            unsigned int alignmentOffset = alignedAddr - freeSpace.addr;

            // If chunk data starts at an offset due to alignment, split on left side of data
            if(alignmentOffset)
            {
                freeSpaceRegions.append({freeSpace.addr, alignmentOffset});
            }

            // If chunk data is smaller than free space, split on right side of data
            if(alignmentOffset + sd.size < freeSpace.size)
            {
                freeSpaceRegions.append({
                    freeSpace.addr + alignmentOffset + sd.size + 12,
                    freeSpace.size - (alignmentOffset + sd.size) - 12
                });
            }

            // Restore temp indexes info about saving chunks
            indexToChunkPtr[sd.index] = alignedAddr;
            chunksToAdd.append(sd);

        } while(1);

allocationComplete:

        // Generate chunkIDtoIndex map
        for(int k = 0; k < chunksToAdd.size(); k++)
        {
            chunkIDtoIndex[chunksToAdd[k].index] = k;
        }

        // Apply source pointer modifications to applicable chunk types
        for(struct SaveData &chunk : chunksToAdd)
        {
            switch(chunk.ChunkType)
            {
            case SaveDataChunkType::InvalidationChunk:
                singleton->GetOutputWidgetPtr()->PrintString(QT_TR_NOOP("Internal error: Chunk allocator created an invalidation chunk"));
            case SaveDataChunkType::PatchListChunk:
            case SaveDataChunkType::PatchChunk:
                continue; // the above chunk types are not associated with a modified pointer in main ROM
            default:;
            }

            unsigned char *ptrLoc = chunk.dest_index ?
                // Source pointer is in another chunk
                chunksToAdd[chunkIDtoIndex[chunk.dest_index]].data + chunk.ptr_addr
                    :
                // Source pointer is in main ROM
                TempFile + chunk.ptr_addr;

            // We add 12 to the pointer location because the chunk ptr starts at the chunk's RATS tag
            *reinterpret_cast<unsigned int*>(ptrLoc) = static_cast<unsigned int>((indexToChunkPtr[chunk.index] + 12) | 0x8000000);
        }

        // Write chunks to TempFile
        for(struct SaveData &chunk : chunksToAdd)
        {
            if (chunk.ChunkType == SaveDataChunkType::InvalidationChunk)
            {
                continue;
            }

            // Write the chunk metadata with RATS format and the chunk data
            unsigned char *destPtr = TempFile + indexToChunkPtr[chunk.index];
            strncpy(reinterpret_cast<char*>(destPtr), "STAR", 4);
            unsigned short chunkLen = (unsigned short) (chunk.size & 0xFFFF);
            unsigned char extLen = (unsigned char) ((chunk.size >> 16) & 0xFF);
            *reinterpret_cast<unsigned short*>(destPtr + 4) = chunkLen;
            *reinterpret_cast<unsigned short*>(destPtr + 6) = ~chunkLen;
            *reinterpret_cast<unsigned int*>(destPtr + 8) = 0;
            destPtr[8] = chunk.ChunkType;
            destPtr[9] = extLen;
            memcpy(destPtr + 12, chunk.data, (unsigned short) chunk.size);
        }

        // Perform post-processing before saving the file
        if(PostProcessingCallback)
        {
            QString ret(PostProcessingCallback(TempFile, indexToChunkPtr));
            if(ret != "")
            {
                success = false;
                goto error;
            }
        }

        { // Prevent goto from crossing initialization of variables here
            // Save the rom file from the CurrentFile copy
            QFile file(filePath);
            file.open(QIODevice::WriteOnly);
            if (file.isOpen())
            {
                file.write(reinterpret_cast<const char*>(TempFile), TempLength);
            }
            else
            {
                // Couldn't open the file to save the ROM
                QMessageBox::warning(singleton, QT_TR_NOOP("Could not save file"),
                     QT_TR_NOOP("Unable to write to or create the ROM file for saving."), QMessageBox::Ok,
                     QMessageBox::Ok);
                goto error;
            }
            file.close();

            // Set the CurrentFile to the copied CurrentFile data
            auto temp = ROMFileMetadata->ROMDataPtr;
            ROMFileMetadata->ROMDataPtr = TempFile;
            delete[] temp;
            ROMFileMetadata->Length = TempLength;
        }

        // Set that there are no changes to the ROM now (so no save prompt is given)
        singleton->SetUnsavedChanges(false);

        // Clean up heap data and return
        success = true;
        if (0)
        {
error:      free(TempFile); // free up temporary file if there was a processing error
        }
        for(struct SaveData chunk : chunksToAdd)
        {
            if (chunk.ChunkType != SaveDataChunkType::InvalidationChunk)
            {
                free(chunk.data);
            }
        }

        return success;
    }

    /// <summary>
    /// Save the currently loaded level to the ROM file.
    /// </summary>
    /// <param name="filePath">
    /// The file name to use when saving the ROM.
    /// </param>
    /// <returns>
    /// True if the save was successful.
    /// </returns>
    bool SaveLevel(QString filePath)
    {
        SaveDataIndex = 1;
        QVector<struct SaveData> chunks;

        // Get save chunks for the level
        LevelComponents::Level *currentLevel = singleton->GetCurrentLevel();
        int levelHeaderOffset = WL4Constants::LevelHeaderIndexTable + currentLevel->GetPassage() * 24 + currentLevel->GetStage() * 4;
        int levelHeaderIndex = ROMUtils::IntFromData(levelHeaderOffset);
        int levelHeaderPointer = WL4Constants::LevelHeaderTable + levelHeaderIndex * 12;
        if(!currentLevel->GetSaveChunks(chunks))
        {
            return false;
        }

        // Get Global instances chunks
        for(int i = 0; i < 92; ++i)
        {
            if(singletonTilesets[i]->IsNewTileset())
            {
                GenerateTilesetSaveChunks(i, chunks);
            }
        }
        for(int i = 0x11; i < 129; ++i) // we skip the first 0x10 sprites, they should be addressed differently
        {
            if(entities[i]->IsNewEntity())
            {
                GenerateEntitySaveChunks(i, chunks);
            }
        }
        for(int i = 0; i < 90; ++i)
        {
            if(entitiessets[i]->IsNewEntitySet())
            {
                GenerateEntitySetSaveChunks(i, chunks);
            }
        }

        // Isolate the room header chunk for post-processing
        struct SaveData roomHeaderChunk = *std::find_if(chunks.begin(), chunks.end(), [](const struct SaveData &chunk) {
            return chunk.ChunkType == SaveDataChunkType::RoomHeaderChunkType;
        });
        unsigned int roomHeaderInROM;

        QVector<unsigned int> invalidationChunks;
        QVector<struct SaveData> addedChunks;
        for(int i = 0; i < chunks.size(); ++i)
        {
            if(chunks[i].ChunkType == SaveDataChunkType::InvalidationChunk)
            {
                invalidationChunks.append(chunks[i].old_chunk_addr);
            }
            else
            {
                if(chunks[i].old_chunk_addr >= WL4Constants::AvailableSpaceBeginningInROM)
                {
                    invalidationChunks.append(chunks[i].old_chunk_addr);
                }
                addedChunks.append(chunks[i]);
            }
        }

        // Save the level
        int chunkIndex = 0;
        bool ret = SaveFile(filePath, invalidationChunks,

            // ChunkAllocator

            [&chunkIndex, addedChunks]
            (unsigned char *TempFile, struct FreeSpaceRegion freeSpace, struct SaveData *sd, bool resetchunkIndex)
            {
                (void) TempFile;

                // This part of code will be triggered when rom size needs to be expanded
                // So all the chunks will be reallocated
                if(resetchunkIndex)
                {
                    chunkIndex = 0;
                }

                if(chunkIndex >= addedChunks.size())
                {
                    return ChunkAllocationStatus::NoMoreChunks;
                }

                // Get the size of the space that would be needed at this address depending on alignment
                unsigned int alignOffset = 0;
                if(addedChunks[chunkIndex].alignment)
                {
                    unsigned int startAddr = (freeSpace.addr + 3) & ~3;
                    alignOffset = startAddr - freeSpace.addr;
                }

                // Check if there is space for the chunk in the offered area
                // required_size > (freespace.size - alignment - 12 bytes (for header))
                if(addedChunks[chunkIndex].size > freeSpace.size - alignOffset - 12)
                {
                    // This will request a larger free area
                    return ChunkAllocationStatus::InsufficientSpace;
                }
                else
                {
                    // Accept the offered free area for this save chunk
                    *sd = addedChunks[chunkIndex++];
                    return ChunkAllocationStatus::Success;
                }
            },

            // PostProcessingCallback

            [levelHeaderPointer, currentLevel, roomHeaderChunk, &roomHeaderInROM]
            (unsigned char *TempFile, std::map<int, int> indexToChunkPtr)
            {
                // Capture pointer to new room header location
                roomHeaderInROM = static_cast<unsigned int>(indexToChunkPtr[roomHeaderChunk.index] + 12);

                // Write the level header to the ROM
                memcpy(TempFile + levelHeaderPointer, currentLevel->GetLevelHeader(), sizeof(struct LevelComponents::__LevelHeader));

                // Write Tileset data length and animtated tiles info
                for(int i = 0; i < 92; ++i)
                {
                    if(singletonTilesets[i]->IsNewTileset())
                    {
                        // Save Animated Tile info table
                        unsigned short *AnimatedTileInfoTable = singletonTilesets[i]->GetAnimatedTileData(0);
                        memcpy(TempFile + i * 32 + WL4Constants::AnimatedTileIdTableSwitchOff, (unsigned char*)AnimatedTileInfoTable, 32);
                        unsigned short *AnimatedTileInfoTable2 = singletonTilesets[i]->GetAnimatedTileData(1);
                        memcpy(TempFile + i * 32 + WL4Constants::AnimatedTileIdTableSwitchOn, (unsigned char*)AnimatedTileInfoTable2, 32);
                        unsigned char *AnimatedTileSwitchInfoTable = singletonTilesets[i]->GetAnimatedTileSwitchTable();
                        memcpy(TempFile + i * 16 + WL4Constants::AnimatedTileSwitchInfoTable, (unsigned char*)AnimatedTileSwitchInfoTable, 16);

                        // Reset size_of bgGFXLen and fgGBXLen
                        int tilesetPtr = singletonTilesets[i]->getTilesetPtr();
                        int fgGFXLenaddr = singletonTilesets[i]->GetfgGFXlen();
                        *(int *) (TempFile + tilesetPtr + 4) = fgGFXLenaddr;
                        int bgGFXLenaddr = singletonTilesets[i]->GetbgGFXlen();
                        *(int *) (TempFile + tilesetPtr + 16) = bgGFXLenaddr;
                        // don't needed, because this is done in the following internal pointers reset code
                        // singletonTilesets[i]->SetChanged(false);
                    }
                }

                // Write Sprite data length info
                for(int i = 0x11; i < 129; ++i) // we skip the first 0x10 sprites, they should be addressed differently
                {
                    if(entities[i]->IsNewEntity())
                    {
                        *(unsigned int *) (TempFile + WL4Constants::EntityTilesetLengthTable + 4 * (i - 0x10)) = entities[i]->GetPalNum() * (32 * 32 * 2);
                    }
                }
                return QString("");
            }
        );

        if(!ret) return false;

        // Set the new internal data pointers for LevelComponents objects, and mark dirty objects as clean
        // --------------------------------------------------------------------
        // Rooms instances internal pointers reset
        // TODO: move out the unset dirty code, it is headache to do all of them here
        // TODO: code needs to be changed if we are going to support saving all the changes in the whole ROM, every level, i mean
        std::vector<LevelComponents::Room*> rooms = currentLevel->GetRooms();
        for(unsigned int i = 0; i < rooms.size(); ++i)
        {
            struct LevelComponents::__RoomHeader *roomHeader = (struct LevelComponents::__RoomHeader*)
                (ROMFileMetadata->ROMDataPtr + roomHeaderInROM + i * sizeof(struct LevelComponents::__RoomHeader));
            unsigned int *layerDataPtrs = (unsigned int*) &roomHeader->Layer0Data;
            LevelComponents::Room *room = rooms[i];
            for(unsigned int j = 0; j < 4; ++j)
            {
                LevelComponents::Layer *layer = room->GetLayer(j);
                layer->SetDataPtr(layerDataPtrs[j] & 0x7FFFFFF);
                layer->SetDirty(false);
            }
            for(unsigned int j = 0; j < 3; ++j)
            {
                room->SetEntityListDirty(j, false);
            }
            struct LevelComponents::__RoomHeader newroomheader;
            memcpy(&newroomheader, roomHeader, sizeof(newroomheader));
            room->ResetRoomHeader(newroomheader);
        }

        // global history changed bool reset
        ResetChangedBoolsThroughHistory();

        // Tilesets instances internal pointers reset
        for(int i = 0; i < 92; ++i)
        {
            if(singletonTilesets[i]->IsNewTileset())
            {
                int tilesetPtr = singletonTilesets[i]->getTilesetPtr();
                singletonTilesets[i]->SetfgGFXptr(ROMUtils::PointerFromData(tilesetPtr));
                singletonTilesets[i]->SetPaletteAddr(ROMUtils::PointerFromData(tilesetPtr + 8));
                singletonTilesets[i]->Setmap16ptr(ROMUtils::PointerFromData(tilesetPtr + 0x14));
                singletonTilesets[i]->SetChanged(false);
            }
        }

        // Entities and Entitysets members reset
        for(int i = 0x11; i < 129; ++i) // we skip the first 0x10 sprites, they should be addressed differently
        {
            if(entities[i]->IsNewEntity())
            {
                entities[i]->SetChanged(false);
            }
        }
        for(int i = 0; i < 90; ++i)
        {
            if(entitiessets[i]->IsNewEntitySet())
            {
                entitiessets[i]->SetChanged(false);
            }
        }
        // --------------------------------------------------------------------
        return true;
    }

    /// Load a palette, 16 colors, from a pointer.
    /// </summary>
    /// <param name="palette">
    /// Pointer of palette instance provided for palette loading.
    /// </param>
    /// <param name="dataptr">
    /// data pointer which keeps RGB55 palette data.
    /// </param>
    void LoadPalette(QVector<QRgb> *palette, unsigned short *dataptr, bool notdisablefirstcolor)
    {
        // First color is transparent
        int k = 1;
        if(!notdisablefirstcolor)
        {
            palette->push_back(0);
        } else {
            k = 0;
        }

        for (int j = k; j < 16; ++j)
        {
            unsigned short color555 = *(dataptr + j);
            int r = ((color555 << 3) & 0xF8) | ((color555 >> 2) & 7);
            int g = ((color555 >> 2) & 0xF8) | ((color555 >> 7) & 7);
            int b = ((color555 >> 7) & 0xF8) | ((color555 >> 12) & 7);
            int a = 0xFF;
            palette->push_back(QColor(r, g, b, a).rgba());
        }
    }

    void GenerateEntitySaveChunks(int GlobalEntityId, QVector<SaveData> &chunks)
    {
        int SpriteTilesetPtrAddr = WL4Constants::EntityTilesetPointerTable + 4 * (GlobalEntityId - 0x10);

        // Create FGTile8x8GraphicData chunk
        int TileGfxDataLen = entities[GlobalEntityId]->GetTilesNum() * 32;
        unsigned char map8x8tiledata[512 * 32];
        QVector<LevelComponents::Tile8x8 *> tile8x8array = entities[GlobalEntityId]->GetTile8x8array();
        for (int j = 0; j < (TileGfxDataLen / 32); ++j)
        {
            memcpy(&map8x8tiledata[32 * j], tile8x8array[j]->CreateGraphicsData().data(), 32);
        }
        struct ROMUtils::SaveData Tile8x8GraphicDataChunk = { static_cast<unsigned int>(SpriteTilesetPtrAddr),
                                                         static_cast<unsigned int>(TileGfxDataLen),
                                                         (unsigned char *) malloc(TileGfxDataLen),
                                                         ROMUtils::SaveDataIndex++,
                                                         true,
                                                         0,
                                                         ROMUtils::PointerFromData(SpriteTilesetPtrAddr),
                                                         ROMUtils::SaveDataChunkType::EntityTile8x8DataChunkType };
        memcpy(Tile8x8GraphicDataChunk.data, map8x8tiledata, TileGfxDataLen);
        chunks.append(Tile8x8GraphicDataChunk);

        // Save palettes
        QVector<QRgb> *palettes = entities[GlobalEntityId]->GetPalettes();
        int palNum = entities[GlobalEntityId]->GetPalNum();
        unsigned short *TilesetPaletteData = new unsigned short[palNum * 16];
        memset((unsigned char *)TilesetPaletteData, 0, palNum * 16 * 2);
        QColor tmp_color;
        for(int i = 0; i < palNum; ++i)
        {
            // First color is transparent
            // RGB555 format: bbbbbgggggrrrrr
            for(int j = 1; j < 16; ++j)
            {
                tmp_color.setRgb(palettes[i][j]);
                int b = (tmp_color.blue() >> 3) & 0x1F;
                int g = (tmp_color.green() >> 3) & 0x1F;
                int r = (tmp_color.red() >> 3) & 0x1F;
                TilesetPaletteData[16 * i + j] = (unsigned short) ((b << 10) | (g << 5) | r);
            }
        }
        int SpritePalettePtrAddr = WL4Constants::EntityPalettePointerTable + 4 * (GlobalEntityId - 0x10);
        struct ROMUtils::SaveData TilesetPalettechunk = { static_cast<unsigned int>(SpritePalettePtrAddr),
                                                         static_cast<unsigned int>(palNum * 16 * 2),
                                                         (unsigned char *) malloc(palNum * 16 * 2),
                                                         ROMUtils::SaveDataIndex++,
                                                         true,
                                                         0,
                                                         ROMUtils::PointerFromData(SpritePalettePtrAddr),
                                                         ROMUtils::SaveDataChunkType::EntityPaletteDataChunkType };
        memcpy(TilesetPalettechunk.data, (unsigned char *)TilesetPaletteData, palNum * 16 * 2);
        delete[] TilesetPaletteData;
        chunks.append(TilesetPalettechunk);
    }

    void GenerateEntitySetSaveChunks(int EntitySetId, QVector<SaveData> &chunks)
    {
        int EntitySetLoadTablePtrAddr = WL4Constants::EntitySetInfoPointerTable + EntitySetId * 4;

        // Create LoadTable Data chunk
        QVector<LevelComponents::EntitySetinfoTableElement> loadtable = entitiessets[EntitySetId]->GetEntityTable();
        int tablesize = loadtable.size() * 2 + 2; // add 2 extra 0x00 as an end mark
        unsigned char *loadtabledata = new unsigned char[tablesize];
        memset(loadtabledata, 0, tablesize);
        for (int j = 0; j < loadtable.size(); ++j)
        {
            loadtabledata[2 * j] = loadtable[j].Global_EntityID;
            loadtabledata[2 * j + 1] = loadtable[j].paletteOffset;
        }
        struct ROMUtils::SaveData Tile8x8GraphicDataChunk = { static_cast<unsigned int>(EntitySetLoadTablePtrAddr),
                                                         static_cast<unsigned int>(tablesize),
                                                         (unsigned char *) malloc(tablesize),
                                                         ROMUtils::SaveDataIndex++,
                                                         true,
                                                         0,
                                                         ROMUtils::PointerFromData(EntitySetLoadTablePtrAddr),
                                                         ROMUtils::SaveDataChunkType::EntitySetLoadTableChunkType };
        memcpy(Tile8x8GraphicDataChunk.data, loadtabledata, tablesize);
        delete[] loadtabledata;
        chunks.append(Tile8x8GraphicDataChunk);
    }

    /// <summary>
    /// Print debug info about chunks in ROM.
    /// </summary>
    void SaveDataAnalysis()
    {
        struct ChunkData
        {
            unsigned int addr;
            unsigned int sizeWithHeader;
            enum SaveDataChunkType chunkType;
        };

        // Get information about the chunks and free space
        QVector<unsigned int> chunks = FindAllChunksInROM(
            ROMFileMetadata->ROMDataPtr,
            ROMFileMetadata->Length,
            WL4Constants::AvailableSpaceBeginningInROM,
            SaveDataChunkType::InvalidationChunk,
            true
        );
        QVector<struct FreeSpaceRegion> freeSpace = FindAllFreeSpaceInROM(ROMFileMetadata->ROMDataPtr, ROMFileMetadata->Length);
        QVector<struct ChunkData> chunkData;
        for(unsigned int chunkAddr : chunks)
        {
            unsigned int chunkLen = *reinterpret_cast<unsigned short*>(ROMFileMetadata->ROMDataPtr + chunkAddr + 4);
            unsigned int extLen = (unsigned int) *reinterpret_cast<unsigned char*>(ROMFileMetadata->ROMDataPtr + chunkAddr + 9) << 16;
            struct ChunkData cd = {
                chunkAddr,
                chunkLen + extLen + 12,
                static_cast<enum SaveDataChunkType>(ROMFileMetadata->ROMDataPtr[chunkAddr + 8])
            };
            chunkData.append(cd);
        }
        unsigned int saveAreaSize = ROMFileMetadata->Length - WL4Constants::AvailableSpaceBeginningInROM;

        // Find total free space
        int totalFreeSpace = 0;
        for(auto fs : freeSpace)
        {
            totalFreeSpace += fs.size;
        }

        // Find total space used by chunks
        int totalUsedSpace = 0;
        int otherTypeCount = 0;
        int otherTypeSpace = 0;
        QMap<enum SaveDataChunkType, unsigned int> chunkTypeCount, chunkTypeSpace;
        for(int i = 0; i < CHUNK_TYPE_COUNT; ++i)
        {
            chunkTypeCount.insert(static_cast<enum SaveDataChunkType>(i), 0);
            chunkTypeSpace.insert(static_cast<enum SaveDataChunkType>(i), 0);
        }
        for(auto c : chunkData)
        {
            totalUsedSpace += c.sizeWithHeader;
            if(c.chunkType < CHUNK_TYPE_COUNT)
            {
                chunkTypeCount[c.chunkType]++;
                chunkTypeSpace[c.chunkType] += c.sizeWithHeader;
            }
            else
            {
                otherTypeCount++;
                otherTypeSpace += c.sizeWithHeader;
            }
        }

        // Calculate statistics
        int nonFragmentedSpace = freeSpace[freeSpace.size() - 1].size;
        int fragmentedSpace = totalFreeSpace - nonFragmentedSpace;
        double freeSpaceP = (double) totalFreeSpace / saveAreaSize;
        double freeSpaceP_frag = (double) fragmentedSpace / totalFreeSpace;
        double freeSpaceP_nonFrag = (double) nonFragmentedSpace / totalFreeSpace;
        double usedSpaceP = (double) totalUsedSpace / saveAreaSize;
        QMap<enum SaveDataChunkType, double> usedSpaceP_ofType;
        int divisor = totalUsedSpace ? totalUsedSpace : 1; // display 0.00% correctly
        for(int i = 0; i < CHUNK_TYPE_COUNT; ++i)
        {
            enum SaveDataChunkType t = static_cast<enum SaveDataChunkType>(i);
            usedSpaceP_ofType.insert(t, (double) chunkTypeSpace[t] / divisor);
        }
        double usedSpaceP_ofTypeOther = (double) otherTypeSpace / divisor;

        // Print statistics
        qDebug() << QString("Save data area: %1").arg(saveAreaSize);
        qDebug() << QString("Free space: %1 (%2%)").arg(totalFreeSpace).arg(100 * freeSpaceP, 6, 'f', 2);
        qDebug() << QString("  Fragmented: %1 (%2%)").arg(fragmentedSpace).arg(100 * freeSpaceP_frag, 6, 'f', 2);
        qDebug() << QString("  Non-fragmented: %1 (%2%)").arg(nonFragmentedSpace).arg(100 * freeSpaceP_nonFrag, 6, 'f', 2);
        qDebug() << QString("Used space: %1 (%2%)").arg(totalUsedSpace).arg(100 * usedSpaceP, 6, 'f', 2);
        for(int i = 0; i < CHUNK_TYPE_COUNT; ++i)
        {
            enum SaveDataChunkType t = static_cast<enum SaveDataChunkType>(i);
            qDebug() << QString("  %1: %2 (%3%, %4 chunks)").arg(ChunkTypeString[i], -37).arg(chunkTypeSpace[t], 7).arg(100 * usedSpaceP_ofType[t], 6, 'f', 2).arg(chunkTypeCount[t]);
        }
        qDebug() << QString("  %1: %2 (%3%, %4 chunks)").arg("Other", -37).arg(otherTypeSpace, 7).arg(100 * usedSpaceP_ofTypeOther, 6, 'f', 2).arg(otherTypeCount);
    }

    /// <summary>
    /// Get and map chunk reference data for all the non-orphaned chunks
    /// </summary>
    /// <returns>
    /// QMap<unsigned int, struct ChunkReference>
    /// the first template param "unsigned int" is ChunkAddress
    /// the second template param "struct ChunkReference" obtains the chunk reference metadata of the current chunk on ChunkAddress
    /// </returns>
    QMap<unsigned int, struct ChunkReference> GetAllChunkReferences()
    {
        QMap<unsigned int, struct ChunkReference> references;
        struct ChunkReference chunkRef;

        // Get global patch references
        unsigned int patchListAddr = ROMUtils::FindChunkInROM(
            ROMUtils::ROMFileMetadata->ROMDataPtr,
            ROMUtils::ROMFileMetadata->Length,
            WL4Constants::AvailableSpaceBeginningInROM,
            ROMUtils::SaveDataChunkType::PatchListChunk
        );
        chunkRef = {PatchListChunk};
        references[patchListAddr] = chunkRef;
        QVector<struct PatchEntryItem> patches = PatchUtils::GetPatchesFromROM();
        for(const struct PatchEntryItem &patch : patches)
        {
            chunkRef = {PatchChunk};
            references[patch.PatchAddress] = chunkRef;
        }

        // Process global references
        // Process all Tilesets
        for(unsigned int tilesetid = 0; tilesetid < 92; ++tilesetid)
        {
            int tilesetPtr = WL4Constants::TilesetDataTable + tilesetid * 36;

            chunkRef = {TilesetForegroundTile8x8DataChunkType};
            unsigned int tilesetFGGFXptr = ROMUtils::PointerFromData(tilesetPtr);
            references[tilesetFGGFXptr - 12] = chunkRef;

            // TODO: add bgGFXptr when needed

            chunkRef = {TilesetMap16DataChunkType};
            unsigned int tilesetMap16ptr = ROMUtils::PointerFromData(tilesetPtr + 0x14);
            references[tilesetMap16ptr - 12] = chunkRef;

            chunkRef = {TilesetMap16EventTableChunkType};
            unsigned int tilesetMap16EventTableAddr = ROMUtils::PointerFromData(tilesetPtr + 28);
            references[tilesetMap16EventTableAddr - 12] = chunkRef;

            chunkRef = {TilesetMap16TerrainChunkType};
            unsigned int tilesetMap16TerrainTypeIDTableAddr = ROMUtils::PointerFromData(tilesetPtr + 24);
            references[tilesetMap16TerrainTypeIDTableAddr - 12] = chunkRef;

            chunkRef = {TilesetPaletteDataChunkType};
            unsigned int tilesetPaletteData = ROMUtils::PointerFromData(tilesetPtr + 8);
            references[tilesetPaletteData - 12] = chunkRef;
        }

        // Process most Entity atm since the editor only supports editing them
        for(unsigned int entityid = 0x11; entityid < 129; ++entityid)
        {
            chunkRef = {EntityPaletteDataChunkType};
            unsigned int entityPaletteAddr = ROMUtils::PointerFromData(WL4Constants::EntityPalettePointerTable + 4 * (entityid - 0x10));
            references[entityPaletteAddr - 12] = chunkRef;

            chunkRef = {EntityTile8x8DataChunkType};
            unsigned int entityTileDataAddr = ROMUtils::PointerFromData(WL4Constants::EntityTilesetPointerTable + 4 * (entityid - 0x10));
            references[entityTileDataAddr - 12] = chunkRef;
        }

        // Process all Entityset
        for(unsigned int entitysetid = 0x11; entitysetid < 129; ++entitysetid)
        {
            chunkRef = {EntitySetLoadTableChunkType};
            unsigned int entitysetLoadTableptr = ROMUtils::PointerFromData(WL4Constants::EntitySetInfoPointerTable + entitysetid * 4);
            references[entitysetLoadTableptr - 12] = chunkRef;
        }

        // Process all passages
        for(unsigned int passageNum = 0; passageNum < 6; ++passageNum)
        {
            // Process all stages within a passage
            for(unsigned int stageNum = 0; stageNum < 5; ++stageNum)
            {
                // skip stages which don't exist
                if (passageNum == 5)
                {
                    switch (stageNum)
                    {
                    case 1:
                    case 2:
                    case 3:
                    {continue;}
                    }
                }
                else if (!passageNum)
                {
                    switch (stageNum)
                    {
                    case 1:
                    case 3:
                    {continue;}
                    }
                }

                // Get level header chunk reference
                unsigned int offset = WL4Constants::LevelHeaderIndexTable + passageNum * 24 + stageNum * 4;
                unsigned int levelHeaderIndex = ROMUtils::IntFromData(offset);
                unsigned int levelHeaderAddr = WL4Constants::LevelHeaderTable + levelHeaderIndex * 12;

                // Get level name chunks
                unsigned int LevelNameAddr = ROMUtils::PointerFromData(WL4Constants::LevelNamePointerTable + passageNum * 24 + stageNum * 4);
                unsigned int LevelNameJAddr = ROMUtils::PointerFromData(WL4Constants::LevelNameJPointerTable + passageNum * 24 + stageNum * 4);
                chunkRef = {LevelNameChunkType};
                references[LevelNameAddr - 12] = chunkRef;
                chunkRef = {LevelNameChunkType};
                references[LevelNameJAddr - 12] = chunkRef;

                // Door table chunk
                unsigned int LevelID = ROMUtils::ROMFileMetadata->ROMDataPtr[levelHeaderAddr];
                unsigned int doorTableAddress = ROMUtils::PointerFromData(WL4Constants::DoorTable + LevelID * 4);
                chunkRef = {DoorChunkType};
                references[doorTableAddress - 12] = chunkRef;

                // Process rooms
                unsigned int roomTableAddress = ROMUtils::PointerFromData(WL4Constants::RoomDataTable + LevelID * 4);
                chunkRef = {RoomHeaderChunkType}; // chunk ref for room header
                unsigned int roomCount = ROMUtils::ROMFileMetadata->ROMDataPtr[levelHeaderAddr + 1];
                int cameraLimitatorRooms = 0;
                for(unsigned int roomId = 0; roomId < roomCount; ++roomId)
                {
                    // Add offsets to child chunks of the room header
                    unsigned int roomHeaderOffset = roomId * sizeof(LevelComponents::__RoomHeader);
                    chunkRef.ChildrenChunkLocalOffset
                            << (roomHeaderOffset + 0x08)
                            << (roomHeaderOffset + 0x0C)
                            << (roomHeaderOffset + 0x10)
                            << (roomHeaderOffset + 0x14)
                            << (roomHeaderOffset + 0x1C)
                            << (roomHeaderOffset + 0x20)
                            << (roomHeaderOffset + 0x24);

                    // Determine if this specific room uses camera limitators
                    unsigned int roomDataPtr = roomHeaderOffset + roomTableAddress;
                    cameraLimitatorRooms += ROMUtils::ROMFileMetadata->ROMDataPtr[roomDataPtr + 24] == LevelComponents::HasControlAttrs;

                    // Process layers
                    struct ChunkReference subChunkRef;
                    for(unsigned int layerNum = 0; layerNum < 4; ++layerNum)
                    {
                        subChunkRef = {LayerChunkType, roomTableAddress - 12};
                        unsigned int layerPtr = ROMUtils::PointerFromData(roomDataPtr + layerNum * 4 + 8);
                        references[layerPtr - 12] = subChunkRef;
                    }

                    // Add entity list chunks
                    for(unsigned int entityListNum = 0; entityListNum < 3; ++entityListNum)
                    {
                        subChunkRef = {EntityListChunk, roomTableAddress - 12};
                        unsigned int listAddress = ROMUtils::PointerFromData(roomDataPtr + 28 + 4 * entityListNum);
                        references[listAddress - 12] = subChunkRef;
                    }
                }
                references[roomTableAddress - 12] = chunkRef;

                // Add camera chunks (if applicable)
                if(cameraLimitatorRooms)
                {
                    unsigned int cameraPointerTablePtr = WL4Constants::CameraControlPointerTable + LevelID * 4;
                    chunkRef = {CameraPointerTableType}; // chunk ref for camera pointer table
                    unsigned int cameraPointerTableAddr = ROMUtils::PointerFromData(cameraPointerTablePtr);

                    // Add chunk refs for camera boundary entries
                    struct ChunkReference subChunkRef;
                    for(int cameraEntry = 0; cameraEntry < cameraLimitatorRooms; ++cameraEntry)
                    {
                        chunkRef.ChildrenChunkLocalOffset << 4 * cameraEntry;
                        unsigned int cameraEntryAddress = ROMUtils::PointerFromData(cameraPointerTableAddr + cameraEntry * 4);

                        subChunkRef = {CameraBoundaryChunkType, cameraPointerTableAddr - 12};
                        references[cameraEntryAddress - 12] = subChunkRef;
                    }
                    references[cameraPointerTableAddr - 12] = chunkRef;
                }
            }
        }

        // Touch up info before returning it
        for(auto key : references.keys())
        {
            // If the data would be in vanilla ROM, remove the chunk reference
            if(key < WL4Constants::AvailableSpaceBeginningInROM)
            {
                references.remove(key);
            }
            else
            {
                // If the parent reference is in vanilla ROM, remove reference
                auto &reference = references[key];
                if(reference.ParentChunkAddress < WL4Constants::AvailableSpaceBeginningInROM)
                {
                    reference.ParentChunkAddress = 0;
                }

                // If any children references are in vanilla ROM, remove them
                auto &children = references[key].ChildrenChunkLocalOffset;
                for(int i = children.size() - 1; i >= 0; --i)
                {
                    unsigned int childPtr = ROMUtils::PointerFromData(key + 12 + children[i]);
                    if(childPtr < WL4Constants::AvailableSpaceBeginningInROM)
                    {
                        children.remove(i);
                    }
                }

                // Sort child offset references by pointer value
                std::sort(children.begin(), children.end(),
                    [key](const unsigned int &a, const unsigned int &b)
                    {
                        unsigned int ptrA = PointerFromData(key + 12 + a);
                        unsigned int ptrB = PointerFromData(key + 12 + b);
                        return ptrA < ptrB;
                    });
            }
        }

        return references;
    }

    /// <summary>
    /// Clean up TmpCurrentFile meta data.
    /// </summary>
    void CleanUpTmpCurrentFileMetaData()
    {
        if (ROMUtils::tmpCurrentFile)
        {
            // RAM cleanup
            ROMUtils::tmpCurrentFileSize = 0;
            ROMUtils::tmpROMFilePath.clear();
            delete[] ROMUtils::tmpCurrentFile;
            ROMUtils::tmpCurrentFile = nullptr;
        }
    }

} // namespace ROMUtils
