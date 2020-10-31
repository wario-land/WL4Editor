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
    EntitySet::EntitySet(const int _EntitySetID) : EntitySetID(_EntitySetID)
    {
        int entitysetptr = ROMUtils::PointerFromData(WL4Constants::EntitySetInfoPointerTable + _EntitySetID * 4);
        int tmpEntityId;
        int k = 0;
        do
        {
            tmpEntityId = (int) ROMUtils::CurrentFile[entitysetptr + 2 * k];
            if (tmpEntityId == 0)
                break;
            EntitySetinfoTableElement Tmp_entitytableElement;
            Tmp_entitytableElement.Global_EntityID = tmpEntityId;
            Tmp_entitytableElement.paletteOffset = (int) ROMUtils::CurrentFile[entitysetptr + 2 * k + 1];
            EntityinfoTable.push_back(Tmp_entitytableElement);
            k++;
        } while (1);
    }

    /// <summary>
    /// Copy construct an instance of EntitySet.
    /// </summary>
    /// <param name="entitySet">
    /// The entitySet used to copy construct new EntitySet.
    /// </param>
    EntitySet::EntitySet(const EntitySet &entitySet): EntitySetID(entitySet.EntitySetID)
    {
        this->EntityinfoTable = entitySet.GetEntityTable();
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
    bool EntitySet::FindEntity(const int entityglobalId) const
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
    QVector<EntitySetinfoTableElement> EntitySet::GetEntityTable() const
    {
        QVector<EntitySetinfoTableElement> newtable(EntityinfoTable);
        return newtable;
    }

    /// <summary>
    /// Render the whole Entityset using one palette.
    /// </summary>
    QPixmap EntitySet::GetPixmap(const int palNum)
    {
        // Initialize the palettes
        InitBlankSubPalette(0, 3);
        // TODO: Load more palettes, and don't forget to delete the old ones

        // Clean up and re-initialize the 8x8 tiles and set all the tiles to blank tiles
        for (int i = 0; i < TilesDefaultNum; ++i)
        {
            if (tile8x8array[i] != blankTile)
            {
                delete tile8x8array[i];
            }
        }
        delete blankTile;
        tile8x8array = new Tile8x8* [TilesDefaultNum];
        memset(tile8x8array, 0, TilesDefaultNum * sizeof(tile8x8array[0]));
        blankTile = Tile8x8::CreateBlankTile(palettes);
        for (int i = 0; i < TilesDefaultNum; ++i)
        {
            tile8x8array[i] = blankTile;
        }

        // TODO: Load sprites tiles from entities

        return QPixmap();
    }

    /// <summary>
    /// Check if an entity is inside this Entityset by global entity id.
    /// </summary>
    /// <param name="palId">
    /// Id of palette to start initialize.
    /// </param>
    /// <param name="rowNum">
    /// The row number to initialize.
    /// </param>
    void EntitySet::InitBlankSubPalette(const int palId, const int rowNum)
    {
        for (int i = palId; i < (palId + rowNum - 1); ++i)
        {
            palettes[i].clear();
            for (int j = 0; j < 16; ++j)
                palettes[i].push_back(QColor(0, 0, 0, 0xFF).rgba());
        }
    }

} // namespace LevelComponents
