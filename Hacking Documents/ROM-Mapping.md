# ROM Mapping

| Offset   | Description | Type | Palette | Width | Height | Image
| -------- | ----------- | ---- | ------- | ----- | ------ | -----
| 0x283F54 | Nintendo logo | compressed 8bit | 0x283F14 | 13 | - | ![Image](/images/rom-mapping/Nintendo-Logo.png)
| 0x285A40 | A few days later. | compressed 4bit | - | 32 | - | ![Image](/images/rom-mapping/A-Few-Days-Later.png)
| 0x28B234 | Hangar | Indexed bitmap | 0x28B210 | 30 | - | -
| 0x28C730 | Intro hangar sprites | compressed 4bit | 0x28C570 index: 0,1,2 | 32 | - | ![Image](/images/rom-mapping/Intro-Hangar-Sprites.png)
| 0x28E880 | Legendary pyramid newspaper | compressed 4bit | 0x28E780 index: 0,1,2 | 32 | - | ![Image](/images/rom-mapping/Legendary-Pyramid-Newspaper.png)
| 0x295954 | Wario logo | compressed 4bit | 0x2943DC, 0x2943FC | 24 | - | -
| 0x29D4D0 | Wario sprites (1) | compressed 4bit | 0x2DDDA0 | 32 | -| ![Image](/images/rom-mapping/Wario-Sprites-1.png)
| 0x2A37C0 | Hole | compressed 4bit | 0x2A37A0 | 16 | - | ![Image](/images/rom-mapping/Hole.png)
| 0x2A4E3C | Wario sprites (2) | compressed 4bit | 0x2DDDA0 | 32 | - | -
| 0x2A53F8 | Wario sprites (3) | compressed 4bit | 0x2DDDA0 | 32 | - | -
| 0x2D41D4 | STEAKS! | compressed 4bit | 0x2D40F4 | 32 | - | -
| 0x2D8390 | The End + push start | compressed 4bit | 0x2D830E (?) index: 0,1,3 | 32 | - | -
| 0x2D9D18 | Intense mode (probably unused) | 4bit | 0x2D9C78 | - | - | -
| 0x368CF0 | Spoiled Rotten & cie | uncompressed 4bit | 0x3B1A70 | 32 | 16 | ![Image](/images/rom-mapping/Spoiled-Rotten-1.png)
| ~0x371C30| Object from Block Tower (1) | - | - | - | - | -
| 0x3754F0 | Object from Block Tower (2) | - | 0x3B1D90 | - | - | -
| 0x400CE8 | Empty health bar |  uncompressed 4bit |0x400AC8 | 8 | 1 | ![Image](/images/rom-mapping/Empty-Health.png)
| 0x405288 | Health bar | uncompressed 4bit | 0x400AC8 | 8 | 19 | ![Image](/images/rom-mapping/Health-Bar.png)
| ~0x47D87C | Tileset of Block Tower | uncompressed 4bit | - | - | - | -
| 0x64C8C4 | All level and boss icons | uncompressed 4bit | - | 32 | 30 | -
| 0x68CF5C | Pyramid | Indexed bitmap | near 646358 somewhere | - | - | -
| 0x6B4288 | CDroms player screen with icons | uncompressed 4bit | 6B41E8 (From 6B4088 to 6B4288) | 32 | 34 | -
| 0x73D764 | Menu | compressed 4bit | - | 32 | - | -
| 0x7409C0 | Menutext | compressed 4bit | - | 32 | - | -
| 0x74140C | Menutext(2) | - | - | - | - | -
| 0x741D00 | Menutext(3) | - | - | - | - | -
| 0x7430FC | Menutext(4) | - | - | - | - | -
| 0x745350 | Menutext(5) | - | - | - | - | -
| 0x745B58 | Menutext(6) | - | - | - | - | -
| 0x74C3D0 | End Paintings | - | - | - | - | -
