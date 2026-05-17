# WL4 Level Design Guide

Document your level design principles and conventions here.
The AI will reference this when generating or modifying rooms.

## Hard Engine Limits (DO NOT EXCEED)

These are enforced by the WL4 game engine and WL4Editor. Generating values beyond these limits will cause import failures or game crashes.

| Limit | Value | Description |
|-------|-------|-------------|
| Max rooms per level | **16** | hardcoded pointer table in RAM, enforced by `AddNewRoom()` |
| Max entities per room per difficulty | **64** | Engine compile-time limit |
| Max entity types per EntitySet | **31 (0x1F)** | `EntityinfoTable` size limit |
| Max camera limitators per room | **100** | `MAX_CAMERA_LIMITATORS` constant |
| Max active enemies simultaneously | **24** | Engine runtime limit |

| Number of Doors per level | **0-255** | Limited by destination door id saved using usigned char |
| Room width range | **19–256 tiles** | Layer dimensions on Map16 mode |
| Room height range | **14–256 tiles** | Layer dimensions on Map16 mode |
| Room width \* height range | **0-5119 (0x1400 bytes)** | Limited by Hardcoded layer's text mode VRAM size on Map16 mode |
| Tileset ID range | **0-91 (0x5B)** | Hardcoded pointer table in ROM |
| Entity ID range | **0-128 (0x80)** | Hardcoded pointer table in ROM |
| EntitySet ID range | **0-89** | Hardcoded pointer table in ROM |
| animated tile group ID range | **0-269** | Hardcoded pointer table in ROM |


## Passage Structure (Outside of Level)

A standard WL4 passage contains:
- **1 or 4 main Levels** — Enter the level to find a keyzer to unlock the next Level, find 4 Gem boxes with 4 gem fragments in them and collect all the 4 fragments make a gem piece for the Boss Door, find an optional CD box which can be played in the sounds room. In EntryPassage and the GoldenPassage, there is only 1 main Level, in the other 4 passages, there are 4 Level. There is an extra debug Level in the EntryPassage which can only be entered by runtime RAM setting using cheat.
- **1 minigame Level** — pay money gains from Levels to play mini game to win big Coin, which can be used in boss Level shop Room.
- **1 boss Level** — at the end, unlocked after collecting all 1 or 4 gem pieces to open the Boss Door then beat the Boss.
- **1 shop room** — There is a shpo Room in the Boss Level for buying items using big Coins, which can be used before the boss fight to decrease the HP bar of the Boss.

## Hub world and Game progression (Outside of Passage)

- Players can only access to the EntryPassage at first. After beating the first Level then the first Boss, 4 passages (EmeraldPassage, RubyPassage, TopazPassage, SapphirePassage) and 1 sounds room appear in the hub world. These 4 passages have 4 main Levels inside.
- Levels in EmeraldPassage are usually natural themed, Levels in RubyPassage are usually industrialisation themed, Levels in TopazPassage are usually game themed, Levels in SapphirePassage are usually thriling themed.
- Beat all the Bosses from the 4 Passages open up the GoldenPassage, there is also only one Level and one Boss in it in paramid theme. beating them will complete the game.

## General Principles

### In Game Coordinate
- when we talk about X position or width, the unit is Tile16, it counts from 0, start from the left top corder of a Room, adding X or width number will make the Room grow bigger rightside.
- when we talk about Y position or height, the unit is Tile16, it counts from 0, start from the left top corder of a Room, adding Y or height number will make the Room grow bigger downside.
- the playable character is Wario, who is 2 Tile16s in height and 1 Tile16 in width.

### Pacing
- Rooms should alternate between platforming, puzzle and combat sections
- Start each room with a safe zone (no enemies at spawn points)
- Introduce new enemy types one at a time with clear visual cues
- Provide heart tiles before difficult sections
- GBA screen shows ~15×10 panels at a time — design encounters within this view

