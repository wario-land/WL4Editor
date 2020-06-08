#include "PatchUtils.h"
#include <ROMUtils.h>
#include <QVector>
#include <cassert>
#include <QDir>
#include <QFile>
#include <SettingsUtils.h>
#include <QProcess>
#include <cstring>
#include "WL4EditorWindow.h"

#define PATCH_CHUNK_VERSION 0
#define PATCH_FIELD_COUNT 7

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

extern WL4EditorWindow *singleton;

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
/// Deserialize patch metadata from the patch list chunk into a patch metadata struct.
/// </summary>
/// <param name="patchTuples">
/// The string list of the fields for the patch metadata.
/// See the format specified in the documentation for UpgradePatchListContents.
/// </param>
/// <returns>
/// The patch metadata struct.
/// </returns>
static struct PatchEntryItem DeserializePatchMetadata(QStringList patchTuples)
{
    int patchType = patchTuples[1].toInt(Q_NULLPTR, 16);
    unsigned int hookAddress = static_cast<unsigned int>(patchTuples[2].toInt(Q_NULLPTR, 16));
    unsigned int patchAddress = static_cast<unsigned int>(patchTuples[3].toInt(Q_NULLPTR, 16));
    //assert(patchChunks.contains(patchAddress) /* Patch chunk list refers to an invalid patch address */);
    bool stubFunction = patchTuples[4] != "0";
    bool thumbMode = patchTuples[5] != "0";
    struct PatchEntryItem entry
    {
        patchTuples[0], // filename
        static_cast<enum PatchType>(patchType),
        hookAddress,
        stubFunction,
        thumbMode,
        patchAddress,
        patchTuples[6], // substituted bytes for the hook
        "" // hook string (not saved to patch list chunk)
    };
    return entry;
}

