#include "Entity.h"

namespace LevelComponents
{
    Entity::Entity(int entityID, EntitySet *currentEntityset)
    {

    }

    /// <summary>
    /// Convert OAM data to Tile8x8 and save them into the entityTiles QVector.
    /// </summary>
    void Entity::OAMtoTiles(unsigned short *singleOAM)
    {

    }

    /// <summary>
    /// Extract all the Sprite tiles8x8 using frame data.
    /// </summary>
    void Entity::ExtractSpritesTiles(int spritesFrameDataPtr)
    {
        unsigned short *u16_attribute = (unsigned short*) (ROMUtils::CurrentFile + spritesFrameDataPtr);
        int OAMnum = (int) u16_attribute[0];
        for(int i = 0; i < OAMnum; i++)
        {
            OAMtoTiles(u16_attribute + 1 + i);
        }
    }
}
