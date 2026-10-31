// WL4Editor room import script - imports a room from a JSON file into the current room
// Uses individual fine-grained import APIs for step-by-step import

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

    // Helper: convert door objects array to delimited string "type,roomID,x1,x2,y1,y2,destID,dx,dy,entitySetID,bgm;..."
    function doorsObjToString(doors) {
        var result = "";
        for (var i = 0; i < doors.length; i++) {
            var d = doors[i];
            if (result !== "") result += ";";
            result += d.type + "," + d.roomID + "," + d.x1 + "," + d.x2 + "," + d.y1 + "," + d.y2 + "," +
                      d.destID + "," + d.dx + "," + d.dy + "," + d.entitySetID + "," + d.bgm + "," + d.globalDoorID;
        }
        return result;
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

    var fileContents = WL4EditorInterface.ReadJsonFileDialog();
    if (!fileContents || fileContents.trim() === "") {
        WL4EditorInterface.log("Room import cancelled: no file selected or file is empty.");
        return;
    }

    // Parse JSON
    var roomObj;
    try {
        roomObj = JSON.parse(fileContents);
    } catch (e) {
        WL4EditorInterface.alert("Failed to parse JSON: " + e.message);
        return;
    }

    // Validate that we have required fields
    if (roomObj.roomId === undefined && roomObj.version === undefined) {
        WL4EditorInterface.alert("Invalid room JSON: missing required fields (roomId / version).");
        return;
    }

    var currentRoomId = WL4EditorInterface.GetCurRoomId();

    // Ask the user to save pending changes before the Graphic Manager data is written to the ROM.
    if (!WL4EditorInterface.EnsureLevelSavedForGraphicImport()) {
        WL4EditorInterface.log("Room import cancelled: the Level was not saved.");
        return;
    }

    // ---- Step 0: Import or reuse the Graphic Manager entries used by this Room ----
    var resolvedGraphics = null;
    if (roomObj.graphics && roomObj.graphics.entries && roomObj.graphics.entries.length) {
        var graphicResult = JSON.parse(
            WL4EditorInterface.ImportGraphicEntries(JSON.stringify(roomObj.graphics.entries))
        );
        if (!graphicResult.success) {
            WL4EditorInterface.alert("ImportGraphicEntries failed: " + graphicResult.error);
            return;
        }
        resolvedGraphics = graphicResult.entries;
    }

    // ---- Step 1: Import room config ----
    var rc = roomObj.roomConfig || {};
    var roomWidth = rc.roomWidth || 32;
    var roomHeight = rc.roomHeight || 20;
    var layer0Width = rc.layer0Width || 32;
    var layer0Height = rc.layer0Height || 20;
    var roomHeaderHex = roomObj.roomHeaderHex || "";

    if (!WL4EditorInterface.ImportRoomConfig(roomWidth, roomHeight, layer0Width, layer0Height, roomHeaderHex)) {
        WL4EditorInterface.alert("ImportRoomConfig failed.");
        return;
    }

    // ---- Step 2: Import layer tiles (layers 0-2 only, Map16 layers only) ----
    var layers = roomObj.layers || [];
    for (var li = 0; li < layers.length; li++) {
        var layer = layers[li];
        if (layer.layerId < 0 || layer.layerId > 2) continue;
        if (layer.mappingType < 0x10 || layer.mappingType >= 0x20) continue; // Only Map16
        if (!layer.data || layer.data.length === 0) continue;

        var tileDataHex = joinRowHexStrings(layer.data);
        if (!WL4EditorInterface.ImportLayerTiles(layer.layerId, layer.width, layer.height, tileDataHex)) {
            WL4EditorInterface.alert("ImportLayerTiles failed for layer " + layer.layerId + ".");
            return;
        }
    }

    // ---- Step 3: Import doors (disable destinations - single room import) ----
    var doors = roomObj.doors || [];
    var doorsStr = doorsObjToString(doors);
    if (!WL4EditorInterface.ImportDoorsDisableDest(doorsStr)) {
        WL4EditorInterface.alert("ImportDoorsDisableDest failed.");
        return;
    }

    // ---- Step 4: Import entity lists ----
    var entityLists = roomObj.entityLists || [];
    for (var ei = 0; ei < entityLists.length; ei++) {
        var listObj = entityLists[ei];
        var difficulty = listObj.difficulty;
        if (difficulty < 0 || difficulty > 2) continue;
        var entities = listObj.entities || [];
        var entityDataHex = entitiesToHexTriples(entities);
        if (!WL4EditorInterface.ImportEntityList(difficulty, entityDataHex)) {
            WL4EditorInterface.alert("ImportEntityList failed for difficulty " + difficulty + ".");
            return;
        }
    }

    // ---- Step 5: Import camera control ----
    var cameraControl = roomObj.cameraControl || {};
    if (cameraControl.type !== undefined) {
        var records = cameraControl.records || [];
        var recordsStr = cameraRecordsToString(records);
        if (!WL4EditorInterface.ImportCameraControl(cameraControl.type, recordsStr)) {
            WL4EditorInterface.alert("ImportCameraControl failed.");
            return;
        }
    }

    // ---- Step 6: Apply the Graphic Manager data to the Tileset and the Room layers ----
    if (resolvedGraphics) {
        if (!WL4EditorInterface.ApplyRoomGraphics(currentRoomId, JSON.stringify(roomObj.graphics),
                                                   JSON.stringify(resolvedGraphics), true)) {
            WL4EditorInterface.alert("ApplyRoomGraphics failed.");
            return;
        }
    } else if (roomHeaderHex.length >= 88) {
        // Version 1 fallback: the graphics section is missing, so the layer pointers are taken
        // from the room header. This only works when the target ROM already contains the data.
        var layer0Type = parseInt(roomHeaderHex.substr(2, 2), 16) & 0x3F;
        var layer3Type = parseInt(roomHeaderHex.substr(8, 2), 16) & 0x3F;
        if (layer3Type !== 0) {
            var layer3Addr = readU32LE(roomHeaderHex, 20);
            if (layer3Addr) {
                WL4EditorInterface.SetRoomLayerMappingDataAddress(currentRoomId, 3, layer3Addr);
            }
        }
        if ((layer0Type & 0x30) === 0x20) {
            var layer0Addr = readU32LE(roomHeaderHex, 8);
            if (layer0Addr) {
                WL4EditorInterface.SetRoomLayerMappingDataAddress(currentRoomId, 0, layer0Addr);
            }
        }
        WL4EditorInterface.log("Room import: no graphics section, used the header layer pointers.");
    }

    WL4EditorInterface.ResetRoomEntitySet(currentRoomId);

    // ---- Step 7: Finalize ----
    WL4EditorInterface.PostImportRefresh();
    WL4EditorInterface.log("Room import completed. Save the Level to persist the Room/Tileset changes.");
})();
