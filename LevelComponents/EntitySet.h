#ifndef ENTITYSET_H
#define ENTITYSET_H

#include "ROMUtils.h"
#include "Tile.h"
#include "WL4Constants.h"

#include <QColor>
#include <QPixmap>
#include <QVector>
#include <vector>

namespace LevelComponents
{
    struct EntitySetinfoTableElement
    {
        int Global_EntityID;
        int paletteOffset;
    };

    struct EntitySetAndEntitylocalId
    {
        int entitysetId;
        int entitylocalId;
    };

    struct EntityPositionalOffset
    {
        int XOffset;
        int YOffset;
    };

    class EntitySet
    {
    public:
        EntitySet(int _EntitySetID, int basicElementPalettePtr);
        ~EntitySet();
        int GetEntitySetId() { return EntitySetID; }
        Tile8x8 **GetTileData() { return tile8x8data; }
        QVector<QRgb> *GetPalettes() { return palettes; }
        int GetEntityPaletteOffset(int _entityID, int entityglobalId);
        int GetEntityTileIdOffset(int _entityID);
        bool IsEntityInside(int entityglobalId);
        bool IncludeBossTiles();
        std::vector<EntitySetinfoTableElement> GetEntityTable();
        static EntitySetAndEntitylocalId EntitySetFromEntityID(int entityglobalId);
        static int GetEntityFirstActionFrameSetPtr(int entityglobalId);
        static EntityPositionalOffset GetEntityPositionalOffset(int entityglobalId);
        QPixmap GetPixmap(int paletteId);

    private:
        int EntitySetID; // from 0 to 89 inclusive in theory(??), but only from 0 to 82 inclusive are available
        QVector<QRgb> palettes[16];
        Tile8x8 *tile8x8data[0x480];
        std::vector<EntitySetinfoTableElement> EntityinfoTable;
        void LoadSubPalettes(int startPaletteId, int paletteNum, int paletteSetPtr);
        void LoadSpritesTiles(int tileaddress, int datalength, int startrow);

        // clang-format off
        static constexpr const unsigned int EntitiesFirstActionFrameSetsPtrsData[129] =
        {
            0,                               // 0x00
            WL4Constants::Entity01_0x3B4F94,
            WL4Constants::Entity02_0x3B4FA4,
            WL4Constants::Entity03_0x3B4F84,
            WL4Constants::Entity04_0x3B4F74,
            WL4Constants::Entity05_0x3B4F24,
            WL4Constants::Entity06_0x3B4FF4,
            WL4Constants::Entity07_0x3B62AC,
            WL4Constants::Entity08_0x3B59EC,
            WL4Constants::Entity09_0x3B492C,
            0,
            0,
            0,
            0,
            0,
            0,
            WL4Constants::Entity10_0x3DA17C, // 0x10
            WL4Constants::Entity11_0x3B416C,
            WL4Constants::Entity12_0x3B43DC,
            WL4Constants::Entity13_0x3B43DC,
            WL4Constants::Entity14_0x3B4534,
            WL4Constants::Entity15_0x3C4B18,
            WL4Constants::Entity16_0x3B75E8,
            WL4Constants::Entity17_0x3B8118,
            WL4Constants::Entity18_0x3B9AD0,
            WL4Constants::Entity19_0x3BB0E8,
            WL4Constants::Entity1A_0x3BB63C,
            WL4Constants::Entity1B_0x3BBD6C,
            WL4Constants::Entity1C_0x3BC570,
            WL4Constants::Entity1D_0x3BC570,
            WL4Constants::Entity1E_0x3BC8E4,
            WL4Constants::Entity1F_0x3BCEFC,
            WL4Constants::Entity20_0x3BD42C, // 0x20
            WL4Constants::Entity21_0x3BD660,
            WL4Constants::Entity22_0x3BDAF0,
            WL4Constants::Entity23_0x3BDD54,
            WL4Constants::Entity24_0x3BDFF0,
            WL4Constants::Entity25_0x3BDFF0,
            WL4Constants::Entity26_0x3BE1D4,
            WL4Constants::Entity27_0x3BEBF4,
            WL4Constants::Entity28_0x3BFB8C,
            WL4Constants::Entity29_0x3B505C,
            WL4Constants::Entity2A_0x3C1270,
            WL4Constants::Entity2B_0x3C1270,
            WL4Constants::Entity2C_0x3C302C,
            WL4Constants::Entity2D_0x3C491C,
            WL4Constants::Entity2E_0x3C48D4,
            WL4Constants::Entity2F_0x3C48F4,
            WL4Constants::Entity30_0x3C4174, // 0x30
            WL4Constants::Entity31_0x3C41F4,
            WL4Constants::Entity32_0x3C4274,
            WL4Constants::Entity33_0x3C4314,
            WL4Constants::Entity34_0x3C43B4,
            WL4Constants::Entity35_0x3C4444,
            WL4Constants::Entity36_0x3C44D4,
            WL4Constants::Entity37_0x3C4574,
            WL4Constants::Entity38_0x3C4614,
            WL4Constants::Entity39_0x3C46B4,
            WL4Constants::Entity3A_0x3C47D4,
            WL4Constants::Entity3B_0x3C959C,
            WL4Constants::Entity3C_0x3C4984,
            WL4Constants::Entity3D_0x3C4CD8,
            WL4Constants::Entity3E_0x3C4F20,
            WL4Constants::Entity3F_0x3C53B4,
            WL4Constants::Entity40_0x3C62FC, // 0x40
            WL4Constants::Entity41_0x3C7034,
            WL4Constants::Entity42_0x3C770C,
            WL4Constants::Entity43_0x3C798C,
            WL4Constants::Entity44_0x3C798C,
            WL4Constants::Entity45_0x3C799C,
            WL4Constants::Entity46_0x3C79AC,
            WL4Constants::Entity47_0x3C7A5C,
            WL4Constants::Entity48_0x3C7DB0,
            WL4Constants::Entity49_0x3C8278,
            WL4Constants::Entity4A_0x3C89F0,
            WL4Constants::Entity4B_0x3C89F0,
            WL4Constants::Entity4C_0x3C99E0,
            WL4Constants::Entity4D_0x3C9C20,
            WL4Constants::Entity4E_0x3CA178,
            WL4Constants::Entity4F_0x3CA898,
            WL4Constants::Entity50_0x3CA898, // 0x50
            WL4Constants::Entity51_0x3CE468,
            WL4Constants::Entity52_0x3B505C,
            WL4Constants::Entity53_0x3CFB58,
            WL4Constants::Entity54_0x3CFB48,
            WL4Constants::Entity55_0x3B4534,
            WL4Constants::Entity56_0x3B4534,
            WL4Constants::Entity57_0x3D0B08,
            WL4Constants::Entity58_0x3D0938,
            WL4Constants::Entity59_0x3D0988,
            WL4Constants::Entity5A_0x3D0A00,
            WL4Constants::Entity5B_0x3D0A28,
            WL4Constants::Entity5C_0x3D0B58,
            WL4Constants::Entity5D_0x3C0A28,
            WL4Constants::Entity5E_0x3D0DB4,
            WL4Constants::Entity5F_0x3D0E94,
            WL4Constants::Entity60_0x3D155C, // 0x60
            WL4Constants::Entity61_0x3D21E4,
            WL4Constants::Entity62_0x3D2570,
            WL4Constants::Entity63_0x3D27C8,
            WL4Constants::Entity64_0x3D2A0C,
            WL4Constants::Entity65_0x3D2BF0,
            WL4Constants::Entity66_0x3D2E1C,
            WL4Constants::Entity67_0x3D3048,
            WL4Constants::Entity68_0x3D3274,
            WL4Constants::Entity69_0x3DABA0,
            WL4Constants::Entity6A_0x3DC264,
            WL4Constants::Entity6B_0x3DC770,
            WL4Constants::Entity6C_0x3DCCBC,
            WL4Constants::Entity6D_0x3DD4CC,
            WL4Constants::Entity6E_0x3DD720,
            WL4Constants::Entity6F_0x3DD658,
            WL4Constants::Entity70_0x3DD668, // 0x70
            WL4Constants::Entity71_0x3DD678,
            WL4Constants::Entity72_0x3DDB14,
            WL4Constants::Entity73_0x3DE0E0,
            WL4Constants::Entity74_0x3DE320,
            WL4Constants::Entity75_0x3DE580,
            WL4Constants::Entity76_0x3DF2D0,
            WL4Constants::Entity77_0x3E0D68,
            WL4Constants::Entity78_0x3E1650,
            WL4Constants::Entity79_0x3E269C,
            WL4Constants::Entity7A_0x3E2A2C,
            WL4Constants::Entity7B_0x3E3878,
            WL4Constants::Entity7C_0x3E45F0,
            WL4Constants::Entity7D_0x3B59EC,
            WL4Constants::Entity7E_0x3F0F04,
            WL4Constants::Entity7F_0x3F122C,
            WL4Constants::Entity80_0x3F1AA0  // 0x80
        };

