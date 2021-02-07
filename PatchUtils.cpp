#include "PatchUtils.h"
#include <ROMUtils.h>
#include <QVector>
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
#define EABI_LD      "arm-none-eabi-ld.exe"
#define EABI_OBJCOPY "arm-none-eabi-objcopy.exe"
#else // _WIN32 (else linux)
#define EABI_GCC     "arm-none-eabi-gcc"
#define EABI_AS      "arm-none-eabi-as"
#define EABI_LD      "arm-none-eabi-ld"
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
        ret += QString("%1").arg(*(data++), 2, 16, QChar('0')).toUpper();
    }
    return ret;
}

/// <summary>
/// Convert a hex string into binary data.
/// </summary>
/// <remarks>
/// The binary data is dynamically allocated. Make sure to delete it after use.
/// </remarks>
/// <param name="buf">
/// The buffer that the binary will be written to.
/// It is assumed that the buffer is allocated to the proper length (half the length of the string)
/// </param>
/// <param name="str">
/// The string to convert into binary.
/// </param>
/// <returns>
/// The binary created from the hex string.
/// </returns>
static unsigned char *HexStringToBinary(QString str)
{
    unsigned char *buf = new unsigned char[str.length() / 2];
    for(int i = 0; i < str.length(); i += 2)
    {
        buf[i / 2] = str.mid(i, 2).toUInt(Q_NULLPTR, 16);
    }
    return buf;
}

/// <summary>
/// Upgrade the format of a patch list chunk by one version.
/// </summary>
/// <remarks>
/// Version 0:
///   Semicolon-delimited (6 fields):
///     0: Filename             (string, may not contain a semicolon)
///     1: Patch type           (int)
///     2: Hook address         (hex string)
///     3: Patch address        (hex string)
///     4: Substituted bytes    (hex string)
///     5: Patch address offset (int)
///     6: Patch description    (string, may not contain a semicolon)
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
    unsigned int hookAddress = patchTuples[2].toUInt(Q_NULLPTR, 16);
    unsigned int patchAddress = patchTuples[3].toUInt(Q_NULLPTR, 16);
    unsigned int patchAddressOffset = patchTuples[5].toUInt(Q_NULLPTR, 16);
    bool hasPointer = patchAddressOffset != (unsigned int) -1;
    unsigned int hookLength = patchTuples[4].length() / 2;
    QString hookString = BinaryToHexString(ROMUtils::CurrentFile + hookAddress, hookLength); // obtain hook string directly from the patched rom data
    if(hasPointer)
    {
        // Splice patch address out of the save chunk binary
        hookString = hookString.mid(0, patchAddressOffset * 2) + hookString.mid((patchAddressOffset + 4) * 2);
    }
    struct PatchEntryItem entry
    {
        patchTuples[0], // filename
        static_cast<enum PatchType>(patchType),
        hookAddress,
        hookString,
        patchAddressOffset,
        patchAddress,
        patchTuples[4], // substituted bytes for the hook
        patchTuples[6] // description
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
    ret += QString::number(patchMetadata.HookAddress, 16).toUpper() + ";";
    ret += QString::number(patchMetadata.PatchAddress, 16).toUpper() + ";";
    ret += patchMetadata.SubstitutedBytes + ";";
    ret += QString::number(patchMetadata.PatchOffsetInHookString, 16).toUpper() + ";";
    ret += patchMetadata.Description;
    return ret;
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
    int chunkVersion = ROMUtils::CurrentFile[chunkDataAddr + 12];
    if(chunkVersion > PATCH_CHUNK_VERSION)
    {
        singleton->GetOutputWidgetPtr()->PrintString(QString(QT_TR_NOOP("Patch list chunk either corrupt or this verison of WL4Editor is old and doesn't support the saved format. Version found: ")) + QString::number(chunkVersion));
        return "";
    }
    QString contents = QString::fromLocal8Bit(reinterpret_cast<const char*>(ROMUtils::CurrentFile + chunkDataAddr + 13), contentSize);
    while(chunkVersion < PATCH_CHUNK_VERSION)
    {
        contents = UpgradePatchListContents(contents, chunkVersion++);
    }
    return contents;
}

