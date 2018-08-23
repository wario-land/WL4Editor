#include "Entity.h"

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
    // TODO documentation
    Entity::Entity(int entityID, EntitySet *_currentEntityset) : currentEntityset(_currentEntityset)
    {

    }

    // TODO documentation
    Entity::~Entity()
    {
        // Free the Tile8x8 objects pushed to the entity tiles vector
        for(auto iter = entityTiles.begin(); iter != entityTiles.end(); ++iter)
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
        int X = attr1 & 0xFF;
        int Y = attr0 & 0x1FF;
        bool xFlip = (attr1 & (1 << 0xC) != 0);
        bool yFlip = (attr1 & (1 << 0xD) != 0);
        int SZ = (attr1 >> 0xD) & 3;
        int SH = (attr0 >> 0xD) & 3;
        int tileID = attr2 & 0x3FF;
        int palNum = (attr2 >> 0xB) & 0xF;

        // Create the tiles
        int OAMindex = SH * 4 + SZ;
        int OAMwidth = OAMDimensions[OAMindex * 2];
        int OAMheight = OAMDimensions[OAMindex * 2 + 1];
        for(int y = 0; y < OAMheight; ++y)
        {
            int fy = yFlip ? OAMheight - y - 1 : y;
            for(int x = 0; x < OAMwidth; ++x)
            {
                int fx = xFlip ? OAMheight - x - 1 : x;
                struct EntityTile* et = new struct EntityTile();
                et->deltaX = fx * 8;
                et->deltaY = fy * 8;
                int offsetID = tileID; // TODO
                int offsetPal = palNum; // TODO
                Tile8x8 *newTile = new Tile8x8(currentEntityset->GetTileData()[offsetID]);
                newTile->SetFlipX(xFlip);
                newTile->SetFlipY(yFlip);
                newTile->SetPaletteIndex(offsetPal);
                et->objTile = newTile;
                entityTiles.push_back(et);
            }
        }
    }

    /// <summary>
    /// Extract all the Sprite tiles8x8 using frame data.
    /// </summary>
    void Entity::ExtractSpritesTiles(int spritesFrameDataPtr)
    {
        unsigned short *u16_attribute = (unsigned short*) (ROMUtils::CurrentFile + spritesFrameDataPtr);
        int OAMnum = (int) u16_attribute[0];
        for(int i = 0; i < OAMnum; ++i)
        {
            OAMtoTiles(u16_attribute + i * 3 + 1);
        }
    }
}
