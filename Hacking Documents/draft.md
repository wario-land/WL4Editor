>you can join my Discord [Server](https://discord.gg/easnNyB) to discuss the following things.

## About Room Header  

total length is 44 Bytes:  

|order in every Byte|usage|about|
|:-:|:-:|:-:|
|1|Tileset ID|-|
|2|mapping Type for Layer 0|0x00, 0x10, 0x20|
|3|mapping Type for Layer 1|must be 0x10|
|4|mapping Type for Layer 2|0x00, 0x10|
|5|mapping Type for Layer 3|0x20, 0x00|
|6-8|SBZ|-|
|9-12|pointer for Layer 0 compressed mapping code|-|
|13-16|pointer for Layer 1 compressed mapping code|-|
|17-20|pointer for Layer 2 compressed mapping code|-|
|21-24|pointer for bg Layer mapping code|-|
|25|camera limitation flag|0x01=when wario go up or down out of range, the camera will move one screen hight to keep up with wario<br>0x02=no limit in camera moving in both x and y direction<br>0x03=there is a control string to control the camera limit|
|26|set BG Layer scrolling|0x01=no, 0x07=yes, 0x03=BG Layer invisible|
|27|Layer priority setting and Layer 0 transparent setting and also for other Layer effect|too long to describe it here|
|28|SBZ(I forget it)|-|
|29-32|Pointer for Hard mode enemy laoding list|-|
|33-36|Pointer for Normal mode enemy laoding list|-|
|37-40|Pointer for S-Hard mode enemy laoding list|-|
|41|unknown|-|
|42|unknown|-|
|43|unknown|-|
|44|unknown|-|

P.S. I remember one of the Flag in [41] to [44] set the waving degree of one layer and BG layer in the back of water block, but I have not get it clear that, I just test one of the Room Headers in level "palmtree paradise".

## About Camera Limitation string  

|order in every Byte|usage|about|
|:-:|:-:|:-:|
|1|Room ID|just used for searching|
|2|record number|>=0x01|
|3|always 0x02|It seems the program use this to keep the camera from the edge of the map by 2 block in any dimension|
|4|x1|-|
|5|x2|-|
|6|y1|-|
|7|y2|-|
|8|x3|-|
|9|y3|-|
|10|offset|0x00 for x1, 0x01 for x2,<br> 0x02 for y1, 0x03 for y2|
|11|change to this|-|
|11+1 to 11+9|another record if necessary|-|
|11+9n+1 to 11+9n+9|another record if necessary|n>1|

the string means that: there is a basic limit of camera from (x1, y1) to (x2, y2), if you want an extend limitation mode then using Byte [8] to [11] or just set [8] to [11] all to 0xFF. the extend limitation mode is that: if the block in (x3, y3) isn't just air, then if wario break the block, then the whole limitation will change to a new range. and the new range is in the base of (x1, y1) to (x2, y2) and you only can change one value in set {x1, x2, y1, y2}. the Byte[10] given the offset of value you want to change and the value shold be change to Byte[11]. If there is not only one Camera Limitation string, you can just append them after the string and just add Byte[2] by 1.

## About Tileset Header  

the GBA game use `Tile` to construct map, and each map we need a `Tileset` to include the Tile we use in a room map. Tile size is 8 by 8 pixels and the GBA use RGB555 to save and display the color, so the depth is 5.

**mapping Type for Layers**

|Flag|about|
|:--------:|:----------:|
|`Flag=0x00`|use this only because you don't need to use this layer and the pointer for mapcoding is also fixed|
|`Flag=0x10`|4 Tiles (2 by 2) make a Block, and the map code by the Block ID|
|`Flag=0x20`|directly render the map with Tile|

**Tileset_header:**
- save the information for Tileset  
- start from `ROM Address = 3F2298` in ROM  
- total length=36 bytes, split them each by 4 bytes and each element should be a pointer or a length information

|order in every 4 Byte|usage|about|
|:-:|:-:|:-:|
|1|pointer for `Tile` graphic data|-|
|2|length of `Tile` graphic data|unit: byte|
|3|pointer for `Tile` palette|-|
|4|pointer for background `Tile` graphic data|BG Tile data and Tile data save in different place|
|5|length of BG `Tile` graphic data|-|
|6|pointer for Block data|including information about the construction of 4 Tiles, if we should turn them around before loaded, and palette ID in used|
|7|wario animation controller table pointer|a flag table for controlling wario animations by different proc for every Tile16, it also can reset Layer0 color blending params|
|8|event id table pointer|an id table of the event it trigger when wario knock into a Tile16|
|9|palettes for basic element contained in every room|5 or 7 lines of palettes, need to be checked|

## About Tile and Block in WL4  

first make it clear that, there is two different arrays I used in program class: `Tile[]`, `Block[]`   
Some of the `Tile ` are different from other, they have their animations, so the game load it in different ways.

**Don't forget to have a Byte exchange when you read data word by word from ROM.**  

the animation Tiles in VRAM were saved just before the static Tiles, and the number of them is fixed (16x4=64). you can extract these Tile out with a information list after `3F8098` in ROM.(there should be some other of them load from `3F91D8` but I don't know the condition yet)  

each record in the list contains 32 Bytes, you should read them word by word. double the value you get then plus  `3F7828` then you will find animation Tiles' Header for each animation Tile, the length of it is 8 Bytes. I don't know what the meaning in each Byte in animation Tiles' Header yet.

**you should load animation Tiles like this**  

> set r4=B1(Byte 1), r2=B8B7B6B5, r3=B3. if(`r3=3` or `r3=6`) then change `r3=r3-1` in other case set `r3=0`.
> `r2=r2-0x08000000+r3x128` , then r2 just get the offset to get Tiles. you should read the Tile data by exchange the high 4 bits and the low 4 bits when reading and just read 128 Bytes. append these data one by one in VRAM and you should load 64 Tiles in all.  

set the 64th Tile data all to 0x00 to make a universal blank block, then load the normal Tile one by one each in 32 Bytes all into array `Tile[]` (I don't know how to load a map in alignment with the pther layer so I don't want to load the BG Tiles into `Tile[]` array now)  

## loading of Block data  
the following tings are VB code, just to describe the way we load block data word by word.  
```
For i = 0 To 1023  'reserve 1024 word for block data
r0 = i * 4
r2 = r0 Or 1
Block(i) = Mid$(TextMapData, r0 * 2 * 2 + 1, 4)
’-------------------Mid$（str, i, j）get substr from str start from i and the length is j
r1 = (r2 + 1) * 2^16
Block(i) = Block(i) & Mid$(TextMapData, r2 * 2 * 2 + 1, 4)
r0 = 128 * 2^9
r2 = r1 + r0
r1 = RSH(r1, 15)
'--------------------RSH() is homemade right shift function for bit operation
Block(i) = Block(i) & Mid$(TextMapData, r1 * 2 + 1, 4)
r2 = RSH(r2, 15)
Block(i) = Block(i) & Mid$(TextMapData, r2 * 2 + 1, 4)
Next i
```  
we finally get the block data each size equal to a word, you can see the flags' meaning:

|bit Number|about|
|:-:|:-:|
|0-3|palette ID|
|4|horizontal reversal 1(set) 0(no)|
|5|vertical reversal 1(set) 0(no)|
|6-15|Tile ID|  

##  about Layer  
GBA use 4 Layer:
- Layer 0 , Layer 1，Layer 2，bg Layer (Layer 3)  

the priority of them can be changed by using GBA IO rigisters.
the game always use DMA to convey all the data to palette RAM  
we can use QImage class in Qt to render map and using QImage::Format_RGB555.

**render Priority**  
Flag01 is just the Room Header Byte number 27
```
If (Flag01 Mod 4) = 0 Then
layerPriority(1) = 1: layerPriority(2) = 2
ElseIf (Flag01 Mod 4) = 1 Then
layerPriority(0) = 1: layerPriority(2) = 2
ElseIf (Flag01 Mod 4) = 2 Then
layerPriority(0) = 1: layerPriority(2) = 2
Else
layerPriority(0) = 2: layerPriority(2) = 1
End If
```
**Alpha**  
```
If Flag01 > 7 Then
Select Case (Flag01 - 8) \ 4
    Case 0: EVA = 7: EVB = 16
    Case 1: EVA = 10: EVB = 16
    Case 2: EVA = 13: EVB = 16
    Case 3: EVA = 16: EVB = 16
    '------------------------------------the following case includes some assumption
    Case 4: EVA = 16: EVB = 0
    Case 5: EVA = 13: EVB = 3
    Case 6: EVA = 10: EVB = 6
    Case 7: EVA = 7: EVB = 9
    Case 8: EVA = 5: EVB = 11
    Case 9: EVA = 3: EVB = 13          'trace once in this condition and exclude a lot of varients
    Case 10: EVA = 0: EVB = 16
    Case Else: EVA = 0: EVB = 16         'I don't know about this
    '-----------------------------------assumption end here
End Select
```

## Sprites  
1. every enemy have a load-information list
2. every Tile for Sprites use one palette
3. Don't know exactly about the loading of enemy yet except the following things:

**Enemy Header**
there is a Enemy Header list for each enemy to describes the loading information of them.  
the structure of one Enemy Header just like this:  

|Offset|Data type|Description|
|:-:|:-:|:-:|
|+0x00|u8|Y-coordinate (top -> bottom)|
|+0x01|u8|X-coordinate (left -> right)|
|+0x02|u8|Enemy index|

A record containing all 0xFF fields marks the end of the list 

**Enemy ID**
you should know that there is a huge enemy pointer list at the tail of the ROM valid data, and every room will load some basic elements universally. so the first 17(from 0x00 to 0x10) elements in every room is fixed. 
In the pointers table of elements and enemies, the first one point to a big block of data contains all the coins and someother things should be seen as a OBJ or elements, and the rest of them are. The rest 16 in the first 17 elements are not all valid, some of the valid oens are:   

|ID|About| 
|:-:|:-:|
|0x01 to 0x04|4 Gemstone Pieces|
|05|CD Box|
|06|full-health box|
|07|Diamond|
|08|Frog switch|
If you try the other IDs, you perhaps will get something but those things cannot be use at all.
and every room will add other Sprites Tiles one by one by using the pointers after 78EBF0 in ROM and the load table pointer after 78EF78. you can get the load table ID when getting in to a room by the Byte in map linker table.

**Elements and Enemies Load table structure**
every record contains 2 bytes, the first one B1 saves the enemy's ID and the second one B2 represents for the palette data stop position.(there is 16 palettes for Sprites at all and the 9th of them should be the base and the offset for it is 0) And the whole table stop by a "\x00\x00" word.

**Loading of Elements and Enemies**
1. get Sprites Tile data pointer by this: ``(B1-0x10)*4+78EBF0``
2. get Sprites palette pointer by this: ``(B1-0x10)*4+78EDB4``
3. the length of Tiles data we should load is calculated by this (unit: byte): ``a=(int16)[(B1-0x10)*4+3B2C90]``
4. the palette length for sprites we should load is calculated by this: ``a/2^8``
5. the writing position in palette RAM: ``(B2+7)*2^5+0x05000000``
  
P.S. perhaps there is something wrong in this record: "``the 9th of them should be the base``" and "``(B2+7)*2^5+0x05000000``", is there any conflict?

  
### the data transmission of the Enemy Header  
  
1. the Enemy Header is extracted out from ROM when Wario entering a new room and the Enemy Header is saved after [03000964].  
2. in sub_801E1C0 and its sub routine sub_801E0EC , the data after 0x03000964 is calculated and transmitted to RAM after 03000104, each block after  03000104 contained the setting of one Sprite and the size is 44 Bytes (0x2C=44)  
3. in sub_801D684 some of the setting data after 03000104 (including x, y position of the Sprites) is sent to RAM after 0x03000A24  
4. in sub_801DA70(int a1), the data after 0x03000A24 was got one by one and used to calculate the final OAM attribute for each OBJ and the final data is saved in RAM after 03001444  
5. in sub_0801BC0C, the procedure use the DMA channel directly DMA all the data after 03001444 to OAM and the first frame of all the Object is determined.  

## Room change Linker Table  

the structure of the linker table just like this, and every Level will have a table and the pointers table for all of them start from 78F21C, the order for room header string pointers table and Room change Linker rercords pointers table are in the same pace, you can use the same offset to find both the pointers just in one time.The table stop by an all-zero record:  
```
struct MAPLinkerLineRecord
{
    unsigned char LinkerTypeFlag; //x01 for portal only, x02 for instant shift, x03 for door and tube, x04 unknown, x05 unknown
    unsigned char RoomID; //start from x00
    unsigned char x1;
    unsigned char x2;
    unsigned char y1;
    unsigned char y2;  //topleft judge block position (x1, y1), bottomleft judge block position (x2, y2), the first block start from (0, 0)
    unsigned char LinkerDestination;
    //multiply x0C make a shift to find another linker record to find the destination ROOM by RoomID, wario will appear at the position(x1, y1) in a (new) MAP
    //and immediately shift the position with vector (HorizontalDisplacement, VerticalDisplacement)
    //if set LinkerTypeFlag=0x01 for protal, then you should set LinkerDestination, HorizontalDisplacement and VerticalDisplacement all be 0x00
    unsigned char HorizontalDisplacement;
    unsigned char VerticalDisplacement;  //the two numbers can be negtivate by using this function: ResultByte = 0x100 + (the negtive number), so -1 just input 0xFF
    unsigned char EnemyAndElementTable_ID;
    unsigned char BGM_ID_FirstByte;  //Low Byte
    unsigned char BGM_ID_SecondByte;  //High Byte
};
```
**how to find the pointer for the table**
use [03000023] to find the correct Level Header and take down the offset when you find the right Level Header, the add the offset by 78F21C and you will find the correct pointer for Room change Linker Table.

## TODO

**TODO in ROMHacking：** 
- Tileset Header last 3 pointers' usage
- Sprites static rendering
- all the setting Flags in Room Header

**TODO in extracting ROM data：**
- extract all the Tile and others' data
