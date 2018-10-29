#include "ROMUtils.h"
#include <cassert>
#include <QFile>
#include <WL4EditorWindow.h>

extern WL4EditorWindow *singleton;

namespace ROMUtils
{
    unsigned char *CurrentFile;
    int CurrentFileSize;
    QString ROMFilePath;
    int SaveDataIndex;

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
        assert(ret < CurrentFileSize); // Fail if the pointer is out of range. TODO proper error handling
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
    unsigned char *LayerRLEDecompress(int address, int outputSize)
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
                    int ctrl = CurrentFile[address++];
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
                            dst[2 * j] = CurrentFile[address];
                        }
                        address++;
                    }
                    else
                    {
                        runData = ctrl;
                        for(int j = 0; j < runData; j++)
                        {
                            dst[2 * j] = CurrentFile[address + j];
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
                    int ctrl = ((int) CurrentFile[address] << 8) | CurrentFile[address + 1];
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
                            dst[2 * j] = CurrentFile[address];
                        }
                        address++;
                    }
                    else
                    {
                        runData = ctrl;
                        for(int j = 0; j < runData; j++)
                        {
                            dst[2 * j] = CurrentFile[address + j];
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
    /// compress Layer data by run-length encoding.
    /// </summary>
    /// <remarks>
    /// the first and second byte as the layer width and height information will not be generated in the function
    /// you have to add them by yourself when saving compressed data.
    /// </remarks>
    /// <param name="_layersize">
    /// the size of the layer, the value equal to (layerwidth * layerheight).
    /// </param>
    /// <param name="LayerData">
    /// unsigned char pointer to the uncompressed layer data.
    /// </param>
    /// <param name="OutputCompressedData">
    /// unsigned char pointer to the compressed layer data.
    /// </param>
    /// <return>the length of compressed data.</return>
    long LayerRLECompress(int _layersize, unsigned char *LayerData, unsigned char **OutputCompressedData)
    {
        unsigned char *cmp = new unsigned char[_layersize];
        *OutputCompressedData = cmp;
        int cmpptr=0;
        int sizeptr=0;
        int dataptr=0;
        unsigned short rl=0;

        for(int j=0; j<2; j++)
        {
            cmp[cmpptr++]=1;
            while(_layersize>dataptr)
            {
                rl = 2;
                while(LayerData[dataptr]==LayerData[dataptr+2] && LayerData[dataptr+2]==LayerData[dataptr+4] && rl<0x7F && _layersize>dataptr)
                {

                    dataptr+=2;
                    rl++;
                }
                if(rl > 2)
                {
                    cmp[cmpptr++]=(unsigned char) ((rl | 0x80) & 0xFF);
                    cmp[cmpptr++]=LayerData[dataptr];
                    dataptr+=4;
                }
                sizeptr=cmpptr;
                cmpptr++;
                rl = 0;
                while((LayerData[dataptr]!=LayerData[dataptr+2] || LayerData[dataptr+2]!=LayerData[dataptr+4]) && rl<0x7F && _layersize>dataptr)
                {

                    cmp[cmpptr++]=LayerData[dataptr];
                    dataptr+=2;
                    rl++;
                }
                if(rl != 0)
                    cmp[sizeptr]=(unsigned char) rl;
                else
                    cmpptr--;
            }
            cmp[cmpptr++]=0;
            dataptr=1;
        }

        return cmpptr;
    }

    /// <summary>
    /// a sub routine for ROMUtils::SaveTemp(...), we had better not use it elsewhere
    /// </summary>
    int FindSpaceInROM(int NewDataLength)
    {
        if(NewDataLength > 0xFFFF)
            return -1;

        int dst = WL4Constants::AvailableSpaceBeginningInROM;
        int runData = 0;
        while(1)
        {
            if((CurrentFile[dst] == (unsigned char) '\xFF') && (dst < 0x800000) && (runData < (NewDataLength + 8)))
            {
                dst++;
                runData++;
                continue;
            }
            else if(dst == 0x800000)
            {
                return -2;
            }
            else if(runData == (NewDataLength + 8))
            {
                return (dst - runData);
            }
            else if(CurrentFile[dst] != (unsigned char) '\xFF')
            {
                if(!strncmp((const char*) (CurrentFile + dst), "STAR", 4))
                {
                    unsigned short val1 = *(unsigned short*) (CurrentFile + dst + 4);
                    unsigned short val2 = *(unsigned short*) (CurrentFile + dst + 6);
                    if(val1 + val2 == 0xFFFF)
                    {
                        dst += (8 + val1);
                        runData = 0;
                        continue;
                    }
                    else
                    {
                        return -3; //TODO: error handling: the ROM is patch by unknown program.
                    }
                }
            }
        }
    }

    /// <summary>
    /// Save change into CurrentFile (NOT THE SOURCE ROM FILE)
    /// </summary>
    /// <param name="tmpData">
    /// a C-type pointer points to the new data array we want to save.
    /// </param>
    /// <param name="pointerAddress">
    /// An address points to a pointer which points to the offset that save data.
    /// </param>
    /// <param name="datalength">
    /// the length of the new data array.
    /// </param>
    /// <return>A pointer to decompressed data.</return>
    int SaveTemp(unsigned char *tmpData, int pointerAddress, int dataLength)
    {
        int OriginalPtr = PointerFromData(pointerAddress);
        int tmpPtr = OriginalPtr - 8;

        // Recover the block in ROM if it is possible
        if((tmpPtr > WL4Constants::AvailableSpaceBeginningInROM) && !strncmp((const char*) (CurrentFile + tmpPtr), "STAR", 4))
        {
            int tmpLength = *(unsigned short*) (CurrentFile + tmpPtr + 4);
            memset((void*) (CurrentFile + tmpPtr), '\xFF', tmpLength + 8);
        }

        // Save New Data
        int newPtr = FindSpaceInROM(dataLength);
        if (newPtr < 0)
            return -1;  // TODO: error handling: the ROM cannot be patched due to some reason.

        memcpy(CurrentFile + newPtr, "STAR", 4); // Write RATS tag
        *(unsigned short*) (CurrentFile + newPtr + 4) = (unsigned short) dataLength;
        *(unsigned short*) (CurrentFile + newPtr + 6) = (unsigned short) (0xFFFF - dataLength);
        *(unsigned int*) (CurrentFile + pointerAddress) = newPtr + 0x8000000; // write pointer

        memcpy((void*) (CurrentFile + newPtr + 8), tmpData, dataLength); // Copy data into RATS protected area

        return -1; // just return some random value which not equal to the one stand for error.
    }

    void SaveFile()
    {
        // Obtain the list of data chucks to save to the rom
        SaveDataIndex = 0;
        QVector<struct SaveData> chunks;
        singleton->GetCurrentLevel()->Save(chunks);

        // Find space for each chunk and write it in a copy of CurrentFile (not the original CurrentFile)


        // Save the rom file from the CurrentFile copy
        QFile file(ROMFilePath);
        file.open(QIODevice::WriteOnly);
        if (file.isOpen()){
            file.write((const char*)CurrentFile, CurrentFileSize* sizeof(unsigned char));
        }
        else{
            // can't open file to save ROM
        }
        file.close();

        // Set the CurrentFile to the copied CurrentFile data (and clean up the old array)

    }
}
