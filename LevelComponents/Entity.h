#ifndef ENTITY_H
#define ENTITY_H

#include <QVector>

#include "LevelComponents/EntitySet.h"

namespace LevelComponents
{
    struct EntityTile
    {
        int deltaX;
        int deltaY;
        Tile *objTile;
    };

    class Entity
    {
    private:
        int EntityID;
        int OAMDataPtr = 0;
        int EntityDeltaX, EntityDeltaY;
        int Priority;
        int PaletteOffset;
        int PaletteOffsetChange = 0;
        bool SemiTransparent = false;
        QVector<EntityTile*> entityTiles;

    public:
        Entity(int entityID, EntitySet *currentEntityset);

    };
}

#endif // ENTITY_H
