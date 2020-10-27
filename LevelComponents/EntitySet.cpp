#include "EntitySet.h"
#include "ROMUtils.h"

namespace LevelComponents
{
    /// <summary>
    /// Construct an instance of EntitySet.
    /// </summary>
    /// <param name="_EntitySetID">
    /// Entity set ID.
    /// </param>
    EntitySet::EntitySet(int _EntitySetID) : EntitySetID(_EntitySetID)
    {
        int entitysetptr = ROMUtils::PointerFromData(WL4Constants::EntitySetInfoPointerTable + _EntitySetID * 4);
        int tmpEntityId;
        int k = 0;
        do
        {
            tmpEntityId = (int) ROMUtils::CurrentFile[entitysetptr + 2 * k];
            if (tmpEntityId == 0)
                break;
            entitylist.push_back(tmpEntityId);
            EntitySetinfoTableElement Tmp_entitytableElement;
            Tmp_entitytableElement.Global_EntityID = tmpEntityId;
            Tmp_entitytableElement.paletteOffset = (int) ROMUtils::CurrentFile[entitysetptr + 2 * k + 1];
            EntityinfoTable.push_back(Tmp_entitytableElement);
            k++;
        } while (1);
    }

    /// <summary>
    /// Deconstruct an instance of EntitySet.
    /// </summary>
    EntitySet::~EntitySet()
    {
        // Add some code here if necessary
    }

    /// <summary>
    /// Check if an entity is inside this Entityset by global entity id.
    /// </summary>
    /// <param name="entityglobalId">
    /// Entity global id.
    /// </param>
    bool EntitySet::IsEntityInside(int entityglobalId)
    {
        if (entityglobalId >= 0 && entityglobalId <= 0x10)
            return true;
        for (unsigned int i = 0; i < EntityinfoTable.size(); ++i)
        {
            if (EntityinfoTable[i].Global_EntityID == entityglobalId)
            {
                return true;
            }
        }
        return false;
    }

    /// <summary>
    /// Get a copy of EntitySetinfoTableElement from this EntitySet.
    /// </summary>
    std::vector<EntitySetinfoTableElement> EntitySet::GetEntityTable()
    {
        std::vector<EntitySetinfoTableElement> newtable;
        newtable.assign(EntityinfoTable.begin(), EntityinfoTable.end());
        return newtable;
    }

} // namespace LevelComponents
