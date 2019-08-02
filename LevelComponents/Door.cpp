#include "Door.h"

namespace LevelComponents
{
    /// <summary>
    /// Determine if a door is unused.
    /// </summary>
    /// <return>
    /// it return a QPoint(xpos, ypos).
    /// </return>
    bool Door::IsUnused() { return DoorEntry.DoorTypeByte != Portal && DestinationDoor == nullptr; }

    /// <summary>
    /// Deep copy constructor for Door.
    /// </summary>
    /// <param name="door">
    /// The Door object to deep copy from.
    /// </param>
    Door::Door(Door &door) :
            RoomID(door.RoomID), DestinationDoor(door.DestinationDoor), is_vortex(door.is_vortex), DoorID(door.DoorID),
            DoorEntry(door.DoorEntry)
    {}

    /// <summary>
    /// Generate Wario's original position (unit: pixel) when appear from the door.
    /// </summary>
    /// <return>
    /// true if the door is unused.
    /// </return>
    QPoint Door::GetWarioOriginalPosition_x4()
    {
        int ypos, xpos;
        if (DoorEntry.DoorTypeByte == NormalDoor)
        {
            xpos = ((DoorEntry.x1 + 1) << 6) - 1;
            ypos = (DoorEntry.y2 + 1) << 6;
            // The ypos is related to current Wario animations, we only generate case for standing Wario
        }
        else
        {
            xpos = ((DoorEntry.x1 + 1) << 6) + 4 * (DoorEntry.HorizontalDelta + 8);
            ypos = ((DoorEntry.y2 + 1) << 6) + 4 * DoorEntry.VerticalDelta - 1;
        }
        QPoint WarioLeftTopPosition;
        WarioLeftTopPosition.setX(xpos);
        WarioLeftTopPosition.setY(ypos);
        return WarioLeftTopPosition;
    }
} // namespace LevelComponents
