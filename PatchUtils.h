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
    QString HookString;
    unsigned int PatchOffsetInHookString;
    unsigned int PatchAddress;
    QString SubstitutedBytes;
    QString Description;

public:
    int GetHookLength() const
    {
        int hookLen = HookString.length() / 2;
        if(PatchOffsetInHookString != (unsigned int) -1) hookLen += 4;
        return hookLen;
    }
};

namespace PatchUtils
{
    extern QString EABI_INSTALLATION;
    QVector<struct PatchEntryItem> GetPatchesFromROM();
    QString SavePatchesToROM(QVector<PatchEntryItem> &entries);
    bool VerifyEABI(QString *missing);
}

#endif // PATCHUTILS_H
