﻿#include "Entity.h"
#include "ROMUtils.h"

#include <QPainter>

constexpr unsigned int LevelComponents::Entity::EntitiesFirstActionFrameSetsPtrsData[129];
constexpr int LevelComponents::Entity::EntityPositinalOffset[258];

// tuples of (width, height) in 8x8 tiles; see TONC table. Row major: size attribute
// clang-format off
static const int OAMDimensions[24] = {
    1, 1, // size * 3 + shape (tuples of OAM width and height in 8x8 tiles)
    2, 1,
    1, 2,
    2, 2,
    4, 1,
    1, 4,
    4, 4,
    4, 2,
    2, 4,
    8, 8,
    8, 4,
    4, 8
};
// clang-format on

namespace LevelComponents
{
    /// <summary>
    /// Construct an instance of Entity.
    /// </summary>
    /// <param name="entityID">
    /// Local entity ID in entity set.
    /// </param>
    /// <param name="entityGlobalId">
    /// Global entity ID.
    /// </param>
    /// </param name="_currentEntityset">
    /// Entire set pointer.
    /// </param>
    Entity::Entity(int entityGlobalId, int basicElementPalettePtr) :
            xOffset(0x7FFFFFFF), yOffset(0x7FFFFFFF), EntityGlobalID(entityGlobalId)
    {
        int animationDataPointer = EntitiesFirstActionFrameSetsPtrsData[entityGlobalId];
        if (animationDataPointer == 0)
        {
            UnusedEntity = true;
            xOffset = yOffset = 0;
            return;
        }

        // Load tiles and palettes
        if (entityGlobalId > 0x10)
        {
            int palettePtr =
                ROMUtils::PointerFromData(WL4Constants::EntityPalettePointerTable + 4 * (entityGlobalId - 0x10));
            EntityPaletteNum =
                ROMUtils::IntFromData(WL4Constants::EntityTilesetLengthTable + 4 * (entityGlobalId - 0x10)) /
                (32 * 32 * 2);
            LoadSubPalettes(EntityPaletteNum, palettePtr);
            int tiledataptr = ROMUtils::PointerFromData(WL4Constants::EntityTilesetPointerTable + 4 * (entityGlobalId - 0x10));
            int tiledatalength = ROMUtils::IntFromData(WL4Constants::EntityTilesetLengthTable + 4 * (entityGlobalId - 0x10));
            LoadSpritesTiles(tiledataptr, tiledatalength);
        }
        else if (EntityGlobalID < 6) // Boxes
        {
            EntityPaletteNum = 1;
            LoadSubPalettes(1, ROMUtils::PointerFromData(WL4Constants::EntityPalettePointerTable));
            LoadSpritesTiles(WL4Constants::TreasureBoxGFXTiles, 2048);
        }
        else // tho there will be perhaps some exception, but just assume all of them using the universal sprites tiles
        {
            EntityPaletteNum = 5;
            LoadSubPalettes(1, basicElementPalettePtr);
            LoadSubPalettes(4, WL4Constants::UniversalSpritesPalette2, 1);
            LoadSpritesTiles(WL4Constants::SpritesBasicElementTiles, 0x3000);
        }

        // Set the OAM tile information
        int spritesActionOAMTablePtr = ROMUtils::PointerFromData(animationDataPointer);
        OAMDataTablePtr = spritesActionOAMTablePtr;
        ExtractSpritesTiles(spritesActionOAMTablePtr,
                            0); // TODO: load a different frame when meet with some of the Entities

        // Set the image offsets for the entity
        foreach (OAMTile *ot, OAMTiles)
        {
            xOffset = qMin(xOffset, ot->Xoff);
            yOffset = qMin(yOffset, ot->Yoff);
        }
    }

    /// <summary>
    /// Deconstruct an instance of Entity.
    /// </summary>
    Entity::~Entity()
    {
        // Free the Tile8x8 objects pushed to the entity tiles vector
        foreach (OAMTile *tile, OAMTiles)
        {
            delete tile;
        }
        foreach (Tile8x8 *tile, tile8x8data)
        {
            delete tile;
        }
    }

