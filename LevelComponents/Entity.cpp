#include "Entity.h"

#include <QPainter>

// tuples of (width, height) in 8x8 tiles; see TONC table. Row major: size attribute
static const int OAMDimensions[24] = {
    1, 1,
    2, 2,
    4, 4,
    8, 8,
    2, 1,
    4, 1,
    4, 2,
    8, 4,
    1, 2,
    1, 4,
    2, 4,
    4, 8
};

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
    Entity::Entity(int entityID, int entityGlobalId, EntitySet *_currentEntityset) :
        EntityID(entityID),
        EntityGlobalID(entityGlobalId),
        currentEntityset(_currentEntityset)
    {
        if(EntitySet::GetEntityFirstActionFrameSetPtr(entityGlobalId) == 0)
        {
            UnusedEntity = true;
            return;
        }
        int spritesActionOAMTablePtr = ROMUtils::PointerFromData(EntitySet::GetEntityFirstActionFrameSetPtr(entityGlobalId));
        OAMDataTablePtr = spritesActionOAMTablePtr;
        ExtractSpritesTiles(spritesActionOAMTablePtr, 0); //TODO: load a different frame when meet with some of the Entities
        // TODO: Load other Entity informations
    }

    /// <summary>
    /// Deconstruct an instance of Entity.
    /// </summary>
    Entity::~Entity()
    {
        // Free the Tile8x8 objects pushed to the entity tiles vector
        for(auto iter = OAMTiles.begin(); iter != OAMTiles.end(); ++iter)
        {
            delete *iter;
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
        newOAM->Xoff = attr1 & 0xFF; // Offset of OAM tile from entity origin
        newOAM->Yoff = attr0 & 0x1FF;
        newOAM->xFlip = (attr1 & (1 << 0xC)) != 0;
        newOAM->yFlip = (attr1 & (1 << 0xD)) != 0;
        int SZ = (attr1 >> 0xD) & 3;
        int SH = (attr0 >> 0xD) & 3;
        int OAMindex = SH * 4 + SZ;
        newOAM->OAMwidth = OAMDimensions[OAMindex * 2]; // unit: 8x8 tiles
        newOAM->OAMheight = OAMDimensions[OAMindex * 2 + 1];
        int tileID = attr2 & 0x3FF;
        int palNum = (attr2 >> 0xB) & 0xF;
        SemiTransparent = (((attr0 >> 0xA) & 3) == 1) ? true : false;
        Priority = (attr2 >> 0xA) & 3;

        // Create the tile8x8 objects
        Tile8x8 **tileData = currentEntityset->GetTileData();
        for(int y = 0; y < newOAM->OAMheight; ++y)
        {
            for(int x = 0; x < newOAM->OAMwidth; ++x)
            {
                struct EntityTile* entityTile = new struct EntityTile();
                entityTile->deltaX = x * 8;
                entityTile->deltaY = y * 8;
                int offsetID, offsetPal;
                // Bosses' offsetID are loaded directly
                if((EntityGlobalID > 0x10) && (EntityGlobalID != 0x18) && (EntityGlobalID != 0x2C) &&
                        (EntityGlobalID != 0x51) && (EntityGlobalID != 0x69) &&
                        (EntityGlobalID != 0x76) && (EntityGlobalID != 0x7D))
                {
                    offsetID = tileID + y * 0x20 + x + currentEntityset->GetEntityTileIdOffset(EntityID);
                    offsetPal = palNum + currentEntityset->GetEntityPaletteOffset(EntityID, EntityGlobalID);
                }
                else if(EntityGlobalID < 7)
                {
                    offsetID = tileID + y * 0x20 + x; // + currentEntityset->GetEntityTileIdOffset(EntityID) //untest
                    offsetPal = palNum/* + 7 + 8*/;
                }
                else //TODO: more cases
                {
                    offsetID = tileID + y * 0x20 + x;
                    offsetPal = palNum;
                }
                Tile8x8 *newTile = new Tile8x8(tileData[offsetID]);
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
        foreach(EntityTile *et, tile8x8)
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
        int width = 0, height = 0;
        foreach(OAMTile *ot, OAMTiles)
        {
            width = qMax(width, ot->OAMwidth * 8 + ot->Xoff);
            height = qMax(height, ot->OAMheight * 8 + ot->Yoff);
        }
        QPixmap pm(width, height);
        pm.fill(Qt::transparent);
        if(UnusedEntity)
        {
            return pm.toImage();
        }
        QPainter p(&pm);
        foreach(OAMTile *ot, OAMTiles)
        {
            p.drawImage(ot->Xoff, ot->Yoff, ot->Render());
        }
        return pm.toImage().mirrored(xFlip, yFlip);
    }

    /// <summary>
    /// Extract all the Sprite tiles8x8 using frame data in one frame.
    /// </summary>
    void Entity::ExtractSpritesTiles(int spritesFrameDataPtr, int frame)
    {
        unsigned short *u16_attribute = (unsigned short*) (ROMUtils::CurrentFile + spritesFrameDataPtr);
        int offset = 0, objectnum = 0, nowframe = 0;
        while(nowframe <= frame)
        {
            objectnum = (int) u16_attribute[offset];
            if(nowframe < frame)
            {
                offset += 1 + 3 * objectnum;
                ++nowframe;
                continue;
            }
            for(int i = 0; i < objectnum; ++i)
            {
                OAMtoTiles(u16_attribute + i * 3 + 1);
            }
            break;
        }
    }
}
