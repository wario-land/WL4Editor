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

    /// <summary>
    /// Decompress ROM data that was compressed with run-length encoding.
    /// </summary>
    /// <remarks>
    /// The <paramref name="outputSize"/> parameter specifies the predicted output size in bytes.
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
        unsigned char *dst;
        int src = address;
        int ctrl;
        int nn;

        for(int ii=0; ii<2; ii++)
        {
            dst = &OutputLayerData[ii];
            if(data[src++] == 1)
            {
                while(1)
                {
                    ctrl = (int) data[src++];
                    if(ctrl == 0)
                        break;
                    else if(ctrl & 0x80)
                    {
                        nn = ctrl & 0x7F;
                        if(data[src])
                            for(int jj=0; jj<nn; jj++)
                                dst[2*jj]= data[src];
                            src++;
                    }
                    else
                    {
                        nn = ctrl;
                        for(int jj=0; jj<nn; jj++)
                            dst[2*jj]= data[src+jj];
                        src += nn;
                    }
                    dst += 2*nn;
                }
            }
            else		// RLE16
            {
                while(1)
                {
                    ctrl = data[src]<<8 | data[src+1];
                    src += 2;                     //offset + 2
                    if(ctrl == 0)
                        break;
                    if(ctrl & 0x8000)
                    {
                        nn = ctrl & 0x7FFF;
                        if(data[src])
                            for(int jj=0; jj<nn; jj++)
                                dst[2*jj] = data[src];
                            src++;
                    }
                    else
                    {
                        nn = ctrl;
                        for(int jj=0; jj<nn; jj++)
                            {
                            dst[2*jj] = data[src+jj];
                            }
                        src += nn;
                    }
                    dst += 2*nn;
                }
            }
        }

        return OutputLayerData;
    }
}
