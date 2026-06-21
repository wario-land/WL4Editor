#include "ChunkUtils.h"
#include "PatchUtils.h"
#include "AssortedGraphicUtils.h"
#include "WL4Constants.h"
#include "LevelComponents/AnimatedTile8x8Group.h"
#include "LevelComponents/Room.h"

#include <cstring>
#include <algorithm>

// ============================================================================
//  Internal helpers
// ============================================================================

/// <summary>
/// Validate a RATS tag at the given ROM address.
/// Checks "STAR" tag + chunkLen == ~chunkComp.
/// </summary>
static inline bool ValidRATS(const unsigned char *ptr)
{
    if (strncmp(reinterpret_cast<const char *>(ptr), "STAR", 4))
        return false;
    short chunkLen = *reinterpret_cast<const short *>(ptr + 4);
    short chunkComp = *reinterpret_cast<const short *>(ptr + 6);
    return chunkLen == ~chunkComp;
}

// ============================================================================
//  Public functions
// ============================================================================

namespace ChunkUtils
{

/// <summary>
/// Check if a RATS header at the given address is valid.
/// </summary>
bool ValidateChunkHeader(const unsigned char *romData, unsigned int chunkAddr)
{
    return ValidRATS(romData + chunkAddr);
}

// ============================================================================
//  Pointer table descriptor for simple table-driven enumeration
// ============================================================================

struct PointerTableDescriptor
{
    unsigned int baseAddr;       // ROM address of pointer table
    unsigned int count;          // number of entries
    unsigned int stride;         // bytes per entry (usually 4)
    unsigned int offsetInEntry;  // offset within entry to read pointer (usually 0)
    ROMUtils::SaveDataChunkType chunkType;
};

// ============================================================================
//  Domain-specific reference collection helpers
// ============================================================================

static void AddPatchReferences(
    QMap<unsigned int, ChunkUtils::ChunkReference> &refs,
    QVector<ChunkUtils::RawPointerEntry> *rawPointers)
{
    using namespace ROMUtils;

    // Find the PatchListChunk
    unsigned int patchListAddr = ROMUtils::FindChunkInROM(
        ROMFileMetadata->ROMDataPtr,
        ROMFileMetadata->Length,
        WL4Constants::AvailableSpaceBeginningInROM,
        PatchListChunk);

    if (patchListAddr)
    {
        ChunkReference patchListRef;
        patchListRef.ChunkType = PatchListChunk;
        patchListRef.ChunkAddress = patchListAddr;
        patchListRef.SourceDescription = "Patch List";
        refs[patchListAddr] = patchListRef;

        // Get individual patch entries
        QVector<struct PatchEntryItem> patches = PatchUtils::GetPatchesFromROM();
        for (const auto &patch : patches)
        {
            if (patch.PatchAddress >= WL4Constants::AvailableSpaceBeginningInROM)
            {
                ChunkReference patchRef;
                patchRef.ChunkType = PatchChunk;
                patchRef.ChunkAddress = patch.PatchAddress;
                patchRef.SourceDescription = QString("Patch: %1").arg(patch.FileName);
                refs[patch.PatchAddress] = patchRef;

                if (rawPointers)
                {
                    // The patch list chunk contains a pointer to this patch
                    rawPointers->append(RawPointerEntry{patchListAddr, patch.PatchAddress + 12, PatchChunk});
                }
            }
        }
    }
}

static void AddTilesetReferences(
    QMap<unsigned int, ChunkUtils::ChunkReference> &refs,
    QVector<ChunkUtils::RawPointerEntry> *rawPointers)
{
    using namespace ROMUtils;

    for (unsigned int tilesetid = 0; tilesetid < 92; ++tilesetid)
    {
        unsigned int tilesetPtr = WL4Constants::TilesetDataTable + tilesetid * 36;

        // FG GFX pointer (offset +0x00)
        unsigned int fgGFXptr = ROMUtils::PointerFromData(tilesetPtr);
        if (fgGFXptr >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            unsigned int chunkAddr = fgGFXptr - 12;
            ChunkReference ref;
            ref.ChunkType = TilesetForegroundTile8x8DataChunkType;
            ref.ChunkAddress = chunkAddr;
            ref.SourceDescription = QString("Tileset %1 — FG GFX").arg(tilesetid);
            refs[chunkAddr] = ref;
            refs[chunkAddr].ReferenceCount++;
            if (rawPointers)
                rawPointers->append(RawPointerEntry{tilesetPtr, fgGFXptr, TilesetForegroundTile8x8DataChunkType});
        }

        // Palette pointer (offset +0x08)
        unsigned int paletteAddr = ROMUtils::PointerFromData(tilesetPtr + 8);
        if (paletteAddr >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            unsigned int chunkAddr = paletteAddr - 12;
            ChunkReference ref;
            ref.ChunkType = TilesetPaletteDataChunkType;
            ref.ChunkAddress = chunkAddr;
            ref.SourceDescription = QString("Tileset %1 — Palette").arg(tilesetid);
            refs[chunkAddr] = ref;
            refs[chunkAddr].ReferenceCount++;
            if (rawPointers)
                rawPointers->append(RawPointerEntry{tilesetPtr + 8, paletteAddr, TilesetPaletteDataChunkType});
        }

        // Map16 pointer (offset +0x14)
        unsigned int map16ptr = ROMUtils::PointerFromData(tilesetPtr + 0x14);
        if (map16ptr >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            unsigned int chunkAddr = map16ptr - 12;
            ChunkReference ref;
            ref.ChunkType = TilesetMap16DataChunkType;
            ref.ChunkAddress = chunkAddr;
            ref.SourceDescription = QString("Tileset %1 — Map16 Data").arg(tilesetid);
            refs[chunkAddr] = ref;
            refs[chunkAddr].ReferenceCount++;
            if (rawPointers)
                rawPointers->append(RawPointerEntry{tilesetPtr + 0x14, map16ptr, TilesetMap16DataChunkType});
        }

        // Map16 terrain type ID table (offset +0x18)
        unsigned int terrainPtr = ROMUtils::PointerFromData(tilesetPtr + 24);
        if (terrainPtr >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            unsigned int chunkAddr = terrainPtr - 12;
            ChunkReference ref;
            ref.ChunkType = TilesetMap16TerrainChunkType;
            ref.ChunkAddress = chunkAddr;
            ref.SourceDescription = QString("Tileset %1 — Map16 Terrain").arg(tilesetid);
            refs[chunkAddr] = ref;
            refs[chunkAddr].ReferenceCount++;
            if (rawPointers)
                rawPointers->append(RawPointerEntry{tilesetPtr + 24, terrainPtr, TilesetMap16TerrainChunkType});
        }

        // Map16 event table (offset +0x1C)
        unsigned int eventPtr = ROMUtils::PointerFromData(tilesetPtr + 28);
        if (eventPtr >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            unsigned int chunkAddr = eventPtr - 12;
            ChunkReference ref;
            ref.ChunkType = TilesetMap16EventTableChunkType;
            ref.ChunkAddress = chunkAddr;
            ref.SourceDescription = QString("Tileset %1 — Map16 Events").arg(tilesetid);
            refs[chunkAddr] = ref;
            refs[chunkAddr].ReferenceCount++;
            if (rawPointers)
                rawPointers->append(RawPointerEntry{tilesetPtr + 28, eventPtr, TilesetMap16EventTableChunkType});
        }
    }
}

static void AddEntityReferences(
    QMap<unsigned int, ChunkUtils::ChunkReference> &refs,
    QVector<ChunkUtils::RawPointerEntry> *rawPointers)
{
    using namespace ROMUtils;

    // Entities 0x11 through 128 (entities 0x00-0x10 are basic sprites, use different tables)
    for (unsigned int entityid = 0x11; entityid < 129; ++entityid)
    {
        // Entity palette
        unsigned int paletteAddr = ROMUtils::PointerFromData(
            WL4Constants::EntityPalettePointerTable + 4 * (entityid - 0x10));
        if (paletteAddr >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            unsigned int chunkAddr = paletteAddr - 12;
            ChunkReference ref;
            ref.ChunkType = EntityPaletteDataChunkType;
            ref.ChunkAddress = chunkAddr;
            ref.SourceDescription = QString("Entity 0x%1 — Palette").arg(entityid, 0, 16);
            refs[chunkAddr] = ref;
            refs[chunkAddr].ReferenceCount++;
            if (rawPointers)
                rawPointers->append(RawPointerEntry{
                    WL4Constants::EntityPalettePointerTable + 4 * (entityid - 0x10),
                    paletteAddr, EntityPaletteDataChunkType});
        }

        // Entity tile data
        unsigned int tileDataAddr = ROMUtils::PointerFromData(
            WL4Constants::EntityTilesetPointerTable + 4 * (entityid - 0x10));
        if (tileDataAddr >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            unsigned int chunkAddr = tileDataAddr - 12;
            ChunkReference ref;
            ref.ChunkType = EntityTile8x8DataChunkType;
            ref.ChunkAddress = chunkAddr;
            ref.SourceDescription = QString("Entity 0x%1 — Tile Data").arg(entityid, 0, 16);
            refs[chunkAddr] = ref;
            refs[chunkAddr].ReferenceCount++;
            if (rawPointers)
                rawPointers->append(RawPointerEntry{
                    WL4Constants::EntityTilesetPointerTable + 4 * (entityid - 0x10),
                    tileDataAddr, EntityTile8x8DataChunkType});
        }
    }
}

static void AddEntitySetReferences(
    QMap<unsigned int, ChunkUtils::ChunkReference> &refs,
    QVector<ChunkUtils::RawPointerEntry> *rawPointers)
{
    using namespace ROMUtils;

    for (unsigned int entitysetid = 0; entitysetid < 90; ++entitysetid)
    {
        unsigned int entitysetPtr = ROMUtils::PointerFromData(
            WL4Constants::EntitySetInfoPointerTable + entitysetid * 4);
        if (entitysetPtr >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            unsigned int chunkAddr = entitysetPtr - 12;
            ChunkReference ref;
            ref.ChunkType = EntitySetLoadTableChunkType;
            ref.ChunkAddress = chunkAddr;
            ref.SourceDescription = QString("EntitySet %1").arg(entitysetid);
            refs[chunkAddr] = ref;
            refs[chunkAddr].ReferenceCount++;
            if (rawPointers)
                rawPointers->append(RawPointerEntry{
                    WL4Constants::EntitySetInfoPointerTable + entitysetid * 4,
                    entitysetPtr, EntitySetLoadTableChunkType});
        }
    }
}

static void AddLevelReferences(
    QMap<unsigned int, ChunkUtils::ChunkReference> &refs,
    QVector<ChunkUtils::RawPointerEntry> *rawPointers)
{
    using namespace ROMUtils;
    unsigned char *ROMData = ROMFileMetadata->ROMDataPtr;

    for (unsigned int passageNum = 0; passageNum < 6; ++passageNum)
    {
        for (unsigned int stageNum = 0; stageNum < 5; ++stageNum)
        {
            // Skip stages which don't exist
            if (passageNum == 5)
            {
                switch (stageNum)
                {
                case 1:
                case 2:
                case 3:
                    continue;
                }
            }
            else if (!passageNum)
            {
                switch (stageNum)
                {
                case 1:
                case 3:
                    continue;
                }
            }

            // Get level header
            unsigned int offset = WL4Constants::LevelHeaderIndexTable + passageNum * 24 + stageNum * 4;
            unsigned int levelHeaderIndex = ROMUtils::IntFromData(offset);
            unsigned int levelHeaderAddr = WL4Constants::LevelHeaderTable + levelHeaderIndex * 12;
            unsigned int LevelID = ROMData[levelHeaderAddr];
            unsigned int roomCount = ROMData[levelHeaderAddr + 1];

            // Level names
            unsigned int LevelNameAddr = ROMUtils::PointerFromData(
                WL4Constants::LevelNamePointerTable + passageNum * 24 + stageNum * 4);
            if (LevelNameAddr >= WL4Constants::AvailableSpaceBeginningInROM)
            {
                unsigned int chunkAddr = LevelNameAddr - 12;
                ChunkReference ref;
                ref.ChunkType = LevelNameChunkType;
                ref.ChunkAddress = chunkAddr;
                ref.SourceDescription = QString("Passage %1, Stage %2 — Level Name [EN]")
                    .arg(passageNum).arg(stageNum);
                refs[chunkAddr] = ref;
                refs[chunkAddr].ReferenceCount++;
                if (rawPointers)
                    rawPointers->append(RawPointerEntry{
                        WL4Constants::LevelNamePointerTable + passageNum * 24 + stageNum * 4,
                        LevelNameAddr, LevelNameChunkType});
            }

            unsigned int LevelNameJAddr = ROMUtils::PointerFromData(
                WL4Constants::LevelNameJPointerTable + passageNum * 24 + stageNum * 4);
            if (LevelNameJAddr >= WL4Constants::AvailableSpaceBeginningInROM)
            {
                unsigned int chunkAddr = LevelNameJAddr - 12;
                ChunkReference ref;
                ref.ChunkType = LevelNameChunkType;
                ref.ChunkAddress = chunkAddr;
                ref.SourceDescription = QString("Passage %1, Stage %2 — Level Name [JA]")
                    .arg(passageNum).arg(stageNum);
                refs[chunkAddr] = ref;
                refs[chunkAddr].ReferenceCount++;
                if (rawPointers)
                    rawPointers->append(RawPointerEntry{
                        WL4Constants::LevelNameJPointerTable + passageNum * 24 + stageNum * 4,
                        LevelNameJAddr, LevelNameChunkType});
            }

            // Door table
            unsigned int doorTableAddress = ROMUtils::PointerFromData(
                WL4Constants::DoorTable + LevelID * 4);
            if (doorTableAddress >= WL4Constants::AvailableSpaceBeginningInROM)
            {
                unsigned int chunkAddr = doorTableAddress - 12;
                ChunkReference ref;
                ref.ChunkType = DoorChunkType;
                ref.ChunkAddress = chunkAddr;
                ref.SourceDescription = QString("Passage %1, Stage %2 — Door Table")
                    .arg(passageNum).arg(stageNum);
                refs[chunkAddr] = ref;
                refs[chunkAddr].ReferenceCount++;
                if (rawPointers)
                    rawPointers->append(RawPointerEntry{
                        WL4Constants::DoorTable + LevelID * 4,
                        doorTableAddress, DoorChunkType});
            }

            // Room header table
            unsigned int roomTableAddress = ROMUtils::PointerFromData(
                WL4Constants::RoomDataTable + LevelID * 4);
            if (roomTableAddress >= WL4Constants::AvailableSpaceBeginningInROM)
            {
                unsigned int chunkAddr = roomTableAddress - 12;
                ChunkReference roomRef;
                roomRef.ChunkType = RoomHeaderChunkType;
                roomRef.ChunkAddress = chunkAddr;
                roomRef.SourceDescription = QString("Passage %1, Stage %2 — Room Header")
                    .arg(passageNum).arg(stageNum);

                // Track camera limitator count for this level
                int cameraLimitatorRooms = 0;

                for (unsigned int roomId = 0; roomId < roomCount; ++roomId)
                {
                    unsigned int roomHeaderOffset = roomId * sizeof(LevelComponents::__RoomHeader);
                    unsigned int roomDataPtr = roomTableAddress + roomHeaderOffset;

                    // Record children chunk offsets within the room header
                    roomRef.ChildrenChunkLocalOffset
                        << (roomHeaderOffset + 0x08)  // Layer0Data
                        << (roomHeaderOffset + 0x0C)  // Layer1Data
                        << (roomHeaderOffset + 0x10)  // Layer2Data
                        << (roomHeaderOffset + 0x14)  // Layer3Data
                        << (roomHeaderOffset + 0x1C)  // EntityTableHard
                        << (roomHeaderOffset + 0x20)  // EntityTableNormal
                        << (roomHeaderOffset + 0x24); // EntityTableSHard

                    // Check for camera control
                    if (ROMData[roomDataPtr + 24] == LevelComponents::HasControlAttrs)
                    {
                        cameraLimitatorRooms++;
                    }

                    // Layer chunks
                    for (unsigned int layerNum = 0; layerNum < 4; ++layerNum)
                    {
                        unsigned int layerPtr = ROMUtils::PointerFromData(
                            roomDataPtr + layerNum * 4 + 8);
                        if (layerPtr >= WL4Constants::AvailableSpaceBeginningInROM)
                        {
                            unsigned int layerChunkAddr = layerPtr - 12;
                            ChunkReference layerRef;
                            layerRef.ChunkType = LayerChunkType;
                            layerRef.ParentChunkAddress = chunkAddr;
                            layerRef.ChunkAddress = layerChunkAddr;
                            layerRef.SourceDescription = QString("Passage %1, Stage %2, Room %3 — Layer %4")
                                .arg(passageNum).arg(stageNum).arg(roomId).arg(layerNum);
                            refs[layerChunkAddr] = layerRef;
                            refs[layerChunkAddr].ReferenceCount++;
                            if (rawPointers)
                                rawPointers->append(RawPointerEntry{
                                    roomDataPtr + layerNum * 4 + 8,
                                    layerPtr, LayerChunkType});
                        }
                    }

                    // Entity list chunks
                    const char *entityListModeNames[3] = {"Hard", "Normal", "SHard"};
                    for (unsigned int entityListNum = 0; entityListNum < 3; ++entityListNum)
                    {
                        unsigned int listAddress = ROMUtils::PointerFromData(
                            roomDataPtr + 28 + 4 * entityListNum);
                        if (listAddress >= WL4Constants::AvailableSpaceBeginningInROM)
                        {
                            unsigned int listChunkAddr = listAddress - 12;
                            ChunkReference listRef;
                            listRef.ChunkType = EntityListChunk;
                            listRef.ParentChunkAddress = chunkAddr;
                            listRef.ChunkAddress = listChunkAddr;
                            listRef.SourceDescription = QString("Passage %1, Stage %2, Room %3 — Entity List [%4]")
                                .arg(passageNum).arg(stageNum).arg(roomId)
                                .arg(entityListModeNames[entityListNum]);
                            refs[listChunkAddr] = listRef;
                            refs[listChunkAddr].ReferenceCount++;
                            if (rawPointers)
                                rawPointers->append(RawPointerEntry{
                                    roomDataPtr + 28 + 4 * entityListNum,
                                    listAddress, EntityListChunk});
                        }
                    }
                }

                refs[chunkAddr] = roomRef;
                refs[chunkAddr].ReferenceCount++;

                if (rawPointers)
                    rawPointers->append(RawPointerEntry{
                        WL4Constants::RoomDataTable + LevelID * 4,
                        roomTableAddress, RoomHeaderChunkType});

                // Camera control chunks
                if (cameraLimitatorRooms)
                {
                    unsigned int cameraPtrTableAddr = ROMUtils::PointerFromData(
                        WL4Constants::CameraControlPointerTable + LevelID * 4);
                    if (cameraPtrTableAddr >= WL4Constants::AvailableSpaceBeginningInROM)
                    {
                        unsigned int camChunkAddr = cameraPtrTableAddr - 12;
                        ChunkReference camRef;
                        camRef.ChunkType = CameraPointerTableType;
                        camRef.ChunkAddress = camChunkAddr;
                        camRef.SourceDescription = QString("Passage %1, Stage %2 — Camera Pointer Table")
                            .arg(passageNum).arg(stageNum);
                        refs[camChunkAddr] = camRef;
                        refs[camChunkAddr].ReferenceCount++;

                        if (rawPointers)
                            rawPointers->append(RawPointerEntry{
                                WL4Constants::CameraControlPointerTable + LevelID * 4,
                                cameraPtrTableAddr, CameraPointerTableType});

                        // Camera boundary entries
                        for (int cameraEntry = 0; cameraEntry < cameraLimitatorRooms; ++cameraEntry)
                        {
                            refs[camChunkAddr].ChildrenChunkLocalOffset << (4 * cameraEntry);

                            unsigned int cameraEntryAddress = ROMUtils::PointerFromData(
                                cameraPtrTableAddr + cameraEntry * 4);
                            if (cameraEntryAddress >= WL4Constants::AvailableSpaceBeginningInROM)
                            {
                                unsigned int boundaryChunkAddr = cameraEntryAddress - 12;
                                ChunkReference boundaryRef;
                                boundaryRef.ChunkType = CameraBoundaryChunkType;
                                boundaryRef.ParentChunkAddress = camChunkAddr;
                                boundaryRef.ChunkAddress = boundaryChunkAddr;
                                boundaryRef.SourceDescription = QString("Passage %1, Stage %2 — Camera Boundary %3")
                                    .arg(passageNum).arg(stageNum).arg(cameraEntry);
                                refs[boundaryChunkAddr] = boundaryRef;
                                refs[boundaryChunkAddr].ReferenceCount++;
                                if (rawPointers)
                                    rawPointers->append(RawPointerEntry{
                                        cameraPtrTableAddr + cameraEntry * 4,
                                        cameraEntryAddress, CameraBoundaryChunkType});
                            }
                        }
                    }
                }
            }
        }
    }
}

static void AddAssortedGraphicReferences(
    QMap<unsigned int, ChunkUtils::ChunkReference> &refs,
    QVector<ChunkUtils::RawPointerEntry> *rawPointers)
{
    using namespace ROMUtils;

    // Assorted graphic entries
    QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> entries =
        AssortedGraphicUtils::GetAssortedGraphicsFromROM();

    int entryIdx = 0;
    for (const auto &entry : entries)
    {
        // Palette chunk
        if (entry.PaletteAddress >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            unsigned int chunkAddr = entry.PaletteAddress;
            ChunkReference ref;
            ref.ChunkType = AssortedGraphicPaletteChunkType;
            ref.ChunkAddress = chunkAddr;
            ref.SourceDescription = QString("Assorted Graphic %1 — Palette").arg(entryIdx);
            refs[chunkAddr] = ref;
            refs[chunkAddr].ReferenceCount++;
            if (rawPointers)
                rawPointers->append(RawPointerEntry{0, entry.PaletteAddress, AssortedGraphicPaletteChunkType});
        }

        // Tile8x8 data chunk
        if (entry.TileDataAddress >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            unsigned int chunkAddr = entry.TileDataAddress;
            ChunkReference ref;
            ref.ChunkType = AssortedGraphicTile8x8DataChunkType;
            ref.ChunkAddress = chunkAddr;
            ref.SourceDescription = QString("Assorted Graphic %1 — Tile Data").arg(entryIdx);
            refs[chunkAddr] = ref;
            refs[chunkAddr].ReferenceCount++;
            if (rawPointers)
                rawPointers->append(RawPointerEntry{0, entry.TileDataAddress, AssortedGraphicTile8x8DataChunkType});
        }

        // Mapping data chunk
        if (entry.MappingDataAddress >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            unsigned int chunkAddr = entry.MappingDataAddress;
            ChunkReference ref;
            ref.ChunkType = AssortedGraphicmappingChunkType;
            ref.ChunkAddress = chunkAddr;
            ref.SourceDescription = QString("Assorted Graphic %1 — Mapping Data").arg(entryIdx);
            refs[chunkAddr] = ref;
            refs[chunkAddr].ReferenceCount++;
            if (rawPointers)
                rawPointers->append(RawPointerEntry{0, entry.MappingDataAddress, AssortedGraphicmappingChunkType});
        }
        ++entryIdx;
    }

    // AssortedGraphicListChunkType — the list metadata itself
    unsigned int listChunkAddr = ROMUtils::FindChunkInROM(
        ROMFileMetadata->ROMDataPtr,
        ROMFileMetadata->Length,
        WL4Constants::AvailableSpaceBeginningInROM,
        AssortedGraphicListChunkType);
    if (listChunkAddr)
    {
        ChunkReference listRef;
        listRef.ChunkType = AssortedGraphicListChunkType;
        listRef.ChunkAddress = listChunkAddr;
        listRef.SourceDescription = "Assorted Graphics — List Metadata";
        refs[listChunkAddr] = listRef;
    }
}

static void AddAnimatedTileGroupReferences(
    QMap<unsigned int, ChunkUtils::ChunkReference> &refs,
    QVector<ChunkUtils::RawPointerEntry> *rawPointers)
{
    using namespace ROMUtils;

    for (int i = 0; i < 270; ++i)
    {
        // Each slot is 8 bytes: type(1), countPerFrame(1), totalFrameCount(1), unused(1), tiledataptr(4)
        unsigned int headerAddr = WL4Constants::AnimatedTileHeaderTable + i * 8;
        unsigned int tiledataPtr = ROMUtils::PointerFromData(headerAddr + 4);

        if (tiledataPtr >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            unsigned int chunkAddr = tiledataPtr - 12;
            ChunkReference ref;
            ref.ChunkType = AnimatedTileGroupTile8x8DataChunkType;
            ref.ChunkAddress = chunkAddr;
            ref.SourceDescription = QString("Animated Tile Group %1").arg(i);
            refs[chunkAddr] = ref;
            refs[chunkAddr].ReferenceCount++;
            if (rawPointers)
                rawPointers->append(RawPointerEntry{
                    headerAddr + 4, tiledataPtr,
                    AnimatedTileGroupTile8x8DataChunkType});
        }
    }
}

// ============================================================================
//  GetAllChunkReferences
// ============================================================================

QMap<unsigned int, ChunkReference> GetAllChunkReferences(
    QVector<RawPointerEntry> *rawPointersOut)
{
    using namespace ROMUtils;
    QMap<unsigned int, ChunkReference> refs;

    // Collect references from all domains
    AddPatchReferences(refs, rawPointersOut);
    AddTilesetReferences(refs, rawPointersOut);
    AddEntityReferences(refs, rawPointersOut);
    AddEntitySetReferences(refs, rawPointersOut);
    AddLevelReferences(refs, rawPointersOut);
    AddAssortedGraphicReferences(refs, rawPointersOut);
    AddAnimatedTileGroupReferences(refs, rawPointersOut);

    // Post-process: remove vanilla ROM references, validate headers
    for (auto key : refs.keys())
    {
        auto &reference = refs[key];

        // Remove references in vanilla ROM
        if (key < WL4Constants::AvailableSpaceBeginningInROM)
        {
            refs.remove(key);
            continue;
        }

        // Clear parent address if parent is in vanilla ROM
        if (reference.ParentChunkAddress &&
            reference.ParentChunkAddress < WL4Constants::AvailableSpaceBeginningInROM)
        {
            reference.ParentChunkAddress = 0;
        }

        // Remove children whose pointers are in vanilla ROM
        auto &children = reference.ChildrenChunkLocalOffset;
        for (int i = children.size() - 1; i >= 0; --i)
        {
            unsigned int childPtr = ROMUtils::PointerFromData(key + 12 + children[i]);
            if (childPtr < WL4Constants::AvailableSpaceBeginningInROM)
            {
                children.remove(i);
            }
        }

        // Sort child offset references by pointer value
        std::sort(children.begin(), children.end(),
            [key](const unsigned int &a, const unsigned int &b)
            {
                unsigned int ptrA = ROMUtils::PointerFromData(key + 12 + a);
                unsigned int ptrB = ROMUtils::PointerFromData(key + 12 + b);
                return ptrA < ptrB;
            });

        // Validate RATS tag and chunk type
        if (!ValidRATS(ROMFileMetadata->ROMDataPtr + reference.ChunkAddress) ||
            (ROMFileMetadata->ROMDataPtr[reference.ChunkAddress + 8] != reference.ChunkType))
        {
            reference.HeaderHasBroken = true;
        }

        // Validate children's RATS tags
        for (int childIdx = 0; childIdx < reference.ChildrenChunkLocalOffset.size(); ++childIdx)
        {
            unsigned int childOffset = reference.ChildrenChunkLocalOffset[childIdx];
            unsigned int absOffset = key + 12 + childOffset;
            unsigned int childPtr = ROMUtils::PointerFromData(absOffset);
            if (childPtr && !ValidRATS(ROMFileMetadata->ROMDataPtr + childPtr - 12))
            {
                reference.BrokenChildrenChunkLocalOffset << childOffset;
            }
        }
    }

    return refs;
}

// ============================================================================
//  ScanChunkIssues
// ============================================================================

QMap<unsigned int, int> ScanChunkIssues(
    const QVector<unsigned int> &allChunks,
    const QMap<unsigned int, ChunkReference> &refs,
    QVector<PointerIssue> &pointerIssuesOut,
    QVector<OverlapIssue> &overlapIssuesOut)
{
    using namespace ROMUtils;
    QMap<unsigned int, int> chunkIssues;

    // Build a set of all raw pointers for misaligned pointer detection.
    QVector<RawPointerEntry> allRawPointers;
    GetAllChunkReferences(&allRawPointers);

    // For each chunk, check all issue types
    for (unsigned int chunkAddr : allChunks)
    {
        int issues = NoIssue;

        // 1. Orphan check: chunk not in references
        if (!refs.contains(chunkAddr))
        {
            issues |= Orphan;
        }

        // 2. Broken header check
        if (!ValidRATS(ROMFileMetadata->ROMDataPtr + chunkAddr))
        {
            issues |= BrokenHeader;
        }
        else
        {
            // Also check if the chunk type byte is known
            unsigned char chunkType = ROMFileMetadata->ROMDataPtr[chunkAddr + 8];
            if (chunkType >= CHUNK_TYPE_COUNT)
            {
                issues |= BrokenHeader;
            }
        }

        // 3. Duplicate reference check
        if (refs.contains(chunkAddr))
        {
            const auto &ref = refs[chunkAddr];
            if (ref.ReferenceCount > 1)
            {
                issues |= DuplicateRef;
            }
        }

        // 4. Misaligned pointer check
        unsigned int chunkSize = ROMUtils::GetChunkDataLength(chunkAddr);
        unsigned int dataStart = chunkAddr + 12;
        unsigned int dataEnd = chunkAddr + 12 + chunkSize;

        for (const auto &rp : allRawPointers)
        {
            if (rp.targetAddr > dataStart &&
                rp.targetAddr < dataEnd)
            {
                issues |= HasMisalignedPtr;

                PointerIssue pi;
                pi.pointerSourceAddr = rp.sourceAddr;
                pi.pointerTargetAddr = rp.targetAddr;
                pi.affectedChunkAddr = chunkAddr;
                pi.expectedChunkType = rp.expectedChunkType;
                pi.description = QString(
                    "Misaligned pointer at ROM 0x%1 points to 0x%2 (offset +%3 inside chunk 0x%4, expected 0x%5)")
                    .arg(rp.sourceAddr, 0, 16)
                    .arg(rp.targetAddr, 0, 16)
                    .arg(rp.targetAddr - dataStart)
                    .arg(chunkAddr, 0, 16)
                    .arg(dataStart, 0, 16);

                pointerIssuesOut.append(pi);
            }
        }

        if (issues)
        {
            chunkIssues[chunkAddr] = issues;
        }
    }

    // 5. Overlap detection: check adjacent chunks for address range collisions.
    // allChunks is sorted by address from FindAllChunksInROM.
    // chunk A ends at A.addr + 12 + A.size; chunk B starts at B.addr.
    // Overlap if A.end > B.addr.
    for (int i = 0; i < allChunks.size() - 1; ++i)
    {
        unsigned int addrA = allChunks[i];
        unsigned int addrB = allChunks[i + 1];
        unsigned int sizeA = ROMUtils::GetChunkDataLength(addrA);
        unsigned int endA = addrA + 12 + sizeA;

        if (endA > addrB)
        {
            // Chunk B overlaps the tail of chunk A
            OverlapIssue oi;
            oi.chunkA = addrA;
            oi.chunkB = addrB;
            oi.overlapStart = addrB;
            oi.overlapEnd = endA;
            overlapIssuesOut.append(oi);

            // Flag both chunks
            int &issuesA = chunkIssues[addrA];
            issuesA |= Overlap;
            int &issuesB = chunkIssues[addrB];
            issuesB |= Overlap;
        }
    }

    return chunkIssues;
}

// ============================================================================
//  CountAffectedChunks
// ============================================================================

int CountAffectedChunks(const QMap<unsigned int, int> &chunkIssues)
{
    return chunkIssues.size();
}

} // namespace ChunkUtils