/// <summary>
/// Serialize a patch struct into a metadata string for the patch list chunk.
/// </summary>
/// <param name="patchMetadata">
/// The metadata struct to serialize.
/// See the format specified in the documentation for UpgradePatchListContents.
/// </param>
/// <returns>
/// The metadata string.
/// </returns>
static QString SerializePatchMetadata(struct PatchEntryItem patchMetadata)
{
    QString ret = patchMetadata.FileName + ";";
    ret += QString::number(patchMetadata.PatchType) + ";";
    ret += QString::number(patchMetadata.HookAddress, 16) + ";";
    ret += QString::number(patchMetadata.FunctionPointerReplacementMode) + ";";
    ret += QString::number(patchMetadata.ThumbMode) + ";";
    ret += QString::number(patchMetadata.PatchAddress, 16) + ";";
    return ret + patchMetadata.SubstitutedBytes;
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
    singleton->GetOutputWidgetPtr()->PrintString("C file does not have correct extension (should be .c)"); // TODO TODO TODO TODO TODO
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

        contents += SerializePatchMetadata(entry);
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
static QByteArray CreateHook(unsigned int patchAddr, bool thumbMode)
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
    for(struct PatchEntryItem entry : entries)
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
        default:;
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

/// <summary>
/// Compile a list of patches which must be added to the ROM.
/// A patch must be added if it is entirely new, or if it exists in the ROM already and has been modified.
/// </summary>
/// <param name="dialogPatches">
/// The patch entries coming from the dialog.
/// </param>
/// <param name="existingPatches">
/// The existing patch entries in the ROM.
/// </param>
/// <returns>
/// The list of patches which must be added to the ROM.
/// </returns>
static QVector<struct PatchEntryItem> DetermineNewPatches(QVector<struct PatchEntryItem> dialogPatches, QVector<struct PatchEntryItem> existingPatches)
{
    QVector<struct PatchEntryItem> newPatches;
    for(struct PatchEntryItem dialogPatch : dialogPatches)
    {
        // Check if save chunk exists in the ROM (by file name)
        struct PatchEntryItem *existingPatch = std::find_if(existingPatches.begin(), existingPatches.end(),
            [dialogPatch](struct PatchEntryItem e){ return e.FileName == dialogPatch.FileName; });
        bool mustCreateChunk, saveChunkExists = existingPatch != existingPatches.end();

        // If the save chunk does not exist in ROM, we must create it
        if(!(mustCreateChunk = !saveChunkExists))
        {
            dialogPatch.PatchAddress = existingPatch->PatchAddress;

            // If the save chunk exists by filename, we must check to see if content matches bin contents
            unsigned short chunkLen = *reinterpret_cast<unsigned short*>(existingPatch->PatchAddress + 4);
            mustCreateChunk = !BinaryMatchWithROM(dialogPatch.FileName, existingPatch->PatchAddress + 12, chunkLen);
        }

        if(mustCreateChunk)
        {
            newPatches.append(dialogPatch);
        }
    }
    return newPatches;
}

/// <summary>
/// Compile a list of patches which must be removed from the ROM.
/// A patch must be removed if it exists in the ROM and has either been modified, or is not in the chunk list from the dialog.
/// </summary>
/// <param name="dialogPatches">
/// The patch entries coming from the dialog.
/// </param>
/// <param name="existingPatches">
/// The existing patch entries in the ROM.
/// </param>
/// <returns>
/// The list of patches which must be removed from the ROM.
/// </returns>
static QVector<struct PatchEntryItem> DetermineRemovalPatches(QVector<struct PatchEntryItem> dialogPatches, QVector<struct PatchEntryItem> existingPatches)
{
    QVector<struct PatchEntryItem> removalPatches;
    for(struct PatchEntryItem existingPatch : existingPatches)
    {
        // Check if save chunk exists in the ROM (by file name)
        struct PatchEntryItem *dialogPatch = std::find_if(dialogPatches.begin(), dialogPatches.end(),
            [existingPatch](struct PatchEntryItem e){ return e.FileName == existingPatch.FileName; });
        bool mustRemoveChunk, saveChunkInDialog = dialogPatch != dialogPatches.end();

        // If the save chunk is not in the dialog, we must remove it
        if(!(mustRemoveChunk = !saveChunkInDialog))
        {
            // If the save chunk is in the dialog, we must check to see if content matches bin contents
            unsigned short chunkLen = *reinterpret_cast<unsigned short*>(existingPatch.PatchAddress + 4);
            mustRemoveChunk = !BinaryMatchWithROM(dialogPatch->FileName, existingPatch.PatchAddress + 12, chunkLen);
        }

        if(mustRemoveChunk)
        {
            removalPatches.append(existingPatch);
        }
    }
    return removalPatches;
}

/// <summary>
/// Convert a region of binary into a hex string.
/// </summary>
/// <param name="data">
/// The data to convert into a hex string.
/// </param>
/// <param name="len">
/// The length of the data to convert into a hex string.
/// </param>
/// <returns>
/// The hex string formatted as uppercase hex digits.
/// </returns>
static QString BinaryToHexString(unsigned char *data, int len)
{
    QString ret;
    while(len--)
    {
        ret += QString("%02X").arg(*(data++));
    }
    return ret;
}

/// <summary>
/// Convert a hex string into binary data.
/// </summary>
/// <param name="str">
/// The string to convert into binary.
/// </param>
/// <returns>
/// The binary created from the hex string.
/// </returns>
static std::shared_ptr<unsigned char[]> HexStringToBinary(QString str)
{
    std::shared_ptr<unsigned char[]> data(new unsigned char[str.length() / 2]);
    for(int i = 0; i < str.length(); i += 2)
    {
        data[i] = str.mid(i, 2).toUInt(Q_NULLPTR, 16);
    }
    return data;
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
                ROMUtils::SaveDataChunkType::PatchChunk
            );

            // Get the patch list information
            QString contents = GetUpgradedPatchListChunkData(patchListAddr);
            assert(contents.length() > 0 /* ROM contains an empty patch list chunk */);
            QStringList patchTuples = contents.split(";");
            assert(!(patchTuples.count() % PATCH_FIELD_COUNT) /* ROM contains a corrupted patch list chunk (field count is not a multiple of PATCH_FIELD_COUNT) */);
            for(int i = 0; i < patchTuples.count(); i += PATCH_FIELD_COUNT)
            {
                struct PatchEntryItem entry = DeserializePatchMetadata(patchTuples.mid(i, 7));
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

        ROMUtils::SaveDataIndex = 1;
        QVector<struct ROMUtils::SaveData> chunks;
        QVector<struct PatchEntryItem> existingPatches = GetPatchesFromROM();
        QVector<struct PatchEntryItem> addPatches = DetermineNewPatches(entries, existingPatches);
        QVector<struct PatchEntryItem> removePatches = DetermineRemovalPatches(entries, existingPatches);
        std::map<int, struct PatchEntryItem*> saveChunkIndexToMetadata, saveChunkIndexToRemoval;

        // Populate the chunk list with patches to add to the ROM
        for(struct PatchEntryItem patch : addPatches)
        {
            QString binName(patch.FileName);
            binName.chop(1);
            binName += "bin";

            // Get data from bin file
            QFile binFile(binName);
            binFile.open(QIODevice::ReadOnly);
            QByteArray binContents = binFile.readAll();
            unsigned char *data = new unsigned char[binFile.size()];
            memcpy(data, binContents.constData(), binFile.size());

            // Create the hook string for the patch
            struct PatchEntryItem *patchPtr = std::find_if(entries.begin(), entries.end(),
                [patch](struct PatchEntryItem origPatch){return patch.FileName == origPatch.FileName;});
            patchPtr->HookString = CreateHook(patchPtr->PatchAddress, patchPtr->ThumbMode); // TODO: This will come from the user, not a function
            saveChunkIndexToMetadata[ROMUtils::SaveDataIndex] = patchPtr;

            // Create the save chunk
            struct ROMUtils::SaveData patchChunk =
            {
                0,
                static_cast<unsigned int>(binFile.size()),
                data,
                ROMUtils::SaveDataIndex++,
                true,
                0,
                patch.PatchAddress,
                ROMUtils::SaveDataChunkType::PatchChunk
            };

            chunks.append(patchChunk);
        }
        
        // Populate the chunk list with invalidation chunks for patches to be removed from the ROM
        for(struct PatchEntryItem patch : removePatches)
        {
            saveChunkIndexToRemoval[ROMUtils::SaveDataIndex] = std::find_if(removePatches.begin(), removePatches.end(),
                [patch](struct PatchEntryItem removePatch){return patch.FileName == removePatch.FileName;});
            struct ROMUtils::SaveData invalidationChunk =
            {
                0,
                0,
                nullptr,
                ROMUtils::SaveDataIndex++,
                false,
                0,
                patch.PatchAddress,
                ROMUtils::SaveDataChunkType::InvalidationChunk
            };
            chunks.append(invalidationChunk);
        }

        // We must invalidate the old patch list chunk (if it exists)
        unsigned int patchListChunkAddr = ROMUtils::FindChunkInROM(
            ROMUtils::CurrentFile,
            ROMUtils::CurrentFileSize,
            WL4Constants::AvailableSpaceBeginningInROM,
            ROMUtils::SaveDataChunkType::PatchListChunk
        );
        if(patchListChunkAddr)
        {
            struct ROMUtils::SaveData PLinvalidationChunk =
            {
                0,
                0,
                nullptr,
                ROMUtils::SaveDataIndex++,
                false,
                0,
                patchListChunkAddr,
                ROMUtils::SaveDataChunkType::InvalidationChunk
            };
            chunks.append(PLinvalidationChunk);
        }

        // Save the chunks to the ROM
        bool firstCallback = true;
        bool ret = ROMUtils::SaveFile(ROMUtils::ROMFilePath, chunks,

            // ChunkAllocationCallback

            [firstCallback, entries, saveChunkIndexToMetadata, saveChunkIndexToRemoval]
            (unsigned char *TempFile, QVector<struct ROMUtils::SaveData> addedChunks, std::map<int, int> indexToChunkPtr) mutable
            {
                // Create and add PatchListChunk after patch chunk locations have been allocated by SaveFile()
                if(firstCallback)
                {
                    // Undo removal patches (if ROM already has patches)
                    QVector<struct PatchEntryItem> patchesInROM = GetPatchesFromROM();
                    if(patchesInROM.length())
                    {
                        for(struct ROMUtils::SaveData chunk : addedChunks)
                        {
                            if(chunk.ChunkType == ROMUtils::SaveDataChunkType::InvalidationChunk &&
                                saveChunkIndexToRemoval.find(chunk.index) != saveChunkIndexToRemoval.end()) // map does not contain an entry for the old patch list chunk's invalidator
                            {
                                // Find metadata for the existing patch in the ROM which we want to remove
                                struct PatchEntryItem *removalPatchInROM = saveChunkIndexToRemoval.at(chunk.index);

                                // Get patch hex string from current patch list chunk, write into TempFile
                                std::shared_ptr<unsigned char[]> originalBytes = HexStringToBinary(removalPatchInROM->SubstitutedBytes);
                                memcpy(TempFile, &originalBytes, removalPatchInROM->SubstitutedBytes.length() / 2);
                            }
                        }
                    }

                    // Update entry structs with information from the added chunks
                    for(struct ROMUtils::SaveData chunk : addedChunks)
                    {
                        if(chunk.ChunkType == ROMUtils::SaveDataChunkType::PatchChunk)
                        {
                            // Set the patch address that was calculated by the save chunk allocator
                            struct PatchEntryItem *patchPtr = saveChunkIndexToMetadata.at(chunk.index);
                            patchPtr->PatchAddress = indexToChunkPtr[chunk.index];

                            // Capture data from hook address for the entry's substituted bytes (depends on size of hook)
                            if(patchPtr->HookAddress)
                            {
                                int hookLength = patchPtr->HookString.length() / 2; // hook string is hex string, 2 digits per byte
                                patchPtr->SubstitutedBytes = BinaryToHexString(TempFile + patchPtr->HookAddress, hookLength);
                            }
                        }
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

            [chunks, saveChunkIndexToMetadata]
            (unsigned char *TempFile, std::map<int, int> indexToChunkPtr)
            {
                (void)indexToChunkPtr;

                // Write hooks to ROM
                for(struct ROMUtils::SaveData chunk : chunks)
                {
                    if(chunk.ChunkType == ROMUtils::SaveDataChunkType::PatchChunk) // hooks only written for PatchChunk save chunk type
                    {
                        // Get patch metadata associated with this save chunk
                        struct PatchEntryItem *patchPtr = saveChunkIndexToMetadata.at(chunk.index);

                        // Write hook to ROM
                        std::shared_ptr<unsigned char[]> hookData = HexStringToBinary(patchPtr->HookString);
                        memcpy(TempFile + patchPtr->PatchAddress, &hookData, patchPtr->HookString.length() / 2);
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
