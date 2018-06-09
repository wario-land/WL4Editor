#include "ROMUtils.h"
#include <cassert>

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
        return *(int*) (data + address); // This program is almost certainly executing on a little-endian architecture
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
        int ret = IntFromData(data, address) & 0x7FFFFFF;
        assert(ret >= CurrentFileSize); // Fail if the pointer is out of range. TODO proper error handling
        return ret;
    }

    /// <summary>
    /// Decompress ROM data that was compressed with run-length encoding.
    /// </summary>
    /// <remarks>
    /// The <paramref name="outputSize"/> parameter specifies the predicted output size in bytes.
    /// The return unsigned char * is on the heap, delete it after using.
    /// </remarks>
    /// <param name="data">
    /// A pointer into the ROM data to start reading from.
    /// </param>
    /// <param name="outputSize">
    /// The predicted size of the output data.(unit: Byte)
    /// </param>
    /// <return>A pointer to decompressed data.</return>
    unsigned char *RLEDecompress(unsigned char *data, int address, int outputSize)
    {
        unsigned char *OutputLayerData = new unsigned char[outputSize];
        int runData;

        for(int i = 0; i < 2; i++)
        {
            unsigned char *dst = &OutputLayerData[i];
            if(data[address++] == 1)
            {
                while(1)
                {
                    int ctrl = data[address++];
                    if(ctrl == 0)
                    {
                        break;
                    }
                    else if(ctrl & 0x80)
                    {
                        runData = ctrl & 0x7F;
                        if(data[address])
                        {
                            for(int j = 0; j < runData; j++)
                            {
                                dst[2 * j] = data[address];
                            }
                            address++;
                        }
                    }
                    else
                    {
                        runData = ctrl;
                        for(int jj = 0; jj < runData; jj++)
                        {
                            dst[2 * jj] = data[address + jj];
                        }
                        address += runData;
                    }
                    dst += 2*runData;
                    if((int)(dst - OutputLayerData) > outputSize)
                    {
                        delete[] OutputLayerData;
                        return nullptr;
                    }
                }
            }
            else // RLE16
            {
                while(1)
                {
                    int ctrl = ((int) data[address] << 8) | data[address + 1];
                    address += 2; //offset + 2
                    if(ctrl == 0)
                    {
                        break;
                    }
                    if(ctrl & 0x8000)
                    {
                        runData = ctrl & 0x7FFF;
                        if(data[address])
                        {
                            for(int j = 0; j < runData; j++)
                            {
                                dst[2 * j] = data[address];
                            }
                            address++;
                        }
                    }
                    else
                    {
                        runData = ctrl;
                        for(int j = 0; j < runData; j++)
                        {
                            dst[2 * j] = data[address + j];
                        }
                        address += runData;
                    }
                    dst += 2 * runData;
                    if((int) (dst - OutputLayerData) > outputSize)
                    {
                        delete[] OutputLayerData;
                        return nullptr;
                    }
                }
            }
        }
        return OutputLayerData;
    }

    void LevelNameFromData(unsigned char *data, int address, std::string &_levelname)
    {
        unsigned char chr;
        for(int ii = 0; ii < 26; ii++)
        {
            chr = data[address];
            if(('/x00' <= chr) && (chr <= '\x09'))
                _levelname.append(1, chr + (unsigned char)48);
            else if(('/x0A' <= chr) && (chr <= '\x23'))
                _levelname.append(1, chr + (unsigned char)55);
            else if(('/x24' <= chr) && (chr <= '\x3D'))
                _levelname.append(1, chr + (unsigned char)61);
            else
                _levelname.append(1, (unsigned char)32);
        }
    }

}
