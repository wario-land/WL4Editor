#ifndef ROMUTILS_H
#define ROMUTILS_H

#include <vector>
#include <string>

namespace ROMUtils
{
    // Global variables
    extern unsigned char *CurrentFile;
    extern int CurrentFileSize;

    // Global functions
    int IntFromData(int address);
    int PointerFromData(int address);
    unsigned char *RLEDecompress(int address, int outputSize);
}

#endif // ROMUTILS_H
