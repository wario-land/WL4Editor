#include "WallPaintUtils.h"

#include "ROMUtils.h"
#include "WL4Constants.h"

#include <QColor>
#include <QVector>

#include <cstring>

namespace WallPaintUtils
{
    namespace
    {
        bool IsRangeValid(unsigned int address, unsigned int size)
        {
            if (!ROMUtils::ROMFileMetadata || !ROMUtils::ROMFileMetadata->ROMDataPtr
                    || !address || !size)
            {
                return false;
            }
            const unsigned int romSize = ROMUtils::ROMFileMetadata->Length;
            return (address <= romSize) && (size <= romSize - address);
        }

        unsigned int ReadPointerIfValid(unsigned int address)
        {
            if (!IsRangeValid(address, 4))
            {
                return 0;
            }
            return ROMUtils::PointerFromData(static_cast<int>(address));
        }
    }

    bool IsValidLevel(int passageIndex, int levelIndex)
    {
        return (passageIndex >= 0) && (passageIndex < PassageCount)
                && (levelIndex >= 0) && (levelIndex < LevelCount);
    }

    BlockLayout GetBlockLayout(int passageIndex, int levelIndex)
    {
        BlockLayout layout = {};

        if (!IsValidLevel(passageIndex, levelIndex))
        {
            return layout;
        }

        // The GFX of one Level consists of five columns. Each column is 5 tiles
        // high and the columns are 1024 bytes apart in the ROM.
        const unsigned int gfxBase =
            WL4Constants::WallPaintGFXAddr
            + static_cast<unsigned int>(passageIndex) * (GFXColumnStride * GFXColumnCount)
            + static_cast<unsigned int>(levelIndex) * GFXColumnSize;
        for (int column = 0; column < GFXColumnCount; ++column)
        {
            layout.gfxColumnAddresses[column] =
                gfxBase + static_cast<unsigned int>(column) * GFXColumnStride;
        }

        // The passage color and gray palettes are stored in fixed tables which
        // contain one entry per passage/local-level pair.
        const unsigned int paletteOffset =
            static_cast<unsigned int>(passageIndex * LevelCount + levelIndex) * PaletteSize;
        layout.passageColorAddress = WL4Constants::WallPaintPalPassageColor + paletteOffset;
        layout.passageGrayAddress = WL4Constants::WallPaintPalPassageGray + paletteOffset;

        // The MMAP palette is referenced by a pointer table and has one palette
        // per passage/local-level pair. Being able to address the unused slots
        // keeps the door open for the expandable Entry/Golden Passage slots.
        const unsigned int mmapTableAddress =
            WL4Constants::WallPaintPalSixInOneMMapColorPtrTable
            + static_cast<unsigned int>(passageIndex) * 4;
        const unsigned int mmapBase = ReadPointerIfValid(mmapTableAddress);
        if (mmapBase)
        {
            layout.mmapColorAddress =
                mmapBase + static_cast<unsigned int>(0xA + levelIndex) * PaletteSize;
        }

        // Only regular levels have a start-level gradient palette. A boss level
        // (local level 4) does not have a valid pointer in this table.
        if (levelIndex < 4)
        {
            const unsigned int startLevelTableAddress =
                WL4Constants::WallPaintPalStartLevelPointerTable
                + static_cast<unsigned int>(passageIndex) * 16
                + static_cast<unsigned int>(levelIndex) * 4;
            layout.startLevelPaletteAddress = ReadPointerIfValid(startLevelTableAddress);
        }

        return layout;
    }

    bool ReadLevelData(int passageIndex, int levelIndex, LevelData &data)
    {
        if (!IsValidLevel(passageIndex, levelIndex))
        {
            return false;
        }

        const BlockLayout layout = GetBlockLayout(passageIndex, levelIndex);
        for (int column = 0; column < GFXColumnCount; ++column)
        {
            if (!IsRangeValid(layout.gfxColumnAddresses[column], GFXColumnSize))
            {
                return false;
            }
        }
        if (!IsRangeValid(layout.passageColorAddress, PaletteSize)
                || !IsRangeValid(layout.passageGrayAddress, PaletteSize))
        {
            return false;
        }

        memset(&data, 0, sizeof(data));
        const unsigned char *romData = ROMUtils::ROMFileMetadata->ROMDataPtr;

        for (int column = 0; column < GFXColumnCount; ++column)
        {
            memcpy(data.gfx + column * GFXColumnSize,
                   romData + layout.gfxColumnAddresses[column], GFXColumnSize);
        }
        memcpy(data.passageColor, romData + layout.passageColorAddress, PaletteSize);
        memcpy(data.passageGray, romData + layout.passageGrayAddress, PaletteSize);

        if (IsRangeValid(layout.mmapColorAddress, PaletteSize))
        {
            data.hasMMapColor = true;
            memcpy(data.mmapColor, romData + layout.mmapColorAddress, PaletteSize);
        }
        if (IsRangeValid(layout.startLevelPaletteAddress, StartLevelPaletteSize))
        {
            data.hasStartLevelPalette = true;
            memcpy(data.startLevelPalette, romData + layout.startLevelPaletteAddress,
                   StartLevelPaletteSize);
        }

        return true;
    }

