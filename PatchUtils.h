#ifndef PATCHUTILS_H
#define PATCHUTILS_H

#include <QString>

enum PatchType
{
    Binary   = 0,
    Assembly = 1,
    C        = 2
};

struct PatchEntryItem
{
    QString FileName;
    enum PatchType PatchType;
    unsigned int HookAddress;
    bool StubFunction;
    bool ThumbMode;
    unsigned int PatchAddress;
    QString SubstitutedBytes;
};

QVector<struct PatchEntryItem> GetPatchesFromROM();

#endif // PATCHUTILS_H
