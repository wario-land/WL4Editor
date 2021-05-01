#ifndef ROMUTILS_H
#define ROMUTILS_H

#include <QString>
#include <map>
#include <functional>
#include <cstring>
#include <string>
#include <vector>

#include "WL4Constants.h"
#include "LevelComponents/Tileset.h"
#include "LevelComponents/EntitySet.h"
#include "LevelComponents/Entity.h"
#include "LevelComponents/Layer.h"

#define CHUNK_TYPE_COUNT 18

namespace ROMUtils
{
    struct ROMFileMetadata
    {
        unsigned int Length;
        QString FilePath;
        unsigned char *ROMDataPtr;
    };

    // Global variables
    extern struct ROMFileMetadata CurrentROMMetadata;
    extern struct ROMFileMetadata TempROMMetadata;
    extern struct ROMFileMetadata *ROMFileMetadata;

    extern unsigned int SaveDataIndex;

    extern LevelComponents::Tileset *singletonTilesets[92];
    extern LevelComponents::Entity *entities[129];
    extern LevelComponents::EntitySet *entitiessets[90];

    extern const char *ChunkTypeString[CHUNK_TYPE_COUNT];

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
        // the next one is not necessary, but i added it by accident -- ssp
        TilesetPaletteDataChunkType           = '\x0E',
        EntityTile8x8DataChunkType            = '\x0F',
        EntityPaletteDataChunkType            = '\x10',
        EntitySetLoadTableChunkType           = '\x11'
    };

    enum ChunkAllocationStatus
    {
        Success           = '\x00',
        InsufficientSpace = '\x01',
        NoMoreChunks      = '\x02',
        ProcessingError   = '\x03'
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
        unsigned int old_chunk_addr; // address of the old chunk that was pointed to. This should point to the start of the chunk data, not the RATS tag
        enum SaveDataChunkType ChunkType;
    };

    struct FreeSpaceRegion
    {
        unsigned int addr;
        unsigned int size;
    };

    struct ChunkReference
    {
        enum SaveDataChunkType ChunkType = InvalidationChunk;
        unsigned int ParentChunkAddress = 0;
        QVector<unsigned int> ChildrenChunkLocalOffset = QVector<unsigned int>();
    };

    // Global functions
    void CleanUpTmpCurrentFileMetaData();
    unsigned int IntFromData(int address);
    unsigned int PointerFromData(int address);
    unsigned char *LayerRLEDecompress(int address, size_t outputSize);
    unsigned int LayerRLECompress(unsigned int _layersize, unsigned short *LayerData, unsigned char **OutputCompressedData);
    unsigned int FindChunkInROM(unsigned char *ROMData, unsigned int ROMLength, unsigned int startAddr, enum SaveDataChunkType chunkType, bool anyChunk = false);
    QVector<unsigned int> FindAllChunksInROM(unsigned char *ROMData, unsigned int ROMLength, unsigned int startAddr, enum SaveDataChunkType chunkType, bool anyChunk = false);
    bool SaveFile(QString filePath, QVector<unsigned int> invalidationChunks,
        std::function<ChunkAllocationStatus (unsigned char*, struct FreeSpaceRegion, struct SaveData*, bool)> ChunkAllocator,
        std::function<QString (unsigned char*, std::map<int, int>)> PostProcessingCallback);
    bool SaveLevel(QString fileName);
    void LoadPalette(QVector<QRgb> *palette, unsigned short *dataptr, bool notdisablefirstcolor = false);
    void GenerateTilesetSaveChunks(int TilesetId, QVector<struct ROMUtils::SaveData> &chunks);
    void GenerateEntitySaveChunks(int GlobalEntityId, QVector<struct ROMUtils::SaveData> &chunks);
    void GenerateEntitySetSaveChunks(int EntitySetId, QVector<struct ROMUtils::SaveData> &chunks);
    unsigned int EndianReverse(unsigned int n);
    void SaveDataAnalysis();
    QMap<unsigned int, struct ChunkReference> GetAllChunkReferences();
    void StaticInitialization();
    
} // namespace ROMUtils

#endif // ROMUTILS_H
