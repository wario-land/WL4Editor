#include "Tile.h"

namespace LevelComponents
{
    Tile8x8::Tile8x8(int dataPtr) : Tile8x8()
    {
        // TODO
    }

    Tile8x8 *Tile8x8::CreateBlankTile()
    {
        Tile8x8 *t = new Tile8x8();
        for(int i = 0; i < 8; ++i)
        {
            for(int j = 0; j < 8; ++j)
            {
                t->ImageData->setPixel(i, j, 0);
            }
        }
        return t;
    }

    void Tile8x8::DrawTile(int x, int y)
    {
        // TODO
    }
}
