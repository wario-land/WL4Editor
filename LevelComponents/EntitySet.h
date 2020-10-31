#ifndef ENTITYSET_H
#define ENTITYSET_H

#include "Tile.h"
#include <QVector>
#include <QPixmap>

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
        EntitySet(const int _EntitySetID);
        EntitySet(const EntitySet &entitySet); // Copy constructor
        ~EntitySet();
        int GetEntitySetId() { return EntitySetID; }
        bool FindEntity(const int entityglobalId) const;
        QVector<EntitySetinfoTableElement> GetEntityTable() const;
        QPixmap GetPixmap(const int palNum);

    private:
        int TilesDefaultNum = 1024;
        int EntitySetID; // from 0 to 89 inclusive in theory(??), but only from 0 to 82 inclusive are available
        QVector<EntitySetinfoTableElement> EntityinfoTable; // max item number 0x20
        QVector<QRgb> palettes[16];
        Tile8x8 **tile8x8array = nullptr;
        Tile8x8 *blankTile = nullptr;

        void InitBlankSubPalette(const int palId, const int rowNum);
    };
} // namespace LevelComponents

#endif // ENTITYSET_H
