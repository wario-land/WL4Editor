#include "EntitySet.h"

constexpr unsigned int LevelComponents::EntitySet::EntitiesFirstActionFrameSetsPtrsData[129];
constexpr int LevelComponents::EntitySet::EntityPositinalOffset[258];

namespace LevelComponents
{
    /// <summary>
    /// sub function used in EntitySet constructor for loading sub palettes for each Entity.
    /// </summary>
    /// <param name="startPaletteId">
    /// Id of the palette where start to reset.
    /// </param>
    /// <param name="paletteNum">
    /// Amount of palettes that will be reset.
    /// </param>
    /// <param name="paletteSetPtr">
    /// Palette data address in ROM.
    /// </param>
    void EntitySet::LoadSubPalettes(int startPaletteId, int paletteNum, int paletteSetPtr)
    {
        for (int i = 0; i < paletteNum; ++i)
        {
            if (palettes[i + startPaletteId].size())
                palettes[i + startPaletteId].clear();
            // First color is transparent
            palettes[i + startPaletteId].push_back(0);
            int subPalettePtr = paletteSetPtr + i * 32;
            for (int j = 1; j < 16; ++j)
            {
                unsigned short color555 = *(unsigned short *) (ROMUtils::CurrentFile + subPalettePtr + j * 2);
                int r = ((color555 << 3) & 0xF8) | ((color555 >> 2) & 3);
                int g = ((color555 >> 2) & 0xF8) | ((color555 >> 7) & 3);
                int b = ((color555 >> 7) & 0xF8) | ((color555 >> 13) & 3);
                int a = 0xFF;
                palettes[i + startPaletteId].push_back(QColor(r, g, b, a).rgba());
            }
        }
    }

    /// <summary>
    /// sub function used in EntitySet constructor for loading Tile8x8s for each Entity.
    /// </summary>
    /// <param name="tileaddress">
    /// Address of Entity tiles in ROM.
    /// </param>
    /// <param name="datalength">
    /// Length of Tiles' data.
    /// </param>
    /// <param name="startrow">
    /// The row number to load new Entity Tiles.
    /// </param>
    void EntitySet::LoadSpritesTiles(int tileaddress, int datalength, int startrow)
    {
        for (int i = 0; i < (datalength / 32); ++i)
        {
            tile8x8data[i + startrow * 32] = new Tile8x8(tileaddress + i * 32, palettes);
        }
    }

