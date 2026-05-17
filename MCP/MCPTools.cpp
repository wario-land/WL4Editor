#include "MCPTools.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QTextStream>

#ifndef WINDOW_INSTANCE_SINGLETON
#define WINDOW_INSTANCE_SINGLETON
#include "WL4EditorWindow.h"
extern WL4EditorWindow *singleton;
#endif

#include "LevelComponents/Level.h"
#include "LevelComponents/Room.h"
#include "LevelComponents/LevelDoorVector.h"
#include "LevelComponents/Tileset.h"
#include "ROMUtils.h"
#include "WL4Constants.h"
#include "Operation.h"
#include "ScriptInterface.h"

// ============================================================================
// Response helpers
// ============================================================================

static QJsonObject makeResult(const QJsonObject &data, ScriptInterface *si = nullptr)
{
    QJsonObject payload = data;
    if (si) {
        QString logBuf = si->GetAndClearLogBuffer();
        if (!logBuf.isEmpty()) payload["_log"] = logBuf.trimmed();
    }
    QJsonObject result;
    QJsonObject content;
    content["type"] = QString("text");
    content["text"] = QString(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    result["content"] = QJsonArray{content};
    result["structuredContent"] = payload;
    return result;
}

static QJsonObject makeTextResult(const QString &text)
{
    QJsonObject result;
    QJsonObject content;
    content["type"] = QString("text");
    content["text"] = text;
    result["content"] = QJsonArray{content};
    return result;
}

static QJsonObject makeErrorResult(const QString &error, ScriptInterface *si = nullptr)
{
    QString fullError = error;
    if (si) {
        QString logBuf = si->GetAndClearLogBuffer();
        if (!logBuf.isEmpty())
            fullError += "\n[ScriptInterface log: " + logBuf.trimmed() + "]";
    }
    QJsonObject result;
    QJsonObject content;
    content["type"] = QString("text");
    content["text"] = fullError;
    result["content"] = QJsonArray{content};
    result["isError"] = true;
    return result;
}

static bool checkRomLoaded()
{
    return singleton && singleton->FirstROMIsLoaded();
}

// ============================================================================
// JSON argument helpers
// ============================================================================

static int argInt(const QJsonObject &a, const QString &name, int def = 0)
{
    if (!a.contains(name)) return def;
    return a[name].toInt(def);
}

static QString argStr(const QJsonObject &a, const QString &name, const QString &def = {})
{
    if (!a.contains(name)) return def;
    return a[name].toString();
}

// ============================================================================
// JSON Schema builder for tool inputSchema
// ============================================================================

struct ParamDef
{
    const char *name;
    const char *jsonType;   // "integer", "string", "boolean", "array", "object"
    const char *desc;
};

static QJsonObject makeSchema(const std::vector<ParamDef> &params,
                              const std::vector<const char *> &required)
{
    QJsonObject props;
    for (const auto &p : params)
        props[p.name] = QJsonObject{{"type", QString(p.jsonType)}, {"description", QString(p.desc)}};
    QJsonArray reqArr;
    for (auto r : required)
        reqArr.append(QString(r));
    QJsonObject schema;
    schema["type"] = QString("object");
    schema["properties"] = props;
    schema["required"] = reqArr; // Always set to force detach — avoids Qt COW leaking prior call's value
    return schema;
}

// Shortcut for defining params inline
#define P(name, type, desc) {name, type, desc}

// Validate entity data hex string format for SetEntityListData.
// Format: "0xYY 0xXX 0xEE ..." — space-separated hex triples (YPos, XPos, EntityID=localIndex).
// Empty string is valid and means "clear all entities for this difficulty".
// Returns empty string on success, error message on failure.
static QString validateEntityDataHex(const QString &data)
{
    if (data.trimmed().isEmpty())
        return {}; // valid — clears the entity list
    QStringList tokens = data.split(' ', Qt::SkipEmptyParts);
    if (tokens.size() % 3 != 0)
        return QString("Entity data length %1 not divisible by 3. Format: each entity = "
                       "\"0xYY 0xXX 0xEE\" (YPos, XPos, EntityID=localIndex), space-separated. "
                       "Use empty string to clear all entities.")
               .arg(tokens.size());
    return {}; // valid
}

// ============================================================================
// Complex read-only helpers (bulk JSON construction — use Room/Layer API directly
// for efficiency, but remain read-only)
// ============================================================================

static QJsonObject exportRoomToJson(LevelComponents::Room *room, LevelComponents::Level *level)
{
    QJsonObject obj;
    int roomId = static_cast<int>(room->GetRoomID());
    obj["roomId"] = roomId;
    obj["version"] = 1;

    QJsonObject config;
    config["roomWidth"] = static_cast<int>(room->GetLayer1Width());
    config["roomHeight"] = static_cast<int>(room->GetLayer1Height());
    config["layer0Width"] = static_cast<int>(room->GetLayer0Width());
    config["layer0Height"] = static_cast<int>(room->GetLayer0Height());
    obj["roomConfig"] = config;
    obj["roomHeaderHex"] = room->GetRoomHeaderHex();
    obj["entitySetId"] = room->GetCurrentEntitySetID();

    // Layers
    QJsonArray layers;
    for (int li = 0; li < 4; li++)
    {
        auto *layer = room->GetLayer(li);
        int mappingType = static_cast<int>(layer->GetMappingType());
        int w = static_cast<int>(layer->GetLayerWidth());
        int h = static_cast<int>(layer->GetLayerHeight());
        QJsonObject layerObj;
        layerObj["layerId"] = li;
        layerObj["mappingType"] = mappingType;
        layerObj["width"] = w;
        layerObj["height"] = h;
        QJsonArray data;
        if (mappingType >= 0x10)
        {
            for (int y = 0; y < h; y++)
            {
                QString rowHex;
                for (int x = 0; x < w; x++)
                {
                    int tileId = (int)layer->GetTileData(x, y);
                    if (mappingType >= 0x20) tileId &= 0x3FF;
                    rowHex += QString::number(tileId, 16).rightJustified(4, '0');
                }
                data.append(rowHex);
            }
        }
        layerObj["data"] = data;
        layers.append(layerObj);
    }
    obj["layers"] = layers;

    // Entity lists
    QJsonArray entityLists;
    for (int d = 0; d < 3; d++)
    {
        auto entities = room->GetEntityListData(d);
        QJsonObject listObj;
        listObj["difficulty"] = d;
        QJsonArray entArr;
        for (const auto &e : entities)
        {
            QJsonObject ent;
            ent["yPos"] = (int)e.YPos;
            ent["xPos"] = (int)e.XPos;
            ent["entityId"] = (int)e.EntityID;
            entArr.append(ent);
        }
        listObj["entities"] = entArr;
        entityLists.append(listObj);
    }
    obj["entityLists"] = entityLists;

    // Doors
    QJsonArray doors;
    auto doorVec = level->GetDoorListRef().GetDoorsByRoomID(static_cast<unsigned char>(roomId));
    for (int i = 0; i < doorVec.size(); i++)
    {
        const auto &d = doorVec[i];
        unsigned char globalId = level->GetDoorListRef().GetGlobalIDByLocalID(
            static_cast<unsigned char>(roomId), static_cast<unsigned char>(i));
        QJsonObject door;
        door["type"] = (int)d.DoorTypeByte;
        door["roomID"] = (int)d.RoomID;
        door["x1"] = (int)d.x1; door["x2"] = (int)d.x2;
        door["y1"] = (int)d.y1; door["y2"] = (int)d.y2;
        door["destID"] = (int)d.DestinationDoorGlobalID;
        door["dx"] = (int)d.HorizontalDeltaWario;
        door["dy"] = (int)d.VerticalDeltaWario;
        door["entitySetID"] = (int)d.EntitySetID;
        door["bgm"] = (int)d.BGM_ID;
        door["globalDoorID"] = (int)globalId;
        doors.append(door);
    }
    obj["doors"] = doors;

    // Camera
    QJsonObject cam;
    cam["type"] = static_cast<int>(room->GetCameraControlType());
    QJsonArray camRecs;
    auto records = room->GetCameraControlRecords();
    for (const auto *rec : records)
    {
        QJsonObject r;
        r["trans"] = (int)rec->TransboundaryControl;
        r["x1"] = (int)rec->x1; r["x2"] = (int)rec->x2;
        r["y1"] = (int)rec->y1; r["y2"] = (int)rec->y2;
        r["x3"] = (int)rec->x3; r["y3"] = (int)rec->y3;
        r["offset"] = (int)rec->ChangeValueOffset;
        r["value"] = (int)rec->ChangedValue;
        camRecs.append(r);
    }
    cam["records"] = camRecs;
    obj["cameraControl"] = cam;

    QJsonObject ptrs;
    ptrs["layer0"] = (int)room->GetLayerDataPtr(0);
    ptrs["layer3"] = (int)room->GetLayerDataPtr(3);
    obj["nonOriginalLayerPointers"] = ptrs;

    return obj;
}

// ============================================================================
// TOOL registration
// ============================================================================

void MCP_RegisterAllTools(MCPServer *server)
{
    if (!server) return;

    // ScriptInterface is the single entry point for all editor operations.
    auto *si = singleton->GetScriptInterface();
    if (!si)
    {
        // ScriptInterface lives in OutputDockWidget; open View > Output Window first
        return;
    }
    si->SetHeadlessMode(true); // Suppress all modal dialogs during MCP operations

    // ========================================================================
    // Room/Level Query (read-only)
    // ========================================================================

    server->registerTool("wl4_export_room",
        "Export the current room as complete JSON. Contains room config, all 4 layers "
        "(tile hex strings), entity lists for all 3 difficulties, doors with full "
        "properties, camera control type and limitator records, and non-original layer "
        "data pointers. This is the primary tool for AI to read current room state.",
        makeSchema({}, {}),
        [si](const QJsonObject &) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            auto *room = singleton->GetCurrentRoom();
            auto *level = singleton->GetCurrentLevel();
            if (!room || !level) return makeErrorResult("No room/level selected");
            return makeResult(exportRoomToJson(room, level));
        });

    server->registerTool("wl4_export_level",
        "Export the entire current level (all rooms) as JSON. Includes level name/nameJ, "
        "header (passage, stage, timers for all 3 difficulties), and all rooms with "
        "their complete data. Switches rooms during export; restores original room after. "
        "Warning: large output for levels with many rooms.",
        makeSchema({}, {}),
        [si](const QJsonObject &) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            auto *level = singleton->GetCurrentLevel();
            if (!level) return makeErrorResult("No level selected");
            int origRoom = singleton->GetCurrentRoomId();
            auto rooms = level->GetRooms();
            QJsonObject levelObj;
            levelObj["version"] = 1;
            levelObj["levelName"] = level->GetLevelName();
            levelObj["levelNameJ"] = level->GetLevelName(1);
            QJsonObject header;
            header["passage"] = static_cast<int>(level->GetPassage());
            header["stage"] = static_cast<int>(level->GetStage());
            header["timerHard"] = level->GetTimeCountdownCounter(LevelComponents::HardDifficulty);
            header["timerNormal"] = level->GetTimeCountdownCounter(LevelComponents::NormalDifficulty);
            header["timerSHard"] = level->GetTimeCountdownCounter(LevelComponents::SHardDifficulty);
            levelObj["levelHeader"] = header;
            levelObj["roomCount"] = static_cast<int>(rooms.size());
            QJsonArray roomsArr;
            for (size_t i = 0; i < rooms.size(); i++)
            {
                singleton->SetCurrentRoomId(static_cast<int>(i));
                roomsArr.append(exportRoomToJson(rooms[i], level));
            }
            levelObj["rooms"] = roomsArr;
            singleton->SetCurrentRoomId(origRoom);
            return makeResult(levelObj);
        });

    server->registerTool("wl4_get_room_list",
        "List all rooms in the current level. Returns for each room: roomId (0-based), "
        "width, height (in tiles), tilesetId, entitySetId, cameraType, doorCount, "
        "totalEntities (across all difficulties), and whether it is the currently selected "
        "room. Use this to understand the level layout before drilling into specific rooms.",
        makeSchema({}, {}),
        [si](const QJsonObject &) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            auto *level = singleton->GetCurrentLevel();
            if (!level) return makeErrorResult("No level selected");
            int curRoom = singleton->GetCurrentRoomId();
            auto roomVec = level->GetRooms();
            QJsonArray rooms;
            for (size_t i = 0; i < roomVec.size(); i++)
            {
                auto *r = roomVec[i];
                QJsonObject s;
                s["roomId"] = static_cast<int>(r->GetRoomID());
                s["width"] = static_cast<int>(r->GetLayer1Width());
                s["height"] = static_cast<int>(r->GetLayer1Height());
                s["tilesetId"] = static_cast<int>(r->GetTilesetID());
                s["entitySetId"] = static_cast<int>(r->GetCurrentEntitySetID());
                s["cameraType"] = static_cast<int>(r->GetCameraControlType());
                auto doors = level->GetDoorListRef().GetDoorsByRoomID(
                    static_cast<unsigned char>(r->GetRoomID()));
                s["doorCount"] = static_cast<int>(doors.size());
                int totalEnt = 0;
                for (int d = 0; d < 3; d++)
                    totalEnt += static_cast<int>(r->GetEntityListData(d).size());
                s["totalEntities"] = totalEnt;
                s["isCurrent"] = (static_cast<int>(i) == curRoom);
                rooms.append(s);
            }
            QJsonObject result;
            result["rooms"] = rooms;
            result["currentRoomId"] = curRoom;
            return makeResult(result);
        });

    server->registerTool("wl4_get_level_info",
        "Get level-wide metadata. Returns: levelName (English), levelNameJ (Japanese), "
        "passage number, stage number, timer values (frames) for Hard/Normal/S-Hard, "
        "roomCount, and levelId. Passages: 1=Entry, 2=Emerald, 3=Ruby, 4=Sapphire, "
        "5=Topaz, 6=Final. Stage: 1-4 within the passage.",
        makeSchema({}, {}),
        [si](const QJsonObject &) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            auto *level = singleton->GetCurrentLevel();
            if (!level) return makeErrorResult("No level selected");
            QJsonObject info;
            info["levelName"] = level->GetLevelName();
            info["levelNameJ"] = level->GetLevelName(1);
            info["passage"] = static_cast<int>(level->GetPassage());
            info["stage"] = static_cast<int>(level->GetStage());
            info["roomCount"] = static_cast<int>(level->GetRooms().size());
            info["timerHard"] = level->GetTimeCountdownCounter(LevelComponents::HardDifficulty);
            info["timerNormal"] = level->GetTimeCountdownCounter(LevelComponents::NormalDifficulty);
            info["timerSHard"] = level->GetTimeCountdownCounter(LevelComponents::SHardDifficulty);
            info["levelId"] = static_cast<int>(level->GetLevelID());
            return makeResult(info);
        });

    server->registerTool("wl4_get_room_graph",
        "Get door connectivity graph for the current level. Returns a mapping from each "
        "roomId to an array of connections, each with: targetRoom (destination room), "
        "type (door type byte), globalDoorId. Use this to understand which rooms connect "
        "to which via doors/warps.",
        makeSchema({}, {}),
        [si](const QJsonObject &) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            auto *level = singleton->GetCurrentLevel();
            if (!level) return makeErrorResult("No level selected");
            auto rooms = level->GetRooms();
            QJsonObject graph;
            for (size_t i = 0; i < rooms.size(); i++)
            {
                int rid = static_cast<int>(rooms[i]->GetRoomID());
                auto doors = level->GetDoorListRef().GetDoorsByRoomID(static_cast<unsigned char>(rid));
                QJsonArray conns;
                for (const auto &d : doors)
                {
                    if (d.DoorTypeByte == 0) continue;
                    int destGid = d.DestinationDoorGlobalID;
                    for (size_t j = 0; j < rooms.size(); j++)
                    {
                        auto dd = level->GetDoorListRef().GetDoorsByRoomID(
                            static_cast<unsigned char>(rooms[j]->GetRoomID()));
                        for (int k = 0; k < dd.size(); k++)
                        {
                            unsigned char gid = level->GetDoorListRef().GetGlobalIDByLocalID(
                                static_cast<unsigned char>(rooms[j]->GetRoomID()), static_cast<unsigned char>(k));
                            if (static_cast<int>(gid) == destGid)
                            {
                                QJsonObject c;
                                c["targetRoom"] = static_cast<int>(rooms[j]->GetRoomID());
                                c["type"] = (int)d.DoorTypeByte;
                                c["globalDoorId"] = destGid;
                                conns.append(c);
                            }
                        }
                    }
                }
                graph[QString::number(rid)] = conns;
            }
            QJsonObject result;
            result["doorGraph"] = graph;
            return makeResult(result);
        });

    server->registerTool("wl4_get_room_config",
        "Get current room configuration. Returns: roomId, mainWidth, mainHeight (layer 1 "
        "dimensions in tiles), layer0Width, layer0Height, tilesetId, entitySetId, "
        "cameraType, bgmVolume, headerHex (44-byte room header), and layerMappingTypes "
        "(mapping type per layer: 0x00=disabled, 0x10=Map16, 0x20=Tile8x8).",
        makeSchema({}, {}),
        [si](const QJsonObject &) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            auto *room = singleton->GetCurrentRoom();
            if (!room) return makeErrorResult("No room selected");
            QJsonObject cfg;
            cfg["roomId"] = static_cast<int>(room->GetRoomID());
            cfg["mainWidth"] = static_cast<int>(room->GetLayer1Width());
            cfg["mainHeight"] = static_cast<int>(room->GetLayer1Height());
            cfg["layer0Width"] = static_cast<int>(room->GetLayer0Width());
            cfg["layer0Height"] = static_cast<int>(room->GetLayer0Height());
            cfg["tilesetId"] = room->GetTilesetID();
            cfg["entitySetId"] = room->GetCurrentEntitySetID();
            cfg["cameraType"] = static_cast<int>(room->GetCameraControlType());
            cfg["bgmVolume"] = (int)room->GetBgmvolume();
            cfg["headerHex"] = room->GetRoomHeaderHex();
            QJsonObject layerTypes;
            for (int i = 0; i < 4; i++)
                layerTypes[QString("layer%1").arg(i)] =
                    static_cast<int>(room->GetLayer(i)->GetMappingType());
            cfg["layerMappingTypes"] = layerTypes;
            return makeResult(cfg);
        });

    // ========================================================================
    // Tileset / Tile Operations
    // ========================================================================

    server->registerTool("wl4_get_tileset_info",
        "Get metadata about a tileset. Returns: id, fgGfxPtr (ROM address of FG tile "
        "graphics), fgGfxLen, bgGfxPtr, bgGfxLen, map16Ptr (ROM address of Map16 table), "
        "paletteAddr, isNew (whether tileset data has been relocated to expanded ROM area). "
        "If tilesetId is omitted, uses the current room's tileset. Valid range: 0-91 (0x5B).",
        makeSchema({
            P("tilesetId", "integer", "Tileset ID (0-91). Omit to use current room's tileset.")
        }, {}),
        [si](const QJsonObject &args) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            int tsId = argInt(args, "tilesetId", -1);
            if (tsId < 0)
            {
                auto *room = singleton->GetCurrentRoom();
                if (!room) return makeErrorResult("No room selected");
                tsId = room->GetTilesetID();
            }
            if (tsId < 0 || tsId >= 92 || !ROMUtils::singletonTilesets[tsId])
                return makeErrorResult("Invalid tileset ID (0-91)");
            auto *ts = ROMUtils::singletonTilesets[tsId];
            QJsonObject info;
            info["id"] = tsId;
            info["fgGfxPtr"] = ts->GetfgGFXptr();
            info["fgGfxLen"] = ts->GetfgGFXlen();
            info["bgGfxPtr"] = ts->GetbgGFXptr();
            info["bgGfxLen"] = ts->GetbgGFXlen();
            info["map16Ptr"] = ts->GetMap16Ptr();
            info["paletteAddr"] = ts->GetPaletteAddr();
            info["isNew"] = ts->IsNewTileset();
            return makeResult(info);
        });

    server->registerTool("wl4_get_tile_info",
        "Get the event ID and terrain type for a Map16 tile index in the current room's "
        "tileset. Only valid for tile indices 0-0x2FF (0-767). Returns eventId (0-255) "
        "and terrainType (0-255). These determine game behavior (solid, slope, water, "
        "spike, conveyor, etc.) and are defined in the tileset's event/terrain tables.",
        makeSchema({
            P("tileId", "integer", "Map16 tile index (0-767, i.e. 0x000-0x2FF)")
        }, {"tileId"}),
        [si](const QJsonObject &args) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            int tileId = argInt(args, "tileId", -1);
            if (tileId < 0) return makeErrorResult("Missing tileId");
            auto *room = singleton->GetCurrentRoom();
            if (!room) return makeErrorResult("No room selected");
            int eventId = si->GetCurTilesetTile16EventId(static_cast<unsigned short>(tileId));
            int terrain = si->GetCurTilesetTile16TerrainType(static_cast<unsigned short>(tileId));
            return makeResult({{"tileId", tileId}, {"eventId", eventId}, {"terrainType", terrain}});
        });

    server->registerTool("wl4_get_tile16",
        "Read tile values from a layer. Two modes: (1) Single tile — provide x, y. "
        "(2) Region — provide startX, startY, width, height. Returns tile values as "
        "integers (16-bit). Each tile value encodes: bits 0-9=tile index (0-1023), "
        "bits 10-11=palette bank (0-3), bit 12=horizontal flip, bit 13=vertical flip. "
        "Single-tile mode additionally decodes these fields for convenience. "
        "Valid layers: 0 (background), 1 (main), 2 (overlay) — must be Map16 type. "
        "layerId defaults to 1 if omitted.",
        makeSchema({
            P("layerId", "integer", "Layer 0-2 (default 1). Must be Map16 mapping type."),
            P("x", "integer", "X position in tile units (single tile mode)"),
            P("y", "integer", "Y position in tile units (single tile mode)"),
            P("startX", "integer", "Start X for region mode"),
            P("startY", "integer", "Start Y for region mode"),
            P("width", "integer", "Width in tiles for region mode (default 1)"),
            P("height", "integer", "Height in tiles for region mode (default 1)")
        }, {}),
        [si](const QJsonObject &args) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            auto *room = singleton->GetCurrentRoom();
            if (!room) return makeErrorResult("No room selected");
            int layerId = argInt(args, "layerId", 1);
            if (args.contains("x") && args.contains("y"))
            {
                int x = argInt(args, "x"), y = argInt(args, "y");
                int tileId = (layerId < 0x10)
                    ? si->GetCurRoomTile16(layerId, x, y)
                    : si->GetCurRoomTile8(layerId, x, y);
                QJsonObject r;
                r["layerId"] = layerId; r["x"] = x; r["y"] = y;
                r["tileId"] = tileId;
                r["tileHex"] = QString("0x") + QString::number(tileId, 16).rightJustified(4, '0');
                if (tileId >= 0)
                {
                    r["paletteBank"] = (tileId >> 10) & 3;
                    r["hFlip"] = (tileId >> 12) & 1;
                    r["vFlip"] = (tileId >> 13) & 1;
                }
                return makeResult(r);
            }
            else
            {
                int w = argInt(args, "width", 1), h = argInt(args, "height", 1);
                int startX = argInt(args, "startX"), startY = argInt(args, "startY");
                QJsonArray rows;
                auto *layer = room->GetLayer(layerId);
                int maxH = static_cast<int>(layer->GetLayerHeight());
                int maxW = static_cast<int>(layer->GetLayerWidth());
                for (int row = startY; row < startY + h && row < maxH; row++)
                {
                    QJsonArray cols;
                    for (int col = startX; col < startX + w && col < maxW; col++)
                        cols.append(static_cast<int>(layer->GetTileData(col, row)));
                    rows.append(cols);
                }
                return makeResult({{"layerId", layerId}, {"startX", startX},
                                   {"startY", startY}, {"tiles", rows}});
            }
        });

    server->registerTool("wl4_set_tile16",
        "Write a single tile value to a Map16 layer. This is UNDOABLE — goes through "
        "the editor's undo/redo system. Parameters:\n"
        "  layerId: 0=background, 1=main, 2=overlay (default 1). Must be Map16.\n"
        "  x, y: tile position in tile units (0 to layerWidth-1, 0 to layerHeight-1).\n"
        "  tileId: 16-bit value. Bits 0-9=tile index (0-1023), bits 10-11=palette bank "
        "(0-3), bit 12=hflip, bit 13=vflip. 0x0000=empty.\n"
        "Errors: 'Invalid layer or out of bounds' if x/y outside layer dimensions or "
        "layer has wrong mapping type.\n"
        "Related: wl4_get_tile16, wl4_get_tile_info, wl4_get_room_config.",
        makeSchema({
            P("layerId", "integer", "Layer 0-2 (default 1). Must be Map16 mapping type."),
            P("x", "integer", "X position in tile units"),
            P("y", "integer", "Y position in tile units"),
            P("tileId", "integer", "16-bit tile value (0x0000-0x3FFF)")
        }, {"x", "y", "tileId"}),
        [si](const QJsonObject &args) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded", si);
            int lid = argInt(args, "layerId", 1);
            int x = argInt(args, "x", -1), y = argInt(args, "y", -1);
            int tid = argInt(args, "tileId", -1);
            if (x < 0 || y < 0 || tid < 0)
                return makeErrorResult("Missing x, y, or tileId", si);
            if (lid < 0 || lid > 2)
                return makeErrorResult("Invalid layerId (0-2)", si);
            si->GetAndClearLogBuffer(); // drain stale log before operation
            int layerW = si->GetCurRoomLayerWidth(lid);
            int layerH = si->GetCurRoomLayerHeight(lid);
            if (x >= layerW || y >= layerH)
                return makeErrorResult(QString("Position (%1,%2) out of bounds. Layer %3 dimensions: %4x%5")
                                      .arg(x).arg(y).arg(lid).arg(layerW).arg(layerH), si);
            int old = si->GetCurRoomTile16(lid, x, y);
            if (old < 0) return makeErrorResult(QString("Layer %1 is not Map16 type").arg(lid), si);
            si->SetCurRoomTile16(lid, tid, x, y);
            return makeResult({{"success", true}, {"layerId", lid}, {"x", x}, {"y", y},
                               {"oldTileId", old}, {"newTileId", tid}}, si);
        });

    server->registerTool("wl4_import_layer_hex",
        "Import tile data to an entire layer from a hex string. UNDOABLE. "
        "The hex string is concatenated 4-char hex values per tile (e.g. \"001A00B2...\"), "
        "row-major. This wraps the native ImportLayerTiles which handles Operation/Undo. "
        "Much faster than calling wl4_set_tile16 individually for bulk edits. "
        "Only works on Map16 layers (mappingType 0x10). "
        "Parameters:\n"
        "  layerId: 0-2 (must be Map16).\n"
        "  width, height: tile dimensions, must match the hex string length (4*width*height chars).\n"
        "  tileDataHex: concatenated hex string, 4 chars per tile.\n"
        "Returns: {\"success\": true} or error.",
        makeSchema({
            P("layerId", "integer", "Layer 0-2 (Map16 only)"),
            P("width", "integer", "Width in tiles"),
            P("height", "integer", "Height in tiles"),
            P("tileDataHex", "string", "Concatenated hex: 4 chars per tile, row-major")
        }, {"layerId", "width", "height", "tileDataHex"}),
        [si](const QJsonObject &args) -> QJsonObject {
            si->GetAndClearLogBuffer(); // drain stale log
            auto ok = [si](const QJsonObject &d) { return makeResult(d, si); };
            auto err = [si](const QString &e) { return makeErrorResult(e, si); };
            if (!checkRomLoaded()) return err("No ROM loaded");
            int lid = argInt(args, "layerId", -1);
            int w = argInt(args, "width", -1);
            int h = argInt(args, "height", -1);
            QString hex = argStr(args, "tileDataHex");
            if (lid < 0 || lid > 2) return err("Invalid layerId (0-2)");
            if (w <= 0 || h <= 0) return err("Invalid width/height");
            if (hex.isEmpty()) return err("tileDataHex is empty");
            int expectedLen = w * h * 4;
            if (hex.length() < expectedLen)
                return err(QString("tileDataHex too short: got %1 chars, need %2 (4*%3*%4)")
                           .arg(hex.length()).arg(expectedLen).arg(w).arg(h));
            bool result = si->ImportLayerTiles(lid, w, h, hex);
            if (!result) return err("ImportLayerTiles failed — check layer mapping type is Map16");
            return ok({{"success", true}, {"layerId", lid},
                       {"width", w}, {"height", h}, {"tilesImported", w * h}});
        });

    server->registerTool("wl4_export_layer_hex",
        "Export an entire layer's tile data as a flat hex string (4 chars per tile, "
        "row-major). Returns the raw 16-bit tile values concatenated. Useful with "
        "wl4_import_layer_hex for bulk read-modify-write workflows. "
        "Only works on Map16 layers (mappingType 0x10).",
        makeSchema({
            P("layerId", "integer", "Layer 0-2 (Map16 only)")
        }, {"layerId"}),
        [si](const QJsonObject &args) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            int lid = argInt(args, "layerId", -1);
            if (lid < 0 || lid > 2) return makeErrorResult("Invalid layerId (0-2)");
            auto *room = singleton->GetCurrentRoom();
            if (!room) return makeErrorResult("No room selected");
            auto *layer = room->GetLayer(lid);
            if (layer->GetMappingType() != LevelComponents::LayerMap16)
                return makeErrorResult(QString("Layer %1 is not Map16 type").arg(lid));
            int w = static_cast<int>(layer->GetLayerWidth());
            int h = static_cast<int>(layer->GetLayerHeight());
            QString hex;
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                    hex += QString::number(layer->GetTileData(x, y), 16).rightJustified(4, '0');
            return makeResult({{"layerId", lid}, {"width", w}, {"height", h},
                               {"tileDataHex", hex}, {"tileCount", w * h}});
        });

    server->registerTool("wl4_get_layer_event_map",
        "the tileset's event table — the returned integer is the event ID (0-255) that "
        "defines what the tile DOES (door, breakable block, conveyor, water, etc.). "
        "Tile index 0 always maps to event ID 0 (nothing). "
        "Cross-reference with tile_reference.md (event ID section). "
        "Layer 0 and 1 are the gameplay layers with meaningful event IDs. "
        "If width/height omitted, returns the full layer from (startX,startY) to edge.",
        makeSchema({
            P("layerId", "integer", "Layer 0 or 1 (gameplay layers with event IDs)"),
            P("startX", "integer", "Start X (default 0)"),
            P("startY", "integer", "Start Y (default 0)"),
            P("width", "integer", "Width in tiles (default: to layer edge)"),
            P("height", "integer", "Height in tiles (default: to layer edge)")
        }, {"layerId"}),
        [si](const QJsonObject &args) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            int lid = argInt(args, "layerId", 1);
            int sx = argInt(args, "startX", 0);
            int sy = argInt(args, "startY", 0);
            int w = argInt(args, "width", -1);
            int h = argInt(args, "height", -1);
            QJsonObject map = si->GetLayerEventMap(lid, sx, sy, w, h);
            if (map.contains("error")) return makeErrorResult(map["error"].toString());
            return makeResult(map);
        });

    server->registerTool("wl4_get_layer_terrain_map",
        "Get the terrain type (collision) map for a layer region. Each tile's value is "
        "resolved through the tileset's terrain table — the returned integer is the "
        "terrain type (0-255) that defines physical collision: 0=air, 1=solid, "
        "2-7=slopes, 0x0C=one-way platform, 0x14-0x3F=switch-gated, 0x80-0x86=alpha "
        "blend overlay. Cross-reference with tile_reference.md (terrain type section). "
        "Layer 1 is the main collision layer; layer 0 terrain matters for foreground. "
        "If width/height omitted, returns the full layer from (startX,startY) to edge.",
        makeSchema({
            P("layerId", "integer", "Layer 0, 1, or 2 (Map16 layers with collision)"),
            P("startX", "integer", "Start X (default 0)"),
            P("startY", "integer", "Start Y (default 0)"),
            P("width", "integer", "Width in tiles (default: to layer edge)"),
            P("height", "integer", "Height in tiles (default: to layer edge)")
        }, {"layerId"}),
        [si](const QJsonObject &args) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            int lid = argInt(args, "layerId", 1);
            int sx = argInt(args, "startX", 0);
            int sy = argInt(args, "startY", 0);
            int w = argInt(args, "width", -1);
            int h = argInt(args, "height", -1);
            QJsonObject map = si->GetLayerTerrainMap(lid, sx, sy, w, h);
            if (map.contains("error")) return makeErrorResult(map["error"].toString());
            return makeResult(map);
        });

    // ========================================================================
    // Entity Operations
    // ========================================================================

    server->registerTool("wl4_get_entity_types",
        "Get the entity types available in the current room's entity set. Returns: "
        "entitySetId, availableEntities array (each with localIndex and globalId), "
        "count (number of types in this set), maxEntities (always 31).\n"
        "localIndex is the 0-based index used by wl4_add_entity/wl4_move_entity/wl4_delete_entity. "
        "globalId matches the entity type numbers in entity_catalog knowledge.\n"
        "An entity set maps up to 31 global entity types into local indices.",
        makeSchema({}, {}),
        [si](const QJsonObject &) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            auto *room = singleton->GetCurrentRoom();
            if (!room) return makeErrorResult("No room selected");
            int esId = si->GetCurRoomEntitySetId();
            QString infoStr = si->GetCurRoomEntitySetInfo();
            QJsonArray types;
            if (!infoStr.isEmpty())
            {
                auto entries = infoStr.split(';', Qt::SkipEmptyParts);
                for (const auto &entry : entries)
                {
                    auto parts = entry.split(',');
                    if (parts.size() >= 2)
                    {
                        types.append(QJsonObject{
                            {"localIndex", parts[0].toInt()},
                            {"globalId", parts[1].toInt()}
                        });
                    }
                }
            }
            return makeResult({{"entitySetId", esId}, {"availableEntities", types},
                               {"count", static_cast<int>(types.size())}, {"maxEntities", 31},
                               {"_note", QString("Use localIndex with wl4_add_entity. "
                                "globalId is the entity type reference from entity_catalog.")}});
        });

    server->registerTool("wl4_get_entity_list",
        "Get the list of placed entities for a specific difficulty in the current room. "
        "Returns: difficulty (0=Hard, 1=Normal, 2=S-Hard), entities array (each with "
        "xPos, yPos in tile units, entityId which is the LOCAL INDEX (0-30) in the room's "
        "EntitySet — the same as localIndex from wl4_get_entity_types), count. "
        "entityId is NOT the global entity ID — use wl4_get_entity_types to map localIndex "
        "to globalId. Max 64 entities per room per difficulty.",
        makeSchema({
            P("difficulty", "integer", "0=Hard, 1=Normal, 2=S-Hard (default 0)")
        }, {}),
        [si](const QJsonObject &args) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            int diff = argInt(args, "difficulty", 0);
            if (diff < 0 || diff > 2) return makeErrorResult("Difficulty 0-2 (0=Hard, 1=Normal, 2=S-Hard)");
            auto *room = singleton->GetCurrentRoom();
            if (!room) return makeErrorResult("No room selected");
            auto entities = room->GetEntityListData(diff);
            QJsonArray arr;
            for (const auto &e : entities)
                arr.append(QJsonObject{{"xPos", (int)e.XPos}, {"yPos", (int)e.YPos}, {"entityId", (int)e.EntityID}});
            return makeResult({{"difficulty", diff}, {"entities", arr}, {"count", static_cast<int>(arr.size())}});
        });

    server->registerTool("wl4_add_entity",
        "Place a new entity in the current room for a specific difficulty. UNDOABLE. "
        "Parameters:\n"
        "  x, y: position in tile units (0 to roomWidth-1, 0 to roomHeight-1).\n"
        "  localIndex: 0-based index in the entity set (from wl4_get_entity_types.localIndex, NOT globalId).\n"
        "  globalEntityId: alternative to localIndex — auto-looked up in the entity set.\n"
        "  difficulty: 0=Hard, 1=Normal, 2=S-Hard (default 0).\n"
        "When globalEntityId is provided, uses AddEntityByGlobalId internally: returns "
        "0=success, -1=not in EntitySet (check wl4_get_entity_types), -2=out of bounds, "
        "-3=invalid difficulty, -4=max 64 reached. "
        "Max 64 entities per difficulty per room.",
        makeSchema({
            P("x", "integer", "X position in tile units"),
            P("y", "integer", "Y position in tile units"),
            P("localIndex", "integer", "Local entity set index from wl4_get_entity_types"),
            P("globalEntityId", "integer", "Global entity ID (auto-mapped to localIndex)"),
            P("difficulty", "integer", "0=Hard, 1=Normal, 2=S-Hard (default 0)")
        }, {"x", "y"}),
        [si](const QJsonObject &args) -> QJsonObject {
            si->GetAndClearLogBuffer(); // drain stale log
            auto ok = [si](const QJsonObject &d) { return makeResult(d, si); };
            auto err = [si](const QString &e) { return makeErrorResult(e, si); };
            if (!checkRomLoaded()) return err("No ROM loaded");
            auto *room = singleton->GetCurrentRoom();
            if (!room) return err("No room selected");
            int x = argInt(args, "x", -1), y = argInt(args, "y", -1);
            int diff = argInt(args, "difficulty", 0);
            if (x < 0 || y < 0) return err("Missing x or y");
            if (diff < 0 || diff > 2) return err("Difficulty 0-2");

            if (args.contains("globalEntityId") && !args.contains("localIndex")) {
                // Use AddEntityByGlobalId — validates entity set membership,
                // room bounds, and count limit internally.
                // Return codes: 0=success, -1=not in entity set, -2=out of bounds,
                // -3=invalid difficulty, -4=max entities reached.
                int globalId = argInt(args, "globalEntityId", -1);
                if (globalId < 0 || globalId > 128) return err("Invalid globalEntityId (0-128)");
                int result = si->AddEntityByGlobalId(globalId, x, y, diff);
                if (result == -1) return err(QString(
                    "Entity global ID %1 not found in current entity set. "
                    "Use wl4_get_entity_types to see available entities.").arg(globalId));
                if (result == -2) return err(QString(
                    "Position (%1,%2) out of room bounds.").arg(x).arg(y));
                if (result == -3) return err("Invalid difficulty (0-2)");
                if (result == -4) return err("Max 64 entities per difficulty reached");
                singleton->RenderScreenFull();
                return ok({{"success", true}, {"x", x}, {"y", y},
                            {"globalEntityId", globalId}, {"difficulty", diff},
                            {"_note", "AddEntityByGlobalId returns 0=ok, -1=notInSet, -2=OOB, -3=badDifficulty, -4=limitReached"}});
            }

            // localIndex path: build the entity list manually
            int localIndex = argInt(args, "localIndex", -1);
            if (localIndex < 0) return err("Missing localIndex or globalEntityId");
            int roomW = si->GetCurRoomLayerWidth(1);
            int roomH = si->GetCurRoomLayerHeight(1);
            if (x >= roomW || y >= roomH)
                return err(QString("Position (%1,%2) out of room bounds (%3x%4)")
                           .arg(x).arg(y).arg(roomW).arg(roomH));
            QString oldData = si->GetEntityListDataSafe(diff);
            int oldCount = oldData.split(' ', Qt::SkipEmptyParts).size() / 3;
            if (oldCount >= 64) return err("Max 64 entities per difficulty reached");
            QString newData = oldData;
            newData += (newData.isEmpty() ? "" : " ") +
                       QString("0x") + QString::number(y, 16).toUpper() + " " +
                       QString("0x") + QString::number(x, 16).toUpper() + " " +
                       QString("0x") + QString::number(localIndex, 16).toUpper();
            QString vErr = validateEntityDataHex(newData);
            if (!vErr.isEmpty()) return err(vErr);
            si->SetEntityListDataSafe(newData, diff);
            singleton->RenderScreenFull();
            return ok({{"success", true}, {"x", x}, {"y", y},
                        {"localIndex", localIndex}, {"difficulty", diff}});
        });

    server->registerTool("wl4_delete_entity",
        "Remove an entity from the current room by its index in the entity list. UNDOABLE. "
        "Parameters:\n"
        "  index: 0-based position in the entity list for the given difficulty "
        "(from wl4_get_entity_list, NOT the localIndex from wl4_get_entity_types).\n"
        "  difficulty: 0=Hard (default), 1=Normal, 2=S-Hard.",
        makeSchema({
            P("index", "integer", "Index in the entity list (0-based)"),
            P("difficulty", "integer", "0=Hard, 1=Normal, 2=S-Hard (default 0)")
        }, {"index"}),
        [si](const QJsonObject &args) -> QJsonObject {
            si->GetAndClearLogBuffer(); // drain stale log
            auto ok = [si](const QJsonObject &d) { return makeResult(d, si); };
            auto err = [si](const QString &e) { return makeErrorResult(e, si); };
            if (!checkRomLoaded()) return err("No ROM loaded");
            int index = argInt(args, "index", -1);
            int diff = argInt(args, "difficulty", 0);
            if (index < 0) return err("Missing index");
            if (diff < 0 || diff > 2) return err("Difficulty 0-2");
            auto *room = singleton->GetCurrentRoom();
            if (!room) return err("No room selected");
            auto list = room->GetEntityListData(diff);
            if (index >= static_cast<int>(list.size()))
                return err("Index out of range");
            QString newData;
            for (int i = 0; i < static_cast<int>(list.size()); i++) {
                if (i == index) continue;
                if (!newData.isEmpty()) newData += " ";
                newData += QString("0x") + QString::number(list[i].YPos, 16).toUpper() + " " +
                           QString("0x") + QString::number(list[i].XPos, 16).toUpper() + " " +
                           QString("0x") + QString::number(list[i].EntityID, 16).toUpper();
            }
            QString vErr = validateEntityDataHex(newData);
            if (!vErr.isEmpty()) return err(vErr);
            si->SetEntityListDataSafe(newData, diff);
            singleton->RenderScreenFull();
            return ok({{"success", true}, {"deletedIndex", index}, {"difficulty", diff}});
        });

    server->registerTool("wl4_move_entity",
        "Change the position of an existing entity. UNDOABLE. Parameters:\n"
        "  index: 0-based position in the entity list (from wl4_get_entity_list).\n"
        "  x, y: new position in tile units.\n"
        "  difficulty: 0=Hard (default), 1=Normal, 2=S-Hard.",
        makeSchema({
            P("index", "integer", "Index in the entity list (0-based)"),
            P("x", "integer", "New X position in tile units"),
            P("y", "integer", "New Y position in tile units"),
            P("difficulty", "integer", "0=Hard, 1=Normal, 2=S-Hard (default 0)")
        }, {"index", "x", "y"}),
        [si](const QJsonObject &args) -> QJsonObject {
            si->GetAndClearLogBuffer(); // drain stale log
            auto ok = [si](const QJsonObject &d) { return makeResult(d, si); };
            auto err = [si](const QString &e) { return makeErrorResult(e, si); };
            if (!checkRomLoaded()) return err("No ROM loaded");
            int index = argInt(args, "index", -1);
            int newX = argInt(args, "x", -1), newY = argInt(args, "y", -1);
            int diff = argInt(args, "difficulty", 0);
            if (index < 0 || newX < 0 || newY < 0)
                return err("Missing index, x, or y");
            if (diff < 0 || diff > 2) return err("Difficulty 0-2");
            int roomW = si->GetCurRoomLayerWidth(1);
            int roomH = si->GetCurRoomLayerHeight(1);
            if (newX >= roomW || newY >= roomH)
                return err(QString("Position (%1,%2) out of room bounds (%3x%4)")
                           .arg(newX).arg(newY).arg(roomW).arg(roomH));
            auto *room = singleton->GetCurrentRoom();
            if (!room) return err("No room selected");
            auto list = room->GetEntityListData(diff);
            if (index >= static_cast<int>(list.size()))
                return err("Index out of range");
            QString newData;
            for (int i = 0; i < static_cast<int>(list.size()); i++) {
                if (!newData.isEmpty()) newData += " ";
                int ex = (i == index) ? newX : (int)list[i].XPos;
                int ey = (i == index) ? newY : (int)list[i].YPos;
                newData += QString("0x") + QString::number(ey, 16).toUpper() + " " +
                           QString("0x") + QString::number(ex, 16).toUpper() + " " +
                           QString("0x") + QString::number(list[i].EntityID, 16).toUpper();
            }
            QString vErr = validateEntityDataHex(newData);
            if (!vErr.isEmpty()) return err(vErr);
            si->SetEntityListDataSafe(newData, diff);
            singleton->RenderScreenFull();
            return ok({{"success", true}, {"index", index},
                        {"newX", newX}, {"newY", newY}, {"difficulty", diff}});
        });

    // ========================================================================
    // Door Operations
    // ========================================================================

    server->registerTool("wl4_get_doors",
        "Get all doors in the current room. Returns for each door: localIndex (position "
        "in the room's door list), type (door type byte: 0=disabled, 1=normal, etc.), "
        "roomID, coordinates (x1,x2,y1,y2 in tile units), destGlobalID (destination "
        "door's global ID), dx/dy (Wario spawn delta), entitySetID, bgm (background "
        "music ID), globalDoorID (unique across entire level).",
        makeSchema({}, {}),
        [si](const QJsonObject &) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            auto *level = singleton->GetCurrentLevel();
            auto *room = singleton->GetCurrentRoom();
            if (!level || !room) return makeErrorResult("No room/level");
            int rid = static_cast<int>(room->GetRoomID());
            auto doors = level->GetDoorListRef().GetDoorsByRoomID(static_cast<unsigned char>(rid));
            QJsonArray arr;
            for (int i = 0; i < doors.size(); i++)
            {
                const auto &d = doors[i];
                unsigned char gid = level->GetDoorListRef().GetGlobalIDByLocalID(
                    static_cast<unsigned char>(rid), static_cast<unsigned char>(i));
                arr.append(QJsonObject{
                    {"localIndex", i}, {"type", (int)d.DoorTypeByte},
                    {"roomID", (int)d.RoomID},
                    {"x1", (int)d.x1}, {"x2", (int)d.x2},
                    {"y1", (int)d.y1}, {"y2", (int)d.y2},
                    {"destGlobalID", (int)d.DestinationDoorGlobalID},
                    {"dx", (int)d.HorizontalDeltaWario},
                    {"dy", (int)d.VerticalDeltaWario},
                    {"entitySetID", (int)d.EntitySetID},
                    {"bgm", (int)d.BGM_ID},
                    {"globalDoorID", (int)gid}
                });
            }
            return makeResult({{"doors", arr}, {"count", static_cast<int>(arr.size())}});
        });

    // ========================================================================
    // Camera Control
    // ========================================================================

    server->registerTool("wl4_get_camera_control",
        "Get the camera control settings for the current room. Returns: type (1=FixedY, "
        "2=NoLimit, 3=HasControlAttrs, 4=VerticalSeperated), records array (limitator "
        "entries, each with: trans,x1,x2,y1,y2,x3,y3,offset,value), recordCount, "
        "maxRecords (100). Camera limitators define scroll boundaries and behavior.",
        makeSchema({}, {}),
        [si](const QJsonObject &) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            auto *room = singleton->GetCurrentRoom();
            if (!room) return makeErrorResult("No room selected");
            QJsonObject cam;
            cam["type"] = si->GetCurRoomCameraControlType();
            QJsonArray recs;
            auto records = room->GetCameraControlRecords();
            for (const auto *rec : records)
            {
                recs.append(QJsonObject{
                    {"trans", (int)rec->TransboundaryControl},
                    {"x1", (int)rec->x1}, {"x2", (int)rec->x2},
                    {"y1", (int)rec->y1}, {"y2", (int)rec->y2},
                    {"x3", (int)rec->x3}, {"y3", (int)rec->y3},
                    {"offset", (int)rec->ChangeValueOffset},
                    {"value", (int)rec->ChangedValue}
                });
            }
            cam["records"] = recs;
            cam["recordCount"] = static_cast<int>(recs.size());
            cam["maxRecords"] = 100;
            return makeResult(cam);
        });

    server->registerTool("wl4_set_camera_control",
        "Set the camera control type and limitator records for the current room. UNDOABLE. "
        "Parameters:\n"
        "  type: 1=FixedY, 2=NoLimit, 3=HasControlAttrs, 4=VerticalSeperated.\n"
        "  records: array of limitator objects, each with fields: trans,x1,x2,y1,y2,x3,y3,offset,value "
        "(all unsigned char, 0-255). x3/y3 default to 0xFF if omitted. Max 100 records.",
        makeSchema({
            P("type", "integer", "Camera type: 1=FixedY, 2=NoLimit, 3=HasControlAttrs, 4=VerticalSeperated"),
            P("records","array","Array of limitator objects: {trans,x1,x2,y1,y2,x3,y3,offset,value}")
        }, {"type", "records"}),
        [si](const QJsonObject &args) -> QJsonObject {
            si->GetAndClearLogBuffer(); // drain stale log
            auto ok = [si](const QJsonObject &d) { return makeResult(d, si); };
            auto err = [si](const QString &e) { return makeErrorResult(e, si); };
            if (!checkRomLoaded()) return err("No ROM loaded");
            auto *room = singleton->GetCurrentRoom();
            if (!room) return err("No room selected");
            int camType = argInt(args, "type", -1);
            QJsonArray recordsArr = args.value("records").toArray();
            QString recordsStr;
            for (const auto &val : recordsArr) {
                QJsonObject rec = val.toObject();
                if (!recordsStr.isEmpty()) recordsStr += ";";
                recordsStr += QString::number(rec["trans"].toInt()) + "," +
                              QString::number(rec["x1"].toInt()) + "," +
                              QString::number(rec["x2"].toInt()) + "," +
                              QString::number(rec["y1"].toInt()) + "," +
                              QString::number(rec["y2"].toInt()) + "," +
                              QString::number(rec.value("x3").toInt(0xFF)) + "," +
                              QString::number(rec.value("y3").toInt(0xFF)) + "," +
                              QString::number(rec["offset"].toInt()) + "," +
                              QString::number(rec["value"].toInt());
            }
            si->ImportCameraControl(camType, recordsStr);
            singleton->RenderScreenFull();
            return ok({{"success", true}, {"type", camType}});
        });

    // ========================================================================
    // Navigation
    // ========================================================================

    server->registerTool("wl4_set_current_room",
        "Switch WL4Editor to a different room by its 0-based room ID. Use "
        "wl4_get_room_list first to see available room IDs and their properties. "
        "All subsequent operations (tile read/write, entity operations, door queries) "
        "operate on the newly selected room.",
        makeSchema({
            P("roomId", "integer", "Room ID (0-based, 0 to roomCount-1)")
        }, {"roomId"}),
        [si](const QJsonObject &args) -> QJsonObject {
            if (!checkRomLoaded()) return makeErrorResult("No ROM loaded");
            int rid = argInt(args, "roomId", -1);
            if (rid < 0) return makeErrorResult("Missing roomId");
            auto *level = singleton->GetCurrentLevel();
            if (!level || rid >= static_cast<int>(level->GetRooms().size()))
                return makeErrorResult("Room ID out of range");
            si->SetCurrentRoomId(rid);
            return makeResult({{"currentRoomId", rid}});
        });

    // ========================================================================
    // Room Import
    // ========================================================================

    server->registerTool("wl4_import_room_json",
        "Import a complete room JSON into the current room. All changes are UNDOABLE "
        "(go through the Operation system). The JSON must contain: roomConfig "
        "(roomWidth, roomHeight, layer0Width, layer0Height), roomHeaderHex (44-byte "
        "header as hex string), layers array (layerId, mappingType, width, height, "
        "data as hex row strings), entityLists array (difficulty, entities), doors "
        "array (type, roomID, x1,x2,y1,y2, dx,dy, entitySetID, bgm), cameraControl "
        "(type, records). Use wl4_export_room to see the expected format.",
        makeSchema({
            P("room_data", "object", "Complete room JSON object matching wl4_export_room format")
        }, {"room_data"}),
        [si](const QJsonObject &args) -> QJsonObject {
            si->GetAndClearLogBuffer(); // drain stale log
            auto ok = [si](const QJsonObject &d) { return makeResult(d, si); };
            auto err = [si](const QString &e) { return makeErrorResult(e, si); };
            if (!checkRomLoaded()) return err("No ROM loaded");
            QJsonObject roomObj = args.value("room_data").toObject();
            if (roomObj.isEmpty())
                return err("Missing 'room_data' with complete room JSON");

            int currentRoomId = singleton->GetCurrentRoomId();
            auto *room = singleton->GetCurrentRoom();
            auto *level = singleton->GetCurrentLevel();
            if (!room || !level) return err("No room/level");

            // Step 1: Room config — delegate to ScriptInterface
            QJsonObject rc = roomObj.value("roomConfig").toObject();
            int rw = rc.value("roomWidth").toInt(32);
            int rh = rc.value("roomHeight").toInt(20);
            int l0w = rc.value("layer0Width").toInt(32);
            int l0h = rc.value("layer0Height").toInt(20);
            QString headerHex = roomObj.value("roomHeaderHex").toString();
            si->ImportRoomConfig(rw, rh, l0w, l0h, headerHex);

            // Step 2: Layer tiles — delegate to ScriptInterface
            QJsonArray layers = roomObj.value("layers").toArray();
            for (const auto &lVal : layers)
            {
                QJsonObject layer = lVal.toObject();
                int li = layer.value("layerId").toInt(-1);
                if (li < 0 || li > 2) continue;
                if ((layer.value("mappingType").toInt(0) & 0x30) != 0x10) continue;
                QJsonArray data = layer.value("data").toArray();
                if (data.isEmpty()) continue;
                int lw = layer.value("width").toInt(0);
                int lh = layer.value("height").toInt(0);
                // Join rows into flat hex string
                QString tileDataHex;
                for (const auto &rowVal : data)
                    tileDataHex += rowVal.toString();
                si->ImportLayerTiles(li, lw, lh, tileDataHex);
            }

            // Step 3: Entity lists — delegate to ScriptInterface
            QJsonArray entityLists = roomObj.value("entityLists").toArray();
            for (const auto &eVal : entityLists)
            {
                QJsonObject listObj = eVal.toObject();
                int diff = listObj.value("difficulty").toInt(-1);
                if (diff < 0 || diff > 2) continue;
                QJsonArray entities = listObj.value("entities").toArray();
                QString entityDataHex;
                for (const auto &entVal : entities)
                {
                    QJsonObject e = entVal.toObject();
                    if (!entityDataHex.isEmpty()) entityDataHex += " ";
                    entityDataHex += QString("0x") + QString::number(e.value("yPos").toInt(), 16).toUpper() + " " +
                                     QString("0x") + QString::number(e.value("xPos").toInt(), 16).toUpper() + " " +
                                     QString("0x") + QString::number(e.value("entityId").toInt(), 16).toUpper();
                }
                si->ImportEntityList(diff, entityDataHex);
            }

            // Step 4: Doors — delegate to ScriptInterface
            QJsonArray doors = roomObj.value("doors").toArray();
            if (!doors.isEmpty())
            {
                QString doorsStr;
                for (const auto &dVal : doors)
                {
                    QJsonObject d = dVal.toObject();
                    if (!doorsStr.isEmpty()) doorsStr += ";";
                    doorsStr += QString::number(d.value("type").toInt()) + "," +
                                QString::number(d.value("roomID").toInt()) + "," +
                                QString::number(d.value("x1").toInt()) + "," +
                                QString::number(d.value("x2").toInt()) + "," +
                                QString::number(d.value("y1").toInt()) + "," +
                                QString::number(d.value("y2").toInt()) + "," +
                                QString::number(d.value("destID").toInt()) + "," +
                                QString::number(d.value("dx").toInt()) + "," +
                                QString::number(d.value("dy").toInt()) + "," +
                                QString::number(d.value("entitySetID").toInt()) + "," +
                                QString::number(d.value("bgm").toInt()) + "," +
                                QString::number(d.value("globalDoorID").toInt());
                }
                si->ImportDoorsDisableDest(doorsStr);
            }

            // Step 5: Camera — delegate to ScriptInterface
            QJsonObject cam = roomObj.value("cameraControl").toObject();
            if (!cam.isEmpty() && cam.contains("type"))
            {
                int camType = cam.value("type").toInt();
                QJsonArray camRecs = cam.value("records").toArray();
                QString recordsStr;
                for (const auto &rVal : camRecs)
                {
                    QJsonObject rec = rVal.toObject();
                    if (!recordsStr.isEmpty()) recordsStr += ";";
                    recordsStr += QString::number(rec["trans"].toInt()) + "," +
                                  QString::number(rec["x1"].toInt()) + "," +
                                  QString::number(rec["x2"].toInt()) + "," +
                                  QString::number(rec["y1"].toInt()) + "," +
                                  QString::number(rec["y2"].toInt()) + "," +
                                  QString::number(rec.value("x3").toInt(0xFF)) + "," +
                                  QString::number(rec.value("y3").toInt(0xFF)) + "," +
                                  QString::number(rec["offset"].toInt()) + "," +
                                  QString::number(rec["value"].toInt());
                }
                si->ImportCameraControl(camType, recordsStr);
            }

            si->ResetRoomEntitySet(currentRoomId);
            si->PostImportRefresh();
            return ok({{"success", true}, {"roomId", currentRoomId}});
        });

    // ========================================================================
    // Knowledge & Scripting
    // ========================================================================

    server->registerTool("wl4_read_knowledge",
        "Read a game knowledge file from the romhack's game_knowledge/ folder (or repo "
        "fallback). Available files: entity_catalog (entity types from DoorConfigDialog, "
        "all 129 types with behaviors), tileset_catalog (tileset names/themes from "
        "RoomConfigDialog), tile_reference (complete event ID and terrain type tables), "
        "design_guide (level design rules, engine limits, camera box info, door/entityset "
        "info, Wario physics), patch_notes (ROM patch documentation). "
        "File name without extension; .json and .md tried automatically.",
        makeSchema({
            P("file", "string", "Knowledge file name: entity_catalog, tileset_catalog, tile_reference, design_guide, patch_notes")
        }, {"file"}),
        [si](const QJsonObject &args) -> QJsonObject {
            QString fileName = argStr(args, "file");
            if (fileName.isEmpty())
                return makeErrorResult("Missing 'file'. Available: entity_catalog, tileset_catalog, tile_reference, design_guide, patch_notes");

            QString romDir = singleton->GetdDialogInitialPath();
            QString knowledgePath;

            auto tryPath = [&](const QString &base, const QString &name) -> QString {
                if (!name.contains('.')) {
                    if (QFile::exists(base + "/" + name + ".json"))
                        return base + "/" + name + ".json";
                    if (QFile::exists(base + "/" + name + ".md"))
                        return base + "/" + name + ".md";
                }
                QString p = base + "/" + name;
                return QFile::exists(p) ? p : QString();
            };

            if (!romDir.isEmpty())
                knowledgePath = tryPath(romDir + "/game_knowledge", fileName);
            if (knowledgePath.isEmpty())
                knowledgePath = tryPath(
                    QCoreApplication::applicationDirPath() + "/../MCP/knowledge", fileName);
            if (knowledgePath.isEmpty())
                knowledgePath = tryPath(
                    QCoreApplication::applicationDirPath() + "/MCP/knowledge", fileName);
            if (knowledgePath.isEmpty())
                return makeErrorResult(QString("Knowledge file not found: %1\n"
                    "Looked in: %2/game_knowledge/, %3/../MCP/knowledge/, %3/MCP/knowledge/")
                    .arg(fileName).arg(romDir).arg(QCoreApplication::applicationDirPath()));

            QFile f(knowledgePath);
            if (!f.open(QFile::ReadOnly | QFile::Text))
                return makeErrorResult("Cannot read: " + knowledgePath);
            QString content = QTextStream(&f).readAll();
            f.close();
            return makeResult({{"file", fileName}, {"path", knowledgePath},
                               {"content", content}, {"size", static_cast<int>(content.size())}});
        });

    server->registerTool("wl4_eval",
        "Execute JavaScript code in WL4Editor's engine. Has access to all ScriptInterface "
        "APIs via the WL4EditorInterface.* global object. Use for advanced or bulk "
        "operations not covered by dedicated tools. The Output Window must be open "
        "(View > Output Window). Returns the JS evaluation result or error details.",
        makeSchema({
            P("code", "string", "JavaScript code to execute. Access WL4EditorInterface.* for all editor APIs.")
        }, {"code"}),
        [si](const QJsonObject &args) -> QJsonObject {
            QString code = argStr(args, "code");
            if (code.isEmpty())
                return makeErrorResult("Missing 'code' parameter");
            auto *outputWidget = singleton->GetOutputWidgetPtr();
            if (!outputWidget)
                return makeErrorResult("Output dock is not open. Open View > Output Window first.");

            // Clear any stale log output before executing
            si->GetAndClearLogBuffer();

            QJSValue result = outputWidget->ExecuteJSScript(code, true);

            // Collect log buffer — contains error messages from log() calls
            // made by ScriptInterface methods during JS execution
            QString logOutput = si->GetAndClearLogBuffer();

            QJsonObject r;
            r["success"] = !result.isError();
            if (result.isError())
            {
                r["error"] = result.toString();
                r["line"] = result.property("lineNumber").toInt();
            }
            else
            {
                r["result"] = result.toString();
            }

            if (!logOutput.isEmpty())
            {
                r["log"] = logOutput.trimmed();
                // Flag common error patterns so AI knows to inspect the log
                if (logOutput.contains("Illegal", Qt::CaseInsensitive) ||
                    logOutput.contains("out of range", Qt::CaseInsensitive) ||
                    logOutput.contains("out of bounds", Qt::CaseInsensitive) ||
                    logOutput.contains("Invalid", Qt::CaseInsensitive) ||
                    logOutput.contains("failed", Qt::CaseInsensitive) ||
                    logOutput.contains("Cannot", Qt::CaseInsensitive) ||
                    logOutput.contains("Position", Qt::CaseInsensitive) ||
                    logOutput.contains("Corruption", Qt::CaseInsensitive))
                {
                    r["warning"] = "The log contains error messages — the operation may have "
                                   "failed silently even though the JS didn't throw.";
                }
            }

            return makeResult(r);
        });
}
