#ifndef COMPRESS_H
#define COMPRESS_H

#include <QVector>

namespace ROMUtils
{
    class RLEMetadata
    {
    private:
        unsigned short *JumpTable;

    protected:
        void *data;
        unsigned int data_len;
        virtual void InitializeJumpTable() = 0;
        virtual int GetTypeIdentifier() = 0;
        virtual int GetMinimumRunSize() = 0;
        virtual void AddOpcode(QVector<unsigned char> &compressedData, unsigned short opcode, bool runmode) = 0;
        RLEMetadata(void *data, unsigned int len) : data(data), data_len(len) {}
        void InitializeJumpTableHelper(unsigned short jumpLimit);
        unsigned int GetCompressedLengthHelper(unsigned int opcodeSize);

    public:
        virtual unsigned int GetCompressedLength() = 0;
        void *GetCompressedData();
        virtual ~RLEMetadata() { delete[] JumpTable; }
    };

    class RLEMetadata8Bit : public RLEMetadata
    {
    protected:
        void InitializeJumpTable() { InitializeJumpTableHelper(0x7F); }
        int GetTypeIdentifier() { return 1; }
        int GetMinimumRunSize() { return 3; }
        void AddOpcode(QVector<unsigned char> &compressedData, unsigned short opcode, bool runmode)
        {
            if (runmode)
                opcode |= 0x80;
            compressedData.append((unsigned char) opcode);
        }

    public:
        RLEMetadata8Bit(void *data, unsigned int len) : RLEMetadata(data, len) { InitializeJumpTable(); }
        unsigned int GetCompressedLength() { return GetCompressedLengthHelper(1); }
    };

    class RLEMetadata16Bit : public RLEMetadata
    {
    protected:
        void InitializeJumpTable() { InitializeJumpTableHelper(0x7FFF); }
        int GetTypeIdentifier() { return 2; }
        int GetMinimumRunSize() { return 5; }
        void AddOpcode(QVector<unsigned char> &compressedData, unsigned short opcode, bool runmode)
        {
            if (runmode)
                opcode |= 0x8000;
            compressedData.append((unsigned char) (opcode >> 8));
            compressedData.append((unsigned char) opcode);
        }

    public:
        RLEMetadata16Bit(void *data, unsigned int len) : RLEMetadata(data, len) { InitializeJumpTable(); }
        unsigned int GetCompressedLength() { return GetCompressedLengthHelper(2); }
    };
} // namespace ROMUtils

#endif // COMPRESS_H
