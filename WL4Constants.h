#ifndef WL4CONSTANTS_H
#define WL4CONSTANTS_H

#define WL4EDITOR_VERSION "0.15.0"

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
    const unsigned int AnimatedTileHeaderTable       = 0x3F7828; // 270 slots, i.e. 0x10E slots

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
    const unsigned int CreditsTiles                = 0x789FCC;

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

    // vanilla BG Tile8x8 sets address
    const unsigned int Tileset_BGTile_default_1 = 0x54b0bc;
    const unsigned int Tileset_BGTile_default_1_size = 0x1840;

    const unsigned int Tileset_BGTile_0x00 = 0x4ea9bc;
    const unsigned int Tileset_BGTile_0x01 = 0x4ea9bc;
    const unsigned int Tileset_BGTile_0x02 = 0x4ee6dc;
    const unsigned int Tileset_BGTile_0x03 = 0x527adc;
    const unsigned int Tileset_BGTile_0x04 = 0x527adc;
    const unsigned int Tileset_BGTile_0x05 = 0x529c9c;
    const unsigned int Tileset_BGTile_0x06 = 0x4f1d3c;
    const unsigned int Tileset_BGTile_0x07 = 0x52bc3c;
    const unsigned int Tileset_BGTile_0x08 = 0x510abc;
    const unsigned int Tileset_BGTile_0x09 = 0x4f1d3c;
    const unsigned int Tileset_BGTile_0x0A = 0x4f6f5c;
    const unsigned int Tileset_BGTile_0x0B = 0x4f735c;
    const unsigned int Tileset_BGTile_0x0C = 0x52bc3c;
    const unsigned int Tileset_BGTile_0x0D = 0x52bc3c;
    const unsigned int Tileset_BGTile_0x0E = 0x52cc1c;
    const unsigned int Tileset_BGTile_0x0F = 0x53033c;
    const unsigned int Tileset_BGTile_0x10 = 0x532f5c;
    const unsigned int Tileset_BGTile_0x11 = 0x4e851c;
    const unsigned int Tileset_BGTile_0x12 = 0x538f3c;
    const unsigned int Tileset_BGTile_0x13 = 0x53ab1c;
    const unsigned int Tileset_BGTile_0x14 = 0x53c65c;
    const unsigned int Tileset_BGTile_0x15 = 0x533f5c;
    const unsigned int Tileset_BGTile_0x16 = 0x533f5c;
    const unsigned int Tileset_BGTile_0x17 = 0x533f5c;
    const unsigned int Tileset_BGTile_0x18 = 0x533f5c;
    const unsigned int Tileset_BGTile_0x19 = 0x533f5c;
    const unsigned int Tileset_BGTile_0x1A = 0x532f5c;
    const unsigned int Tileset_BGTile_0x1B = 0x532f5c;
    const unsigned int Tileset_BGTile_0x1C = 0x532f5c;
    const unsigned int Tileset_BGTile_0x1D = 0x51599c;
    const unsigned int Tileset_BGTile_0x1E = 0x51df5c;
    const unsigned int Tileset_BGTile_0x1F = 0x4fa07c;
    const unsigned int Tileset_BGTile_0x20 = 0x519f5c;
    const unsigned int Tileset_BGTile_0x21 = 0x522f9c;
    const unsigned int Tileset_BGTile_0x22 = 0x523efc;
    const unsigned int Tileset_BGTile_0x23 = 0x522f9c;
    const unsigned int Tileset_BGTile_0x24 = 0x522f9c;
    const unsigned int Tileset_BGTile_0x25 = 0x522f9c;
    const unsigned int Tileset_BGTile_0x26 = 0x4fee1c;
    const unsigned int Tileset_BGTile_0x27 = 0x52515c;
    const unsigned int Tileset_BGTile_0x28 = 0x4fd4fc;
    const unsigned int Tileset_BGTile_0x29 = Tileset_BGTile_default_1;
    const unsigned int Tileset_BGTile_0x2A = 0x53ec5c;
    const unsigned int Tileset_BGTile_0x2B = 0x53ec5c;
    const unsigned int Tileset_BGTile_0x2C = 0x53ec5c;
    const unsigned int Tileset_BGTile_0x2D = 0x53ec5c;
    const unsigned int Tileset_BGTile_0x2E = 0x53ec5c;
    const unsigned int Tileset_BGTile_0x2F = 0x53fc7c;
    const unsigned int Tileset_BGTile_0x30 = 0x53ec5c;
    const unsigned int Tileset_BGTile_0x31 = 0x53ec5c;
    const unsigned int Tileset_BGTile_0x32 = 0x52e39c;
    const unsigned int Tileset_BGTile_0x33 = 0x52e39c;
    const unsigned int Tileset_BGTile_0x34 = 0x4ee6dc;
    const unsigned int Tileset_BGTile_0x35 = 0x502f5c;
    const unsigned int Tileset_BGTile_0x36 = 0x50cb3c;
    const unsigned int Tileset_BGTile_0x37 = 0x54191c;
    const unsigned int Tileset_BGTile_0x38 = 0x507f5c;
    const unsigned int Tileset_BGTile_0x39 = 0x54efbc;
    const unsigned int Tileset_BGTile_0x3A = 0x53ec5c;
    const unsigned int Tileset_BGTile_0x3B = 0x53ec5c;
    const unsigned int Tileset_BGTile_0x3C = 0x53ec5c;
    const unsigned int Tileset_BGTile_0x3D = 0x53ec5c;
    const unsigned int Tileset_BGTile_0x3E = 0x526cfc;
    const unsigned int Tileset_BGTile_0x3F = 0x510abc;
    const unsigned int Tileset_BGTile_0x40 = 0x51df5c;
    const unsigned int Tileset_BGTile_0x41 = 0x53857c;
    const unsigned int Tileset_BGTile_0x42 = Tileset_BGTile_default_1;
    const unsigned int Tileset_BGTile_0x43 = 0x54e3bc;
    const unsigned int Tileset_BGTile_0x44 = Tileset_BGTile_default_1;
    const unsigned int Tileset_BGTile_0x45 = 0x54523c;
    const unsigned int Tileset_BGTile_0x46 = 0x54ab1c;
    const unsigned int Tileset_BGTile_0x47 = 0x4e851c;
    const unsigned int Tileset_BGTile_0x48 = Tileset_BGTile_default_1;
    const unsigned int Tileset_BGTile_0x49 = Tileset_BGTile_default_1;
    const unsigned int Tileset_BGTile_0x4A = 0x54e3bc;
    const unsigned int Tileset_BGTile_0x4B = 0x54e3bc;
    const unsigned int Tileset_BGTile_0x4C = 0x54e3bc;
    const unsigned int Tileset_BGTile_0x4D = 0x54e3bc;
    const unsigned int Tileset_BGTile_0x4E = 0x54e3bc;
    const unsigned int Tileset_BGTile_0x4F = 0x54e13c;
    const unsigned int Tileset_BGTile_0x50 = 0x4e851c;
    const unsigned int Tileset_BGTile_0x51 = 0x4fa07c;
    const unsigned int Tileset_BGTile_0x52 = 0x4f735c;
    const unsigned int Tileset_BGTile_0x53 = 0x53ab1c;
    const unsigned int Tileset_BGTile_0x54 = 0x53ab1c;
    const unsigned int Tileset_BGTile_0x55 = 0x53ab1c;
    const unsigned int Tileset_BGTile_0x56 = 0x52db1c;
    const unsigned int Tileset_BGTile_0x57 = 0x522f9c;
    const unsigned int Tileset_BGTile_0x58 = 0x4ea9bc;
    const unsigned int Tileset_BGTile_0x59 = 0x4ea9bc;
    const unsigned int Tileset_BGTile_0x5A = 0x54c8fc;
    const unsigned int Tileset_BGTile_0x5B = 0x529c9c;
}

#endif // WL4CONSTANTS_H
