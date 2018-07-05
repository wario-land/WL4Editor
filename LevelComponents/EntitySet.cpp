#include "EntitySet.h"

namespace LevelComponents
{
void EntitySet::SetPalettes(int startPaletteId, int paletteNum, int paletteSetPtr)
{
    for(int i = 0; i < paletteNum; ++i)
    {
        // First color is transparent
        palettes[i + startPaletteId].push_back(0);
        int subPalettePtr = paletteSetPtr + i * 32;
        for(int j = 1; j < 16; ++j)
        {
            unsigned short color555 = *(unsigned short*) (ROMUtils::CurrentFile + subPalettePtr + j * 2);
            int r = ((color555 << 3) & 0xF8) | ((color555 >> 2) & 3);
            int g = ((color555 >> 2) & 0xF8) | ((color555 >> 7) & 3);
            int b = ((color555 >> 7) & 0xF8) | ((color555 >> 13) & 3);
            int a = 0xFF;
            palettes[i + startPaletteId].push_back(QColor(r, g, b, a).rgba());
        }
    }
}

EntitySet::EntitySet(int _EntitySetID, int basicElementPalettePtr)
    {
        this->EntitySetID = _EntitySetID;
        int entitysetptr = ROMUtils::PointerFromData(WL4Constants::EntitySetInfoPointerTable + _EntitySetID * 4);

        // Create all 16 color palettes
        int palettePtr;
        int tmpEntityId;
        int EntityPaletteNum;
        int k = 0;
        int currentpaletteID = 0;
        do  // Load palette 8 - 14 if exist for entities
        {
            tmpEntityId = (int) ROMUtils::CurrentFile[entitysetptr + 2 * k];
            EntityPaletteNum = (int) ROMUtils::CurrentFile[entitysetptr + 2 * k + 1];
            if((tmpEntityId > 0x10) && (EntityPaletteNum != currentpaletteID))
            {
                palettePtr = ROMUtils::PointerFromData(WL4Constants::EntityPalettePointerTable + 4 * (tmpEntityId - 0x10));
                SetPalettes(8, EntityPaletteNum - currentpaletteID, palettePtr);
                currentpaletteID = EntityPaletteNum;
            }
            k++;
        }while((tmpEntityId != 0) && (currentpaletteID != 7));
        if(currentpaletteID < 7) // Set palette before and not include 15 to be 0 if not exist
        {
            for(int i = (8 + currentpaletteID); i < 15; ++i)
            {
                for(int j = 1; j < 16; ++j)
                {
                    palettes[i].push_back(0);
                }
            }
        }
        SetPalettes(3, 5, basicElementPalettePtr); // Load palette 3 - 7 for Basic Element used in the room
        SetPalettes(15, 1, ROMUtils::PointerFromData(WL4Constants::EntityPalettePointerTable)); // Load palette 15 for treasure boxes
        for(int i = 0; i < 2; ++i) // Set palette 0 - 2 all to 0 for Wario Sprites only
        {
            for(int j = 1; j < 16; ++j)
            {
                palettes[i].push_back(0);
            }
        }

        // TODO
    }
}