### Connectivity
- Doors must connect rooms: each door has a destination door ID
- Door 0 in Room 0 typically is where Wario enter and exit the Level with a portal door entity (sprite) cling to and above it. (Door 0 at [8, 9], then the portal entity at [8, 8])
- Main path should be intuitive — player should naturally find the way forward
- Optional side paths for collectibles (treasure boxes, coins) and rewards

### Entity Placement
- Don't place enemies directly at door entry points (leave 2-3 tiles of safe space)
- Match enemy types to the room's tileset theme (see tileset_catalog.json themes)
- Balance entity counts across difficulties: Normal < Hard < S-Hard (i know the difficulty id is confusing)
- Typically 3-15 entities per room depending on room size
- Max 24 active enemies simultaneously (engine limit), max 64 placed per room
- Provide enough horizontal space for enemy patrol routes (minimum 4-5 tiles)

### Layer Usage
- **Layer 1 (BG1)**: Core gameplay — floors, walls, platforms, interactive event tiles. event tiles only works in this layer. You can only modify this layer if it is in Map16 mode, don't modify this layer if it is in Tile8x8 mapping mode.
- **Layer 0 (BG0)**: Foreground overlay — decorations, additional obstacles, water tiles usually placed here with Room configuration set alpha blending to make them semi-transparent. Map16 mode only.
- **Layer 2 (BG2)**: Background decorations — non-interactive visual elements. Map16 mode only.
- **Layer 3 (BG3)**: Far background / parallax layer — screen background layer, don't modify this layer.

### Tile Placement Rules
1. **Tile16 index #0 (0x0000) is the air/empty tile.** Always use tile 0 for empty space, sky, and pass-through areas. Never place a non-zero tile where you want Wario to fall or move freely.
2. Read terrain type part in tile_reference.md to know how every Tile16 will block or pass Wario and enemies. Use `wl4_get_layer_terrain_map` to see collision per tile.
3. Read event ID part in tile_reference.md to know how every Tile16 triggers special behavior. Use `wl4_get_layer_event_map` to see event IDs per tile.
4. Floor tiles must use solid collision (terrain type 0x01) or platform (0x0C).
5. Wall tiles must be solid to block Wario.
6. One-way platforms (terrain 0x0C) allow Wario to jump through from below but land on top.
7. Don't modify tile graphics if you cannot read images.

## Design of Room Size
- plan regions for platforming, puzzle and combat zones first. use tunnels or passages to connect them in a Room. 
- don't make the zones get too close to each other.


## Difficulty Scaling

Three difficulty levels affect:
1. **Timer**: Different countdown values per difficulty
2. **Entity placement**: Each difficulty has separate entity list (ucTekistr/e/h)
3. **Enemy stats**: Some enemies are faster/stronger on higher difficulties

Design convention:
- **Hard**: Baseline difficulty. Place a moderate number of enemies.
- **Normal**: Easier. Reduce enemy count or replace with weaker variants.
- **S-Hard**: Hardest. Increase enemy count. Add spike/needle variants.

## Things to Avoid
- **Blind jumps**: Player cannot see the landing. GBA screen is only 15×10 tiles.
- **Softlock positions**: Player trapped with no way out and no way to die
- **Enemy spam**: Don't place more than 5-6 active enemies per screen width
- **Impossible gaps**: Wario's max jump is 3.5 tiles high, ~6 tiles horizontal dash jump
- **Misleading paths**: Background decorations (Layer 2/3) that look like Layer 1 platforms
- **Unreachable collectibles**: Blocked by collision or too high without launch mechanism
- **Door without destination**: Every door entity must have a valid destination door ID
- **Entity set mismatch**: Entity type not in the room's EntitySet — the entity won't spawn
- **Out of bounds**: Entity/Door coordinates must stay within room dimensions
- **No water flag with water tiles**: set room config with layer 0 alpha blending for layer 0 with water tiles.
- **Fire blocks without fire source**: Fire blocks requires a YFIRE tile (0x0069) or fire-causing enemy somewhere reachable
- **Ice blocks without ice enemy**: Ice blocks requires an ice-causing enemy nearby

