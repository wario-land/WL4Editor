# WL4Editor MCP Server

AI-assisted Wario Land 4 level design via the Model Context Protocol (MCP).

## Overview

The MCP server runs inside WL4Editor, exposing 26 tools for room/level querying, tile editing, entity manipulation, door/camera control, layer event/terrain maps, bulk layer import/export, and knowledge lookup. Any MCP-compatible AI client (Claude Code, Cursor, etc.) can connect and operate WL4Editor programmatically.

```
┌──────────────┐     SSE/HTTP      ┌─────────────────────┐
│  Claude Code  │◄═══════════════►│  WL4Editor.exe       │
│  (AI Agent)   │  localhost:9876  │  ┌───────────────┐  │
│              │                   │  │ MCP Server     │  │
│  Tools:      │                   │  │ • TCP+SSE      │  │
│  - export    │                   │  │ • 23 tools     │  │
│  - get tile  │                   │  │ • JS eval      │  │
│  - add entity│                   │  └───────┬─────────┘  │
│  - import    │                   │          │             │
│  ...         │                   │  ┌───────▼─────────┐  │
└──────────────┘                   │  │ WL4Editor Core  │  │
                                    │  │ • Room/Level    │  │
                                    │  │ • Operation     │  │
                                    │  │ • ScriptIface   │  │
                                    │  └─────────────────┘  │
                                    └─────────────────────┘
```

**Transport:** TCP with SSE (Server-Sent Events) on `localhost:9876`.  
**Protocol:** JSON-RPC 2.0 per MCP specification.  
**Undo/Redo:** All modifications go through WL4Editor's Operation system — fully undoable.

## Setup

### 1. Build WL4Editor

The MCP server is part of the normal WL4Editor build. No extra dependencies beyond Qt (Core, Gui, Network, Qml, Widgets).

```
qmake && make   # or build in Qt Creator
```

### 2. Copy knowledge files to your romhack

The AI needs game design knowledge (entity types, tile mechanics, Wario forms, engine limits). Copy the knowledge files into your romhack folder:

```
YourRomHack/
├── ROM.gba
└── game_knowledge/          # Copy from MCP/knowledge/
    ├── entity_catalog.json   # All 129 entity types with behaviors
    ├── tileset_catalog.json  # Tileset names and theme info
    ├── tile_reference.md     # Complete event ID & terrain type tables
    ├── design_guide.md       # Level design rules, engine limits, camera/door/entity info
    └── patch_notes.md        # Document your ROM patches here
```

The AI reads these files via the `wl4_read_knowledge` tool. Edit them to match any patches you've applied to your ROM.

### 3. Prepare Claude Code config

Create `.claude/mcp.json` in your project (or home directory):

```json
{
  "mcpServers": {
    "wl4editor": {
      "type": "sse",
      "url": "http://localhost:9876/sse"
    }
  }
}
```

> **Note**: The `"type": "sse"` field is required by recent Claude Code versions. Without it, the client may fail to list tools.

Or from the CLI:
```bash
claude mcp add wl4editor --transport sse http://localhost:9876/sse
```

## Usage

### Start WL4Editor and the MCP Server

1. Launch WL4Editor normally
2. Open your ROM file
3. Open the Output Window: **View → Output Window** (opened up by default after loading ROM)
4. In the Output Window's command line, execute:
   ```
   WL4EditorInterface.StartMCPServer();
   ```
   The Output Window will show: `MCP Server: Started on http://localhost:9876/sse`

To start on a different port:
```
WL4EditorInterface.StartMCPServerTcp(9999);
```

To stop:
```
WL4EditorInterface.StopMCPServer();
```

### Connect Claude Code

After the server is running:
```bash
claude mcp enable wl4editor
```

Verify connection:
```bash
claude mcp list
```

You can remove configuration by:
```bash
claude mcp remove wl4editor
```

### First AI Session — Load Knowledge

On first connection, ask the AI to read the game knowledge:

> Please read the game knowledge files: entity_catalog, tileset_catalog, tile_reference, and design_guide.

The AI will call `wl4_read_knowledge` for each file. After this, it understands WL4 entity types (with DoorConfigDialog names), tileset themes, tile event/terrain tables, engine limits, camera box rules, door info, and EntitySet mechanics.

## Tool Reference

### Room/Level Query (read-only)

