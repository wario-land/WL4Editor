#include "Compress.h"

namespace ROMUtils
{
    /// <summary>
    /// Helper function to populate the dynamic programming jump table for fast RLE compression.
    /// </summary>
    /// <param name="jumpLimit">
    /// The maximum jump size for the table (different for 8-bit/16-bit).
    /// </param>
    void RLEMetadata::InitializeJumpTableHelper(unsigned short jumpLimit)
    {
        unsigned char *data = (unsigned char*) this->data;
        // TODO reference data[i] through a virtual function to get chars or shorts
        unsigned short *R = new unsigned short[data_len * 2];
        R[data_len - 1] = 1;
        for(unsigned int i = data_len - 1; i >= 1; --i)
        {
            R[i - 1] = (R[i] == jumpLimit || data[i] != data[i - 1]) ? 1 : R[i] + 1;
        }
        unsigned short *C = R + data_len;
        int cons = 0;
        for(unsigned int i = 0; i < data_len; ++i)
        {
            if(R[i] <= 2 && cons < jumpLimit)
            {
                ++cons;
            }
            else
            {
                C[i - cons] = cons;
                cons = 0;
                i += R[i] - 1;
            }
        }
        if(cons)
        {
            C[data_len - cons] = cons;
        }
        JumpTable = R;
    }

    /// <summary>
    /// Helper function to get the length of compressed data using the RLE metadata's jump table.
    /// </summary>
    /// <param name="opcodeSize">
    /// Number of bytes to use for compression opcodes (different for 8-bit/16-bit).
    /// </param>
    unsigned int RLEMetadata::GetCompressedLengthHelper(unsigned int opcodeSize)
    {
        unsigned int i = 0, size = 0;
        unsigned short *R = JumpTable, *C = JumpTable + data_len;
        while(i < data_len)
        {
            bool runmode = R[i] >= 3;
            unsigned short len = runmode ? R[i] : C[i];
            size += opcodeSize * (runmode ? 2 : len + 1);
            i += len;
        }
        return size + opcodeSize; // termination value
    }

    /// <summary>
    /// Initialize the jump table using an 8-bit jump limit.
    /// </summary>
    void RLEMetadata8Bit::InitializeJumpTable()
    {
        InitializeJumpTableHelper(0x7F);
    }

    /// <summary>
    /// Initialize the jump table using a 16-bit jump limit.
    /// </summary>
    void RLEMetadata16Bit::InitializeJumpTable()
    {
        InitializeJumpTableHelper(0x7FFF);
    }

    /// <summary>
    /// Get the compressed length of 8-bit compressed data.
    /// </summary>
    unsigned int RLEMetadata8Bit::GetCompressedLength()
    {
        return GetCompressedLengthHelper(1);
    }

    /// <summary>
    /// Get the compressed length of 16-bit compressed data.
    /// </summary>
    unsigned int RLEMetadata16Bit::GetCompressedLength()
    {
        return GetCompressedLengthHelper(2);
    }
}