/// <summary>
/// Helper function for running a process (since it is used in several places)
/// </summary>
/// <param name="executable">
/// The executable to run.
/// </param>
/// <param name="args">
/// The arguments to the process.
/// </param>
/// <returns>
/// An empty string if successful, or an error string if failure.
/// </returns>
static QString RunProcess(QString executable, QStringList args)
{
    QProcess process;
    process.start(executable, args);
    process.waitForFinished();
    return !process.exitCode() ? "" : QString(process.readAllStandardError());
}

/// <summary>
/// Compile a C file into an assembly file.
/// </summary>
/// <param name="cfile">
/// The C file to compile.
/// </param>
/// <returns>
/// An empty string if successful, or an error string if failure.
/// </returns>
static QString CompileCFile(QString cfile)
{
    if(!cfile.endsWith(".c"))
    {
        QString msg = QString(QT_TR_NOOP("C file does not have correct extension (should be .c): ")) + cfile;
        singleton->GetOutputWidgetPtr()->PrintString(msg);
        return msg;
    }

    // Create args
    QString outfile(cfile);
    REPLACE_EXT(outfile, ".c", ".s");
    QString executable(QString(PatchUtils::EABI_INSTALLATION) + "/" + EABI_GCC);
    QStringList args;
    args << "-MMD" << "-MP" << "-MF" << "-g" << "-Wall" << "-mcpu=arm7tdmi" << "-mtune=arm7tdmi" <<
        "-fomit-frame-pointer" << "-ffast-math" << "-mthumb" << "-mthumb-interwork" << "-O2" <<
        "-mlong-calls" << "-S" << cfile << "-o" << outfile;

    // Run GCC
    return RunProcess(executable, args);
}

/// <summary>
/// Assemble an ASM file into an object file.
/// </summary>
/// <param name="sfile">
/// The ASM file to assemble.
/// </param>
/// <returns>
/// An empty string if successful, or an error string if failure.
/// </returns>
static QString AssembleSFile(QString sfile)
{
    if(!sfile.endsWith(".s"))
    {
        QString msg = QString(QT_TR_NOOP("ASM file does not have correct extension (should be .s): ")) + sfile;
        singleton->GetOutputWidgetPtr()->PrintString(msg);
        return msg;
    }

    // Create args
    QString outfile(sfile);
    REPLACE_EXT(outfile, ".s", ".o");
    QString executable(QString(PatchUtils::EABI_INSTALLATION) + "/" + EABI_AS);
    QStringList args;
    args << "-mthumb" << sfile << "-o" << outfile;

    // Run AS
    return RunProcess(executable, args);
}

/// <summary>
/// Create the linker script for an object file.
/// </summary>
/// <param name="entry">
/// The patch entry with information about the type of file.
/// </param>
/// <returns>
/// An empty string if successful, or an error string if failure.
/// </returns>
static QString CreateLinkerScript(struct PatchEntryItem entry)
{
    QString romFileDir = QFileInfo(ROMUtils::ROMFilePath).dir().path();
    QString ofile(romFileDir + QDir::separator() + entry.FileName);
    QString ldfile(ofile);
    REPLACE_EXT(ofile, ".c", ".o"); // works for .s files too
    REPLACE_EXT(ldfile, ".c", ".ld");

    QString pa = QString::number(0x8000000 | entry.PatchAddress + 12, 16);
    QString scriptContents =
        QString("SECTIONS\n") +
        "{\n" +
        "    .text   0x" + pa + " : { \"" + ofile + "\" (.text) }\n" +
        "    .data : { \"" + ofile + "\" (.data) }\n" +
        "    .rodata : { \"" + ofile + "\" (.rodata) }\n" +
        "}\n" +
        "memcpy = 0x80950D9;";

    QFile file(ldfile);
    file.open(QIODevice::WriteOnly);
    if (!file.isOpen())
    {
        QString msg = QString(QT_TR_NOOP("Could not open linker script file for writing: ")) + ldfile;
        singleton->GetOutputWidgetPtr()->PrintString(msg);
        return msg;
    }
    else
    {
        file.write(scriptContents.toUtf8());
    }
    return "";
}

