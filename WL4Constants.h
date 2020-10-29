#ifndef WL4CONSTANTS_H
#define WL4CONSTANTS_H

#define WL4EDITOR_VERSION "beta-12"

namespace WL4Constants
{
    // Definitions for the beginning of tables
    const unsigned int TilesetDataTable              = 0x3F2298;
    const unsigned int LevelHeaderTable              = 0x639068;
    const unsigned int LevelHeaderIndexTable         = 0x6391C4;
    const unsigned int LevelNamePointerTable         = 0x63A3AC;
    const unsigned int LevelNameJPointerTable        = 0x63A31C;
    const unsigned int DoorTable                     = 0x78F21C;
    const unsigned int RoomDataTable                 = 0x78F280;
    const unsigned int CameraControlPointerTable     = 0x78F540;
    const unsigned int EntitySetInfoPointerTable     = 0x78EF78;
    const unsigned int EntityTilesetPointerTable     = 0x78EBF0;
    const unsigned int EntityPalettePointerTable     = 0x78EDB4;
    const unsigned int EntityTilesetLengthTable      = 0x3B2C90;
    const unsigned int AnimatedTileIdTableSwitchOff  = 0x3F8098;
    const unsigned int AnimatedTileIdTableSwitchOn   = 0x3F91D8;
    const unsigned int AnimatedTileSwitchInfoTable   = 0x3F8C18;

    // Miscellaneous definitions
    const unsigned int CameraRecordSentinel        = 0x3F9D58;
    const unsigned int SpritesBasicElementTiles    = 0x400AE8; // 0x3000 bytes in length
    const unsigned int BGLayerDefaultPtr           = 0x58DA7C;
    const unsigned int NormalLayerDefaultPtr       = 0x3F2263;
    const unsigned int ToxicLandfillDustyLayer0Ptr = 0x601854;
    const unsigned int FieryCavernDustyLayer0Ptr   = 0x60D934;
    const unsigned int UniversalSpritesPalette     = 0x556DDC;
    const unsigned int UniversalSpritesPalette2    = 0x400A68;
    const unsigned int TreasureBoxGFXTiles         = 0x352CF0;

    // Other
    const unsigned int AvailableSpaceBeginningInROM = 0x78F970;

    // BG addresses
    const unsigned int BG_0x5FA6D0 = 0x5FA6D0;
    const unsigned int BG_0x5FB2CC = 0x5FB2CC;
    const unsigned int BG_0x5FB8DC = 0x5FB8DC;
    const unsigned int BG_0x5FC2D0 = 0x5FC2D0;
    const unsigned int BG_0x5FC9A0 = 0x5FC9A0;
    const unsigned int BG_0x5FD078 = 0x5FD078;
    const unsigned int BG_0x5FD484 = 0x5FD484;
    const unsigned int BG_0x5FD680 = 0x5FD680;
    const unsigned int BG_0x5FD9BC = 0x5FD9BC;
    const unsigned int BG_0x5FE008 = 0x5FE008;
    const unsigned int BG_0x5FE540 = 0x5FE540;
    const unsigned int BG_0x5FE918 = 0x5FE918;
    const unsigned int BG_0x5FEED8 = 0x5FEED8;
    const unsigned int BG_0x5FF264 = 0x5FF264;
    const unsigned int BG_0x5FF684 = 0x5FF684;
    const unsigned int BG_0x5FF960 = 0x5FF960;
    const unsigned int BG_0x5FFD94 = 0x5FFD94;
    const unsigned int BG_0x600388 = 0x600388;
    const unsigned int BG_0x6006C4 = 0x6006C4;
    const unsigned int BG_0x600EF8 = 0x600EF8;
    const unsigned int BG_0x6013D4 = 0x6013D4;
    const unsigned int BG_0x601A0C = 0x601A0C;
    const unsigned int BG_0x60221C = 0x60221C;
    const unsigned int BG_0x602858 = 0x602858;
    const unsigned int BG_0x603064 = 0x603064;
    const unsigned int BG_0x60368C = 0x60368C;
    const unsigned int BG_0x603E98 = 0x603E98;
    const unsigned int BG_0x6045C4 = 0x6045C4;
    const unsigned int BG_0x604ACC = 0x604ACC;
    const unsigned int BG_0x605270 = 0x605270;
    const unsigned int BG_0x605A7C = 0x605A7C;
    const unsigned int BG_0x6063B0 = 0x6063B0;
    const unsigned int BG_0x606CF4 = 0x606CF4;
    const unsigned int BG_0x6074C4 = 0x6074C4;
    const unsigned int BG_0x607CD0 = 0x607CD0;
    const unsigned int BG_0x6084DC = 0x6084DC;
    const unsigned int BG_0x608CE8 = 0x608CE8;
    const unsigned int BG_0x6094F4 = 0x6094F4;
    const unsigned int BG_0x609A84 = 0x609A84;
    const unsigned int BG_0x60A1D8 = 0x60A1D8;
    const unsigned int BG_0x60A1E8 = 0x60A1E8;
    const unsigned int BG_0x60A350 = 0x60A350;
    const unsigned int BG_0x60A4B4 = 0x60A4B4;
    const unsigned int BG_0x60AD10 = 0x60AD10;
    const unsigned int BG_0x60B29C = 0x60B29C;
    const unsigned int BG_0x60BC54 = 0x60BC54;
    const unsigned int BG_0x60C5FC = 0x60C5FC;
    const unsigned int BG_0x60CF98 = 0x60CF98;
    const unsigned int BG_0x60E044 = 0x60E044;
    const unsigned int BG_0x60E860 = 0x60E860;
    const unsigned int BG_0x60E870 = 0x60E870;
    const unsigned int BG_0x60E96C = 0x60E96C;
    const unsigned int BG_0x60ED78 = 0x60ED78;
}

#endif // WL4CONSTANTS_H
