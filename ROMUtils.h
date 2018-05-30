#ifndef ROMUTILS_H
#define ROMUTILS_H

#include <vector>

namespace ROMUtils
{
    // Global variables
    extern unsigned char *CurrentFile;
    extern int CurrentFileSize;

    // Global functions
    int IntFromData(unsigned char *data, int address);
    int PointerFromData(unsigned char *data, int address);
    // TODO RLE decompress
}

#endif // ROMUTILS_H
