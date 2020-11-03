#include "EntitySet.h"
#include "ROMUtils.h"

#include "WL4EditorWindow.h"
extern WL4EditorWindow *singleton;

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
        this->InitTile8x8array();
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
        this->InitTile8x8array();
    }

    /// <summary>
    /// Deconstruct an instance of EntitySet.
    /// </summary>
    EntitySet::~EntitySet()
    {
        for (int i = 0; i < TilesDefaultNum; ++i)
        {
            if (tile8x8array[i] != blankTile)
            {
                delete tile8x8array[i];
            }
        }
        delete blankTile;
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
    /// <param name="palNum">
    /// Palette number used to render the current entityset.
    /// </param>
    QPixmap EntitySet::GetPixmap(const int palNum)
    {
        // Initialize the palettes
        ResetPalettes();
        ResetTile8x8Array();

        // Render and return pixmap
        QPixmap pixmap(8 * 32, 8 * 32);
        pixmap.fill(Qt::transparent);

        // drawing
        for (int i = 0; i < 32; ++i)
        {
            for (int j = 0; j < 32; ++j)
            {
                if (tile8x8array[i * 32 + j] == blankTile) continue;
                tile8x8array[i * 32 + j]->SetPaletteIndex(palNum);
                tile8x8array[i * 32 + j]->DrawTile(&pixmap, j * 8, i * 8);
            }
        }
        return pixmap;
    }

    /// <summary>
    /// Initialize tile8x8array.
    /// </summary>
    void EntitySet::InitTile8x8array()
    {
        tile8x8array = new Tile8x8* [TilesDefaultNum];
        memset(tile8x8array, 0, TilesDefaultNum * sizeof(tile8x8array[0]));
        blankTile = Tile8x8::CreateBlankTile(palettes);
        for (int i = 0; i < TilesDefaultNum; ++i)
        {
            tile8x8array[i] = blankTile;
        }
    }

    /// <summary>
    /// Re-Initialize palettes and delete the old ones.
    /// </summary>
    void EntitySet::ResetPalettes()
    {
        for (int i = 0; i < 3; ++i)
        {
            palettes[i].clear();
            for (int j = 0; j < 16; ++j)
                palettes[i].push_back(QColor(0, 0, 0, 0xFF).rgba());
        }
        for (int i = 3; i < 15; ++i)
        {
            palettes[i].clear();
        }
        for (int i = 3; i < 8; ++i) // load universal palettes [3, 7]
        {
            palettes[i] << ROMUtils::entities[6]->GetPalette(i - 3);
        }
        int offset = 8;
        int localEntityId = 0;
        bool overwriteBoxtiles = false;
        do
        {
            int tmpEntityGlobalId = EntityinfoTable[localEntityId].Global_EntityID;
            int tmpEntityPalOffset = EntityinfoTable[localEntityId].paletteOffset;
            ++localEntityId;

            int tmpEntityPalNum = ROMUtils::entities[tmpEntityGlobalId]->GetPalNum();
            offset = tmpEntityPalNum + tmpEntityPalOffset;
            if (offset > 14)
                overwriteBoxtiles = true;
            if (offset > 15)
            {
                // TODO: deal with exception
                singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("load entityset error: loading palette's id out of bound."));
                continue;
            }
            for (int i = tmpEntityPalOffset; i < offset; ++i) // load specified sprites' tiles
            {
                palettes[i].clear(); // sometimes palettes will overwrite each other
                palettes[i] << ROMUtils::entities[tmpEntityGlobalId]->GetPalette(i - tmpEntityPalOffset);
            }
        } while(EntityinfoTable.size() > localEntityId);
        if(!overwriteBoxtiles)
        {
            palettes[15] << ROMUtils::entities[0]->GetPalette(0);
        }
        for (int i = 3; i < 16; ++i)
        {
            if (!palettes[i].size())
            {
                for (int j = 0; j < 16; ++j)
                    palettes[i].push_back(QColor(0, 0, 0, 0xFF).rgba());
            }
        }
    }

    /// <summary>
    /// Re-Initialize tile8x8array and delete the old ones.
    /// </summary>
    void EntitySet::ResetTile8x8Array()
    {
        // Clean up and re-initialize the 8x8 tiles and set all the tiles to blank tiles
        for (int i = (0x20 * 4); i < TilesDefaultNum; ++i)
        {
            if (tile8x8array[i] != blankTile)
            {
                delete tile8x8array[i];
            }
        }
        // Load universal sprites
        QVector<Tile8x8 *> tmptilesarray = ROMUtils::entities[6]->GetSpriteTiles(palettes);
        for (int i = (0x20 * 4); i < (0x20 * 16); ++i)
        {
            tile8x8array[i] = tmptilesarray[i - 0x20 * 4];
            tile8x8array[i]->SetIndex(i);
        }
        int offset = 8; // 2 rows count 1 in the offset, keep the loading progress the same as palette loading
        int localEntityId = 0; // used to contain the current entity being loaded tiles
        bool overwriteBoxtiles = false;
        do
        {
            int tmpEntityGlobalId = EntityinfoTable[localEntityId].Global_EntityID;
            int tmpEntityPalOffset = EntityinfoTable[localEntityId].paletteOffset;
            ++localEntityId;
            tmptilesarray.clear();
            tmptilesarray = ROMUtils::entities[tmpEntityGlobalId]->GetSpriteTiles(palettes);

            int tmpEntityPalNum = ROMUtils::entities[tmpEntityGlobalId]->GetPalNum();
            offset = tmpEntityPalNum + tmpEntityPalOffset;
            if (offset > 14)
                overwriteBoxtiles = true;
            if (offset > 15)
            {
                // TODO: deal with exception
                continue;
            }
            for (int i = (0x20 * tmpEntityPalOffset * 2); i < (0x20 * offset * 2); ++i) // load specified sprites' tiles
            {
                // sometimes palettes will overwrite each other
                if (tile8x8array[i] != blankTile)
                {
                    delete tile8x8array[i];
                }
                tile8x8array[i] = tmptilesarray[i - 0x20 * tmpEntityPalOffset * 2];
                tile8x8array[i]->SetIndex(i);
            }
        } while(EntityinfoTable.size() > localEntityId);
        if(!overwriteBoxtiles)
        {
            tmptilesarray.clear();
            tmptilesarray = ROMUtils::entities[0]->GetSpriteTiles(palettes);
            for (int i = (0x20 * 15 * 2); i < (0x20 * 16 * 2); ++i) // load treasure boxes tiles
            {
                tile8x8array[i] = tmptilesarray[i - 0x20 * 15 * 2];
                tile8x8array[i]->SetIndex(i);
            }
        }
    }

} // namespace LevelComponents