| Tool | Description |
|------|-------------|
| `wl4_export_room` | Export current room as complete JSON (layers, entities, doors, camera) |
| `wl4_export_level` | Export entire level (all rooms) as JSON |
| `wl4_get_room_list` | List all rooms: ID, dimensions, tileset, entity set, door/entity counts |
| `wl4_get_level_info` | Level metadata: name, passage, stage, timers, room count |
| `wl4_get_room_graph` | Door connectivity graph (which rooms connect to which) |
| `wl4_get_room_config` | Room dimensions, tileset, entity set, layer mapping types, header hex |

### Tileset / Tile Operations

| Tool | Description |
|------|-------------|
| `wl4_get_tileset_info` | Tileset metadata: FG/BG GFX pointers, palette address |
| `wl4_get_tile_info` | Event ID and terrain type for a Map16 tile index |
| `wl4_get_tile16` | Read tile values from a layer (single tile or region). Returns tile ID, palette bank, flip flags |
| `wl4_set_tile16` | Write a single tile to a layer (undoable) |
| `wl4_import_layer_hex` | Import tile data to an entire layer from hex string (bulk, undoable) |
| `wl4_export_layer_hex` | Export an entire layer's tile data as flat hex string (for bulk read-modify-write) |
| `wl4_get_layer_event_map` | Get event ID map for a layer region (what each tile DOES) |
| `wl4_get_layer_terrain_map` | Get terrain type map for a layer region (collision per tile) |

**Tile value encoding (16-bit):**
- Bits 0-9: Tile index (0-1023)
- Bits 10-11: Palette bank (0-3)
- Bit 12: Horizontal flip
- Bit 13: Vertical flip

Example: `0x4001` = tile index 1 with palette bank 1. `0x0000` = empty.

### Entity Operations

| Tool | Description |
|------|-------------|
| `wl4_get_entity_types` | Available entity types in current room's entity set (max 31) |
| `wl4_get_entity_list` | Placed entities for a difficulty (0=Hard, 1=Normal, 2=S-Hard) |
| `wl4_add_entity` | Place a new entity at (x, y) with given global ID (undoable) |
| `wl4_delete_entity` | Remove entity by index (undoable) |
| `wl4_move_entity` | Change entity position (undoable) |

### Door & Camera

| Tool | Description |
|------|-------------|
| `wl4_get_doors` | All doors in current room: type, coordinates, destination, BGM |
| `wl4_get_camera_control` | Camera type and limitator records (max 100) |
| `wl4_set_camera_control` | Set camera type and limitator records (undoable) |

Camera types: 1=FixedY, 2=NoLimit, 3=HasControlAttrs, 4=VerticalSeperated.

### Navigation & Import

| Tool | Description |
|------|-------------|
| `wl4_set_current_room` | Switch to a different room by ID |
| `wl4_import_room_json` | Import a complete room JSON (undoable) |

### Knowledge & Scripting

| Tool | Description |
|------|-------------|
| `wl4_read_knowledge` | Read a game knowledge file: entity_catalog, tileset_catalog, tile_reference, design_guide, patch_notes |
| `wl4_eval` | Execute arbitrary JavaScript in WL4Editor's engine (has access to all `WL4EditorInterface.*` APIs) |

## Workflow Examples

### Example 1: Analyze a Room

```
User: Export room 0 and tell me about the entity placement.

Claude calls: wl4_get_room_list → wl4_set_current_room(0) → wl4_export_room

Response: "Room 0 is 41x23 tiles using tileset 80. Hard difficulty has 3 entities:
  - Entity 17 (Mask Spear) at (6,7)
  - Entity 17 (Mask Spear) at (33,10)  
  - Entity 20 (Wood Platform) at (20,18)
  Normal has 3, S-Hard has 2. Room has 3 doors, camera type 1 (FixedY)."
```

### Example 2: Modify Tile Layout

```
User: Replace the solid block at (10,5) on layer 1 with a coin block (tile 0x0009).

Claude calls: wl4_get_tile16(layerId=1, x=10, y=5)
→ tileId is 0x0001 (solid block)
Claude calls: wl4_set_tile16(layerId=1, x=10, y=5, tileId=0x0009)
→ Success, oldTileId=0x0001, newTileId=0x0009
```

### Example 3: Add Enemies

