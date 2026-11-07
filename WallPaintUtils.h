#ifndef WALLPAINTUTILS_H
#define WALLPAINTUTILS_H

#include <vector>

// Wall paint data is stored once per Level in fixed and pointer-referenced ROM
// blocks. A Level is identified by its passage index and its local level index
// (4 is the boss level). The colored passage palette is the primary palette;
// the gray, start-level gradient and MMAP palettes are derived from it.
namespace WallPaintUtils
{
    const int PassageCount = 6;
    const int LevelCount = 5;
    const int GFXColumnCount = 5;
    const int GFXColumnSize = 5 * 32;                   // 5 tiles of 32 bytes
    const int GFXColumnStride = 1024;                   // distance between two columns
    const int GFXSize = GFXColumnCount * GFXColumnSize; // 800 bytes
    const int PaletteSize = 32;                         // 16 colors, 2 bytes each
    const int StartLevelPaletteSize = 8 * PaletteSize;  // 8 gradient palettes

    struct LevelData
    {
        unsigned char gfx[GFXSize];
        unsigned char passageColor[PaletteSize];
        // The palettes below are derived from passageColor. They are kept here
        // so Undo/Redo can restore the exact blocks which were written.
        unsigned char passageGray[PaletteSize];
        unsigned char mmapColor[PaletteSize];
        bool hasMMapColor;
        unsigned char startLevelPalette[StartLevelPaletteSize];
        bool hasStartLevelPalette;
    };

    // A single ROM block which belongs to one Level and must be restored by
    // Undo/Redo. The data pointers stay owned by the caller.
    struct BlockContent
    {
        unsigned int address;
        unsigned int size;
        const unsigned char *oldData;
        const unsigned char *newData;
    };

    // Addresses of all blocks of one Level. Optional blocks are 0 when the
    // Level has no such block (for example a boss level has no gradient palette).
    struct BlockLayout
    {
        unsigned int gfxColumnAddresses[GFXColumnCount];
        unsigned int passageColorAddress;
        unsigned int passageGrayAddress;
        unsigned int mmapColorAddress;
        unsigned int startLevelPaletteAddress;
    };

    bool IsValidLevel(int passageIndex, int levelIndex);

    // Read the complete wall paint data of one Level from the loaded ROM.
    bool ReadLevelData(int passageIndex, int levelIndex, LevelData &data);

    // Generate the gray, start-level gradient and MMAP palettes from the
    // passage color palette, following the palette import logic of
    // WallPaintEditorDialog. The first six colors of the color, gray and
    // gradient palettes are kept unchanged, because the editor treats them as
    // fixed colors.
    void GenerateDerivedPalettes(LevelData &data, const LevelData &oldData);

    // Compare all data which BuildBlocks() would write. Optional blocks are
    // ignored when `first` does not contain them.
    bool LevelDataEquals(const LevelData &first, const LevelData &second);

    BlockLayout GetBlockLayout(int passageIndex, int levelIndex);

    // Build the list of ROM blocks for one Level. Only blocks which are present
    // in both data sets are returned, so Undo/Redo never writes an absent block.
    std::vector<BlockContent> BuildBlocks(int passageIndex, int levelIndex,
                                          const LevelData &oldData,
                                          const LevelData &newData);
}

#endif // WALLPAINTUTILS_H
