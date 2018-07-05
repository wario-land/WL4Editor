#ifndef ENTITYSET_H
#define ENTITYSET_H

#include "ROMUtils.h"

#include <QVector>
#include <QColor>

namespace LevelComponents
{
    class EntitySet
    {
    private:
        int EntitySetID;  //maximun 89 (from 0 to 89)
        QVector<QRgb> palettes[16];
        void SetPalettes(int startPaletteId, int paletteNum, int paletteSetPtr);
    public:
        EntitySet(int _EntitySetID, int basicElementPalettePtr);
    };
}

#endif // ENTITYSET_H