```
User: Add 3 spear enemies (entity 17) on the platforms in room 2, Normal difficulty.

Claude calls: wl4_get_room_list → wl4_set_current_room(2) → wl4_get_entity_types
→ Spear enemy (17) is available in entity set 19
Claude calls: wl4_export_room → analyzes platform positions from layer data
Claude calls: wl4_add_entity(x=12, y=8, entityId=17, difficulty=1)
              wl4_add_entity(x=25, y=8, entityId=17, difficulty=1)  
              wl4_add_entity(x=38, y=12, entityId=17, difficulty=1)
→ All 3 added. User sees them in WL4Editor, presses Ctrl+Z to undo if needed.
```

### Example 4: Full Room Design

```
User: Design a new room with factory theme. 50x20 tiles, tileset 7 (Factory Interior).
      Add conveyor belts on the floor, 2 piston traps, and 4 spear enemies on Hard.

Claude calls: wl4_read_knowledge("tile_reference") → learns conveyor belt tile 0x006B
              wl4_read_knowledge("entity_catalog") → learns piston=79, spear=17
              
Claude creates room JSON with:
  - roomConfig: 50x20, tileset 7
  - Layer 1: floor tiles with conveyor belt row at y=18
  - Entities: 2x piston(79) at (15,17) and (35,17), 
              4x spear(17) on platforms
              
Claude calls: wl4_import_room_json(room_data={...})
→ Room imported. User checks in WL4Editor, makes manual tweaks.
```

### Example 5: Use wl4_eval for Advanced Operations

```
User: Run the ExportRoom.js script and save to a specific path.

Claude calls: wl4_eval(code="
  WL4EditorInterface.WriteJsonFile('C:/Users/AAA/Desktop/exports/room.json', 
    JSON.stringify(/* ... */));
")
```

## Engine Limits

The AI reads these from `design_guide.md`. Key constraints:

| Limit | Value |
|-------|-------|
| Max rooms per level | 16 |
| Max entities per room per difficulty | 64 |
| Max entity types per EntitySet | 31 (0x1F) |
| Max camera limitators per room | 100 |
| Room width range | 19–256 tiles |
| Room height range | 14–256 tiles |
| Max entity ID | 128 (0x80) |
| Max tileset ID | 91 (0x5B) |

## Architecture Notes

### How the TCP Transport Works

1. WL4Editor's MCP server listens on `localhost:9876`
2. AI client connects via `GET /sse` → receives a session ID and SSE event stream
3. JSON-RPC requests are sent via `POST /message?sessionId=<id>`
4. Responses flow back through the SSE stream
5. Multiple AI clients can connect simultaneously (each gets a session)
6. `GET /health` returns server status

### Thread Safety

- The MCP server lives on the Qt main thread
- TCP connections are handled by QTcpServer (event-driven, non-blocking)
- All tool handlers run synchronously on the main thread
- Operations use WL4Editor's existing undo/redo system

### Adding Custom Tools

To add a new tool, edit `MCP/MCPTools.cpp`:

```cpp
// 1. Write the handler function
static QJsonObject t_my_tool(const QJsonObject &args)
{
    // ... implement tool logic ...
    return makeResult({{"key", "value"}});
}

// 2. Register it in MCP_RegisterAllTools()
server->registerTool("wl4_my_tool",
    "Description of what this tool does.",
    QJsonObject{{"type", "object"}, {"properties", QJsonObject{
        {"param1", QJsonObject{{"type", "string"}, {"description", "..."}}}
    }}, {"required", QJsonArray{"param1"}}},
    t_my_tool);
```

## Troubleshooting

**"MCP Server: Cannot start — no ROM loaded"**
→ Load a ROM file first, then run `WL4EditorInterface.StartMCPServer()`.

**"Failed to start TCP on port 9876"**
→ The port is already in use. Try a different port: `WL4EditorInterface.StartMCPServerTcp(9877)`.

**Claude Code can't connect**
→ Verify the server is running: open `http://localhost:9876/health` in a browser. Should show `{"status":"ok","sessions":0}`.
→ Verify your `.claude/mcp.json` has the correct URL: `"url": "http://localhost:9876/sse"`.

**AI doesn't know tile IDs or entity types**
→ Ask the AI to read knowledge files first: "Read entity_catalog, tileset_catalog, and tile_reference from the knowledge files."

**wl4_eval returns error**
→ The Output Window must be open (View → Output Window). The QJSEngine lives there.

**"Knowledge file not found"**
→ Copy `MCP/knowledge/*` to your romhack as `game_knowledge/`. Or ensure the built copy is at `<exe_dir>/../MCP/knowledge/`.
