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

    if (globals.wallPaintData) {
        var wp = globals.wallPaintData;
        WL4EditorInterface.ImportGlobalWallPaint(
            wp.gfxHex,
            wp.passageColorHex,
            wp.passageGrayHex
        );
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
    WL4EditorInterface.log("Level import completed.");
})();
