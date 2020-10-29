#ifndef ENTITYSET_H
#define ENTITYSET_H

#include "Tile.h"

#include <QColor>
#include <QPixmap>
#include <QVector>
#include <vector>

namespace LevelComponents
{
    struct EntitySetinfoTableElement
    {
        int Global_EntityID;
        int paletteOffset;
    };

    class EntitySet
    {
    public:
        EntitySet(int _EntitySetID);
        ~EntitySet();
        int GetEntitySetId() { return EntitySetID; }
        bool IsEntityInside(int entityglobalId);
        std::vector<EntitySetinfoTableElement> GetEntityTable();

    private:
        int EntitySetID; // from 0 to 89 inclusive in theory(??), but only from 0 to 82 inclusive are available
        QVector<int> entitylist; // global id list to store all the entity used in this entity set
        QVector<EntitySetinfoTableElement> EntityinfoTable; // max item number 0x20
    };
} // namespace LevelComponents

#endif // ENTITYSET_H
