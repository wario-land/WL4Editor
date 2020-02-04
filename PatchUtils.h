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
    bool FunctionPointerReplacementMode;
    bool ThumbMode;
    unsigned int PatchAddress;
    QString SubstitutedBytes;
};

namespace PatchUtils
{
    extern QString EABI_INSTALLATION;
    QVector<struct PatchEntryItem> GetPatchesFromROM();
    QString SavePatchesToROM(QVector<struct PatchEntryItem> entries);
    bool VerifyEABI(QString *missing);
}

#endif // PATCHUTILS_H
