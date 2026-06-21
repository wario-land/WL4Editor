#ifndef CHUNKUTILS_H
#define CHUNKUTILS_H

#include "ROMUtils.h"
#include <QMap>
#include <QVector>
#include <QString>

namespace ChunkUtils
{
    // ---- Data Types ----

    struct ChunkReference
    {
        enum ROMUtils::SaveDataChunkType ChunkType = ROMUtils::InvalidationChunk;
        unsigned int ParentChunkAddress = 0;  // 0 = root (pointed from main ROM)
        unsigned int ChunkAddress = 0;        // address of RATS tag (chunk header start)
        bool HeaderHasBroken = false;
        QVector<unsigned int> ChildrenChunkLocalOffset;
        QVector<unsigned int> BrokenChildrenChunkLocalOffset;

        // Count of distinct pointer sources referencing this chunk (for DuplicateRef detection)
        int ReferenceCount = 0;

        // Human-readable description of where this chunk comes from
        // e.g. "Tileset 5 — Map16 Data", "Passage 2, Stage 0, Room 1 — Layer 0"
        QString SourceDescription;
    };

    // Per-chunk issue flags (bitmask-compatible)
    enum ChunkIssue {
        NoIssue           = 0x00,
        Orphan            = 0x01,  // no reference in any pointer table
        BrokenHeader      = 0x02,  // bad RATS tag/checksum/type
        DuplicateRef      = 0x04,  // multiple pointers point to this chunk
        HasMisalignedPtr  = 0x08,  // some pointer targets inside this chunk's body (not at +12)
        Overlap           = 0x10,  // chunk's address range overlaps with another chunk
    };

    // Describes an overlap between two chunks
    struct OverlapIssue
    {
        unsigned int chunkA = 0;        // the earlier chunk (lower address)
        unsigned int chunkB = 0;        // the later chunk (higher address, overlaps A)
        unsigned int overlapStart = 0;  // first address where B intrudes into A
        unsigned int overlapEnd = 0;    // last address of A that B covers
    };

    // Describes a bad pointer found during scanning
    struct PointerIssue
    {
        unsigned int pointerSourceAddr = 0;   // ROM address where the bad pointer is stored
        unsigned int pointerTargetAddr = 0;   // the value the pointer holds
        unsigned int affectedChunkAddr = 0;   // the chunk being pointed into
        enum ROMUtils::SaveDataChunkType expectedChunkType = ROMUtils::InvalidationChunk;
        QString description;                  // human-readable
    };

    // Records a raw pointer observed during chunk reference enumeration.
    // Used for cross-checking misaligned pointers in ScanChunkIssues.
    struct RawPointerEntry
    {
        unsigned int sourceAddr;     // ROM address where this pointer is stored
        unsigned int targetAddr;     // the pointer value (points to chunk data, i.e. chunkAddr + 12)
        enum ROMUtils::SaveDataChunkType expectedChunkType;
    };

    // ---- Functions ----

    // Check if a RATS header at the given address is valid
    bool ValidateChunkHeader(const unsigned char *romData, unsigned int chunkAddr);

    // Build the full chunk reference map by walking all known pointer tables.
    // Also populates rawPointersOut with every pointer seen during enumeration.
    QMap<unsigned int, ChunkReference> GetAllChunkReferences(
        QVector<RawPointerEntry> *rawPointersOut = nullptr);

    // Scan all chunks and return per-chunk issue flags + any pointer-level issues
    QMap<unsigned int, int> ScanChunkIssues(
        const QVector<unsigned int> &allChunks,
        const QMap<unsigned int, ChunkReference> &refs,
        QVector<PointerIssue> &pointerIssuesOut,
        QVector<OverlapIssue> &overlapIssuesOut);

    // Get the number of unique issue types present across all chunks
    int CountAffectedChunks(const QMap<unsigned int, int> &chunkIssues);

} // namespace ChunkUtils

#endif // CHUNKUTILS_H
