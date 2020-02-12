#ifndef ROMUTILS_H
#define ROMUTILS_H

#include <QString>
#include <cstdio> //include definition for FILE
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "LevelComponents/Tileset.h"
#include "WL4Constants.h"

namespace ROMUtils
{
    // Global variables
    extern unsigned char *CurrentFile;
    extern unsigned int CurrentFileSize;
    extern QString ROMFilePath;
    extern unsigned int SaveDataIndex;

    // Global functions
    unsigned int IntFromData(int address);
    unsigned int PointerFromData(int address);
    unsigned char *LayerRLEDecompress(int address, size_t outputSize);
    unsigned int LayerRLECompress(unsigned int _layersize, const unsigned short *LayerData,
                                  unsigned char **OutputCompressedData);
    int FindSpaceInROM(unsigned char *ROMData, int ROMLength, int startAddr, int chunkSize);

    enum SaveDataChunkType
    {
        InvalidationChunk                     = '\x00',
        RoomHeaderChunkType                   = '\x01',
        DoorChunkType                         = '\x02',
        LayerChunkType                        = '\x03',
        LevelNameChunkType                    = '\x04',
        EntityListChunk                       = '\x05',
        CameraPointerTableType                = '\x06',
        CameraBoundaryChunkType               = '\x07',
        PatchListChunk                        = '\x08',
        PatchChunk                            = '\x09',
        TilesetForegroundTile8x8DataChunkType = '\x0A',
        TilesetMap16EventTableChunkType       = '\x0B',
        TilesetMap16TerrainChunkType          = '\x0C',
        TilesetMap16DataChunkType             = '\x0D',
        TilesetPaletteDataChunkType           = '\x0E'
    };

    struct SaveData
    {
        unsigned int ptr_addr;
        unsigned int size;
        unsigned char *data;
        unsigned int index;
        bool alignment;          // false: do not perform special alignment of the save chunk
        unsigned int dest_index; // 0: the address of the pointer that points to this chunk is in main ROM. Else, it is
                                 // in another chunk
        unsigned int old_chunk_addr; // address of the old chunk that was pointed to
        enum SaveDataChunkType ChunkType;
    };

    // Global functions
    unsigned int IntFromData(int address);
    unsigned int PointerFromData(int address);
    unsigned char *LayerRLEDecompress(int address, size_t outputSize);
    unsigned int LayerRLECompress(unsigned int _layersize, const unsigned short *LayerData,
                                  unsigned char **OutputCompressedData);
    int FindSpaceInROM(unsigned char *ROMData, int ROMLength, int startAddr, int chunkSize);
    unsigned int FindChunkInROM(unsigned char *ROMData, unsigned int ROMLength, unsigned int startAddr,
                                enum SaveDataChunkType chunkType);
    QVector<unsigned int> FindAllChunksInROM(unsigned char *ROMData, unsigned int ROMLength, unsigned int startAddr,
                                             enum SaveDataChunkType chunkType);
    bool SaveFile(const QString fileName, QVector<struct SaveData> chunks,
                  const std::function<void(QVector<struct SaveData>, std::map<int, int>)> &ChunkAllocationCallback,
                  const std::function<void(unsigned char *, std::map<int, int>)> &PostProcessingCallback);
    bool SaveLevel(QString filePath);
    void LoadPalette(QVector<QRgb> *palette, const unsigned short *dataptr);
    void GenerateTilesetSaveChunks(int TilesetId, QVector<struct ROMUtils::SaveData> &chunks);

    // Global variables
    extern unsigned char *CurrentFile;
    extern unsigned int CurrentFileSize;
    extern QString ROMFilePath;
    extern unsigned int SaveDataIndex;
    extern LevelComponents::Tileset *singletonTilesets[92];

} // namespace ROMUtils

#endif // ROMUTILS_H
