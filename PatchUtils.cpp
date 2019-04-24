#include "PatchUtils.h"
#include <ROMUtils.h>
#include <QVector>
#include <cassert>
#include <QDir>
#include <SettingsUtils.h>
#include <QProcess>
#include <cstring>

#define PATCH_CHUNK_VERSION 0

#ifdef _WIN32
#define EABI_GCC     "arm-none-eabi-gcc.exe"
#define EABI_AS      "arm-none-eabi-as.exe"
#define EABI_OBJCOPY "arm-none-eabi-objcopy.exe"
#else // _WIN32 (else linux)
#define EABI_GCC     "arm-none-eabi-gcc"
#define EABI_AS      "arm-none-eabi-as"
#define EABI_OBJCOPY "arm-none-eabi-objcopy"
#endif

#define REPLACE_EXT(qstr,fromstr,tostr) do { \
    QString tempstr(qstr);                   \
    tempstr.chop(strlen(fromstr));             \
    qstr = tempstr + tostr;                  \
} while(0)

struct CompileEntry {
    QString FileName;
    enum PatchType Type;
};

/// <summary>
/// Upgrade the format of a patch list chunk by one version.
/// </summary>
/// <remarks>
/// Version 0:
///   Semicolon-delimited (7 fields):
///     0: Filename
///     1: Patch type (int)
///     2: Hook address (hex string)
///     3: Patch address (hex string)
///     4: Stub function (0 or 1)
///     5: ARM/Thumb (0 or 1)
///     6: Substituted bytes (hex string)
/// </remarks>
/// <param name="contents">
/// The patch list chunk contents to upgrade.
/// </param>
/// <param name="version">
/// The version of the data being passed in.
/// </param>
/// <returns>
/// The patch list chunk contents, upgraded one version.
/// </returns>
static QString UpgradePatchListContents(QString contents, int version)
{
    switch(version)
    {
        case 0:
            // TODO implement when there are new versions
            break;
    }
    return contents;
}

/// <summary>
/// Obtain the patch list chunk contents in the current version's format.
/// </summary>
/// <param name="chunkDataAddr">
/// The address where the chunk data (including header) starts.
/// </param>
/// <returns>
/// The updated patch list chunk's contents, regardless of the version in the ROM.
/// </returns>
static QString GetUpgradedPatchListChunkData(unsigned int chunkDataAddr)
{
    unsigned short contentSize = *reinterpret_cast<unsigned short*>(ROMUtils::CurrentFile + chunkDataAddr + 4) - 1;
    QString contents = QString::fromLocal8Bit(reinterpret_cast<const char*>(ROMUtils::CurrentFile + chunkDataAddr + 13), contentSize);
    int chunkVersion = ROMUtils::CurrentFile[chunkDataAddr + 12];
    assert(chunkVersion <= PATCH_CHUNK_VERSION /* Patch list chunk either corrupt or this verison of WL4Editor is old and doesn't support the saved format */);
    while(chunkVersion < PATCH_CHUNK_VERSION)
    {
        contents = UpgradePatchListContents(contents, chunkVersion++);
    }
    return contents;
}

/// <summary>
/// Compile a C file into an assembly file.
/// </summary>
/// <param name="cfile">
/// The C file to compile.
/// </param>
static QString CompileCFile(QString cfile)
{
    // Create args
    assert(cfile.endsWith(".c") /* C file does not have correct extension (should be .c) */);
    QString outfile(cfile);
    REPLACE_EXT(outfile, ".c", ".s");
    QString executable(QString(PatchUtils::EABI_INSTALLATION) + "/" + EABI_GCC);
    QStringList args;
    args << "-MMD" << "-MP" << "-MF" << "-g" << "-Wall" << "-mcpu=arm7tdmi" << "-mtune=arm7tdmi" <<
        "-fomit-frame-pointer" << "-ffast-math" << "-mthumb" << "-mthumb-interwork" << "-S" <<
        cfile << "-o" << outfile;

    // Run GCC
    QProcess process;
    process.start(executable, args);
    process.waitForFinished();
    return !process.exitCode() ? "" : QString(process.readAllStandardError());
}

/// <summary>
/// Assemble an ASM file into an object file.
/// </summary>
/// <param name="sfile">
/// The ASM file to assemble.
/// </param>
static QString AssembleSFile(QString sfile)
{
    // Create args
    assert(sfile.endsWith(".s") /* ASM file does not have correct extension (should be .s) */);
    QString outfile(sfile);
    REPLACE_EXT(outfile, ".s", ".o");
    QString executable(QString(PatchUtils::EABI_INSTALLATION) + "/" + EABI_AS);
    QStringList args;
    args << sfile << "-o" << outfile;

    // Run GCC
    QProcess process;
    process.start(executable, args);
    process.waitForFinished();
    return !process.exitCode() ? "" : QString(process.readAllStandardError());
}

/// <summary>
/// Extract the binary from an object file.
/// </summary>
/// <param name="ofile">
/// The object file from which to extract the binary.
/// </param>
static QString ExtractOFile(QString ofile)
{
    // Create args
    assert(ofile.endsWith(".o") /* Object file does not have correct extension (should be .o) */);
    QString outfile(ofile);
    REPLACE_EXT(outfile, ".o", ".bin");
    QString executable(QString(PatchUtils::EABI_INSTALLATION) + "/" + EABI_OBJCOPY);
    QStringList args;
    args << "-O" << "binary" << "--only-section=.text" << ofile << outfile;

    // Run GCC
    QProcess process;
    process.start(executable, args);
    process.waitForFinished();
    return !process.exitCode() ? "" : QString(process.readAllStandardError());
}

namespace PatchUtils
{
    QString EABI_INSTALLATION;

