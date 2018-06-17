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
    int IntFromData(int address)
    {
        return *(int*) (CurrentFile + address); // This program is almost certainly executing on a little-endian architecture
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
    int PointerFromData(int address)
    {
        int ret = IntFromData(address) & 0x7FFFFFF;
        assert(ret >= ROMUtils::CurrentFileSize); // Fail if the pointer is out of range. TODO proper error handling
        return ret;
    }
#include <cstdio>
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
    unsigned char *RLEDecompress(int address, int outputSize)
    {
        unsigned char *OutputLayerData = new unsigned char[outputSize];
        int runData;

        for(int i = 0; i < 2; i++)
        {
            unsigned char *dst = OutputLayerData + i;
            if(ROMUtils::CurrentFile[address++] == 1)
            {
                while(1)
                {
                    int ctrl = ROMUtils::CurrentFile[address++];
                    if(ctrl == 0)
                    {
                        break;
                    }

                    int temp = (int) (dst - OutputLayerData);
                    if(temp > outputSize)
                    {
                        delete[] OutputLayerData;
                        return nullptr;
                    }

                    else if(ctrl & 0x80)
                    {
                        runData = ctrl & 0x7F;
                        for(int j = 0; j < runData; j++)
                        {
                            dst[2 * j] = ROMUtils::CurrentFile[address];
                        }
                        address++;
                    }
                    else
                    {
                        runData = ctrl;
                        for(int j = 0; j < runData; j++)
                        {
                            dst[2 * j] = ROMUtils::CurrentFile[address + j];
                        }
                        address += runData;
                    }

                    dst += 2 * runData;
                }
            }
            else // RLE16
            {
                while(1)
                {
                    int ctrl = ((int) ROMUtils::CurrentFile[address] << 8) | ROMUtils::CurrentFile[address + 1];
                    address += 2; //offset + 2
                    if(ctrl == 0)
                    {
                        break;
                    }

                    int temp = (int) (dst - OutputLayerData);
                    if(temp > outputSize)
                    {
                        delete[] OutputLayerData;
                        return nullptr;
                    }

                    if(ctrl & 0x8000)
                    {
                        runData = ctrl & 0x7FFF;
                        for(int j = 0; j < runData; j++)
                        {
                            dst[2 * j] = ROMUtils::CurrentFile[address];
                        }
                        address++;
                    }
                    else
                    {
                        runData = ctrl;
                        for(int j = 0; j < runData; j++)
                        {
                            dst[2 * j] = ROMUtils::CurrentFile[address + j];
                        }
                        address += runData;
                    }

                    dst += 2 * runData;
                }
            }
        }
        return OutputLayerData;
    }

    /// <summary>
    /// a sub routine for ROMUtils::SaveTemp(...), we had better not use it elsewhere
    /// </summary>
    int FindSpaceInROM(int NewDataLength)
    {
        if(NewDataLength > 0xFFFF)
            return 0;

        int dst = WL4Constants::AvailableSpaceInROM;
        int runData = 0;
        int ss, II;
        while(1)
        {
            if((ROMUtils::CurrentFile[dst] == '\xFF') && (dst < 0x800000) && (runData < (NewDataLength + 8)))
            {
                dst++; runData++; continue;
            }else if(dst = 0x800000){
                return 0;
            }else if(runData == (NewDataLength + 8)){
                return (dst - runData);
            }else if(ROMUtils::CurrentFile[dst] != '\xFF'){
                if((ROMUtils::CurrentFile[dst] == 'S') && (ROMUtils::CurrentFile[dst+1] == 'T') && \
                        (ROMUtils::CurrentFile[dst+2] == 'A') && (ROMUtils::CurrentFile[dst+3] == 'R'))
                {
                    ss = ROMUtils::CurrentFile[dst+4] | (ROMUtils::CurrentFile[dst+5] << 8);
                    II = ROMUtils::CurrentFile[dst+6] | (ROMUtils::CurrentFile[dst+7] << 8);
                    if((ss + II) == 0xFFFF)
                    {
                        dst += (8 + ss); runData = 0; continue;
                    }else{
                        return 0; //TODO: error handling: the ROM is patch by unknown program.
                    }
                }
            }
        }
    }

    /// <summary>
    /// Save change into ROMUtils::CurrentFile (NOT THE SOURCE ROM FILE)
    /// </summary>
    /// <param name="PointerAddress">
    /// An address points to a pointer which points to the offset that save data.
    /// </param>
    /// <param name="tmpdata">
    /// a C-type pointer points to the new data array we want to save.
    /// </param>
    /// <param name="datalength">
    /// the length of the new data array.
    /// </param>
    /// <return>A pointer to decompressed data.</return>
    int SaveTemp(int PointerAddress, unsigned char *tmpdata, int datalength)
    {
        int OriginalPtr = ROMUtils::PointerFromData(PointerAddress);
        int tmpPtr = OriginalPtr - 8;

        //Recover the block in ROM if it is posible
        if((tmpPtr > WL4Constants::AvailableSpaceInROM) && (ROMUtils::CurrentFile[tmpPtr] == 'S') && \
                (ROMUtils::CurrentFile[tmpPtr+1] == 'T') && (ROMUtils::CurrentFile[tmpPtr+2] == 'A') && \
                (ROMUtils::CurrentFile[tmpPtr+3] == 'R'))
        {
            int tmpLength = ROMUtils::CurrentFile[tmpPtr+4] | (ROMUtils::CurrentFile[tmpPtr+5] << 8);
            for(int i = tmpPtr; i < (tmpPtr + 8 + tmpLength); i++)
                ROMUtils::CurrentFile[i] = '\xFF';
        }

        //Save New Data
        int newPtr = ROMUtils::FindSpaceInROM(datalength);
        if (newPtr == 0)
            return 0;  //TODO: error handling: the ROM cannot be patched due to some rreasons.
        ROMUtils::CurrentFile[newPtr] = 'S';
        ROMUtils::CurrentFile[newPtr+1] = 'T';
        ROMUtils::CurrentFile[newPtr+2] = 'A';
        ROMUtils::CurrentFile[newPtr+3] = 'R';  //set "STAR" Tag
        ROMUtils::CurrentFile[newPtr+4] = (unsigned char) (datalength & 0xFF);
        ROMUtils::CurrentFile[newPtr+5] = (unsigned char) ((datalength & 0xFF00) >> 8);  //write ss
        ROMUtils::CurrentFile[newPtr+6] = (unsigned char) ((0xFFFF - datalength) & 0xFF);
        ROMUtils::CurrentFile[newPtr+7] = (unsigned char) (((0xFFFF - datalength) & 0xFF00) >> 8);  //write II
        ROMUtils::CurrentFile[PointerAddress] = (unsigned char) ((newPtr + 0x8000000) & 0xFF);
        ROMUtils::CurrentFile[PointerAddress+1] = (unsigned char) (((newPtr + 0x8000000)>>8) & 0xFF);
        ROMUtils::CurrentFile[PointerAddress+2] = (unsigned char) (((newPtr + 0x8000000)>>16) & 0xFF);
        ROMUtils::CurrentFile[PointerAddress+3] = (unsigned char) (((newPtr + 0x8000000)>>24) & 0xFF);  //write pointer
        int j = 0;
        for(int i = (newPtr + 8); i < (newPtr + 8 + datalength); i++)
        {
            ROMUtils::CurrentFile[i] = tmpdata[j];
            j++;
        }  //write new data

        return 1; //just return some random value which not equal to the one stand for error.
    }

    void SaveFile()
    {
        FILE *outfile = fopen(ROMUtils::ROMFilePath, "wb");
        fwrite(ROMUtils::CurrentFile, sizeof(unsigned char) * ROMUtils::CurrentFileSize, 1, outfile);
        fclose(outfile);
    }

}
