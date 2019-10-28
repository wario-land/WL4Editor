# ROM Mapping

| Offset   | Description | Type | Palette | Width | Height | Image
| -------- | ----------- | ---- | ------- | ----- | ------ | -----
| 0x283F54 | Nintendo logo | compressed 8bit | 0x283F14 | 13 | - | ![Image](/images/rom-mapping/Nintendo-Logo.png)
| 0x285A40 | A few days later. | compressed 4bit | - | 32 | - | ![Image](/images/rom-mapping/A-Few-Days-Later.png)
| 0x28B234 | Hangar | compressed indexed bitmap | 0x28B210 | 30 | 16 | ![Image](/images/rom-mapping/Compressed-Hangar.png)
| 0x28C730 | Intro hangar sprites | compressed 4bit | 0x28C570 index: 0,1,2 | 32 | - | ![Image](/images/rom-mapping/Intro-Hangar-Sprites.png)
| 0x28E880 | Legendary pyramid newspaper | compressed 4bit | 0x28E780 index: 0,1,2 | 32 | - | ![Image](/images/rom-mapping/Legendary-Pyramid-Newspaper.png)
| 0x295954 | Wario logo | compressed 4bit | (0x2943BC), 0x2943DC, 0x2943FC, (0x29441C) | 24 | 18 | ![Image](/images/rom-mapping/Compressed-Wario-Logo.png)
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
| 0x405288 | Health bar | uncompressed 4bit | 0x400AC8 | 8 | 19 | ![Image](/images/rom-mapping/Health-Bar.png) Palette of blue crystal (10) first occurence 00583E7C
| ~0x418A1C | Tileset of Hall of Hieroglyph | uncompressed 4bit | 0x583DDC Pipe of Hall of Hieroglyph palette | - | - | -
| ~0x47D87C | Tileset of Block Tower | uncompressed 4bit | - | - | - | -
| 0x6428D8 | Overworld with passages icons | uncompressed 4bit | First passage palette 6429F8 | - | - | -
| 0x64C8C4 | All level and boss icons | uncompressed 4bit | 6A0A48 (level selection) 65EE1C (starting level) 6A0E08 (level selection black and white) 6616DC (starting level black and white) [? 6617BC 69FF88 6A0188 6A0388 6A0588 6A0788 6A0988 74287C] 1st Level  | 32 | 30 | -
| 0x68CF5C | Pyramid | Indexed bitmap | near 646358 somewhere | - | - | -
| 0x6B4288 | CDroms player screen with icons | uncompressed 4bit | 6B41E8 (From 6B4088 to 6B4288) | 32 | 34 | -
| 0x73D764 | Menu | compressed 4bit | - | 32 | - | -
| 0x7409C0 | Menutext | compressed 4bit | - | 32 | - | -
| 0x74140C | Menutext(2) | - | - | - | - | -
| 0x741D00 | Menutext(3) | - | - | - | - | -
| 0x7430FC | Menutext(4) | - | - | - | - | -
| 0x745350 | Menutext(5) | - | - | - | - | -
| 0x745B58 | Menutext(6) | - | - | - | - | -
| 0x74C3D0 | End Paintings (Cractus) | uncompressed 4bit | 0x74C3B0 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-Cractus.png)
| 0x74FFF0 | End Paintings (Forest Apple) | uncompressed 4bit | 0x74FFD0 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-Forest-Apple.png)
| 0x753C10 | End Paintings (Wario swimming) | uncompressed 4bit | 0x753BF0 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-Swimming.png)
| 0x757830 | End Paintings (Wario holding monster) | uncompressed 4bit | 0x757810 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-Holding-Monster.png)
| 0x75B450 | End Paintings (Wario crescent moon village) | uncompressed 4bit | 0x75B430 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-Crescent-Moon-Village.png)
| 0x75F070 | End Paintings (Wario sleep moon) | uncompressed 4bit | 0x75F050 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-Sleep-Moon.png)
| 0x762C90 | End Paintings (Wario toy) | uncompressed 4bit | 0x762C70 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-Toy.png)
| 0x7668B0 | End Paintings (Wario Hoggus) | uncompressed 4bit | 0x766890 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-Hoggus.png)
| 0x76A4D0 | End Paintings (Wario GBA) | uncompressed 4bit | 0x76A4B0 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-GBA.png)
| 0x76E0F0 | End Paintings (Wario Robot) | uncompressed 4bit | 0x76E0D0 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-Robot.png)
| 0x771D10 | End Paintings (Wario space rocket) | uncompressed 4bit | 0x771CF0 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-Space-Rocket.png)
| 0x775930 | End Paintings (Wario Cuckoo Condor) | uncompressed 4bit | 0x775910 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-Cuckoo-Condor.png)
| 0x779550 | End Paintings (Wario Sport Car) | uncompressed 4bit | 0x779530 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-Sport-Car.png)
| 0x77D170 | End Paintings (Wario two women) | uncompressed 4bit | 0x77D150 | 32 | 15 | ![Image](/images/rom-mapping/End-Painting-Wario-Two-Women.png)