/// <summary>
/// Link an object file into an ELF file.
/// </summary>
/// <param name="ofile">
/// The object file to link into an ELF file.
/// </param>
/// <returns>
/// An empty string if successful, or an error string if failure.
/// </returns>
static QString LinkOFile(QString ofile)
{
    if(!ofile.endsWith(".o"))
    {
        QString msg = QString(QT_TR_NOOP("Object file does not have correct extension (should be .o): ")) + ofile;
        singleton->GetOutputWidgetPtr()->PrintString(msg);
        return msg;
    }

    // Create args
    QString outfile(ofile);
    REPLACE_EXT(outfile, ".o", ".elf");
    QString ldfile(ofile);
    REPLACE_EXT(ldfile, ".o", ".ld");
    QString executable(QString(PatchUtils::EABI_INSTALLATION) + "/" + EABI_LD);
    QStringList args;
    args << "-T" << ldfile << "-o" << outfile;

    // Run LD
    return RunProcess(executable, args);
}

/// <summary>
/// Extract the binary from an object file.
/// </summary>
/// <param name="elfFile">
/// The ELF file from which to extract the binary.
/// </param>
/// <returns>
/// An empty string if successful, or an error string if failure.
/// </returns>
static QString ExtractELFFile(QString elfFile)
{
    if(!elfFile.endsWith(".elf"))
    {
        QString msg = QString(QT_TR_NOOP("ELF file does not have correct extension (should be .elf): ")) + elfFile;
        singleton->GetOutputWidgetPtr()->PrintString(msg);
        return msg;
    }

    // Create args
    QString outfile(elfFile);
    REPLACE_EXT(outfile, ".elf", ".bin");
    QString executable(QString(PatchUtils::EABI_INSTALLATION) + "/" + EABI_OBJCOPY);
    QStringList args;
    args << "-O" << "binary" << "-j" << ".text" << "-j" << ".rodata" << elfFile << outfile;

    // Run OBJCOPY
    return RunProcess(executable, args);
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
    if(length > 0xFFFF)
    {
        singleton->GetOutputWidgetPtr()->PrintString(QString(QT_TR_NOOP("Invalid comparison length in BinaryMatchWithROM: 0x")) + QString::number(length, 16).toUpper());
        return false;
    }

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
    for(struct PatchEntryItem entry : entries)
    {
        // Delimit entries with semicolon
        if(first) first = false; else contents += ";";

        contents += SerializePatchMetadata(entry);
    }
    return contents;
}

