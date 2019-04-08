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
    QString fileName;
    enum PatchType patchType;
    int hookAddress;
    int patchAddress;
};

QVector<struct PatchEntryItem> GetPatchesFromROM();

#endif // PATCHUTILS_H
