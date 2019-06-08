#include "ROMUtils.h"
#include "Compress.h"
#include <QFile>
#include <WL4EditorWindow.h>
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
        assert(ret < CurrentFileSize /* Pointer is out of range */); // TODO proper error handling
        return ret;
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
    /// Save the currently loaded level to the ROM file.
    /// </summary>
    bool SaveFile(QString filePath)
    {
        // Obtain the list of data chunks to save to the rom
        SaveDataIndex = 1;
        bool success = false;
        QVector<struct SaveData> chunks;
        LevelComponents::Level *currentLevel = singleton->GetCurrentLevel();
        int levelHeaderOffset =
            WL4Constants::LevelHeaderIndexTable + currentLevel->GetPassage() * 24 + currentLevel->GetStage() * 4;
        int levelHeaderIndex = ROMUtils::IntFromData(levelHeaderOffset);
        int levelHeaderPointer = WL4Constants::LevelHeaderTable + levelHeaderIndex * 12;
        currentLevel->GetSaveChunks(chunks);

        // Finding space for the chunks can be done faster if the chunks are ordered by size
        std::sort(chunks.begin(), chunks.end(),
                  [](const struct SaveData &a, const struct SaveData &b) { return a.size < b.size; });
        std::map<int, int> chunkIDtoIndex;
        for (int i = 0; i < chunks.size(); ++i)
        {
            chunkIDtoIndex[chunks[i].index] = i;
        }

        // Invalidate old chunk data
        unsigned char *TempFile = (unsigned char *) malloc(CurrentFileSize);
        unsigned int TempLength = CurrentFileSize;
        memcpy(TempFile, CurrentFile, CurrentFileSize);
        foreach (struct SaveData chunk, chunks)
        {
            if (chunk.old_chunk_addr > WL4Constants::AvailableSpaceBeginningInROM)
            {
                unsigned char *RATSaddr = TempFile + chunk.old_chunk_addr - 12;
                if (ValidRATS(RATSaddr))
                {
                    strncpy((char *) RATSaddr, "STAR_INV", 8);
                }
                else
                    std::cout << "Invalid RATS identifier for existing chunk, index " << chunk.index << std::endl;
            }
        }

        // Find space in the ROM for each chunk and assign addresses for the chunks
        // also expand the ROM size as necessary (up to 32MB) to hold the new data.
        std::map<int, int> indexToChunkPtr;
        int startAddr = WL4Constants::AvailableSpaceBeginningInROM;
        foreach (struct SaveData chunk, chunks)
        {
            int chunkSize = chunk.size + 12 + (chunk.alignment ? 3 : 0);
        findspace:
            int chunkAddr = FindSpaceInROM(TempFile, TempLength, startAddr, chunkSize);
            if (chunk.alignment)
                chunkAddr = (chunkAddr + 3) & ~3; // align the chunk address
            if (!chunkAddr)
            {
                // Expand ROM (double the size and align to 8MB)
                unsigned int newSize = (TempLength << 1) & ~0x7FFFFF;
                if (newSize <= 0x2000000)
                {
                    unsigned char *newTempFile = (unsigned char *) realloc(TempFile, newSize);
                    if (!newTempFile)
                    {
                        // Realloc failed due to system memory constraints
                        QMessageBox::warning(singleton, "Out of memory",
                                             "Unable to save changes because your computer is out of memory.",
                                             QMessageBox::Ok, QMessageBox::Ok);
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
                    QMessageBox::warning(singleton, "ROM too large",
                                         QString("Unable to save changes because ") + QString::number(chunkSize) +
                                             " contiguous free bytes are necessary, but such a region could not be"
                                             " found, and the ROM file cannot be expanded larger than 32MB.",
                                         QMessageBox::Ok, QMessageBox::Ok);
                    goto error;
                }
            }
            indexToChunkPtr[chunk.index] = chunkAddr;
            startAddr = chunkAddr + chunk.size + 12; // do not re-search old areas of the ROM
        }

        // Apply source pointer modifications
        foreach (struct SaveData chunk, chunks)
        {
            if (chunk.ChunkType == SaveDataChunkType::InvalidationChunk)
                continue;

            unsigned char *ptrLoc = chunk.dest_index ?
                                                     // Source pointer is in another chunk
                                        chunks[chunkIDtoIndex[chunk.dest_index]].data + chunk.ptr_addr
                                                     :
                                                     // Source pointer is in main ROM
                                        TempFile + chunk.ptr_addr;

            // We add 12 to the pointer location because the chunk ptr starts at the chunk's RATS tag
            *(unsigned int *) ptrLoc = (indexToChunkPtr[chunk.index] + 12) | 0x8000000;
        }

        // Write each chunk to TempFile
        foreach (struct SaveData chunk, chunks)
        {
            if (chunk.ChunkType == SaveDataChunkType::InvalidationChunk)
                continue;

            // Create the RATS tag
            unsigned char *destPtr = TempFile + indexToChunkPtr[chunk.index];
            strncpy((char *) destPtr, "STAR", 4);
            if (chunk.size & 0xFFFF0000)
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
            unsigned short chunkLen = (unsigned short) chunk.size;
            *(unsigned short *) (destPtr + 4) = chunkLen;
            *(unsigned short *) (destPtr + 6) = ~chunkLen;

            // Write the chunk metadata
            *(unsigned int *) (destPtr + 8) = 0;
            destPtr[8] = chunk.ChunkType;

            // Write the data
            memcpy(destPtr + 12, chunk.data, chunkLen);
        }

        // Write the level header to the ROM
        memcpy(TempFile + levelHeaderPointer, currentLevel->GetLevelHeader(),
               sizeof(struct LevelComponents::__LevelHeader));

        { // Prevent goto from crossing initialization of variables here
            // Save the rom file from the CurrentFile copy
            QFile file(filePath);
            file.open(QIODevice::WriteOnly);
            if (file.isOpen())
            {
                file.write((const char *) TempFile, TempLength);
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
            free(CurrentFile);
            CurrentFile = TempFile;
            CurrentFileSize = TempLength;

            // Set the new internal data pointer for LevelComponents objects, and mark dirty objects as clean
            struct SaveData roomHeaderChunk =
                *std::find_if(chunks.begin(), chunks.end(),
                              [](const struct SaveData &chunk)
                              {
                                  return chunk.ChunkType == SaveDataChunkType::RoomHeaderChunkType;
                              });
            unsigned int roomHeaderInROM = indexToChunkPtr[roomHeaderChunk.index] + 12;
            std::vector<LevelComponents::Room *> rooms = currentLevel->GetRooms();
            for (unsigned int i = 0; i < rooms.size(); ++i)
            {
                struct LevelComponents::__RoomHeader *roomHeader =
                    (struct LevelComponents::__RoomHeader *) (CurrentFile + roomHeaderInROM +
                                                              i * sizeof(struct LevelComponents::__RoomHeader));
                unsigned int *layerDataPtrs = (unsigned int *) &roomHeader->Layer0Data;
                LevelComponents::Room *room = rooms[i];
                room->SetCameraBoundaryDirty(false);
                for (unsigned int j = 0; j < 4; ++j)
                {
                    LevelComponents::Layer *layer = room->GetLayer(j);
                    layer->SetDataPtr(layerDataPtrs[j] & 0x7FFFFFF);
                    layer->SetDirty(false);
                }
                for (unsigned int j = 0; j < 3; ++j)
                {
                    room->SetEntityListDirty(j, false);
                }
                struct LevelComponents::__RoomHeader newroomheader;
                memcpy(&newroomheader, roomHeader, sizeof(newroomheader));
                room->ResetRoomHeader(newroomheader);
                // TODO: if more elements in the game are going to be support customizing in the editor, more pointers
                // need to be reset here
            }
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
        foreach (struct SaveData chunk, chunks)
        {
            if (chunk.ChunkType != SaveDataChunkType::InvalidationChunk)
                free(chunk.data);
        }
        return success;
    }
} // namespace ROMUtils