    /// <summary>
    /// Construct an instance of EntitySet.
    /// </summary>
    /// <param name="_EntitySetID">
    /// Entity set ID.
    /// </param>
    /// <param name="basicElementPalettePtr">
    /// Basic Universal Entities' palettes ptr which saved in Tileset.
    /// </param>
    EntitySet::EntitySet(int _EntitySetID, int basicElementPalettePtr) : EntitySetID(_EntitySetID)
    {
        int entitysetptr = ROMUtils::PointerFromData(WL4Constants::EntitySetInfoPointerTable + _EntitySetID * 4);

        // Load 16 color palettes, ignore the first 3 rows, they are only for wario tiles
        int palettePtr, lastpalettePtr;
        palettePtr = lastpalettePtr = 0;
        int tmpEntityId;
        int EntityPaletteNum;
        int k = 0;
        int currentpaletteID = 0;
        do // Load palette 8 - 14 if exist for entities
        {
            tmpEntityId = (int) ROMUtils::CurrentFile[entitysetptr + 2 * k];
            if ((tmpEntityId > 0x10))
            {
                palettePtr =
                    ROMUtils::PointerFromData(WL4Constants::EntityPalettePointerTable + 4 * (tmpEntityId - 0x10));
                EntityPaletteNum =
                    ROMUtils::IntFromData(WL4Constants::EntityTilesetLengthTable + 4 * (tmpEntityId - 0x10)) /
                    (32 * 32 * 2);
                if (lastpalettePtr != palettePtr)
                {
                    LoadSubPalettes(8 + currentpaletteID, EntityPaletteNum, palettePtr);
                    currentpaletteID += EntityPaletteNum;
                }
                lastpalettePtr = palettePtr;
            }
            k++;
        } while ((tmpEntityId != 0) && (currentpaletteID != 8));
        // Set palette before and not include 15 to be 0 if not exist
        if (currentpaletteID < 7)
        {
            for (int i = (8 + currentpaletteID); i < 15; ++i)
            {
                for (int j = 0; j < 16; ++j)
                {
                    palettes[i].push_back(0);
                }
            }
        }
        // Load palette 3 - 7 for Basic Element used in the room
        LoadSubPalettes(3, 1, basicElementPalettePtr);
        LoadSubPalettes(4, 4, WL4Constants::UniversalSpritesPalette2);
        // Load palette 15 for treasure boxes if necessary
        if (currentpaletteID <= 7)
            LoadSubPalettes(15, 1, ROMUtils::PointerFromData(WL4Constants::EntityPalettePointerTable));
        // Set palette 0 - 2 all to 0 for Wario Sprites only
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 16; ++j)
            {
                palettes[i].push_back(0);
            }
        }

        // Load 1024 sprites tiles, ignore the first 4 rows, they are wario tiles
        memset(tile8x8data, 0, sizeof(tile8x8data));

        // Load Basic Universal Entities' tile8x8s.
        int tiledataptr, tiledatalength;
        tiledataptr = WL4Constants::SpritesBasicElementTiles;
        tiledatalength = 0x3000;
        LoadSpritesTiles(tiledataptr, tiledatalength, 4);

        // Load Entities' tile8x8s which differ amongst all entitysets
        k = 0;
        int currentrow = 16;
        int lasttiledataptr = 0;
        do
        {
            tmpEntityId = (int) ROMUtils::CurrentFile[entitysetptr + 2 * k];
            if (tmpEntityId == 0)
                break;
            EntitySetinfoTableElement Tmp_entitytableElement;
            Tmp_entitytableElement.Global_EntityID = tmpEntityId;
            Tmp_entitytableElement.paletteOffset = (int) ROMUtils::CurrentFile[entitysetptr + 2 * k + 1];
            EntityinfoTable.push_back(Tmp_entitytableElement);
            tiledataptr = ROMUtils::PointerFromData(WL4Constants::EntityTilesetPointerTable + 4 * (tmpEntityId - 0x10));
            tiledatalength = ROMUtils::IntFromData(WL4Constants::EntityTilesetLengthTable + 4 * (tmpEntityId - 0x10));
            if (tiledataptr != lasttiledataptr)
            {
                LoadSpritesTiles(tiledataptr, tiledatalength, currentrow);
                currentrow += tiledatalength / (32 * 32);
            }
            k++;
            lasttiledataptr = tiledataptr;
        } while (1);
        for (unsigned int i = 0; i < sizeof(tile8x8data) / sizeof(tile8x8data[0]); ++i)
        {
            if (!tile8x8data[i])
                tile8x8data[i] = Tile8x8::CreateBlankTile(palettes);
        }

        // Load Treasure/CD Boxes tile8x8s when this Entityset is not a Boss Entityset
        if ((!IncludeBossTiles()) && (currentrow < 31))
        {
            //            tiledataptr = ROMUtils::PointerFromData(WL4Constants::EntityTilesetPointerTable);
            //            tiledatalength = ROMUtils::IntFromData(WL4Constants::EntityTilesetLengthTable);
            //            LoadSpritesTiles(tiledataptr, tiledatalength, 30);
            LoadSpritesTiles(WL4Constants::TreasureBoxGFXTiles, 2048, 30);
        }

        // TODO: set other entity information
    }

    /// <summary>
    /// Get Entity palette offset by local entity id.
    /// </summary>
    /// <param name="_entityID">
    /// Entity local id.
    /// </param>
    /// <param name="entityglobalId">
    /// Entity global id.
    /// </param>
    int EntitySet::GetEntityPaletteOffset(int _entityID, int entityglobalId)
    {
        if (_entityID == -1) // TODO: find what the game does
        {
            return 0;
        }
        else
        {
            int Paloffset = EntityinfoTable[_entityID].paletteOffset + 8;

            // these Entities have an extra relative palette offset
            if ((entityglobalId == 0x12) || (entityglobalId == 0x1D) || (entityglobalId == 0x2B) ||
                (entityglobalId == 0x30) || (entityglobalId == 0x31) || (entityglobalId == 0x32) ||
                (entityglobalId == 0x33) || (entityglobalId == 0x34) || (entityglobalId == 0x35) ||
                (entityglobalId == 0x36) || (entityglobalId == 0x37) || (entityglobalId == 0x38) ||
                (entityglobalId == 0x39))
            {
                Paloffset++;
            }
            if (entityglobalId == 0x13)
            {
                Paloffset += 2;
            }

            return Paloffset;
        }
    }

    /// <summary>
    /// Get Entity tileid offset by local entity id.
    /// </summary>
    /// <param name="_entityID">
    /// Entity local id.
    /// </param>
    int EntitySet::GetEntityTileIdOffset(int _entityID)
    {
        if (_entityID == -1) // TODO: find what the game does
        {
            return 0;
        }
        else
        {
            return 64 * (EntityinfoTable[_entityID].paletteOffset);
        }
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
    /// Check if an Boss entity is inside this Entityset.
    /// </summary>
    bool EntitySet::IncludeBossTiles()
    {
        return IsEntityInside(0x18) || IsEntityInside(0x2C) || IsEntityInside(0x51) || IsEntityInside(0x69) ||
               IsEntityInside(0x76) || IsEntityInside(0x7D);
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

    /// <summary>
    /// Find an EntitySet which includes the entity with id.
    /// </summary>
    /// <param name="entityglobalId">
    /// Entity global id.
    /// </param>
    EntitySetAndEntitylocalId EntitySet::EntitySetFromEntityID(int entityglobalId)
    {
        struct EntitySetAndEntitylocalId tmpEntitySetAndEntitylocalId;
        if (entityglobalId < 0x11)
        {
            tmpEntitySetAndEntitylocalId.entitysetId = 1;
            tmpEntitySetAndEntitylocalId.entitylocalId = -1;
            return tmpEntitySetAndEntitylocalId;
        }
        for (int j = 1; j < 90; ++j)
        {
            int entitysetptr = ROMUtils::PointerFromData(WL4Constants::EntitySetInfoPointerTable + 4 * j);
            int i = 0;
            while (ROMUtils::CurrentFile[entitysetptr + 2 * i] != (unsigned char) 0)
            {
                unsigned char *entityidtmp = ROMUtils::CurrentFile + entitysetptr + 2 * i;
                if (*entityidtmp == (unsigned char) entityglobalId)
                {
                    tmpEntitySetAndEntitylocalId.entitysetId = j;
                    tmpEntitySetAndEntitylocalId.entitylocalId = i;
                    return tmpEntitySetAndEntitylocalId;
                }
                i++;
            }
        }
        memset(&tmpEntitySetAndEntitylocalId, 0, sizeof(tmpEntitySetAndEntitylocalId));
        return tmpEntitySetAndEntitylocalId; // TODO: Error handling
    }

    /// <summary>
    /// Get the first action set of a choosed entity by its global id.
    /// </summary>
    /// <param name="entityglobalId">
    /// Entity global id.
    /// </param>
    int EntitySet::GetEntityFirstActionFrameSetPtr(int entityglobalId)
    {
        return EntitiesFirstActionFrameSetsPtrsData[entityglobalId];
    }

    /// <summary>
    /// Get Entity Positional offset by its global id.
    /// </summary>
    /// <param name="entityglobalId">
    /// Entity global id.
    /// </param>
    EntityPositionalOffset EntitySet::GetEntityPositionalOffset(int entityglobalId)
    {
        EntityPositionalOffset tmpEntityPositionalOffset;
        tmpEntityPositionalOffset.XOffset = EntityPositinalOffset[2 * entityglobalId];
        tmpEntityPositionalOffset.YOffset = EntityPositinalOffset[2 * entityglobalId + 1];
        return tmpEntityPositionalOffset;
    }

    /// <summary>
    /// Use one of the palette to render the whole Entityset for testing.
    /// </summary>
    /// <param name="paletteId">
    /// palette id.
    /// </param>
    /// <returns>
    /// the EntitySet rendered a pixmap.
    /// </returns>
    QPixmap EntitySet::GetPixmap(int paletteId)
    {
        QPixmap pixmap(8 * 32, 8 * 32);
        pixmap.fill(Qt::transparent);
        for (int L = 0; L < 32; ++L)
        {
            for (int num = 0; num < 32; ++num)
            {
                tile8x8data[num + 32 * L]->SetPaletteIndex(paletteId);
                tile8x8data[num + 32 * L]->DrawTile(&pixmap, num * 8, L * 8);
            }
        }
        return pixmap;
    }

    /// <summary>
    /// Deconstruct an instance of EntitySet.
    /// </summary>
    EntitySet::~EntitySet()
    {
        // Delete Tile8x8 information
        // BlankTile may be disjoint in the array, so we skip over those and delete it afterward
        for (unsigned int i = 0; i < sizeof(tile8x8data) / sizeof(tile8x8data[0]); ++i)
        {
            delete tile8x8data[i];
        }
    }
} // namespace LevelComponents
