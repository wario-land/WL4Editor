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
    extern bool ChunkTypeAlignment[CHUNK_TYPE_COUNT];

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
        unsigned int ChunkAddress = 0;
        bool HeaderHasBroken = false;
        QVector<unsigned int> ChildrenChunkLocalOffset = QVector<unsigned int>();
        QVector<unsigned int> BrokenChildrenChunkLocalOffset = QVector<unsigned int>();
        // Save where all the possible conflicts from, the conflicts' details should be generated in the UI logic
        // won't be really useful since I have add sanity check to prevent chunk overwrite in SaveFile function
        // and it is headache to implement the logic to do the check, so for now, let me just comment this out -- ssp
//        QVector<unsigned int> ChildrenChunkConflictWith = QVector<unsigned int>();

        // some practice about writing different constructors, and the result is: just don't do it (doge) -- ssp
        // default constructor is needed once we have other types of constructor of this struct
//        ChunkReference() : ChunkType(InvalidationChunk), ParentChunkAddress(0), ChunkAddress(0) {}

        // copy constructor, which's equivalent to move constructor by default
//        ChunkReference(const ChunkReference &chunkreference) { }

        // initializer_list for some weird syntax: ChunkReference cr{data, data2, ...}; // but the elements should be of the same type
        // cannot use it here
//        ChunkReference(std::initializer_list<typeof_some_elements_with_the_same_type> chunkreference_initlist)
//        {
//            ChunkType = std::static_cast<typeof_some_elements_with_the_same_type>(*chunkreference_initlist.begin());
//        }

        ChunkReference &operator = (const ChunkReference &chunkreference)
        {
            this->ChunkType = chunkreference.ChunkType;
            this->ParentChunkAddress = chunkreference.ParentChunkAddress;
            this->ChunkAddress = chunkreference.ChunkAddress;
            this->HeaderHasBroken = chunkreference.HeaderHasBroken;
            this->ChildrenChunkLocalOffset.clear();
            this->BrokenChildrenChunkLocalOffset.clear();
//            this->ChildrenChunkConflictWith.clear();
            this->ChildrenChunkLocalOffset.append(chunkreference.ChildrenChunkLocalOffset);
            this->BrokenChildrenChunkLocalOffset.append(chunkreference.BrokenChildrenChunkLocalOffset);
//            this->ChildrenChunkConflictWith.append(chunkreference.ChildrenChunkConflictWith);
            return *this;
        }

        // will be useful when sort
//        bool operator < (const ChunkReference &chunkreference) const
//        {
//            if (this->ChunkAddress < chunkreference.ChunkAddress)
//                return true;
//            return false;
//        }
    };

    // Exposed helper functions
    void FormatPathSeperators(QString &path);

    // Global functions
    void CleanUpTmpCurrentFileMetaData();
    unsigned int IntFromData(int address);
    unsigned int PointerFromData(int address);
    unsigned int PackScreen(unsigned short *screenCharData, unsigned short *&outputCompressedData, bool skipzeros = true);
    unsigned short *UnPackScreen(uint32_t address);
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
    QString SaveDataAnalysis();
    QMap<unsigned int, struct ChunkReference> GetAllChunkReferences();
    void StaticInitialization();
    bool WriteChunkSanityCheck(const struct SaveData &chunk, const unsigned int chunk_addr, const QVector<unsigned int> &existChunks);
    bool DefragmentChunks(QVector<unsigned int> chunks);

} // namespace ROMUtils

#endif // ROMUTILS_H
