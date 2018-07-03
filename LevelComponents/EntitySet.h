#ifndef ENTITYSET_H
#define ENTITYSET_H

#include "ROMUtils.h"

namespace LevelComponents
{
    class EntitySet
    {
    private:
        int EntitySetID;  //maximun 89 (from 0 to 89)
    public:
        EntitySet(int _EntitySetID);
    };
}

#endif // ENTITYSET_H