## Camera Limitator Box Rules

Camera limitators define scroll boundaries within a room. Their bounds are constrained by the room dimensions and hardware limits.

### Limitator Box Parameters (use with `wl4_set_camera_control` records array)
| Parameter | Description | Valid Range |
|-----------|-------------|-------------|
| `x1` | Left edge (tile units) | 2 to roomWidth - 17 |
| `y1` | Top edge (tile units) | 2 to roomHeight - 12 |
| `width` | Box width in tiles (spans x1 to x2) | 15 to roomWidth - x1 - 2 |
| `height` | Box height in tiles (spans y1 to y2) | 10 to roomHeight - y1 - 2 |
| `x2` | Right edge (auto: x1 + width - 1) | — |
| `y2` | Bottom edge (auto: y1 + height - 1) | — |
| `x3` | Trigger block X position | 0 to roomWidth - 3 |
| `y3` | Trigger block Y position | 0 to roomHeight - 3 |
| `trans` | Boundary control type | 0=Fixed, 1=ResetLeft, 2=ResetRight, 3=ResetUpper, 4=ResetLower |
| `offset` | Which boundary to change | 0=x1, 1=x2, 2=y1, 3=y2 |
| `value` | New value for the changed boundary | 0-255 |

### Hard Constraints
- **Edge margin**: 2 Tile16s from room edge (x1 >= 2, y1 >= 2)
- **Minimum box width**: 15 Tile16s
- **Minimum box height**: 10 Tile16s
- **Right margin**: width <= roomWidth - x1 - 2 (2-tile margin from right edge)
- **Bottom margin**: height <= roomHeight - y1 - 2 (2-tile margin from bottom edge)
- **Max limitators per room**: 100

### Camera Types (use with `wl4_set_camera_control` type parameter)
| Value | Name | Behavior |
|-------|------|----------|
| 1 | FixedY | Camera Y position locked |
| 2 | NoLimit | No camera boundaries, follows Wario freely |
| 3 | HasControlAttrs | Uses limitator records for scroll boundaries |
| 4 | VerticalSeperated | Vertically separated camera zones |

## Tileset Information

### Tile16 Encoding
Each placed tile on a Map16 layer is a 16-bit value:
- **Bits 0-9** (0x000-0x3FF): Tile index into the tileset's Map16 table (0-1023)
- **Bits 10-11**: Palette bank (0-3)
- **Bit 12**: Horizontal flip (1 = mirrored)
- **Bit 13**: Vertical flip (1 = flipped upside-down)
- **0x0000**: Empty/transparent tile

### Event ID Table
Each Map16 tile index (0-0x2FF) maps to an **event ID** byte (0-0xFF) that defines special behavior. Use `wl4_get_tile_info` with parameter `tileId` (0-0x2FF) to query `eventId`. See tile_reference.md for complete list.

### Terrain Type Table
Each Map16 tile index (0-0x2FF) maps to a **terrain type** byte (0-0xFF) that defines collision/physics. Use `wl4_get_tile_info` to query `terrainType`. See tile_reference.md for complete list.

### How AI Reads Tile Properties
1. Call `wl4_get_room_config` to find the tileset ID for a room (`tilesetId` field)
2. Call `wl4_get_tile_info` with `tileId` (0-0x2FF) to get `eventId` and `terrainType`
3. Cross-reference values with tile_reference.md via `wl4_read_knowledge("tile_reference")`

## EntitySet Information

### Entity Global ID vs Local Index
- **Global Entity ID** (0-128): The entity type as stored in ROM. Each global ID has unique sprite, behavior, and OAM data. Never changes across rooms. Cross-reference with entity_catalog via `wl4_read_knowledge("entity_catalog")`.
- **Local Index** (0-30): Position within a room's EntitySet. Each room has ONE EntitySet mapping up to 31 global IDs to local indices. Used by `wl4_add_entity`, `wl4_get_entity_list`, `wl4_move_entity`, `wl4_delete_entity`.