    /// <summary>
    /// Convert OAM data to Tile8x8 and save them into the entityTiles QVector.
    /// </summary>
    void Entity::OAMtoTiles(unsigned short *singleOAM)
    {
        // Obtain short values for the OAM tile
        unsigned short attr0 = singleOAM[0];
        unsigned short attr1 = singleOAM[1];
        unsigned short attr2 = singleOAM[2];

        // Obtain the tile parameters for the OAM tile
        struct OAMTile *newOAM = new struct OAMTile();
        newOAM->Xoff = (attr1 & 0xFF) - (attr1 & 0x100); // Offset of OAM tile from entity origin
        newOAM->Yoff = (attr0 & 0x7F) - (attr0 & 0x80);
        newOAM->xFlip = (attr1 & (1 << 0xC)) != 0;
        newOAM->yFlip = (attr1 & (1 << 0xD)) != 0;
        int SZ = (attr1 >> 0xE) & 3;                         // object size
        int SH = (attr0 >> 0xE) & 3;                         // object shape
        newOAM->OAMwidth = OAMDimensions[2 * (SZ * 3 + SH)]; // unit: 8x8 tiles
        newOAM->OAMheight = OAMDimensions[2 * (SZ * 3 + SH) + 1];
        int tileID = attr2 & 0x3FF;
        int palNum = (attr2 >> 0xB) & 0xF;
        SemiTransparent = (((attr0 >> 0xA) & 3) == 1) ? true : false;
        Priority = (attr2 >> 0xA) & 3;

        // Create the tile8x8 objects
        for (int y = 0; y < newOAM->OAMheight; ++y)
        {
            for (int x = 0; x < newOAM->OAMwidth; ++x)
            {
                struct EntityTile *entityTile = new struct EntityTile();
                entityTile->deltaX = x * 8;
                entityTile->deltaY = y * 8;
                int offsetID = 0, offsetPal = 0;
                // Bosses' offsetID are loaded directly
                if ((EntityGlobalID > 0x10) && (EntityGlobalID != 0x18) && (EntityGlobalID != 0x2C) &&
                    (EntityGlobalID != 0x51) && (EntityGlobalID != 0x69) && (EntityGlobalID != 0x76) &&
                    (EntityGlobalID != 0x7D) && (EntityGlobalID != 0x1B) && (EntityGlobalID != 0x1F) &&
                    (EntityGlobalID != 0x47)) // Normal Entities
                {
                    offsetID = tileID + (y - 16) * 0x20 + x;
                    offsetPal = palNum;
                }
                else if ((EntityGlobalID < 8) && (EntityGlobalID > 5)) // Diamond and Frog switch
                {
                    offsetID = tileID + (y - 4) * 0x20 + x;
                    offsetPal = 2;
                }
                else if ((EntityGlobalID < 16) && (EntityGlobalID > 7)) // Keyzer
                {
                    offsetID = tileID + (y - 4) * 0x20 + x;
                    // the Keyzer use 2 palette (6 and 7), but the OAM data are set by only 1 palette
                    // Using palette 7 here makes the render result more similar to the real graphic
                    offsetPal = 4;
                }
                else if (EntityGlobalID < 6) // Boxes
                {
                    offsetID = tileID + (y - 16) * 0x20 + x;
                    offsetPal = 0;
                }
                else if (EntityGlobalID == 0x18) // Boss: Cuckoo Condor
                {
                    offsetID = tileID + (y - 16) * 0x20 + x;
                    offsetPal = 1;
                }
                else if ((EntityGlobalID == 0x2C) || (EntityGlobalID == 0x51)) // Boss: Spoiled Rotten and Catbat
                {
                    offsetID = tileID + (y - 16) * 0x20 + x;
                    offsetPal = palNum;
                }
                else if (EntityGlobalID == 0x76) // Boss: Cractus
                {
                    offsetID = tileID + (y - 16) * 0x20 + x;
                    offsetPal = palNum;
                } // only showing flowerpot
                else if (EntityGlobalID == 0x7D) // Boss: Golden Diva
                {
                    offsetID = tileID + (y - 8) * 0x20 + x;
                    offsetPal = palNum - 5;
                } // Golden Diva is a Frog Switch ??
                else if (EntityGlobalID == 0x69) // Boss: Aerodent
                {
                    offsetID = tileID + (y - 16) * 0x20 + x;
                    offsetPal = palNum - 3;
                }
                else if (EntityGlobalID == 0x10) // Shining gem above boxes
                {
                    offsetID = tileID + (y - 4) * 0x20 + x;
                    offsetPal = palNum - 3;
                }
                else if (EntityGlobalID == 0x1B || (EntityGlobalID == 0x1F))
                {
                    // moguramen, Togerobo
                    offsetID = tileID + (y - 16) * 0x20 + x;
                    offsetPal = 0;
                }
                else if (EntityGlobalID == 0x47)
                {
                    // toy door
                    offsetID = tileID + (y - 16) * 0x20 + x;
                    offsetPal = 1;
                }
                else // TODO: more cases
                {
                    offsetID = tileID + (y - 16) * 0x20 + x;
                    offsetPal = palNum;
                }

                // these Entities have an extra relative palette offset
                // blue spear mask, harimenzetto, red marumen
                if (EntityGlobalID == 0x12 || (EntityGlobalID == 0x1D) || (EntityGlobalID == 0x2B))
                {
                    offsetPal++;
                }
                if (EntityGlobalID == 0x13) // red spear mask
                {
                    offsetPal += 2;
                }
                if ((EntityGlobalID == 0x30) ||
                       (EntityGlobalID == 0x31) || (EntityGlobalID == 0x32) || (EntityGlobalID == 0x33) ||
                       (EntityGlobalID == 0x34) || (EntityGlobalID == 0x35) || (EntityGlobalID == 0x36) ||
                       (EntityGlobalID == 0x37) || (EntityGlobalID == 0x38) || (EntityGlobalID == 0x39) ||
                       (EntityGlobalID == 0x58) || (EntityGlobalID == 0x59) || (EntityGlobalID == 0x5A) ||
                       (EntityGlobalID == 0x5B) || (EntityGlobalID == 0x5C))
                {
                    offsetPal--;
                }
                if (EntityGlobalID == 0x5D || (EntityGlobalID == 0x70))
                    offsetPal = 0;
                if (EntityGlobalID == 0x71)
                    offsetPal = 1;

                assert(offsetID >= 0);
                if(EntityGlobalID < 0x11)
                    assert(offsetID < ((EntityPaletteNum + 1) * 32 * 2));
                else
                    assert(offsetID < (EntityPaletteNum * 32 * 2));
                assert(offsetPal < EntityPaletteNum);
                assert(offsetPal >= 0);

                Tile8x8 *newTile = new Tile8x8(tile8x8data[offsetID]);
                newTile->SetPaletteIndex(offsetPal);
                entityTile->objTile = newTile;
                newOAM->tile8x8.push_back(entityTile);
            }
        }
        OAMTiles.push_back(newOAM);
    }