    /// <summary>
    /// Obtain the patch entries from the currently loaded ROM file.
    /// </summary>
    /// <returns>
    /// A list of all patch entries, which is empty if none exist.
    /// </returns>
    QVector<struct PatchEntryItem> GetPatchesFromROM()
    {
        // Obtain the patch list chunk, if it exists
        QVector<struct PatchEntryItem> patchEntries;
        unsigned int patchListAddr = ROMUtils::FindChunkInROM(
            ROMUtils::CurrentFile,
            ROMUtils::CurrentFileSize,
            WL4Constants::AvailableSpaceBeginningInROM,
            ROMUtils::SaveDataChunkType::PatchListChunk
        );
        if(patchListAddr)
        {
            // Obtain the patch chunks
            QVector<unsigned int> patchChunks = ROMUtils::FindAllChunksInROM(
                ROMUtils::CurrentFile,
                ROMUtils::CurrentFileSize,
                WL4Constants::AvailableSpaceBeginningInROM,
                ROMUtils::SaveDataChunkType::PatchListChunk
            );

            // Get the patch list information
            QString contents = GetUpgradedPatchListChunkData(patchListAddr);
            assert(contents.length() > 0 /* ROM contains an empty patch list chunk */);
            QStringList patchTuples = contents.split(";");
            assert(!(patchTuples.count() % 4) /* ROM contains a corrupted patch list chunk (field count is not a multiple of 4) */);
            for(int i = 0; i < patchTuples.count(); i += 4)
            {
                // Add the patch entry
                int patchType = patchTuples[i + 1].toInt(Q_NULLPTR, 16);
                unsigned int hookAddress = static_cast<unsigned int>(patchTuples[i + 2].toInt(Q_NULLPTR, 16));
                unsigned int patchAddress = static_cast<unsigned int>(patchTuples[i + 3].toInt(Q_NULLPTR, 16));
                assert(patchChunks.contains(patchAddress) /* Patch chunk list refers to an invalid patch address */);
                bool stubFunction = patchTuples[i + 4] != "0";
                bool thumbMode = patchTuples[i + 5] != "0";
                struct PatchEntryItem entry
                {
                    patchTuples[i],
                    static_cast<enum PatchType>(patchType),
                    hookAddress,
                    stubFunction,
                    thumbMode,
                    patchAddress,
                    patchTuples[i + 6]
                };
                patchEntries.append(entry);
            }
        }
        return patchEntries;
    }

    /// <summary>
    /// Save the list of path entries to the ROM.
    /// </summary>
    /// <param name="entries">
    /// The patch entries to save to the ROM.
    /// </param>
    /// <returns>
    /// The error string if saving failed, or empty if successful.
    /// </returns>
    QString SavePatchesToROM(QVector<struct PatchEntryItem> entries)
    {
        QDir ROMdir(ROMUtils::ROMFilePath);
        ROMdir.cdUp();

        // Create binaries from C and asm
        QVector<struct CompileEntry> compileEntries;
        foreach(struct PatchEntryItem entry, entries)
        {
            QString fname(entry.FileName);
            switch(entry.PatchType)
            {
            case PatchType::C:
                compileEntries.append({ROMdir.absolutePath() + "/" + fname, PatchType::C});
            case PatchType::Assembly:
                REPLACE_EXT(fname, ".c", ".s");
                compileEntries.append({ROMdir.absolutePath() + "/" + fname, PatchType::Assembly});
                REPLACE_EXT(fname, ".s", ".o");
                compileEntries.append({ROMdir.absolutePath() + "/" + fname, PatchType::Binary});
            }
        }
        std::sort(compileEntries.begin(), compileEntries.end(),
            [](const struct CompileEntry& c1, const struct CompileEntry& c2){ return c1.Type > c2.Type; });
        foreach(struct CompileEntry entry, compileEntries)
        {
            QString output;
            switch(entry.Type)
            {
            case PatchType::C:
                if((output = CompileCFile(entry.FileName)) != "")
                {
                    return QString("Compiler error: ") + output;
                }
                break;
            case PatchType::Assembly:
                if((output = AssembleSFile(entry.FileName)) != "")
                {
                    return QString("Assembler error: ") + output;
                }
                break;
            case PatchType::Binary:
                if((output = ExtractOFile(entry.FileName)) != "")
                {
                    return QString("ObjCopy error: ") + output;
                }
            }
        }

        // For all entries, if the binary does not match the existing save chunk,
        // create a new save chunk and invalidate the old one


        // Create the save chunk for the PatchListChunk


        // Save the chunks to the ROM


        // Success
        return "";
    }

    /// <summary>
    /// Verify that the required files are found in the EABI installation bin directory.
    /// </summary>
    /// <param name="missing">
    /// The name of the first missing element encountered in verification.
    /// </param>
    /// <returns>
    /// True if the directory exists and the required binaries are found.
    /// </returns>
    bool VerifyEABI(QString *missing)
    {
        QDir eabiBinDir(EABI_INSTALLATION);
        if(!eabiBinDir.exists())
        {
            goto error;
        }
        if(!eabiBinDir.exists(EABI_GCC)) // compiling C
        {
            eabiBinDir.cd(EABI_GCC);
            goto error;
        }
        if(!eabiBinDir.exists(EABI_AS)) // assembling ARM/Thumb
        {
            eabiBinDir.cd(EABI_AS);
            goto error;
        }
        if(!eabiBinDir.exists(EABI_OBJCOPY)) // extracting binary from object file
        {
            eabiBinDir.cd(EABI_OBJCOPY);
            goto error;
        }
        return true;
error:
        *missing = EABI_INSTALLATION == "" ? "" : eabiBinDir.absolutePath();
        return false;
    }
}
