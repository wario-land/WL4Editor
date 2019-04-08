#include "PatchUtils.h"
#include <ROMUtils.h>
#include <QVector>
#include <cassert>

#define PATCH_CHUNK_VERSION 0

/// <summary>
/// Upgrade the format of a patch list chunk by one version.
/// </summary>
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
static QString GetUpgradedPatchListChunkData(int chunkDataAddr)
{
    unsigned short contentSize = *reinterpret_cast<unsigned short*>(ROMUtils::CurrentFile + chunkDataAddr + 4) - 1;
    QString contents = QString::fromLocal8Bit((const char*)(ROMUtils::CurrentFile + chunkDataAddr + 13), contentSize);
    int chunkVersion = ROMUtils::CurrentFile[chunkDataAddr + 12];
    assert(chunkVersion <= PATCH_CHUNK_VERSION /* Patch list chunk either corrupt or this verison of WL4Editor is old and doesn't support the saved format */);
    while(chunkVersion < PATCH_CHUNK_VERSION)
    {
        contents = UpgradePatchListContents(contents, chunkVersion++);
    }
}

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
    int patchListAddr = ROMUtils::FindChunkInROM(
        ROMUtils::CurrentFile,
        ROMUtils::CurrentFileSize,
        WL4Constants::AvailableSpaceBeginningInROM,
        ROMUtils::SaveDataChunkType::PatchListChunk
    );
    if(patchListAddr)
    {
        // Obtain the patch chunks
        QVector<int> patchChunks = ROMUtils::FindAllChunksInROM(
            ROMUtils::CurrentFile,
            ROMUtils::CurrentFileSize,
            WL4Constants::AvailableSpaceBeginningInROM,
            ROMUtils::SaveDataChunkType::PatchListChunk
        );

        // Get the patch list information
        QString contents = GetUpgradedPatchListChunkData(patchListAddr);
        assert(contents > 0 /* ROM contains an empty patch list chunk */);
        QStringList patchTuples = contents.split(";");
        assert(!(patchTuples.count() % 3) /* ROM contains a corrupted patch list chunk (field count is not a multiple of 3) */);
        for(int i = 0; i < patchTuples.count(); i += 3)
        {
            // Validate that the chunks shown in the patch list chunk are in the ROM
            int patchAddress = patchTuples[i + 3].toInt(Q_NULLPTR, 16);
            assert(patchChunks.contains(patchAddress) /* Patch chunk list refers to an invalid patch address */);

            // Add the patch entry
            int patchType = patchTuples[i + 1].toInt(Q_NULLPTR, 16);
            int hookAddress = patchTuples[i + 2].toInt(Q_NULLPTR, 16);
            struct PatchEntryItem entry
            {
                patchTuples[i],
                static_cast<enum PatchType>(patchType),
                hookAddress,
                patchAddress
            };
            patchEntries.append(entry);
        }
    }
    return patchEntries;
}
