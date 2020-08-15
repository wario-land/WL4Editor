#include "ROMUtils.h"
#include "Compress.h"
#include <QFile>
#include "WL4EditorWindow.h"
#include <cassert>
#include <iostream>

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
    unsigned int SaveDataIndex;
    LevelComponents::Tileset *singletonTilesets[92];

    /// <summary>
    /// Get a 4-byte, little-endian integer from ROM data.
    /// </summary>
    /// <param name="data">
    /// The ROM data to read from.
    /// </param>
    /// <param name="address">
    /// The address to get the integer from.
    /// </param>
    unsigned int IntFromData(int address) { return *reinterpret_cast<unsigned int *>(CurrentFile + address); }

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
    unsigned int PointerFromData(int address)
    {
        unsigned int ret = IntFromData(address) & 0x7FFFFFF;
        if(ret >= CurrentFileSize)
        {
            singleton->GetOutputWidgetPtr()->PrintString("Internal or corruption error: Attempted to read a pointer which is larger than the ROM's file size");
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
            if (ROMUtils::CurrentFile[address++] == 1)
            {
                while (1)
                {
                    int ctrl = CurrentFile[address++];
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
                            dst[2 * j] = CurrentFile[address];
                        }
                        address++;
                    }
                    else
                    {
                        runData = ctrl;
                        for (int j = 0; j < runData; j++)
                        {
                            dst[2 * j] = CurrentFile[address + j];
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
                    int ctrl = (static_cast<int>(CurrentFile[address]) << 8) | CurrentFile[address + 1];
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
                            dst[2 * j] = CurrentFile[address];
                        }
                        address++;
                    }
                    else
                    {
                        runData = ctrl;
                        for (int j = 0; j < runData; j++)
                        {
                            dst[2 * j] = CurrentFile[address + j];
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
    /// Find the next available address in ROM data that is free based on RATS.
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
    /// <param name="chunkSize">
    /// The size of the chunk for which we must find free space.
    /// </param>
    /// <returns>
    /// The pointer to the available space, or 0 if none exists.
    /// </returns>
    int FindSpaceInROM(unsigned char *ROMData, int ROMLength, int startAddr, int chunkSize)
    {
        int freeBytes = 0; // number of free bytes found from startAddr
        if (startAddr + chunkSize > ROMLength)
            return 0; // fail if not enough room in ROM
        while (freeBytes < chunkSize)
        {
            // Optimize search by incrementing more with partial matches
            int STARmatch = StrMatch(ROMData + startAddr + freeBytes, "STAR");
            if (STARmatch < 4)
            {
                // STAR not found at current address
                freeBytes += qMax(STARmatch, 1);
            }
            else
            {
                // STAR found at current address: validate the RATS checksum
                if (ValidRATS(ROMData + startAddr + freeBytes))
                {
                    // Checksum pass: Restart the search at end of the chunk
                    unsigned short chunkLen = *reinterpret_cast<unsigned short *>(ROMData + startAddr + freeBytes + 4);
                    startAddr += freeBytes + 12 + chunkLen;
                    if (startAddr + chunkSize > ROMLength)
                        return 0; // fail if not enough room in ROM
                    freeBytes = 0;
                }
                else
                {
                    // Checksum fail: Advance freeBytes past the invalid RATS identifier
                    freeBytes += 4;
                }
            }
        }
        return startAddr;
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
        LevelComponents::Tile8x8 **tile8x8array = singletonTilesets[TilesetId]->GetTile8x8arrayPtr();
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
        LevelComponents::TileMap16** map16data = singletonTilesets[TilesetId]->GetMap16arrayPtr();
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
    /// <returns>
    /// The next chunk of a specific type, or 0 if none exists.
    /// </returns>
    unsigned int FindChunkInROM(unsigned char *ROMData, unsigned int ROMLength, unsigned int startAddr, enum SaveDataChunkType chunkType)
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
                if(ValidRATS(ROMData + startAddr) && ROMData[startAddr + 8] == chunkType)
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
    /// <returns>
    /// A list of all chunks of a specific type.
    /// </returns>
    QVector<unsigned int> FindAllChunksInROM(unsigned char *ROMData, unsigned int ROMLength, unsigned int startAddr, enum SaveDataChunkType chunkType)
    {
        QVector<unsigned int> chunks;
        while(startAddr < ROMLength)
        {
            unsigned int chunk = FindChunkInROM(ROMData, ROMLength, startAddr, chunkType);
            if(chunk)
            {
                chunks.append(chunk);
                unsigned short chunkLen = *reinterpret_cast<unsigned short*>(ROMData + startAddr + 4);
                startAddr += chunkLen + 12;
            }
            else break;
        }
        return chunks;
    }

    /// <summary>
    /// Save a list of chunks to the ROM file.
    /// </summary>
    /// <param name="filePath">
    /// The file name to use when saving the ROM.
    /// </param>
    /// <param name="chunks">
    /// The chunks to save to the ROM.
    /// </param>
    /// <param name="ChunkAllocationCallback">
    /// Callback function that allocates additional chunks based on data from chunks which have been
    /// allocated by this function before saving to ROM. This function may not allocate invalidation chunks.
    /// This function returns an error string if unsuccessful, or an empty string if successful.
    /// </param>
    /// <param name="PostProcessingCallback">
    /// Post-processing to perform after writing the save chunks, but before saving the file itself.
    /// This function returns an error string if unsuccessful, or an empty string if successful.
    /// </param>
    /// <returns>
    /// True if the save was successful.
    /// </returns>
    bool SaveFile(QString filePath, QVector<struct SaveData> chunks,
        std::function<QString (unsigned char*, QVector<struct SaveData>&, std::map<int, int>)> ChunkAllocationCallback,
        std::function<QString (unsigned char*, std::map<int, int>)> PostProcessingCallback)
    {
        // Finding space for the chunks can be done faster if the chunks are ordered by size
        unsigned char *TempFile = (unsigned char *) malloc(CurrentFileSize);
        unsigned int TempLength = CurrentFileSize;
        memcpy(TempFile, CurrentFile, CurrentFileSize);
        std::sort(chunks.begin(), chunks.end(),
                  [](const struct SaveData &a, const struct SaveData &b) { return a.size < b.size; });
        std::map<int, int> chunkIDtoIndex;
        for (int i = 0; i < chunks.size(); ++i)
        {
            chunkIDtoIndex[chunks[i].index] = i;
        }

        // Invalidate old chunk data
        for(struct SaveData chunk : chunks)
        {
            if (chunk.old_chunk_addr > WL4Constants::AvailableSpaceBeginningInROM)
            {
                unsigned char *RATSaddr = TempFile + chunk.old_chunk_addr - 12;
                if (ValidRATS(RATSaddr)) // old_chunk_addr should point to the start of the chunk data, not the RATS tag
                {
                    strncpy((char *) RATSaddr, "STAR_INV", 8);
                }
                else
                {
                    singleton->GetOutputWidgetPtr()->PrintString("Internal error while saving changes to ROM: Invalidation chunk references an invalid RATS identifier for existing chunk. Save chunk index: " +
                        QString::number(chunk.index) + ". Address: 0x" + QString::number(chunk.old_chunk_addr - 12, 16).toUpper() + ". Changes not saved.");
                    return false;
                }
            }
        }

        // Find space in the ROM for each chunk and assign addresses for the chunks
        // also expand the ROM size as necessary (up to 32MB) to hold the new data.
        bool success = false;
        std::map<int, int> indexToChunkPtr;
        QVector<struct SaveData> chunksToAdd(chunks);
        do {
            int startAddr = WL4Constants::AvailableSpaceBeginningInROM;
            for(struct SaveData chunk : chunksToAdd)
            {
                if(chunk.ChunkType == SaveDataChunkType::InvalidationChunk) continue; // do not allocate for invalidation chunks

                int chunkSize = chunk.size + 12 + (chunk.alignment ? 3 : 0);
findspace:      int chunkAddr = FindSpaceInROM(TempFile, TempLength, startAddr, chunkSize);
                if(chunk.alignment) chunkAddr = (chunkAddr + 3) & ~3; // align the chunk address
                if(!chunkAddr)
                {
                    // Expand ROM (double the size and align to 8MB)
                    unsigned int newSize = (TempLength << 1) & ~0x7FFFFF;
                    if(newSize <= 0x2000000)
                    {
                        unsigned char *newTempFile = (unsigned char*) realloc(TempFile, newSize);
                        if(!newTempFile)
                        {
                            // Realloc failed due to system memory constraints
                            QMessageBox::warning(
                                singleton,
                                "Out of memory",
                                "Unable to save changes because your computer is out of memory.",
                                QMessageBox::Ok,
                                QMessageBox::Ok
                            );
                            goto error;
                        }
                        TempFile = newTempFile;
                        memset(TempFile + TempLength, 0xFF, newSize - TempLength);
                        TempLength = newSize;
                        goto findspace;
                    }
                    else
                    {
                        // Size cannot exceed 32MB
                        QMessageBox::warning(
                            singleton,
                            "ROM too large",
                            QString("Unable to save changes because ") + QString::number(chunkSize) +
                                " contiguous free bytes are necessary, but such a region could not be"
                                " found, and the ROM file cannot be expanded larger than 32MB.",
                            QMessageBox::Ok,
                            QMessageBox::Ok
                        );
                        goto error;
                    }
                }
                indexToChunkPtr[chunk.index] = chunkAddr;

                // Mark the region for the chunk as used, with RATS
                unsigned char *destPtr = TempFile + chunkAddr;
                strncpy(reinterpret_cast<char*>(destPtr), "STAR", 4);
                unsigned short chunkLen = (unsigned short) chunk.size;
                *reinterpret_cast<unsigned short*>(destPtr + 4) = chunkLen;
                *reinterpret_cast<unsigned short*>(destPtr + 6) = ~chunkLen;
                *reinterpret_cast<unsigned int*>(destPtr + 8) = 0;
                destPtr[8] = chunk.ChunkType;

                startAddr = chunkAddr + chunk.size + 12; // do not search through old areas of the ROM again
            }

            // Perform chunk allocation callback
            if(ChunkAllocationCallback)
            {
                QString ret(ChunkAllocationCallback(TempFile, chunksToAdd, indexToChunkPtr));
                if(ret != "")
                {
                    success = false;
                    goto error;
                }
                chunks.append(chunksToAdd); // Add any additional chunks created in the callback
            }
            else
            {
                chunksToAdd.clear();
            }
        } while(!chunksToAdd.empty());

        // Apply source pointer modifications to applicable chunk types
        for(struct SaveData chunk : chunks)
        {
            switch(chunk.ChunkType)
            {
            case SaveDataChunkType::InvalidationChunk:
            case SaveDataChunkType::PatchListChunk:
            case SaveDataChunkType::PatchChunk:
                continue; // the above chunk types are not associated with a modified pointer in main ROM
            default:;
            }

            unsigned char *ptrLoc = chunk.dest_index ?
                // Source pointer is in another chunk
                chunks[chunkIDtoIndex[chunk.dest_index]].data + chunk.ptr_addr
                    :
                // Source pointer is in main ROM
                TempFile + chunk.ptr_addr;

            // We add 12 to the pointer location because the chunk ptr starts at the chunk's RATS tag
            *reinterpret_cast<unsigned int*>(ptrLoc) = static_cast<unsigned int>((indexToChunkPtr[chunk.index] + 12) | 0x8000000);
        }

        // Write each chunk to TempFile
        for(struct SaveData chunk : chunks)
        {
            if (chunk.ChunkType == SaveDataChunkType::InvalidationChunk)
                continue;

            // Sanity check that chunks are not above the allowed size
            if(chunk.size & 0xFFFF0000)
            {
                // Chunk size must be a 16-bit value
                QMessageBox::warning(singleton, "RATS chunk too large",
                     QString("Unable to save changes because ") + QString::number(chunk.size) +
                         " contiguous free bytes are necessary for some save chunk of type " +
                         QString::number(chunk.ChunkType) +
                         ", but the editor currently"
                         " only supports up to size " +
                         QString::number(0xFFFF) + ".",
                     QMessageBox::Ok, QMessageBox::Ok);
                goto error;
            }

            // Write the chunk metadata
            unsigned char *destPtr = TempFile + indexToChunkPtr[chunk.index];
            *reinterpret_cast<unsigned int*>(destPtr + 8) = 0;
            destPtr[8] = chunk.ChunkType;

            // Write the data
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
                QMessageBox::warning(singleton, "Could not save file",
                     "Unable to write to or create the ROM file for saving.", QMessageBox::Ok,
                     QMessageBox::Ok);
                goto error;
            }
            file.close();

            // Set the CurrentFile to the copied CurrentFile data
            auto temp = CurrentFile;
            CurrentFile = TempFile;
            delete[] temp;
            CurrentFileSize = TempLength;
        }

        // Set that there are no changes to the ROM now (so no save prompt is given)
        singleton->SetUnsavedChanges(false);

        // Clean up heap data and return
        success = true;
        if (0)
        {
        error:
            free(TempFile);
        }
        for(struct SaveData chunk : chunks)
        {
            if (chunk.ChunkType != SaveDataChunkType::InvalidationChunk)
                free(chunk.data);
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
        currentLevel->GetSaveChunks(chunks);

        // Get Tilesets chunks
        for(int i = 0; i < 92; ++i)
        {
            if(singletonTilesets[i]->IsNewTileset())
            {
                GenerateTilesetSaveChunks(i, chunks);
            }
        }

        // Isolate the room header chunk for post-processing
        struct SaveData roomHeaderChunk = *std::find_if(chunks.begin(), chunks.end(), [](const struct SaveData &chunk) {
            return chunk.ChunkType == SaveDataChunkType::RoomHeaderChunkType;
        });
        unsigned int roomHeaderInROM;

        // Save the level
        bool ret = SaveFile(filePath, chunks, nullptr,
            [levelHeaderPointer, currentLevel, roomHeaderChunk, &roomHeaderInROM]
            (unsigned char *TempFile, std::map<int, int> indexToChunkPtr)
            {
                // Capture pointer to new room header location
                roomHeaderInROM = static_cast<unsigned int>(indexToChunkPtr[roomHeaderChunk.index] + 12);

                // Write the level header to the ROM
                memcpy(TempFile + levelHeaderPointer, currentLevel->GetLevelHeader(), sizeof(struct LevelComponents::__LevelHeader));

                // Write Tileset data length info
                for(int i = 0; i < 92; ++i)
                {
                    if(singletonTilesets[i]->IsNewTileset())
                    {
                        // Save Animated Tile info table
                        unsigned short *AnimatedTileInfoTable = singletonTilesets[i]->GetAnimatedTileData();
                        memcpy(TempFile + i * 32 + WL4Constants::AnimatedTileIdTableCase2, (unsigned char*)AnimatedTileInfoTable, 32);

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
                return QString("");
            });
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
                (CurrentFile + roomHeaderInROM + i * sizeof(struct LevelComponents::__RoomHeader));
            unsigned int *layerDataPtrs = (unsigned int*) &roomHeader->Layer0Data;
            LevelComponents::Room *room = rooms[i];
            room->SetCameraBoundaryDirty(false);
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

} // namespace ROMUtils