### EntitySet Workflow
1. Get entitySetId from `wl4_get_room_config` -> `entitySetId`
2. Call `wl4_get_entity_types` to see the mapping: each entry has `localIndex` and `globalId`
3. Use `localIndex` with entity placement tools
4. Or pass `globalEntityId` to `wl4_add_entity` for auto-lookup

### EntitySet Rules
- Entity type NOT in the room's EntitySet **won't spawn** (engine skips unknown types)
- Always check `wl4_get_entity_types` before placing entities
- Max 31 entity types per EntitySet, max 64 placed entities per room per difficulty

## Door Information

### Door Parameters (for `wl4_get_doors` response / `wl4_import_room_json` doors array)
| Field | Description | Range |
|-------|-------------|-------|
| `type` | Door type byte | 1=Portal&Door, 2=Warp, 3=Pipe, 4=BossDoor, 5=ItemShopDoor |
| `roomID` | Room containing this door | 0-15 |
| `x1,y1` | Top-left corner (tile units) | Within room dimensions |
| `x2,y2` | Bottom-right corner (tile units) | x2>=x1, y2>=y1 |
| `destGlobalID` | Destination door's global ID | 0-255 (0=disabled) |
| `dx,dy` | Wario spawn offset (signed) | -128 to 127 |
| `entitySetID` | Entity set for destination room | 0-89 |
| `bgm` | BGM track ID for destination | 0-65535 |
| `globalDoorID` | Unique ID across entire level | 0-255 |

### Door Placement Rules
- Every door must have a valid `destGlobalID` (unless type=0/disabled)
- Door 0 in Room 0 is the Portal — Wario exits the level here; place portal entity (global ID 41) nearby
- Door coordinates define a rectangular trigger zone in tile units
- Wario spawns at (x1+dx, y1+dy) after transition

## Wario Physics Reference

| Ability | Value | Level Design Implication |
|---------|-------|--------------------------|
| Max jump height | ~4 tiles | Platforms above 4 tiles need alternate route |
| Max dash jump distance | ~6 tiles horizontal | Gaps wider than 6 tiles need bridge/mechanism |
| Max crouch jump | ~3 tiles | Low ceiling sections must allow crouch jump |
| Ground pound descent | 48 px/frame | Timing for crusher/piston puzzles |
| Wario size | 1 wide x 2 tall (Tile16s) | Minimum passage: 1 tile wide, 2 tiles tall |

## Switch System

The game has 4 usable global switches (MapSw[1-4]) per passage. Switches toggle when Wario touches or ground-pounds a switch tile (event IDs 0x40-0x43).

| Switch | Typical Use |
|--------|-------------|
| MapSw[1] | General puzzles, domino chains |
| MapSw[2] | General puzzles |
| MapSw[3] | General puzzles |
| MapSw[4] | J-Switch / Frog switch — controls Vortex doors and water currents |

Switch state affects: collision tiles (terrain types 0x14-0x3F), event tiles (0x38-0x3F, 0x90-0x9C), water currents (0x4C-0x4F). See tile_reference.md for per-ID details.

## Block Destruction by Wario Form

| Block Type | Required Form/Attack | Event ID Range |
|------------|---------------------|-----------------|
| Normal blocks | Dash attack, Ground pound | 0x0B-0x1E |
| Hard blocks | Power ground pound (crouch+jump→down) | 0x1F-0x2A |
| Fire blocks | W_FIRE form (touch fire tile 0x0069) | 0x2B-0x2F |
| Snow blocks type A | W_SNOW form (large snowball) | 0x30-0x31 |
| Snow blocks type B | W_SNOW form | 0x32-0x33 |
| Return blocks | Appear on return trip after gate activation | 0x34-0x35 |
| Ice/Slippery blocks | W_ICE form | 0x59-0x5D |
| Enemy-triggered block | Throw enemy onto it | 0x36 |
