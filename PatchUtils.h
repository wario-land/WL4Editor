#ifndef PATCHUTILS_H
#define PATCHUTILS_H

#include <QRegularExpression>
#include <QString>
#include <QVector>

enum PatchType
{
    Binary       = 0,
    Assembly     = 1,
    C            = 2,
    C_dependency = 3
};

struct PatchEntryItem
{
    QString FileName;
    enum PatchType PatchType;
    unsigned int HookAddress;             // where to overwrite hookstring
    QString HookString;
    unsigned int PatchOffsetInHookString; // where the P is in the hookstring
    unsigned int PatchAddress;            // where the patch we insert into the ROM
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
    QString SavePatchesToROM(QVector<PatchEntryItem> entries);
    bool VerifyEABI(QString *missing);

    // Regexes shared by the patch source parser and the PatchEditDialog input validators.
    const QRegularExpression &HookAddressRegex();
    const QRegularExpression &HookStringRegex();

    // Convert hook text to the normalized form stored in PatchEntryItem::HookString:
    // no whitespace, uppercase hex digits, and no "P" placeholder.
    QString NormalizeHookText(const QString &str);

    // Return the byte offset of the "P" placeholder in the hook text, or (unsigned int)-1
    // when the hook text does not contain a placeholder.
    unsigned int GetPatchOffset(const QString &str);

    // Parse the metadata comments of a C/ASM patch source file into the given entry.
    // The PatchType is inferred from the file extension. FileName, PatchAddress and
    // SubstitutedBytes are not modified. When a C file has no @HookString, the type is
    // changed to C_dependency (same behavior as PatchEditDialog).
    bool ParsePatchSourceFile(const QString &absoluteFilePath,
                              struct PatchEntryItem &entry,
                              QString *error = nullptr,
                              bool *hookStringMissing = nullptr);
}

#endif // PATCHUTILS_H
