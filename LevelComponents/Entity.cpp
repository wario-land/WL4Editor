#include "Entity.h"
#include "ROMUtils.h"

#include <QPixmap>
#include <QPainter>

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
    Entity::Entity(int entityGlobalId, int basicElementPalettePtr) :
            xOffset(0x7FFFFFFF), yOffset(0x7FFFFFFF), EntityGlobalID(entityGlobalId)
    {
        if (EntitySampleOamNumArray[entityGlobalId] == 0)
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
        ExtractSpritesTiles();

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
        int palNum = (attr2 >> 0xC) & 0xF;
        SemiTransparent = ((attr0 >> 0xA) & 3) == 1;
        Priority = (attr2 >> 0xA) & 3;

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
        QVector<QRgb> temPalette(palettes[palId]);
        return temPalette;
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
        for (int i = 0; i < EntitySampleOamNumArray[EntityGlobalID]; ++i)
        {
            OAMtoTiles(const_cast<unsigned short *>(EntitiesOamSampleSets[EntityGlobalID]) + i * 3);
        }
    }
} // namespace LevelComponents
