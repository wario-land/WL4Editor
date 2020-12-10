#include "Entity.h"
#include "ROMUtils.h"

#include <QPixmap>
#include <QPainter>

#include <cassert>

constexpr unsigned char LevelComponents::Entity::EntitySampleOamNumArray[129];
constexpr int LevelComponents::Entity::EntityPositinalOffset[258];
constexpr unsigned short LevelComponents::Entity::EntitiesOamSampleSets[129][0x2A * 3];

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
    /// <param name="entityGlobalId">
    /// Global entity ID.
    /// </param>
    /// </param name="basicElementPalettePtr">
    /// Pointer for basic sprites element palette differs in every passage, usually used for gem color stuff.
    /// </param>
    Entity::Entity(int entityGlobalId, int basicElementPalettePtr) : EntityGlobalID(entityGlobalId)
    {
        if (EntitySampleOamNumArray[entityGlobalId] == 0)
        {
            UnusedEntity = true;
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
        ExtractSpritesTiles();

        // Set the image offsets for the entity
        foreach (OAMTile *ot, OAMTiles)
        {
            xOffset = qMin(xOffset, ot->Xoff);
            yOffset = qMin(yOffset, ot->Yoff);
        }

        // Extra blank tile used as dummy tiles when adding tiles to the entity
        blankTile = Tile8x8::CreateBlankTile(palettes);
    }

    /// <summary>
    /// Copy construct an instance of Entity.
    /// </summary>
    /// <param name="entity">
    /// The entity used to copy construct new Entity.
    /// </param>
    Entity::Entity(const Entity &entity) : xOffset(entity.xOffset), yOffset(entity.yOffset),
        EntityGlobalID(entity.EntityGlobalID), EntityPaletteNum(entity.EntityPaletteNum), UnusedEntity(entity.UnusedEntity)
    {
        if (UnusedEntity) return;

        for (int i = 0; i < EntityPaletteNum; ++i)
        {
            palettes[i] = entity.GetPalette(i);
        }
        tile8x8data = entity.GetSpriteTiles(palettes);
        ExtractSpritesTiles();
    }

    /// <summary>
    /// Deconstruct an instance of Entity.
    /// </summary>
    Entity::~Entity()
    {
        // Free the Tile8x8 objects pushed to the entity tiles vector
        for (OAMTile *tile: OAMTiles)
        {
            delete tile;
        }
        for (Tile8x8 *tile: tile8x8data)
        {
            if (tile != blankTile) delete tile;
        }
        delete blankTile;
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
        int palNum = (attr2 >> 0xC) & 0xF;

        // Create the tile8x8 objects
        for (int y = 0; y < newOAM->OAMheight; ++y)
        {
            for (int x = 0; x < newOAM->OAMwidth; ++x)
            {
                struct EntityTile *entityTile = new struct EntityTile();
                entityTile->deltaX = x * 8;
                entityTile->deltaY = y * 8;                
                int offsetID = tileID + y * 0x20 + x;

                // this part of logic will be needed when we want to support custom oam, so keep it here
                // i put it here only for loading data testing -- ssp
#ifndef QT_NO_DEBUG
                assert(offsetID >= 0);
                if(EntityGlobalID < 0x11)
                    assert(offsetID < ((EntityPaletteNum + 1) * 32 * 2));
                else
                    assert(offsetID < (EntityPaletteNum * 32 * 2));
                assert(palNum < EntityPaletteNum);
                assert(palNum >= 0);
#endif

                if (tile8x8data[offsetID] == blankTile) continue; // skip dummy tiles if used
                Tile8x8 *newTile = new Tile8x8(tile8x8data[offsetID]);
                newTile->SetPaletteIndex(palNum);
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
        if (UnusedEntity)
        {
            return QImage();
        }
        int maxX = 0x80000000, maxY = 0x80000000;
        foreach (OAMTile *ot, OAMTiles)
        {
            maxX = qMax(maxX, ot->OAMwidth * 8 + (ot->Xoff));
            maxY = qMax(maxY, ot->OAMheight * 8 + (ot->Yoff));
        }
        int width = maxX - xOffset, height = maxY - yOffset;
        QPixmap pm(width, height);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        // OAM tiles must be rendered in reverse order as per the GBA graphical specifications
        for (auto iter = OAMTiles.rbegin(); iter != OAMTiles.rend(); ++iter)
        {
            OAMTile *ot = *iter;
            p.drawImage(ot->Xoff - xOffset, ot->Yoff - yOffset, ot->Render());
        }
        return pm.toImage();
    }

    /// <summary>
    /// Get tilemap image used by this Entity.
    /// </summary>
    /// <param name="palNum">
    /// Palette number used to render the current entityset.
    /// </param>
    /// <returns>
    /// The rendered QImage object.
    /// </returns>
    QImage Entity::GetTileMap(const int palNum)
    {
        // Render and return pixmap
        int rowNum = tile8x8data.size() >> 5; // tile8x8data.size() / 32
        QPixmap pixmap(8 * 32, 8 * rowNum);
        pixmap.fill(Qt::transparent);

        // drawing
        for (int i = 0; i < rowNum; ++i)
        {
            for (int j = 0; j < 32; ++j)
            {
                tile8x8data[i * 32 + j]->SetPaletteIndex(palNum);
                tile8x8data[i * 32 + j]->DrawTile(&pixmap, j * 8, i * 8);
            }
        }
        return pixmap.toImage();
    }

    /// <summary>
    /// Render oam array in test mode.
    /// </summary>
    /// <param name="oamNum">
    /// the number of oam.
    /// </param>
    /// <param name="nakedOAMarray">
    /// Naked OAM array without oam number as the first element
    /// </param>
    /// <param name="noReferrneceBox">
    /// Don't draw the yellow referrence Box if set this to true
    /// </param>
    QImage Entity::TestRenderOams(int oamNum, unsigned short *nakedOAMarray, bool noReferrneceBox)
    {
        if (!oamNum) return QImage();

        QVector<OAMTile *> tmpOAMTiles;
        for (int i = 0; i < oamNum; ++i)
        {
            // Obtain short values for the OAM tile
            unsigned short attr0 = nakedOAMarray[3 * i];
            unsigned short attr1 = nakedOAMarray[3 * i + 1];
            unsigned short attr2 = nakedOAMarray[3 * i + 2];
            // skip priority reset and Char Id (tiles ID) reset
            // skip orientation change
            // skip affine transformation (double size)
            // skip alpha blend

            // Obtain the tile parameters for the OAM tile
            struct OAMTile *newOAM = new struct OAMTile();
            newOAM->Xoff = (attr1 & 0xFF) - (attr1 & 0x100); // Offset of OAM tile from entity origin
            newOAM->Yoff = (attr0 & 0x7F) - (attr0 & 0x80); // they are signed char
            newOAM->xFlip = (attr1 & (1 << 0xC)) != 0;
            newOAM->yFlip = (attr1 & (1 << 0xD)) != 0;
            int SZ = (attr1 >> 0xE) & 3;                         // object size
            int SH = (attr0 >> 0xE) & 3;                         // object shape
            newOAM->OAMwidth = OAMDimensions[2 * (SZ * 3 + SH)]; // unit: 8x8 tiles
            newOAM->OAMheight = OAMDimensions[2 * (SZ * 3 + SH) + 1];
            int tileID = attr2 & 0x3FF;
            int palNum = (attr2 >> 0xC) & 0xF;

            // Create the tile8x8 objects
            for (int y = 0; y < newOAM->OAMheight; ++y)
            {
                for (int x = 0; x < newOAM->OAMwidth; ++x)
                {
                    struct EntityTile *entityTile = new struct EntityTile();
                    entityTile->deltaX = x * 8;
                    entityTile->deltaY = y * 8;
                    int offsetID = tileID + y * 0x20 + x;
                    if (offsetID >= 0x200) offsetID = offsetID - 0x200; // the game engine use char Id with offset in the oam data
                    if (offsetID >= tile8x8data.size()) continue; // this data have problems, just skip it
                    if (tile8x8data[offsetID] == blankTile) continue; // skip dummy tiles if used
                    Tile8x8 *newTile = new Tile8x8(tile8x8data[offsetID]);
                    if (palNum > 7) palNum = palNum - 8; // the game engine use pal Id with offset in the oam data
                    newTile->SetPaletteIndex(palNum);
                    entityTile->objTile = newTile;
                    newOAM->tile8x8.push_back(entityTile);
                }
            }
            tmpOAMTiles.push_back(newOAM);
        }

        // In oam attributes, x and y params has a range from 0 below 511 and 255
        // And we want to put the (x, y) = (0, 0) point on the central of the pixmap
        // So we double the size again
        int width = 512 * 2; // x
        int height = 256 * 2; // y
        QPixmap pm(width, height);
        pm.fill(Qt::transparent);
        QPainter p(&pm);

        // OAM tiles must be rendered in reverse order as per the GBA graphical specifications
        for (auto iter = tmpOAMTiles.rbegin(); iter != tmpOAMTiles.rend(); ++iter)
        {
            OAMTile *ot = *iter;
            // x + 8, y + 16, this works in the room rendering to match the box
            p.drawImage(ot->Xoff + 512 + 8, ot->Yoff + 256 + 16, ot->Render());
            delete ot;
        }
        tmpOAMTiles.clear();

        // Draw position referrence Box
        if (!noReferrneceBox)
        {
            QPen EntityBoxPen = QPen(QBrush(QColor(0xFF, 0xFF, 0, 0xFF)), 2);
            EntityBoxPen.setJoinStyle(Qt::MiterJoin);
            p.setPen(EntityBoxPen);
            p.drawRect(512, 256, 16, 16);
        }

        return pm.toImage();
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
    /// Get palettes used by this Entity.
    /// </summary>
    /// <param name="palId">
    /// Palette Id used to return the specified palette QVector.
    /// </param>
    QVector<QRgb> Entity::GetPalette(const int palId) const
    {
        if(palId > EntityPaletteNum || palId < 0)
        {
            QVector<QRgb> firstPalette(palettes[0]);
            return firstPalette;
        }
        QVector<QRgb> tmpPalette(palettes[palId]);
        return tmpPalette;
    }

    /// <summary>
    /// Get tiles used by this Entity.
    /// </summary>
    /// <param name="newPal">
    /// newPal should be provided so it can be migrated to some EntitySet instance.
    /// </param>
    QVector<Tile8x8 *> Entity::GetSpriteTiles(QVector<QRgb> *newPal) const
    {
        QVector<Tile8x8 *> tmptiles;
        for(int i = 0; i < tile8x8data.size(); ++i)
        {
            if(newPal)
            {
                tmptiles.push_back(new Tile8x8(tile8x8data[i], newPal));
            }
            else
            {
                tmptiles.push_back(new Tile8x8(tile8x8data[i]));
            }
        }
        return tmptiles;
    }

    /// <summary>
    /// sub function used in Entity constructor for loading sub palettes for each Entity.
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
    /// sub function used in Entity constructor for loading Tile8x8s for each Entity.
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
    void Entity::ExtractSpritesTiles()
    {
        // Clear old oams if exist
        for (OAMTile *oam: OAMTiles)
        {
            delete oam;
        }
        OAMTiles.clear();

        // Reset oams
        for (int i = 0; i < EntitySampleOamNumArray[EntityGlobalID]; ++i)
        {
            OAMtoTiles(const_cast<unsigned short *>(EntitiesOamSampleSets[EntityGlobalID]) + i * 3);
        }
    }

    /// <summary>
    /// Add 2 rows of tiles8x8 and one row of palette.
    /// </summary>
    void Entity::AddTilesAndPaletteByOneRow()
    {
        if (EntityGlobalID < 0x11) return;
        for (int i = 0; i < 64; ++i)
        {
            tile8x8data.push_back(blankTile);
        }
        EntityPaletteNum++;
        palettes[EntityPaletteNum - 1].push_back(0);
        for (int i = 1; i < 16; ++i)
        {
            palettes[EntityPaletteNum - 1].push_back(Qt::black);
        }
    }

    /// <summary>
    /// Delete 2 rows of tiles8x8 and one row of palette.
    /// </summary>
    void Entity::DeleteTilesAndPaletteByOneRow(int palID)
    {
        if (EntityGlobalID < 0x11) return;
        if (EntityPaletteNum == 1) return;
        for (int i = palID * 64; i < (palID + 1) * 64; ++i)
        {
            if (tile8x8data[i] != blankTile) delete tile8x8data[i];
        }
        tile8x8data.remove(palID * 64, 64);
        SwapPalettes(palID, EntityPaletteNum - 1);
        palettes[EntityPaletteNum - 1].clear();
        EntityPaletteNum--;
    }

    /// <summary>
    /// Swap 2 existing palettes.
    /// </summary>
    /// <param name="palID_1">
    /// The first palette id.
    /// </param>
    /// <param name="palID_2">
    /// The second palette id.
    /// </param>
    void Entity::SwapPalettes(int palID_1, int palID_2)
    {
        if (palID_1 >= EntityPaletteNum || palID_2 >= EntityPaletteNum) return;
        for (int i = 1; i < 16; ++i)
        {
            QRgb colorData = palettes[palID_1][i];
            palettes[palID_1][i] = palettes[palID_2][i];
            palettes[palID_2][i] = colorData;
        }
    }
} // namespace LevelComponents