    void GenerateDerivedPalettes(LevelData &data, const LevelData &oldData)
    {
        // The generated blocks exist only when the target Level has them.
        data.hasStartLevelPalette = oldData.hasStartLevelPalette;
        data.hasMMapColor = oldData.hasMMapColor;

        // The Wall Paint editor keeps the first six colors of the palette
        // unchanged, so keep them from the old data.
        memcpy(data.passageColor, oldData.passageColor, 6 * 2);

        unsigned short colorData[16];
        memcpy(colorData, data.passageColor, PaletteSize);
        QVector<QRgb> colors;
        ROMUtils::LoadPalette(&colors, colorData, true);

        // The gray palette is generated from the passage color palette.
        unsigned short grayData[16];
        memcpy(grayData, oldData.passageGray, PaletteSize);
        QRgb grayColors[16];
        for (int j = 6; j < 16; ++j)
        {
            grayColors[j] = ROMUtils::QRgbGrayScale(colors[j]);
            grayData[j] = ROMUtils::QRgbToData(grayColors[j]);
        }
        memcpy(data.passageGray, grayData, PaletteSize);

        // The eight start-level gradient palettes interpolate from the gray
        // palette (index 0) to the passage color palette (index 7).
        if (data.hasStartLevelPalette)
        {
            unsigned short gradientData[8][16];
            memcpy(gradientData, oldData.startLevelPalette, StartLevelPaletteSize);
            for (int j = 6; j < 16; ++j)
            {
                const QColor startColor = QColor::fromRgb(colors[j]);
                const QColor endColor = QColor::fromRgb(grayColors[j]);
                for (int p = 0; p < 8; ++p)
                {
                    const double factor = (7 - p) / 7.0;
                    int red = startColor.red()
                        + static_cast<int>((endColor.red() - startColor.red()) * factor);
                    int green = startColor.green()
                        + static_cast<int>((endColor.green() - startColor.green()) * factor);
                    int blue = startColor.blue()
                        + static_cast<int>((endColor.blue() - startColor.blue()) * factor);
                    if (red < 0) red = 0;
                    if (red > 255) red = 255;
                    if (green < 0) green = 0;
                    if (green > 255) green = 255;
                    if (blue < 0) blue = 0;
                    if (blue > 255) blue = 255;
                    gradientData[p][j] = ROMUtils::QRgbToData(qRgb(red, green, blue));
                }
            }
            memcpy(data.startLevelPalette, gradientData, StartLevelPaletteSize);
        }

        // The MMAP palette is a direct copy of the passage color palette.
        if (data.hasMMapColor)
        {
            memcpy(data.mmapColor, data.passageColor, PaletteSize);
        }
    }

    bool LevelDataEquals(const LevelData &first, const LevelData &second)
    {
        if (memcmp(first.gfx, second.gfx, GFXSize) != 0)
        {
            return false;
        }
        if (memcmp(first.passageColor, second.passageColor, PaletteSize) != 0)
        {
            return false;
        }
        if (memcmp(first.passageGray, second.passageGray, PaletteSize) != 0)
        {
            return false;
        }
        // When the new data does not include the MMAP palette or the start-level
        // palette, keep the existing blocks untouched and do not treat them as a
        // difference. This makes partial imports idempotent.
        if (first.hasMMapColor)
        {
            if (!second.hasMMapColor)
            {
                return false;
            }
            if (memcmp(first.mmapColor, second.mmapColor, PaletteSize) != 0)
            {
                return false;
            }
        }
        if (first.hasStartLevelPalette)
        {
            if (!second.hasStartLevelPalette)
            {
                return false;
            }
            if (memcmp(first.startLevelPalette, second.startLevelPalette,
                       StartLevelPaletteSize) != 0)
            {
                return false;
            }
        }

        return true;
    }

    std::vector<BlockContent> BuildBlocks(int passageIndex, int levelIndex,
                                          const LevelData &oldData,
                                          const LevelData &newData)
    {
        std::vector<BlockContent> blocks;
        if (!IsValidLevel(passageIndex, levelIndex))
        {
            return blocks;
        }

        const BlockLayout layout = GetBlockLayout(passageIndex, levelIndex);

        for (int column = 0; column < GFXColumnCount; ++column)
        {
            if (!layout.gfxColumnAddresses[column])
            {
                continue;
            }
            BlockContent block;
            block.address = layout.gfxColumnAddresses[column];
            block.size = GFXColumnSize;
            block.oldData = oldData.gfx + column * GFXColumnSize;
            block.newData = newData.gfx + column * GFXColumnSize;
            blocks.push_back(block);
        }

        if (layout.passageColorAddress)
        {
            BlockContent passageColorBlock;
            passageColorBlock.address = layout.passageColorAddress;
            passageColorBlock.size = PaletteSize;
            passageColorBlock.oldData = oldData.passageColor;
            passageColorBlock.newData = newData.passageColor;
            blocks.push_back(passageColorBlock);
        }

        if (layout.passageGrayAddress)
        {
            BlockContent passageGrayBlock;
            passageGrayBlock.address = layout.passageGrayAddress;
            passageGrayBlock.size = PaletteSize;
            passageGrayBlock.oldData = oldData.passageGray;
            passageGrayBlock.newData = newData.passageGray;
            blocks.push_back(passageGrayBlock);
        }

        if (newData.hasMMapColor && oldData.hasMMapColor && layout.mmapColorAddress)
        {
            BlockContent mmapBlock;
            mmapBlock.address = layout.mmapColorAddress;
            mmapBlock.size = PaletteSize;
            mmapBlock.oldData = oldData.mmapColor;
            mmapBlock.newData = newData.mmapColor;
            blocks.push_back(mmapBlock);
        }

        if (newData.hasStartLevelPalette && oldData.hasStartLevelPalette
                && layout.startLevelPaletteAddress)
        {
            BlockContent startLevelBlock;
            startLevelBlock.address = layout.startLevelPaletteAddress;
            startLevelBlock.size = StartLevelPaletteSize;
            startLevelBlock.oldData = oldData.startLevelPalette;
            startLevelBlock.newData = newData.startLevelPalette;
            blocks.push_back(startLevelBlock);
        }

        return blocks;
    }
}
