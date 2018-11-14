#ifndef ROMUTILS_H
#define ROMUTILS_H

#include <vector>
#include <string>
#include <cstdio>  //include definition for FILE
#include <cstring>
#include <QString>

#include "WL4Constants.h"

#define MIN_VAL(A,B) ((A)<(B)?(A):(B))
#define MAX_VAL(A,B) ((A)>(B)?(A):(B))

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
    unsigned char *LayerRLEDecompress(int address, int outputSize);
    unsigned int LayerRLECompress(unsigned int _layersize, unsigned short *LayerData, unsigned char **OutputCompressedData);
    unsigned int LayerRLECompress2(unsigned int _layersize, unsigned short *LayerData, unsigned char **OutputCompressedData);
    int FindSpaceInROM(int NewDataLength);
    int SaveTemp(unsigned char *tmpData, int pointerAddress, int dataLength);
    void SaveFile();

    struct SaveData {
        unsigned int ptr_addr;
        unsigned int size;
        unsigned char *data;
        unsigned int index;
        bool alignment; // false: do not perform special alignment of the save chunk
        unsigned int dest_index; // 0: the address of the pointer that points to this chunk is in main ROM. Else, it is in another chunk
    };
}

#endif // ROMUTILS_H