/// <summary>
/// Compile a patch entry file to get the binary to save to the ROM.
/// </summary>
/// <param name="entry">
/// The patch entry to compile.
/// </param>
/// <returns>
/// The error string if compilation failed, or empty if successful.
/// </returns>
static QString CompilePatchEntry(struct PatchEntryItem entry)
{
    if(!entry.FileName.length() || entry.PatchType == PatchType::Binary) return "";

    QDir ROMdir = QFileInfo(ROMUtils::ROMFilePath).dir();
    QString filename(ROMdir.absolutePath() + QDir::separator() + entry.FileName);

    QString output;
    switch(entry.PatchType)
    {
    case PatchType::C:
        if((output = CompileCFile(filename)) != "")
        {
            return QString(QT_TR_NOOP("Compiler error: ")) + output;
        }
        REPLACE_EXT(filename, ".c", ".s");
    case PatchType::Assembly:
        if((output = AssembleSFile(filename)) != "")
        {
            return QString(QT_TR_NOOP("Assembler error: ")) + output;
        }
        REPLACE_EXT(filename, ".s", ".o");
        if((output = CreateLinkerScript(entry)) != "")
        {
            return QString(QT_TR_NOOP("Error creating linker script: ")) + output;
        }
        if((output = LinkOFile(filename)) != "")
        {
            return QString(QT_TR_NOOP("Linker error: ")) + output;
        }
        REPLACE_EXT(filename, ".o", ".elf");
        if((output = ExtractELFFile(filename)) != "")
        {
            return QString(QT_TR_NOOP("ObjCopy error: ")) + output;
        }
        break;
    default:;
    }
    return ""; // success
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
    QString output;
    for(struct PatchEntryItem entry : entries)
    {
        if((output = CompilePatchEntry(entry)) != "")
        {
            return output;
        }
    }
    return "";
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
            [existingPatch](struct PatchEntryItem e){ return e.HookAddress == existingPatch.HookAddress; });
        bool mustRemoveChunk, saveChunkInDialog = dialogPatch != dialogPatches.end();

        // If the save chunk is not in the dialog, we must remove it
        if(!(mustRemoveChunk = !saveChunkInDialog))
        {
            // If the save chunk is in the dialog, we must check to see if content matches bin contents
            unsigned short chunkLen = *reinterpret_cast<unsigned short*>(ROMUtils::CurrentFile + existingPatch.PatchAddress + 4);
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
/// Create a save chunk for a patch list entry
/// </summary>
/// <param name="patch">
/// The patch entry for which to create a save chunk.
/// </param>
/// <returns>
/// The save chunk.
/// </returns>
static struct ROMUtils::SaveData CreatePatchSaveChunk(struct PatchEntryItem &patch)
{
    QString binName(patch.FileName);
    binName.chop(binName.length() - binName.lastIndexOf('.') - 1);
    binName += "bin";

    // Get data from bin file
    QString romFileDir = QFileInfo(ROMUtils::ROMFilePath).dir().path();
    QFile binFile(romFileDir + QDir::separator() + binName);
    binFile.open(QIODevice::ReadOnly);
    QByteArray binContents = binFile.readAll();
    int fileSize = binFile.size();
    unsigned char *data = new unsigned char[fileSize];
    memcpy(data, binContents.constData(), fileSize);

    // Create the save chunk
    return
    {
        0,
        static_cast<unsigned int>(fileSize),
        data,
        ROMUtils::SaveDataIndex++,
        true,
        0,
        0,
        ROMUtils::SaveDataChunkType::PatchChunk
    };
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
            if(!contents.length())
            {
                singleton->GetOutputWidgetPtr()->PrintString(QT_TR_NOOP("ROM contains an empty patch list chunk (this should not be possible)"));
                return patchEntries;
            }
            QStringList patchTuples = contents.split(";");
            if(patchTuples.count() % PATCH_FIELD_COUNT)
            {
                singleton->GetOutputWidgetPtr()->PrintString(QT_TR_NOOP("ROM contains a corrupted patch list chunk (field count is not a multiple of ") + QString::number(PATCH_FIELD_COUNT) + ")");
                return patchEntries;
            }
            for(int i = 0; i < patchTuples.count(); i += PATCH_FIELD_COUNT)
            {
                struct PatchEntryItem entry = DeserializePatchMetadata(patchTuples.mid(i, PATCH_FIELD_COUNT));
                if(!std::find_if(patchChunks.begin(), patchChunks.end(), [entry](unsigned int addr){return addr == entry.PatchAddress;}))
                {
                    singleton->GetOutputWidgetPtr()->PrintString(QT_TR_NOOP("Corruption error: Patch chunk list entry refers to an invalid patch address: 0x") + QString::number(entry.PatchAddress, 16).toUpper());
                    continue;
                }
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
        QVector<struct PatchEntryItem> removePatches = DetermineRemovalPatches(entries, existingPatches);
        std::map<int, struct PatchEntryItem*> saveChunkIndexToMetadata, saveChunkIndexToRemoval;

        // Populate the chunk list with patches to add to the ROM
        for(struct PatchEntryItem patch : entries)
        {
            if(!patch.FileName.length()) continue; // no save chunk to create for hook-only patches

            QString binName(patch.FileName);
            binName.chop(1);
            binName += "bin";

            struct ROMUtils::SaveData patchChunk = CreatePatchSaveChunk(patch);

            // Save the mapping for the save chunk onto the added patch
            saveChunkIndexToMetadata[ROMUtils::SaveDataIndex] = std::find_if(entries.begin(), entries.end(),
                [patch](struct PatchEntryItem origPatch){return patch.HookAddress == origPatch.HookAddress;});

            chunks.append(patchChunk);
        }
        
        // Populate the chunk list with invalidation chunks for patches to be removed from the ROM
        QVector<unsigned int> invalidationChunks;
        for(struct PatchEntryItem patch : removePatches)
        {
            if(!patch.FileName.length()) continue; // no save chunks to invalidate for hook-only patches

            saveChunkIndexToRemoval[ROMUtils::SaveDataIndex] = std::find_if(removePatches.begin(), removePatches.end(),
                [patch](struct PatchEntryItem removePatch){return patch.HookAddress == removePatch.HookAddress;});
            invalidationChunks.append(patch.PatchAddress + 12);
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
            invalidationChunks.append(patchListChunkAddr + 12);
        }

        // Start the iterator on the first patch which is not a hex edit
        auto patchAllocIter = std::find_if(entries.begin(), entries.end(), []
            (const struct PatchEntryItem p){return p.FileName.length();});
        std::map<unsigned int, unsigned int> neededSizeMap; // key: hook address
        QString errorMsg;
        bool plcAllocated = !entries.size(); // patch list chunk status
        bool firstCallback = true;

        // Allocate and save the chunks to the ROM
        bool ret = ROMUtils::SaveFile(ROMUtils::ROMFilePath, invalidationChunks,

            // ChunkAllocator

            [&neededSizeMap, &patchAllocIter, &entries, &errorMsg, &plcAllocated, &firstCallback, removePatches]
            (unsigned char *TempFile, struct ROMUtils::FreeSpaceRegion freeSpace, struct ROMUtils::SaveData *sd, bool resetchunkIndex)
            {
                if(resetchunkIndex)
                {
                    patchAllocIter = std::find_if(entries.begin(), entries.end(), []
                    (const struct PatchEntryItem p){return p.FileName.length();});
                }
                // On the first callback, we must recalculate the substituted bytes for the hook strings
                // of the unmodified ROM. This must occur strictly before the new patch list chunk is created.
                if(firstCallback)
                {
                    // Undo removal patches (if there are patches to remove)
                    for(struct PatchEntryItem patch : removePatches)
                    {
                        // Get patch hex string from removal patch struct, write into TempFile
                        unsigned char *originalBytes = HexStringToBinary(patch.SubstitutedBytes);
                        memcpy(TempFile + patch.HookAddress, originalBytes, patch.SubstitutedBytes.length() / 2);
                        delete[] originalBytes;
                    }

                    // Capture data from hook address for the entry's substituted bytes (depends on size of hook)
                    for(struct PatchEntryItem &patch : entries)
                    {
                        int hookLength = patch.HookString.length() / 2; // hook string is hex string, 2 digits per byte
                        if(patch.PatchOffsetInHookString != static_cast<unsigned int>(-1))
                        {
                            hookLength += 4;
                        }
                        patch.SubstitutedBytes = BinaryToHexString(TempFile + patch.HookAddress, hookLength);
                    }

                    firstCallback = false;
                }

                // Create save data for all the patches in the iterator
                if(patchAllocIter != entries.end())
                {
                    // If we have already reported insufficient space for this patch, then
                    // reject all regions smaller than required space
                    auto neededSizePair = std::find_if(neededSizeMap.begin(), neededSizeMap.end(), [patchAllocIter]
                        (std::pair<const unsigned int, const unsigned int> p){return p.first == patchAllocIter->HookAddress;});
                    if(neededSizePair != neededSizeMap.end())
                    {
                        unsigned int neededSize = (*neededSizePair).second;
                        if(freeSpace.size < neededSize)
                        {
                            return ROMUtils::ChunkAllocationStatus::InsufficientSpace;
                        }
                    }

                    // If it is the first time processing this patch or we've been given a
                    // new free space region for this patch, compile and link at proposed
                    // address to find the required size
                    int alignOffset = ((freeSpace.addr + 3) & ~3) - freeSpace.addr;
                    patchAllocIter->PatchAddress = freeSpace.addr + alignOffset;
                    if((errorMsg = CompilePatchEntry(*patchAllocIter)) != "")
                    {
                        return ROMUtils::ChunkAllocationStatus::ProcessingError;
                    }

                    // We must get the size of the compiled binary and check it against the space we were offered
                    // To see if the data will fit, we must include the alignment offset and the size of the RATS header
                    struct ROMUtils::SaveData saveData = CreatePatchSaveChunk(*patchAllocIter);
                    if(saveData.size + alignOffset + 12 > freeSpace.size)
                    {
                        delete saveData.data;
                        return ROMUtils::ChunkAllocationStatus::InsufficientSpace;
                    }

                    // Allocation success
                    *sd = saveData;

                    // Advance patch iterator to next non-hex-edit patch
                    patchAllocIter++;
                    while(patchAllocIter != entries.end() && !patchAllocIter->FileName.length())
                    {
                        patchAllocIter++;
                    }

                    return ROMUtils::ChunkAllocationStatus::Success;
                }
                else
                {
                    // Create patch list chunk after all patches have been processed
                    if(!plcAllocated)
                    {
                        // Create patch list chunk contents, make sure there is sufficient space
                        QString patchListChunkContents = CreatePatchListChunkData(entries);
                        // To see if the data will fit, we must include the text contents, size of the RATS header, and one byte for versioning
                        if((unsigned int)patchListChunkContents.length() + 13 > freeSpace.size)
                        {
                            return ROMUtils::ChunkAllocationStatus::InsufficientSpace;
                        }

                        // Create the save chunk data
                        unsigned char *data = new unsigned char[patchListChunkContents.length() + 1];
                        memcpy(data + 1, patchListChunkContents.toLocal8Bit().constData(), patchListChunkContents.length());
                        data[0] = PATCH_CHUNK_VERSION;
                        struct ROMUtils::SaveData patchListChunk =
                        {
                            0,
                            static_cast<unsigned int>(patchListChunkContents.length() + 1),
                            data,
                            ROMUtils::SaveDataIndex++,
                            false,
                            0,
                            0,
                            ROMUtils::SaveDataChunkType::PatchListChunk
                        };
                        *sd = patchListChunk;

                        plcAllocated = true;
                        return ROMUtils::ChunkAllocationStatus::Success;
                    }

                    return ROMUtils::ChunkAllocationStatus::NoMoreChunks;
                }
            },

            // PostProcessingCallback

            [removePatches, chunks, &entries]
            (unsigned char *TempFile, std::map<int, int> indexToChunkPtr)
            {
                (void)indexToChunkPtr;

                // Write hooks to ROM
                for(struct PatchEntryItem patch : entries)
                {
                    QString hookString = patch.HookString;

                    // Splice patch address into hook string
                    if(patch.PatchOffsetInHookString != static_cast<unsigned int>(-1))
                    {
                        uint32_t patchAddress = 0x8000000 | (patch.PatchAddress + 13); // STAR header + 1 so that BL goes into thumb mode
                        patchAddress = ROMUtils::EndianReverse(patchAddress);
                        QString patchAddressString = QString("%1").arg(patchAddress, 8, 16, QChar('0')).toUpper();
                        hookString = hookString.mid(0, patch.PatchOffsetInHookString * 2) +
                            patchAddressString + hookString.mid(patch.PatchOffsetInHookString * 2);
                    }

                    // Convert hook string to binary and save to ROM
                    unsigned char *hookData = HexStringToBinary(hookString);
                    memcpy(TempFile + patch.HookAddress, hookData, hookString.length() / 2);
                    delete[] hookData;
                }

                return QString("");
            }
        );

        return ret ? "" : errorMsg;
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
        if(!eabiBinDir.exists(EABI_LD)) // linking object into ELF
        {
            eabiBinDir.cd(EABI_LD);
            goto error;
        }
        if(!eabiBinDir.exists(EABI_OBJCOPY)) // extracting binary from ELF file
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
