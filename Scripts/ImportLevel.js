// WL4Editor level import script - imports a full level from a JSON file
// Uses individual fine-grained import APIs for step-by-step import
// Doors are bulk-imported at the end via raw door vector string

(function () {
    // Helper: join hex row strings into a single hex string
    function joinRowHexStrings(rows) {
        return rows.join("");
    }

    // Helper: convert entities array to hex triple string "YPos XPos EntityID ..."
    function entitiesToHexTriples(entities) {
        var result = "";
        for (var i = 0; i < entities.length; i++) {
            var e = entities[i];
            if (result !== "") result += " ";
            result += e.yPos.toString(16).toUpperCase() + " " +
                      e.xPos.toString(16).toUpperCase() + " " +
                      e.entityId.toString(16).toUpperCase();
        }
        return result;
    }

    // Helper: serialize all doors into raw hex byte string for LevelDoorVector(QString&) constructor.
    // Each door becomes 12 hex bytes: DoorTypeByte, RoomID, x1, x2, y1, y2,
    // DestinationDoorGlobalID, HorizontalDeltaWario, VerticalDeltaWario, EntitySetID, BGM_ID(low), BGM_ID(high)
    function buildDoorVecRawHex(allDoors) {
        var parts = [];
        for (var i = 0; i < allDoors.length; i++) {
            var d = allDoors[i];
            var bgmLow = d.bgm & 0xFF;
            var bgmHigh = (d.bgm >> 8) & 0xFF;
            parts.push(
                "0x" + (d.type & 0xFF).toString(16),
                "0x" + (d.roomID & 0xFF).toString(16),
                "0x" + (d.x1 & 0xFF).toString(16),
                "0x" + (d.x2 & 0xFF).toString(16),
                "0x" + (d.y1 & 0xFF).toString(16),
                "0x" + (d.y2 & 0xFF).toString(16),
                "0x" + (d.destID & 0xFF).toString(16),
                "0x" + (d.dx & 0xFF).toString(16),
                "0x" + (d.dy & 0xFF).toString(16),
                "0x" + (d.entitySetID & 0xFF).toString(16),
                "0x" + bgmLow.toString(16),
                "0x" + bgmHigh.toString(16)
            );
        }
        return parts.join(", ");
    }

    // Helper: convert camera records to delimited string "trans,x1,x2,y1,y2,x3,y3,offset,value;..."
    function cameraRecordsToString(records) {
        var result = "";
        for (var i = 0; i < records.length; i++) {
            var r = records[i];
            if (result !== "") result += ";";
            result += r.trans + "," + r.x1 + "," + r.x2 + "," + r.y1 + "," + r.y2 + "," + r.x3 + "," + r.y3 + "," + r.offset + "," + r.value;
        }
        return result;
    }

    // Helper: read a little-endian 32-bit value from the room header hex string.
    function readU32LE(hex, byteOffset) {
        var b0 = parseInt(hex.substr(byteOffset * 2, 2), 16);
        var b1 = parseInt(hex.substr(byteOffset * 2 + 2, 2), 16);
        var b2 = parseInt(hex.substr(byteOffset * 2 + 4, 2), 16);
        var b3 = parseInt(hex.substr(byteOffset * 2 + 6, 2), 16);
        return ((b0 | (b1 << 8) | (b2 << 16) | (b3 << 24)) >>> 0) & 0x7FFFFFF;
    }

    function applyHeaderLayerPointers(roomId, headerHex) {
        if (!headerHex || headerHex.length < 88) return;
        var layer0Type = parseInt(headerHex.substr(2, 2), 16) & 0x3F;
        var layer3Type = parseInt(headerHex.substr(8, 2), 16) & 0x3F;
        if (layer3Type !== 0) {
            var layer3Addr = readU32LE(headerHex, 20);
            if (layer3Addr) WL4EditorInterface.SetRoomLayerMappingDataAddress(roomId, 3, layer3Addr);
        }
        if ((layer0Type & 0x30) === 0x20) {
            var layer0Addr = readU32LE(headerHex, 8);
            if (layer0Addr) WL4EditorInterface.SetRoomLayerMappingDataAddress(roomId, 0, layer0Addr);
        }
    }

    var fileContents = WL4EditorInterface.ReadJsonFileDialog();
    if (!fileContents || fileContents.trim() === "") {
        WL4EditorInterface.log("Level import cancelled: no file selected or file is empty.");
        return;
    }

    // Parse JSON
    var levelObj;
    try {
        levelObj = JSON.parse(fileContents);
    } catch (e) {
        WL4EditorInterface.alert("Failed to parse JSON: " + e.message);
        return;
    }

    // Validate basic structure
    if (!levelObj.version || levelObj.levelName === undefined) {
        WL4EditorInterface.alert("Invalid level JSON: missing required fields.");
        return;
    }

    // Helper: extract one Level's slice from the legacy global wall paint data.
    // Legacy level files stored the wall paint of all passages and local levels
    // in one global block. The source Level indices from the file select the
    // slice, which is then written to the current Level by ImportLevelWallPaint.
    function importLegacyWallPaint(legacy) {
        var lh = levelObj.levelHeader || {};
        var passage = lh.passage;
        var level = lh.stage;
        if (passage === undefined || level === undefined) {
            // Older files without a level header fall back to the current Level.
            passage = WL4EditorInterface.ExportGetLevelPassage();
            level = WL4EditorInterface.ExportGetLevelStage();
        }
        if (passage < 0 || passage > 5 || level < 0 || level > 4) {
            WL4EditorInterface.log("ImportLevel: legacy wall paint data has no valid Level indices, skipped.");
            return true;
        }

        var gfxHex = legacy.gfxHex || "";
        var colorHex = legacy.passageColorHex || "";
        var gfxSize = 1024 * 5 * 6;
        var palSize = 32 * 5 * 6;
        if (gfxHex.length !== gfxSize * 2 ||
            colorHex.length !== palSize * 2) {
            WL4EditorInterface.log("ImportLevel: legacy wall paint data has an unexpected size, skipped.");
            return true;
        }

        function sliceHex(hex, byteOffset, byteLength) {
            return hex.substr(byteOffset * 2, byteLength * 2);
        }

        var gfxParts = "";
        for (var c = 0; c < 5; c++) {
            gfxParts += sliceHex(
                gfxHex,
                (1024 * 5) * passage + (5 * 32) * level + c * 1024,
                5 * 32
            );
        }

        var paletteOffset = 32 * 5 * passage + 32 * level;
        WL4EditorInterface.log("ImportLevel: importing legacy wall paint data for Level " +
                               passage + "-" + level + ".");
        return WL4EditorInterface.ImportLevelWallPaint(
            gfxParts,
            sliceHex(colorHex, paletteOffset, 32)
        );
    }

    // Ask the user to save pending changes before the Graphic Manager data is written to the ROM.
    if (!WL4EditorInterface.EnsureLevelSavedForGraphicImport()) {
        WL4EditorInterface.log("Level import cancelled: the Level was not saved.");
        return;
    }

    // ---------- Import or reuse the Graphic Manager entries used by the Level ----------
    var resolvedGraphics = null;
    if (levelObj.graphics && levelObj.graphics.entries && levelObj.graphics.entries.length) {
        var graphicResult = JSON.parse(
            WL4EditorInterface.ImportGraphicEntries(JSON.stringify(levelObj.graphics.entries))
        );
        if (!graphicResult.success) {
            WL4EditorInterface.alert("ImportGraphicEntries failed: " + graphicResult.error);
            return;
        }
        resolvedGraphics = graphicResult.entries;
    }

    // ---------- Import global data (per-element with Undo/Redo compatibility) ----------
    var globals = levelObj.changedGlobals || {};

    var tilesets = globals.tilesets || [];
    for (var ti = 0; ti < tilesets.length; ti++) {
        var ts = tilesets[ti];
        WL4EditorInterface.ImportTileset(
            ts.id,
            ts.fggfxPtr,
            ts.fggfxLen,
            ts.tile8x8Data,
            ts.map16Data,
            ts.paletteData,
            ts.eventTable,
            ts.terrainTable,
            ts.animatedSwitchTable,
            ts.animatedTileData0,
            ts.animatedTileData1
        );
    }

    var entities = globals.entities || [];
    for (var ei = 0; ei < entities.length; ei++) {
        var ent = entities[ei];
        WL4EditorInterface.ImportEntity(
            ent.id,
            ent.paletteData.join("\n"),
            ent.tile8x8Data.join("\n")
        );
    }

    var entitySets = globals.entitySets || [];
    for (var esi = 0; esi < entitySets.length; esi++) {
        WL4EditorInterface.ImportEntitySet(
            entitySets[esi].id,
            entitySets[esi].infoTable.join("\n")
        );
    }

    var animatedTileGroups = globals.animatedTileGroups || [];
    for (var agi = 0; agi < animatedTileGroups.length; agi++) {
        var ag = animatedTileGroups[agi];
        WL4EditorInterface.ImportAnimatedTileGroup(
            ag.id,
            ag.animType,
            ag.countPerFrame,
            ag.totalFrameCount,
            ag.tileData.join("\n")
        );
    }

    // ---------- Import the current Level's wall paint ----------
    // Only the GFX and the colored passage palette are stored. The gray,
    // start-level gradient and MMAP palettes are generated from the colored
    // palette by ImportLevelWallPaint, following the Wall Paint editor logic.
    if (levelObj.wallPaintData) {
        var wp = levelObj.wallPaintData;
        if (!WL4EditorInterface.ImportLevelWallPaint(
                wp.gfxHex || "",
                wp.passageColorHex || "")) {
            WL4EditorInterface.alert("ImportLevelWallPaint failed.");
            return;
        }
    } else if (globals.wallPaintData) {
        // Legacy files stored all Levels' wall paint in one global block.
        if (!importLegacyWallPaint(globals.wallPaintData)) {
            WL4EditorInterface.alert("ImportLevelWallPaint (legacy) failed.");
            return;
        }
    }

    if (globals.creditsData && globals.creditsData !== "") {
        WL4EditorInterface.ImportGlobalCredits(globals.creditsData);
    }

    // ---------- Import each room (skip doors — handled in bulk at the end) ----------
    var currentRoomCount = WL4EditorInterface.GetRoomNum();
    var rooms = levelObj.rooms || [];
    if (rooms.length > currentRoomCount) {
        WL4EditorInterface.log("JSON has " + rooms.length + " rooms, but current level has " +
                               currentRoomCount + ". Adding " + (rooms.length - currentRoomCount) + " rooms...");
        while (WL4EditorInterface.GetRoomNum() < rooms.length) {
            var newId = WL4EditorInterface.AddNewRoom();
            if (newId < 0) {
                WL4EditorInterface.alert("Failed to add room (max 16).");
                return;
            }
        }
    }
    for (var i = 0; i < rooms.length; i++) {
        var room = rooms[i];
        WL4EditorInterface.SetCurrentRoomId(room.roomId);

        // ---- Step 1: Import room config ----
        var rc = room.roomConfig || {};
        var roomWidth = rc.roomWidth || 32;
        var roomHeight = rc.roomHeight || 20;
        var layer0Width = rc.layer0Width || 32;
        var layer0Height = rc.layer0Height || 20;
        var roomHeaderHex = room.roomHeaderHex || "";

        if (!WL4EditorInterface.ImportRoomConfig(roomWidth, roomHeight, layer0Width, layer0Height, roomHeaderHex)) {
            WL4EditorInterface.alert("ImportRoomConfig failed for room " + room.roomId + ".");
            return;
        }

        // ---- Step 2: Import layer tiles (layers 0-2 only, Map16 layers only) ----
        var layers = room.layers || [];
        for (var li = 0; li < layers.length; li++) {
            var layer = layers[li];
            if (layer.layerId < 0 || layer.layerId > 2) continue;
            if (layer.mappingType < 0x10 || layer.mappingType >= 0x20) continue;
            if (!layer.data || layer.data.length === 0) continue;

            var tileDataHex = joinRowHexStrings(layer.data);
            if (!WL4EditorInterface.ImportLayerTiles(layer.layerId, layer.width, layer.height, tileDataHex)) {
                WL4EditorInterface.alert("ImportLayerTiles failed for room " + room.roomId + " layer " + layer.layerId + ".");
                return;
            }
        }

        // ---- Step 3: Import entity lists ----
        var entityLists = room.entityLists || [];
        for (var ei = 0; ei < entityLists.length; ei++) {
            var listObj = entityLists[ei];
            var difficulty = listObj.difficulty;
            if (difficulty < 0 || difficulty > 2) continue;
            var entities = listObj.entities || [];
            var entityDataHex = entitiesToHexTriples(entities);
            if (!WL4EditorInterface.ImportEntityList(difficulty, entityDataHex)) {
                WL4EditorInterface.alert("ImportEntityList failed for room " + room.roomId + " difficulty " + difficulty + ".");
                return;
            }
        }

        // ---- Step 4: Import camera control ----
        var cameraControl = room.cameraControl || {};
        if (cameraControl.type !== undefined) {
            var records = cameraControl.records || [];
            var recordsStr = cameraRecordsToString(records);
            if (!WL4EditorInterface.ImportCameraControl(cameraControl.type, recordsStr)) {
                WL4EditorInterface.alert("ImportCameraControl failed for room " + room.roomId + ".");
                return;
            }
        }

        // Version 1 fallback: the graphics section is missing, so the layer pointers are taken
        // from the room header. This only works when the target ROM already contains the data.
        if (!resolvedGraphics && room.roomHeaderHex) {
            applyHeaderLayerPointers(room.roomId, room.roomHeaderHex);
        }
    }

    // ---------- Apply the Graphic Manager data to the Tilesets and the Room layers ----------
    if (resolvedGraphics) {
        if (!WL4EditorInterface.ApplyLevelGraphics(JSON.stringify(levelObj.graphics),
                                                    JSON.stringify(resolvedGraphics), true)) {
            WL4EditorInterface.alert("ApplyLevelGraphics failed.");
            return;
        }
    } else {
        WL4EditorInterface.log("Level import: no graphics section, used the header layer pointers.");
    }

    // ---------- Bulk import all doors ----------
    // Collect all doors sorted by globalDoorID so destIDs naturally match
    // the new global positions without any remapping.
    var allDoors = [];
    var nextGid = 0;
    while (true) {
        var found = false;
        for (var i = 0; i < rooms.length && !found; i++) {
            var roomDoors = rooms[i].doors || [];
            for (var j = 0; j < roomDoors.length && !found; j++) {
                if (roomDoors[j].globalDoorID === nextGid) {
                    var src = roomDoors[j];
                    allDoors.push({
                        type:         src.type,
                        roomID:       src.roomID,
                        x1:           src.x1,
                        x2:           src.x2,
                        y1:           src.y1,
                        y2:           src.y2,
                        destID:       src.destID,
                        dx:           src.dx,
                        dy:           src.dy,
                        entitySetID:  src.entitySetID,
                        bgm:          src.bgm,
                        globalDoorID: src.globalDoorID
                    });
                    found = true;
                }
            }
        }
        if (!found) break;
        nextGid++;
    }
    if (allDoors.length > 0) {
        // Serialize and replace the entire door vector.
        // Sorting by globalDoorID guarantees new position equals old global ID,
        // so destIDs are already correct (no remapping needed).
        var doorVecHex = buildDoorVecRawHex(allDoors);
        WL4EditorInterface.log("ImportLevel: bulk-importing " + allDoors.length +
                               " doors across " + rooms.length + " rooms");
        if (!WL4EditorInterface.ImportDoorVecString(doorVecHex)) {
            WL4EditorInterface.alert("ImportDoorVecString failed.");
            return;
        }
    }

    // ---------- Import level configuration ----------
    var lh = levelObj.levelHeader || {};
    WL4EditorInterface.ImportLevelConfig(
        levelObj.levelName || "",
        levelObj.levelNameJ || "",
        lh.timerHard || 0,
        lh.timerNormal || 0,
        lh.timerSHard || 0
    );

    // ---------- Finalize ----------
    WL4EditorInterface.PostImportRefresh();
    WL4EditorInterface.log("Level import completed. Save the Level to persist the Room/Tileset changes.");
})();
