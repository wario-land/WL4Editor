#ifndef ENTITYSET_H
#define ENTITYSET_H

#define TilesDefaultNum 1024

#include "Entity.h"
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
        void ClearEntityLoadTable() { EntityinfoTable.clear(); }
        void EntityLoadTablePushBack(EntitySetinfoTableElement newelement) {if(EntityinfoTable.size() < 0x1F) EntityinfoTable.push_back(newelement); }
        QPixmap GetPixmap(const int palNum);
        void SetExtraEntities(QVector<LevelComponents::Entity*> newEntities) { extraEntities = newEntities; }
        void ClearExtraEntities() { extraEntities.clear(); }
        void SetChanged(bool change) { Changed = change; }

    private:
        int EntitySetID; // from 0 to 89 inclusive in theory(??), but only from 0 to 82 inclusive are available
        QVector<EntitySetinfoTableElement> EntityinfoTable; // max item number 0x20
        QVector<QRgb> palettes[16];
        Tile8x8 **tile8x8array = nullptr;
        Tile8x8 *blankTile = nullptr;
        bool Changed = false;

        QVector<LevelComponents::Entity*> extraEntities; // only used in sprites editor

        void InitTile8x8array();
        void ResetPalettes();
        void ResetTile8x8Array();
    };
} // namespace LevelComponents

#endif // ENTITYSET_H
