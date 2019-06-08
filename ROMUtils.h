#ifndef ROMUTILS_H
#define ROMUTILS_H

#include <QString>
#include <cstdio> //include definition for FILE
#include <cstring>
#include <string>
#include <vector>

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
    unsigned int LayerRLECompress(unsigned int _layersize, unsigned short *LayerData,
                                  unsigned char **OutputCompressedData);
    int FindSpaceInROM(unsigned char *ROMData, int ROMLength, int startAddr, int chunkSize);
    bool SaveFile(QString fileName);

    enum SaveDataChunkType
    {
        InvalidationChunk = '\x00',
        RoomHeaderChunkType = '\x01',
        DoorChunkType = '\x02',
        LayerChunkType = '\x03',
        LevelNameChunkType = '\x04',
        EntityListChunk = '\x05',
        CameraPointerTableType = '\x06',
        CameraBoundaryChunkType = '\x07',
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
} // namespace ROMUtils

#endif // ROMUTILS_H
