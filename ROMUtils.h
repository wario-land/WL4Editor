#ifndef ROMUTILS_H
#define ROMUTILS_H

#include <vector>
#include <string>

#include "WL4Constants.h"

namespace ROMUtils
{
    // Global variables
    extern unsigned char *CurrentFile;
    extern int CurrentFileSize;

    // Global functions
    int IntFromData(int address);
    int PointerFromData(int address);
    unsigned char *RLEDecompress(int address, int outputSize);
    int FindSpaceInROM(int NewDataLength);
    int SaveTemp(int PointerAddress, unsigned char *tmpdata, int datalength);
}

#endif // ROMUTILS_H