    /// <summary>
    /// Render an OAM tile as a QImage.
    /// </summary>
    /// <returns>
    /// The rendered QImage object.
    /// </returns>
    QImage OAMTile::Render()
    {
        QPixmap pm(OAMwidth * 8, OAMheight * 8);
        pm.fill(Qt::transparent);
        foreach (EntityTile *et, tile8x8)
        {
            et->objTile->DrawTile(&pm, et->deltaX, et->deltaY);
        }
        return pm.toImage().mirrored(xFlip, yFlip);
    }

    /// <summary>
    /// Render an entity's constituent OAM tiles as a QImage.
    /// </summary>
    /// <returns>
    /// The rendered QImage object.
    /// </returns>
    QImage Entity::Render()
    {
        int maxX = 0x80000000, maxY = 0x80000000;
        foreach (OAMTile *ot, OAMTiles)
        {
            maxX = qMax(maxX, ot->OAMwidth * 8 + (ot->Xoff));
            maxY = qMax(maxY, ot->OAMheight * 8 + (ot->Yoff));
        }
        int width = maxX - xOffset, height = maxY - yOffset;
        QPixmap pm(width, height);
        pm.fill(Qt::transparent);
        if (UnusedEntity)
        {
            return pm.toImage();
        }
        QPainter p(&pm);
        // OAM tiles must be rendered in reverse order as per the GBA graphical specifications
        for (auto iter = OAMTiles.rbegin(); iter != OAMTiles.rend(); ++iter)
        {
            OAMTile *ot = *iter;
            p.drawImage(ot->Xoff - xOffset, ot->Yoff - yOffset, ot->Render());
        }
        return pm.toImage().mirrored(xFlip, yFlip);
    }

