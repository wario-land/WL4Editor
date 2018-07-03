#ifndef WL4CONSTANTS_H
#define WL4CONSTANTS_H

namespace WL4Constants
{
    // Definitions for the beginning of tables
    const int TilesetDataTable          = 0x3F2298;
    const int LevelHeaderTable          = 0x639068;
    const int LevelHeaderIndexTable     = 0x6391C4;
    const int LevelNamePointerTable     = 0x63A3AC;
    const int DoorTable                 = 0x78F21C;
    const int RoomDataTable             = 0x78F280;
    const int CameraControlPointerTable = 0x78F540;
    const int EntitySetInfoPointerTable = 0x78EF78;
    const int EntityTilesetPointerTable = 0x78EBF0;
    const int EntityPalettePointerTable = 0x78EDB4;

    // Miscellaneous definitions
    const int CameraRecordSentinel      = 0x3F9D58;

    // Other
    const int AvailableSpaceBeginningInROM       = 0x78F970;
}

#endif // WL4CONSTANTS_H
