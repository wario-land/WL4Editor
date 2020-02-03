#include "PatchUtils.h"
#include <ROMUtils.h>
#include <QVector>
#include <cassert>
#include <QDir>
#include <QFile>
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
    tempstr.chop(strlen(fromstr));           \
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
        "-fomit-frame-pointer" << "-ffast-math" << "-mthumb" << "-mthumb-interwork" <<
        "-mgeneral-regs-only" << "-S" << cfile << "-o" << outfile;

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

/// <summary>
/// Determine if a binary file's contents match a region in the currently loaded ROM.
/// </summary>
/// <param name="file">
/// The binary file to compare (relative path).
/// </param>
/// <param name="startAddr">
/// The starting address in the ROM data.
/// </param>
/// <param name="length">
/// The length of the data in the ROM.
/// </param>
/// <returns>
/// True if the contents of the binary file match the region in the currently loaded ROM.
/// </returns>
static bool BinaryMatchWithROM(QString file, unsigned int startAddr, unsigned int length)
{
    assert(length <= 0xFFFF /* Corrupted length value was passed to BinaryMatchWithROM function */);
    if(startAddr + length > ROMUtils::CurrentFileSize) return false;
    QFile binFile(file);
    binFile.open(QIODevice::ReadOnly);
    bool ret = false;
    if(binFile.size() == length)
    {
        QByteArray binContents = binFile.readAll();
        ret = !memcmp(binContents.constData(), ROMUtils::CurrentFile + startAddr, length);
    }
    binFile.close();
    return ret;
}

/// <summary>
/// Create the data for a patch list chunk.
/// </summary>
/// <param name="entries">
/// The patch entries used to create the patch list chunk.
/// </param>
/// <returns>
/// The data as an allocated char array.
/// </returns>
static QString CreatePatchListChunkData(QVector<struct PatchEntryItem> entries)
{
    QString contents;
    bool first = true;
    foreach(struct PatchEntryItem entry, entries)
    {
        // Delimit entries with semicolon
        if(first) first = false; else contents += ";";

        // Format matches description from UpgradePatchListContents
        contents += entry.FileName + ";";
        contents += QString::number(entry.PatchType) + ";";
        contents += QString::number(entry.HookAddress, 16).toUpper() + ";";
        contents += QString::number(entry.PatchAddress, 16).toUpper() + ";";
        contents += QString(entry.FunctionPointerReplacementMode ? "1" : "0") + ";";
        contents += QString(entry.ThumbMode ? "1" : "0") + ";";
        contents += entry.SubstitutedBytes;
    }
    return contents;
}

/// <summary>
/// Create the data for a hook.
/// </summary>
/// <param name="patchAddr">
/// The address that the hook will branch to.
/// </param>
/// <param name="stubFunction">
/// If true, stub the function by returning early.
/// </param>
/// <param name="thumbMode">
/// If true, specify that the code being hooked is in thumb mode.
/// </param>
/// <returns>
/// The hook payload.
/// </returns>
static QByteArray CreateHook(unsigned int patchAddr, bool stubFunction, bool thumbMode)
{
    if(thumbMode)
    {
        const char thumbHook[14] = {
            '\x01', '\xB5', // PUSH R0, LR
            '\x01', '\x48', // LDR R0, 4
            '\x80', '\x47', // BLX R0
            '\x01', '\xE0', // B 4
            '\0', '\0', '\0', '\0', // hook address goes here
            '\x00', '\xBD'  // POP LR
        };
        QByteArray hook(thumbHook, sizeof(thumbHook));
        *(unsigned int*)(hook.data() + 8) = patchAddr | 0x8000000;
        return hook;
    }
    else
    {
        // TODO populate an array for ARM mode
        return QByteArray();
    }
}