        static constexpr const int EntityPositinalOffset[258] =
        {
               0,    0, //0x00
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -67,  -70,
             -98,  -66,
            -130, -130,
               0,    0,
               0,    0,
               0,    0,
               0,    0,
               0,    0,
               0,    0,
             -98,  -66, //0x10
            -101,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -98,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66, //0x20
             -98,  -98,
             -98,  -66,
             -98,  -98,
             -98,  -98,
             -98,  -98,
             -98,  -78,
             -98,  -66,
             -67, -130,
             -98,  -98,
            -100,  -66,
            -100,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
            -130,  -66, //0x30
             -67,  -66,
            -130,  -66,
             -67,  -66,
            -130,  -66,
             -67,  -66,
            -130,  -66,
             -67,  -66,
            -130,  -66,
             -67,  -66,
            -130,  -66,
             -67,  -66,
             -98,  -66,
             -98,  -74,
             -98,  -66,
             -98,  -66,
            -100,  -98, //0x40
             -98,  -66,
             -98,  -66,
             -67,  -66,
             -67,  -66,
             -67,  -66,
             -67,  -66,
             -67,  -98,
            -100,  -66,
            -101,  -66,
             -98,  -66,
             -98,  -66,
            -101,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66, //0x50
             -98,   -3,
             -98,  -98,
             -98,  -66,
             -98,  -66,
             -98,  -98,
             -98,  -98,
             -98,  -66,
             -67, -130,
             -67, -130,
             -67, -130,
             -67, -130,
             -67, -130,
             -67, -130,
             -98,  -66,
             -35, -130,
             -98,  -66, //0x60
            -100,  -66,
             -67,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
            -194, -321,
             -98,  -66,
             -98,  -66,
             -98,  -66,
            -102,  -34,
             -98,  -66,
             -98,  -66,
             -98,  -66, //0x70
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
             -98,  -66,
            -100,  -66,
            -100,  -66,
            -101,  -66,
             -98,  -66,
            -100,  -66,
             -98,  -66,
               0,    0,
             -67,  -66,
             -98,  -98,
             -98,  -66  //0x80
        };
        // clang-format on
    };
} // namespace LevelComponents

#endif // ENTITYSET_H
