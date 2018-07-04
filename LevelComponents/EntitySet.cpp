#include "EntitySet.h"

namespace LevelComponents
{
    EntitySet::EntitySet(int _EntitySetID)
    {
        this->EntitySetID = _EntitySetID;
        //entitysetptr = ROMUtils::PointerFromData(WL4Constants::EntitySetInfoPointerTable + _EntitySetID * 4);
    }
}