/// <summary>
/// Compile files in a list of patches to save to the ROM.
/// </summary>
/// <param name="entries">
/// The patch entries to compile.
/// </param>
/// <returns>
/// The error string if compilation failed, or empty if successful.
/// </returns>
static QString CompilePatchEntries(QVector<struct PatchEntryItem> entries)
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
    return ""; // success
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
        QString compileErrorMsg = CompilePatchEntries(entries);
        if(compileErrorMsg != "") return compileErrorMsg;

        /* For all entries:
         *   1. Save chunk does not exist:
         *     Create save chunk
         *   2. Save chunk exists, but does not match binary contents of entry:
         *     Invalidate save chunk, create new one
         *   3. Save chunk exists, and matches binary contents of entry:
         *     Do not invalidate save chunk, but still re-write hook in post-processing
         */
        ROMUtils::SaveDataIndex = 1;
        QVector<struct ROMUtils::SaveData> chunks;
        std::map<int, int> chunkIndexToEntryIndex;
        QVector<struct PatchEntryItem> existingPatches = GetPatchesFromROM();
        for(int i = 0; i < entries.size(); ++i)
        {
            struct PatchEntryItem entry = entries[i];
            QString binName(entry.FileName);
            binName.chop(1);
            binName += "bin";

            // Determine if the patch already exists in the ROM
            struct PatchEntryItem *existingPatch = std::find_if(existingPatches.begin(), existingPatches.end(),
                [entry](struct PatchEntryItem e){ return e.FileName == entry.FileName; });
            bool mustCreateChunk, saveChunkExists = existingPatch != existingPatches.end();

            // Determine if we must create a new save chunk
            if(!(mustCreateChunk = !saveChunkExists))
            {
                // Save chunk exists: check to see if content matches bin contents
                unsigned short chunkLen = *reinterpret_cast<unsigned short*>(existingPatch->PatchAddress + 4);
                mustCreateChunk = !BinaryMatchWithROM(entry.FileName, existingPatch->PatchAddress + 12, chunkLen);
            }

            // Create the save chunk
            if(mustCreateChunk)
            {
                // Get data from bin file
                QFile binFile(binName);
                binFile.open(QIODevice::ReadOnly);
                QByteArray binContents = binFile.readAll();
                unsigned char *data = new unsigned char[binFile.size()];
                memcpy(data, binContents.constData(), binFile.size());

                // Create the save chunk
                struct ROMUtils::SaveData patchChunk =
                {
                    0,
                    static_cast<unsigned int>(binFile.size()),
                    data,
                    ROMUtils::SaveDataIndex++,
                    true,
                    0,
                    saveChunkExists ? existingPatch->PatchAddress : 0,
                    ROMUtils::SaveDataChunkType::PatchChunk
                };
                chunks.append(patchChunk);
                chunkIndexToEntryIndex[patchChunk.index] = i;
            }
        }
        
        // Find which existing patch chunks should be removed
        for(int i = 0; i < existingPatches.size(); ++i)
        {
            
        }

        // Save the chunks to the ROM
        bool firstCallback = true;
        bool ret = ROMUtils::SaveFile(ROMUtils::ROMFilePath, chunks,
            // ChunkAllocationCallback
            [firstCallback, entries, chunkIndexToEntryIndex]
            (QVector<struct ROMUtils::SaveData> addedChunks, std::map<int, int> indexToChunkPtr) mutable
            {
                // Create and add PatchListChunk after patch chunk locations have been allocated by SaveFile()
                if(firstCallback)
                {
                    // Update entry structs with information from the added chunks
                    foreach(struct ROMUtils::SaveData chunk, addedChunks)
                    {
                        int entryIndex = chunkIndexToEntryIndex[chunk.index];
                        entries[entryIndex].PatchAddress = indexToChunkPtr[chunk.index];

                        // TODO Capture data from hook address for the entry's substituted bytes (depends on size of hook)

                    }
                    addedChunks.clear();

                    // Create the save chunk for the PatchListChunk
                    unsigned int patchListAddr = ROMUtils::FindChunkInROM(
                        ROMUtils::CurrentFile,
                        ROMUtils::CurrentFileSize,
                        WL4Constants::AvailableSpaceBeginningInROM,
                        ROMUtils::SaveDataChunkType::PatchListChunk
                    );
                    QString patchListChunkContents = CreatePatchListChunkData(entries);
                    unsigned char *data = new unsigned char[patchListChunkContents.length()];
                    memcpy(data, patchListChunkContents.toLocal8Bit().constData(), patchListChunkContents.length());
                    struct ROMUtils::SaveData patchListChunk =
                    {
                        0,
                        static_cast<unsigned int>(patchListChunkContents.length()),
                        data,
                        ROMUtils::SaveDataIndex++,
                        false,
                        0,
                        patchListAddr, // will be 0 if it does not already exist
                        ROMUtils::SaveDataChunkType::PatchListChunk
                    };
                    addedChunks.append(patchListChunk);

                    firstCallback = false;
                }
                else
                {
                    addedChunks.clear();
                }
            },
            // PostProcessingCallback
            [chunks, entries, chunkIndexToEntryIndex]
            (unsigned char *TempFile, std::map<int, int> indexToChunkPtr)
            {
                // Restore substituted data to ROM for patches that have been removed
                for(int i = 0; i < chunks.size(); ++i)
                {
                    struct ROMUtils::SaveData chunk = chunks[i];
                    if(chunk.ChunkType == ROMUtils::SaveDataChunkType::InvalidationChunk)
                    {
                        int hookSize = CreateHook(0, false, false).size();
                        // TODO
                    }
                }

                // Write hooks to ROM
                for(int i = 0; i < chunks.size(); ++i)
                {
                    struct ROMUtils::SaveData chunk = chunks[i];
                    if(chunk.ChunkType == ROMUtils::SaveDataChunkType::PatchChunk)
                    {
                        // For each patch chunk, convert its index to entry index to get matching entry info
                        PatchEntryItem entry = entries[chunkIndexToEntryIndex.at(i)];
                        QByteArray hookCode = CreateHook(entry.PatchAddress, entry.FunctionPointerReplacementMode, entry.ThumbMode);
                        assert(entry.PatchAddress + hookCode.size() < ROMUtils::CurrentFileSize /* Hook code outside valid ROM area */);
                        memcpy(TempFile + entry.PatchAddress, hookCode.data(), hookCode.size());
                    }
                }
            }
        );

        // Success
        return ret ? "" : "Error saving ROM file";
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
