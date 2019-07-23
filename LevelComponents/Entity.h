#ifndef ENTITY_H
#define ENTITY_H

#include <QVector>

#include "LevelComponents/EntitySet.h"
#include "ROMUtils.h"

namespace LevelComponents
{
    struct EntityTile
    {
        int deltaX;
        int deltaY;
        Tile8x8 *objTile;

        // Deconstructor for the EntityTile struct
        ~EntityTile() { delete objTile; }
    };

    struct OAMTile
    {
        int Xoff;
        int Yoff;
        int OAMwidth;
        int OAMheight;
        bool xFlip;
        bool yFlip;
        QVector<EntityTile *> tile8x8;

        QImage Render();

        // Deconstructor for the OAMTile struct
        ~OAMTile()
        {
            foreach (EntityTile *t, tile8x8)
            {
                delete t;
            }
        }
    };

    class Entity
    {
    public:
        Entity(int entityID, int entityGlobalId, EntitySet *_currentEntityset);
        ~Entity();
        QImage Render();
        int GetPriority() { return Priority; }
        EntityPositionalOffset GetLevelComponents()
        {
            return currentEntityset->GetEntityPositionalOffset(EntityGlobalID);
        }
        int GetEntityID() { return EntityID; }
        int GetEntityGlobalID() { return EntityGlobalID; }
        int GetXOffset() { return xOffset; }
        int GetYOffset() { return yOffset; }

    private:
        bool xFlip = false;
        bool yFlip = false;
        int xOffset, yOffset;
        int EntityID = 0;
        int EntityGlobalID = 0;
        int OAMDataTablePtr = 0;
        int EntityDeltaX = 0, EntityDeltaY = 0;
        int Priority = 0;
        int PaletteOffset = 0;
        bool SemiTransparent = false;
        bool UnusedEntity = false;
        QVector<OAMTile *> OAMTiles;
        EntitySet *currentEntityset;
        void OAMtoTiles(unsigned short *singleOAM);
        void ExtractSpritesTiles(int spritesFrameDataPtr, int frame);
    };
} // namespace LevelComponents

#endif // ENTITY_H
