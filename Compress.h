#ifndef COMPRESS_H
#define COMPRESS_H

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
        RLEMetadata(void *data, unsigned int len) : data(data), data_len(len) { }
        void InitializeJumpTableHelper(unsigned short jumpLimit);
        unsigned int GetCompressedLengthHelper(unsigned int opcodeSize);
    public:
        virtual unsigned int GetCompressedLength() = 0;
        void *GetCompressedData();
    };

    class RLEMetadata8Bit : public RLEMetadata
    {
    protected:
        void InitializeJumpTable();
    public:
        RLEMetadata8Bit(void *data, unsigned int len) : RLEMetadata(data, len) { InitializeJumpTable(); }
        unsigned int GetCompressedLength();
    };

    class RLEMetadata16Bit : public RLEMetadata
    {
    protected:
        void InitializeJumpTable();
    public:
        RLEMetadata16Bit(void *data, unsigned int len) : RLEMetadata(data, len) { InitializeJumpTable(); }
        unsigned int GetCompressedLength();
    };
}

#endif // COMPRESS_H
