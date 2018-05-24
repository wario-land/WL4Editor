#include "ROMUtils.h"

namespace ROMUtils
{
    unsigned char *CurrentFile;
    int CurrentFileSize;

    /// <summary>
    /// Get a 4-byte, little-endian integer from ROM data.
    /// </summary>
    /// <param name="data">
    /// The ROM data to read from.
    /// </param>
    /// <param name="address">
    /// The address to get the integer from.
    /// </param>
    int IntFromData(unsigned char *data, int address)
    {
        int a = 0xFF & data[address];
        int b = 0xFF & data[address + 1];
        int c = 0xFF & data[address + 2];
        int d = 0xFF & data[address + 3];
        return a | (b << 8) | (c << 16) | (d << 24);
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
    int PointerFromData(unsigned char *data, int address)
    {
        int a = 0xFF & data[address];
        int b = 0xFF & data[address + 1];
        int c = 0xFF & data[address + 2];
        int d = 0xFF & data[address + 3];
        return ((a | (b << 8) | (c << 16) | (d << 24)) & 0x7FFFFFF);
    }

    void GetUnit8sFromData(unsigned char *data, int address, int _ByteLength, std::vector<unsigned char> &_result)
    {
        int i;
        for(i = 0; i < _ByteLength; i++)
            _result.push_back(0xFF & data[address + i]);
    }

}