    /// <summary>
    /// Get Entity Positional offset by its global id.
    /// </summary>
    /// <param name="entityglobalId">
    /// Entity global id.
    /// </param>
    EntityPositionalOffset Entity::GetEntityPositionalOffset(int entityglobalId)
    {
        EntityPositionalOffset tmpEntityPositionalOffset;
        tmpEntityPositionalOffset.XOffset = EntityPositinalOffset[2 * entityglobalId];
        tmpEntityPositionalOffset.YOffset = EntityPositinalOffset[2 * entityglobalId + 1];
        return tmpEntityPositionalOffset;
    }

    /// <summary>
    /// sub function used in EntitySet constructor for loading sub palettes for each Entity.
    /// </summary>
    /// <param name="paletteNum">
    /// Amount of palettes that will be reset.
    /// </param>
    /// <param name="paletteSetPtr">
    /// Palette data address in ROM.
    /// </param>
    /// <param name="startPaletteId">
    /// Id of the palette where start to reset.
    /// </param>
    void Entity::LoadSubPalettes(int paletteNum, int paletteSetPtr, int startPaletteId)
    {
        for (int i = 0; i < paletteNum; ++i)
        {
            if (palettes[i + startPaletteId].size())
                palettes[i + startPaletteId].clear();
            // First color is transparent
            ROMUtils::LoadPalette(&palettes[i + startPaletteId], (unsigned short *) (ROMUtils::CurrentFile + paletteSetPtr + i * 32));
        }
    }

    /// <summary>
    /// sub function used in EntitySet constructor for loading Tile8x8s for each Entity.
    /// </summary>
    /// <param name="tileaddress">
    /// Address of Entity tiles in ROM.
    /// </param>
    /// <param name="datalength">
    /// Length of Tiles' data.
    /// </param>
    void Entity::LoadSpritesTiles(int tileaddress, int datalength)
    {
        for (int i = 0; i < (datalength / 32); ++i)
        {
            tile8x8data.push_back(new Tile8x8(tileaddress + i * 32, palettes));
        }
    }

    /// <summary>
    /// Extract all the Sprite tiles8x8 using frame data in one frame.
    /// </summary>
    void Entity::ExtractSpritesTiles(int spritesFrameDataPtr, int frame)
    {
        unsigned short *u16_attribute = (unsigned short *) (ROMUtils::CurrentFile + spritesFrameDataPtr);
        int offset = 0, objectnum = 0, nowframe = 0;
        while (nowframe <= frame)
        {
            objectnum = (int) u16_attribute[offset];
            if (nowframe < frame)
            {
                offset += 1 + 3 * objectnum;
                ++nowframe;
                continue;
            }
            for (int i = 0; i < objectnum; ++i)
            {
                OAMtoTiles(u16_attribute + i * 3 + 1);
            }
            break;
        }
    }
} // namespace LevelComponents
