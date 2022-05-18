#include "ROMUtils.h"
#include "Compress.h"
#include "Operation.h"
#include <QFile>
#include <QTranslator>
#include "WL4EditorWindow.h"
#include "PatchUtils.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
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
        "EntitySetLoadTableChunkType",
        "ScatteredGraphicListChunkType",
        "ScatteredGraphicTile8x8DataChunkType",
        "ScatteredGraphicmappingChunkType",
        "ScatteredGraphicPaletteChunkType"
    };

    bool ChunkTypeAlignment[CHUNK_TYPE_COUNT] = {
        false, // InvalidationChunk
        true,  // RoomHeaderChunkType
        true,  // DoorChunkType
        false, // LayerChunkType
        false, // LevelNameChunkType
        false, // EntityListChunk
        true,  // CameraPointerTableType
        true,  // CameraBoundaryChunkType
        false, // PatchListChunk
        true,  // PatchChunk
        true,  // TilesetForegroundTile8x8DataChunkType
        true,  // TilesetMap16EventTableChunkType
        true,  // TilesetMap16TerrainChunkType
        true,  // TilesetMap16DataChunkType
        true,  // TilesetPaletteDataChunkType
        true,  // EntityTile8x8DataChunkType
        true,  // EntityPaletteDataChunkType
        true,  // EntitySetLoadTableChunkType
        false, // ScatteredGraphicListChunkType         = '\x12',
        true,  // ScatteredGraphicTile8x8DataChunkType  = '\x13',
        false, // ScatteredGraphicmappingChunkType      = '\x14',
        true,  // ScatteredGraphicPaletteChunkType      = '\x15'
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
    /// Get an 32 bytes uchar array of Tile8x8 graphic data, then X flip the data of its graphic.
    /// </summary>
    /// <param name="source">
    /// The pointer to read uchar array Tile8x8 data.
    /// </param>
    /// <param name="destination">
    /// The pointer to save changed Tile8x8 data to.
    /// </param>
    void Tile8x8DataXFlip(unsigned char *source, unsigned char *destination)
    {
        for (int col = 0; col < 8; col++)
        {
            for (int row = 0; row < 4; row++)
            {
                unsigned char curByte = source[col * 4 + row];
                curByte = ((curByte & 0xF) << 4) | ((curByte & 0xF0) >> 4);
                destination[col * 4 + (3 - row)] = curByte;
            }
        }
    }

    /// <summary>
    /// Get an 32 bytes uchar array of Tile8x8 graphic data, then Y flip the data of its graphic.
    /// </summary>
    /// <param name="source">
    /// The pointer to read uchar array Tile8x8 data.
    /// </param>
    /// <param name="destination">
    /// The pointer to save changed Tile8x8 data to.
    /// </param>
    void Tile8x8DataYFlip(unsigned char *source, unsigned char *destination)
    {
        for (int col = 0; col < 8; col++)
        {
            memcpy(&destination[col * 4], &source[(7 - col) * 4], 4 * sizeof(unsigned char));
        }
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
    /// Compress a whole screen of character data.
    /// </summary>
    /// <param name="screenCharData">
    /// A pointer to a whole screen of character data.
    /// </param>
    /// <param name="outputCompressedData">
    /// A pointer to the output compressed character data.
    /// </param>
    /// <return>The length of output data (number of unsigned short).</return>
    unsigned int PackScreen(unsigned short *screenCharData, unsigned short *&outputCompressedData, bool skipzeros)
    {
        /*** compressed data format:
         * Compressed data format:
         * 1st ushort: t | o o o o o o o o o o | n n n n n
         * 2nd ushort: the unsigned short value of the current character
         ************************************************
         * number(n): the loop counter
         * offset(o): the offset of the current character in the non-compressed char data array
         * type(t): there are 2 cases:
         * if t = 0, then the decompressed data will be:
         * o, o + 1, o + 2, ... , o + n - 1. (n continuous numbers in total)
         * if t = 1, then the decompressed data will be:
         * o, o, o, o, ... , o. (duplicate o by n times)
         ************************************************
         * The compressed data array should end with an additional 0x0000,
         * the decompression function ingame need it to stop decompression
         */
        int offset = 0; // should be in range of [0, 0x3FF]
        QVector<unsigned short> output;
        while (offset < 0x3FF)
        {
            unsigned short curChar = screenCharData[offset];
            int num_dup = 0;
            int num_AddByOne = 0;
            for (int i = 1; i < 32; ++i) // type = 1
            {
                if ((curChar != screenCharData[offset + i]) || ((offset + i) == 0x3FF))
                {
                    break;
                }
                num_dup++;
            }
            for (int i = 1; i < 32; ++i) // type = 0
            {
                if (((curChar + i) != screenCharData[offset + i]) || ((offset + i) == 0x3FF))
                {
                    break;
                }
                num_AddByOne++;
            }

            if (num_dup >= num_AddByOne)
            {
                if (skipzeros && !curChar)
                {
                    goto skip_append_output_1;
                }
                output << ((0x8000 | ((offset & 0x3FF) << 5) | num_dup) & 0xFFFF);
                output << curChar;
skip_append_output_1:
                offset += num_dup + 1;
            }
            else // num_dup < num_AddByOne, type = 1
            {
                if (skipzeros && !curChar)
                {
                    goto skip_append_output_2;
                }
                output << ((((offset & 0x3FF) << 5) | num_AddByOne) & 0x7FFF);
                output << curChar;
skip_append_output_2:
                offset += num_AddByOne + 1;
            }
        }
        output << 0x0000;
        int output_size = output.size();
        outputCompressedData = new unsigned short[output_size];
        unsigned short *operationPtr = outputCompressedData;
        memset((unsigned char *)operationPtr, 0, sizeof(unsigned short) * output_size);
        for (int i = 0; i < output_size; ++i)
        {
            *operationPtr = output[i];
            operationPtr++;
        }
        return output_size;
    }

    /// <summary>
    /// Decompress a whole screen of character data.
    /// </summary>
    /// <param name="address">
    /// A pointer into the ROM data to start reading from.
    /// </param>
    /// <return>A pointer to decompressed data.</return>
    unsigned short *UnPackScreen(uint32_t address)
    {
        // directly modified from disassembled rom's code, C code generated by IDA pro
        unsigned short *v2, *src;
        unsigned short i;
        unsigned short *dst = new unsigned short[32 * 32];
        unsigned short *v5;
        unsigned short *v6;
        unsigned short v7;
        unsigned short j;
        unsigned short v9;
        unsigned short k;
        unsigned short v11;

        memset(dst, 0, 32 * 32 * sizeof(unsigned short));

        // check if address is an odd number
        if (address & 1)
        {
            singleton->GetOutputWidgetPtr()->PrintString(QT_TR_NOOP("Error in ROMUtils::UnPackScreen(int address): input parameter 'address' should be an odd number."));
            return dst;
        }
        v2 = src = (unsigned short *)(ROMFileMetadata->ROMDataPtr + address);
        for (i = *src; *v2; i = *v2)
        {
            v5 = (dst + ((i >> 5) & 0x3FF));
            v6 = v2 + 1;
            if ((i & 0x8000) != 0)
            {
                v7 = *v6;
                v2 = v6 + 1;
                for (j = i & 0x1F; j != 0xFFFF; --j)
                {
                    // don't write memory if memory leak
                    if (v5 > (dst + 32 * 32 - 1))
                    {
                        v5++;
                        continue;
                    }
                    *v5++ = v7;
                }
            }
            else
            {
                v9 = *v6;
                v2 = v6 + 1;
                for (k = i & 0x1F; k != 0xFFFF; --k)
                {
                    v11 = v9++;
                    // don't write memory if memory leak
                    if (v5 > (dst + 32 * 32 - 1))
                    {
                        v5++;
                        continue;
                    }
                    *v5++ = v11;
                }
            }
        }
        return dst;
    }

    /// <summary>
    /// Decompress ROM data that was compressed with run-length encoding.
    /// </summary>
    /// <remarks>
    /// The <paramref name="outputSize"/> parameter specifies the predicted output size in bytes.
    /// The return unsigned char * is on the heap, delete it after using.
    /// </remarks>
    /// <param name="address">
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
    /// Callback used for allocating chunks from a list.
    /// Must call init before using this function as a callback.
    /// </summary>
    static int CHUNK_INDEX;
    static QVector<struct SaveData> CHUNK_ALLOC;
    static ChunkAllocationStatus AllocateChunksFromList(unsigned char *TempFile, struct FreeSpaceRegion freeSpace, struct SaveData *sd, bool resetchunkIndex)
    {
        (void) TempFile;

        // This part of code will be triggered when rom size needs to be expanded
        // So all the chunks will be reallocated
        if(resetchunkIndex)
        {
            CHUNK_INDEX = 0;
        }

        if(CHUNK_INDEX >= CHUNK_ALLOC.size())
        {
            return ChunkAllocationStatus::NoMoreChunks;
        }

        // Get the size of the space that would be needed at this address depending on alignment
        unsigned int alignOffset = 0;
        if(CHUNK_ALLOC[CHUNK_INDEX].alignment)
        {
            unsigned int startAddr = (freeSpace.addr + 3) & ~3;
            alignOffset = startAddr - freeSpace.addr;
        }

        // Check if there is space for the chunk in the offered area
        // required_size > (freespace.size - alignment - 12 bytes (for header))
        if(CHUNK_ALLOC[CHUNK_INDEX].size > freeSpace.size - alignOffset - 12)
        {
            // This will request a larger free area
            return ChunkAllocationStatus::InsufficientSpace;
        }
        else
        {
            // Accept the offered free area for this save chunk
            *sd = CHUNK_ALLOC[CHUNK_INDEX++];
            return ChunkAllocationStatus::Success;
        }
    }
    static ChunkAllocationStatus AllocateChunksFromListInit(QVector<struct SaveData> chunksToAllocate)
    {
        CHUNK_INDEX = 0;
        CHUNK_ALLOC = chunksToAllocate;
        return ChunkAllocationStatus::Success;
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

            // Restore temp indices info about saving chunks
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

        // Write chunks to TempFile with Sanity check
        for(struct SaveData &chunk : chunksToAdd)
        {
            if (chunk.ChunkType == SaveDataChunkType::InvalidationChunk)
            {
                continue;
            }

            // update existChunks every time since it changes every time
            QVector<unsigned int> existChunks = FindAllChunksInROM(
                        TempFile,
                        TempLength,
                        WL4Constants::AvailableSpaceBeginningInROM,
                        SaveDataChunkType::InvalidationChunk,
                        true
                        );

            // Write the chunk metadata with RATS format and the chunk data
            if (WriteChunkSanityCheck(chunk, indexToChunkPtr[chunk.index], existChunks))
            {
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
            else
            {
                singleton->GetOutputWidgetPtr()->PrintString(QT_TR_NOOP("Internal error: Write chunk into an occupied chunk"));
                goto error;
            }
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
            // Save another copy of the current rom file if Rolling Save feature is toggled on
            QFileInfo curfileinfo(filePath);
            for (int i = 0; i < 2; i++)
            {
                QString tmpFilePath = filePath;
                if (i == 1)
                {
                    QString dirstr = QFileInfo(filePath).dir().path() + "/auto_backups/";
                    FormatPathSeperators(dirstr);
                    if (int maxTmpFileNum = SettingsUtils::GetKey(SettingsUtils::IniKeys::RollingSaveLimit).toInt())
                    {
                        QDir dir;
                        if (!dir.exists(dirstr))
                        {
                            singleton->GetOutputWidgetPtr()->PrintString("auto_backups folder does not exist, try to mkdir.");
                            dir.setPath(QFileInfo(filePath).dir().path());
                            if (dir.mkdir("auto_backups"))
                            {
                                singleton->GetOutputWidgetPtr()->PrintString("mkdir success!");
                            }
                        }

                        // Delete old files
                        if (maxTmpFileNum > 0)
                        {
                            dir.setPath(dirstr);
                            dir.setFilter(QDir::Files | QDir::NoSymLinks); // dir.setSorting(QDir::Name); sort by name by default
                            QFileInfoList fileInfoList = dir.entryInfoList();
                            int count = 0, id = 0;
                            QVector<int> matchedfileId;
                            for (auto &fileinfo : fileInfoList)
                            {
                                QString tmpfileName = fileinfo.completeBaseName();
                                if (tmpfileName.contains(curfileinfo.completeBaseName() + "_"))
                                {
                                    matchedfileId << id;
                                    count++;
                                    if (count >= maxTmpFileNum)
                                    {
                                        QString delete_file_path = fileInfoList[matchedfileId[count - maxTmpFileNum]].filePath();
                                        singleton->GetOutputWidgetPtr()->PrintString("Remove file: " + delete_file_path);
                                        QFile::remove(delete_file_path);
                                    }
                                }
                                id++;
                            }
                        }
                        /***From the documentation of fuction QDir::separator():
                         * You do not need to use this function to build file paths.
                         * If you always use "/",
                         * Qt will translate your paths to conform to the underlying operating system.
                         * -------------------------------------
                         * the seperator will always cause problem in different system,
                         * especially used with some "/" or "\" add by ourselves.
                         * just follow the documentations and use the void ROMUtils::FormatPathSeperators(). --- ssp*/
                        tmpFilePath = dirstr +
                                curfileinfo.completeBaseName() +
                                "_" +
                                QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz") +
                                ".gba";
                        FormatPathSeperators(tmpFilePath);
                        singleton->GetOutputWidgetPtr()->PrintString("Create backup file: " + tmpFilePath);
                    }
                    else
                    {
                        break;
                    }
                }
                QFile file(tmpFilePath);
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
            }

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
        for(struct SaveData &chunk : chunksToAdd)
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
        AllocateChunksFromListInit(addedChunks);
        bool ret = SaveFile(filePath, invalidationChunks,

            // ChunkAllocator

            AllocateChunksFromList,

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

    /// <summary>
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
    QString SaveDataAnalysis()
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
            struct ChunkData cd = {
                chunkAddr,
                ROMUtils::GetChunkDataLength(chunkAddr) + 12,
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
        QString result;
        result = QString("Save data area: %1\n").arg(saveAreaSize) ;
        result += QString("Free space: %1 (%2%)\n").arg(totalFreeSpace).arg(100 * freeSpaceP, 6, 'f', 2);
        result += QString("  Fragmented: %1 (%2%)\n").arg(fragmentedSpace).arg(100 * freeSpaceP_frag, 6, 'f', 2);
        result += QString("  Non-fragmented: %1 (%2%)\n").arg(nonFragmentedSpace).arg(100 * freeSpaceP_nonFrag, 6, 'f', 2);
        result += QString("Used space: %1 (%2%)\n").arg(totalUsedSpace).arg(100 * usedSpaceP, 6, 'f', 2);
        for(int i = 0; i < CHUNK_TYPE_COUNT; ++i)
        {
            enum SaveDataChunkType t = static_cast<enum SaveDataChunkType>(i);
            result += QString("  %1:\n%2 (%3%, %4 chunks)\n")
                        .arg(ChunkTypeString[i]/*, -37*/)
                        .arg(chunkTypeSpace[t], 7)
                        .arg(100 * usedSpaceP_ofType[t], 6, 'f', 2)
                        .arg(chunkTypeCount[t]);
        }
        result += QString("  %1:\n%2 (%3%, %4 chunks)\n")
                    .arg("Other"/*, -37*/)
                    .arg(otherTypeSpace, 7)
                    .arg(100 * usedSpaceP_ofTypeOther, 6, 'f', 2)
                    .arg(otherTypeCount);
        return result;
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

    /// <summary>
    /// Check if the space to write the current chunk is legal
    /// The chunks used to compare are generated instantly
    /// </summary>
    /// <param name="chunk">
    /// Contains the writing chunk's data
    /// </param>
    /// <param name="chunk_addr">
    /// Contains the address of the chunk being written
    /// </param>
    /// <param name="existChunks">
    /// Contains all the exist chunks data
    /// make this as a parameter so we can modify the chunks list then push it into this function
    /// Also generate the data outside of the function can make the code execute potentially faster
    /// </param>
    /// <returns>
    /// True if the writing is legal.
    /// </returns>
    bool WriteChunkSanityCheck(const SaveData &chunk, const unsigned int chunk_addr, const QVector<unsigned int> &existChunks)
    {
        if ((chunk_addr > WL4Constants::AvailableSpaceBeginningInROM) || (chunk.ChunkType == SaveDataChunkType::InvalidationChunk))
        {
            return true;
        }

        unsigned int chunk_size = chunk.size;
        unsigned int chunkNum = existChunks.size();
        unsigned low = 0, high = chunkNum, middle = 0;
        while (low < high)
        {
            middle = (low + high) / 2;
            unsigned int existChunkAddr_Middle = existChunks[middle];
            unsigned int existChunkSize_Middle =ROMUtils::GetChunkDataLength(existChunkAddr_Middle + 4);

            // exist chunk range: [existChunkAddr_Middle, existChunkAddr_Middle + 12 + existChunkSize_Middle)
            // new chunk range: [chunk_addr, chunk_addr + 12 + chunk_size)
            unsigned int existChunkRangeL_middle = existChunkAddr_Middle;
            unsigned int existChunkRangeR_middle = existChunkAddr_Middle + 12 + existChunkSize_Middle;
            unsigned int newChunkRangeL = chunk_addr;
            unsigned int newChunkRangeR = chunk_addr + 12 + chunk_size;
            if((newChunkRangeL < existChunkRangeR_middle && newChunkRangeL >= existChunkRangeL_middle) ||
                    (newChunkRangeR > existChunkRangeL_middle && newChunkRangeR <= existChunkRangeR_middle)) {
                return false;
            } else if(newChunkRangeR <= existChunkRangeL_middle) {
                high = middle;
            } else if(newChunkRangeL >= existChunkRangeR_middle) {
                low = middle + 1;
            }
        }
        return true;
    }

    /// <summary>
    /// Replace all the things like '/', '\', '\\', '\/', '/\' and '//' with '/'
    /// then the path string can be used by Qt file things
    /// </summary>
    /// <param name="path">
    /// QString of path with some shit seperators in it.
    /// </param>
    /// <returns>
    /// QString of path without shit seperator.
    /// </returns>
    void FormatPathSeperators(QString &path)
    {
        while (path.contains('\\'))
        {
            path.replace('\\', '/');
        }
        while (path.contains("//"))
        {
            path.replace("//", "/");
        }
    }

    /// <summary>
    /// Get the length of data in an existing chunk
    /// </summary>
    /// <param name="chunkheaderAddr">
    /// The address of an existing chunk.
    /// </param>
    /// <returns>
    /// the length of the data in the chunk.
    /// </returns>
    unsigned int GetChunkDataLength(unsigned int chunkheaderAddr)
    {
        unsigned int chunkLen = *reinterpret_cast<unsigned short*>(ROMFileMetadata->ROMDataPtr + chunkheaderAddr + 4);
        unsigned int extLen = (unsigned int) *reinterpret_cast<unsigned char*>(ROMFileMetadata->ROMDataPtr + chunkheaderAddr + 9) << 16;
        return chunkLen + extLen;
    }

} // namespace ROMUtils
