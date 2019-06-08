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
        // Define variables used in the creation of the jump table
        unsigned char *data = (unsigned char *) this->data;
        unsigned short *R = new unsigned short[data_len * 2];
        unsigned short *C = R + data_len;
        int cons = 0, minrun = GetMinimumRunSize();

        // Seed the dynamic programming jump table
        R[data_len - 1] = 1;

        // Populate R backwards in the jump table
        for (unsigned int i = data_len - 1; i >= 1; --i)
        {
            R[i - 1] = (R[i] == jumpLimit || data[i] != data[i - 1]) ? 1 : R[i] + 1;
        }

        // Populate C forwards in the jump table
        for (unsigned int i = 0; i < data_len; ++i)
        {
            if (R[i] < minrun && cons < jumpLimit)
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
        if (cons)
        {
            C[data_len - cons] = cons;
        }

        JumpTable = R; // set the jump table since it has been populated
    }

    /// <summary>
    /// Helper function to get the length of compressed data using the RLE metadata's jump table.
    /// </summary>
    /// <param name="opcodeSize">
    /// Number of bytes to use for compression opcodes (different for 8-bit/16-bit).
    /// </param>
    unsigned int RLEMetadata::GetCompressedLengthHelper(unsigned int opcodeSize)
    {
        // Define the variables used to traverse the jump table
        unsigned int size = 0;
        unsigned int i = 0, minrun = GetMinimumRunSize();
        unsigned short *R = JumpTable, *C = JumpTable + data_len;

        // Calculate the size
        while (i < data_len)
        {
            bool runmode = R[i] >= minrun;
            unsigned short len = runmode ? R[i] : C[i];
            size += opcodeSize + (runmode ? 1 : len);
            i += len;
        }
        return 1 + size + opcodeSize; // type identifier, data, termination value
    }

    /// <summary>
    /// Helper function to get the compressed data using the RLE metadata's jump table.
    /// </summary>
    /// <returns>
    /// The compressed data in a dynamically allocated array.
    /// </returns>
    void *RLEMetadata::GetCompressedData()
    {
        // Define variables used by compression
        QVector<unsigned char> compressedData;
        unsigned int i = 0, minrun = GetMinimumRunSize();
        unsigned short *R = JumpTable, *C = JumpTable + data_len;
        unsigned char *data = (unsigned char *) this->data;

        // Populate the compressed data
        compressedData.append(GetTypeIdentifier());
        while (i < data_len)
        {
            bool runmode = R[i] >= minrun;
            unsigned short len = runmode ? R[i] : C[i];
            AddOpcode(compressedData, len, runmode);
            for (int j = 0; j < (runmode ? 1 : len); ++j)
            {
                compressedData.append(data[i + j]);
            }
            i += len;
        }
        AddOpcode(compressedData, 0, false);

        // Create the dynamically allocated char array
        unsigned char *compressed = new unsigned char[compressedData.size()];
        for (int i = 0; i < compressedData.size(); ++i)
        {
            compressed[i] = compressedData[i];
        }
        return compressed;
    }
} // namespace ROMUtils
