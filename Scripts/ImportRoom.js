// WL4Editor room import script - imports a room from a JSON file into the current room
// Uses individual fine-grained import APIs for step-by-step import

(function () {
    // Helper: convert array of numbers to hex string (each number as 4 hex chars)
    function arrayToTileHex(arr) {
        var result = "";
        for (var i = 0; i < arr.length; i++) {
            var hex = arr[i].toString(16);
            while (hex.length < 4) hex = "0" + hex;
            result += hex;
        }
        return result;
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
                      d.destID + "," + d.dx + "," + d.dy + "," + d.entitySetID + "," + d.bgm;
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

    // Save old door global IDs before import, so we can delete them after
    // new doors are added (DeleteDoor refuses to delete the last door of a room)
    var oldDoorIdsStr = WL4EditorInterface.GetCurRoomDoorGlobalIds();
    var oldDoorIds = oldDoorIdsStr ? oldDoorIdsStr.split(",") : [];

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

    // ---- Step 2: Import layer tiles (layers 0-2 only, Map16 layers) ----
    var layers = roomObj.layers || [];
    var nonOriginalPtrs = roomObj.nonOriginalLayerPointers || {};
    for (var li = 0; li < layers.length; li++) {
        var layer = layers[li];
        if (layer.layerId < 0 || layer.layerId > 2) continue;
        if (layer.mappingType < 0x10 || layer.mappingType >= 0x20) continue; // Only Map16
        if (!layer.data || layer.data.length === 0) continue;
        // Skip layer if it used a non-original pointer (data won't exist in target ROM)
        if (layer.layerId === 0 && nonOriginalPtrs.layer0 >= 0x78F970) continue;
        if (layer.layerId === 3 && nonOriginalPtrs.layer3 >= 0x78F970) continue;

        var tileDataHex = arrayToTileHex(layer.data);
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

    // ---- Step 6: Delete old doors (now that new doors are in place) ----
    for (var di = 0; di < oldDoorIds.length; di++) {
        var gid = parseInt(oldDoorIds[di], 10);
        if (!isNaN(gid) && gid > 0) {
            WL4EditorInterface.DeleteDoorByGlobalId(gid);
        }
    }
    WL4EditorInterface.ResetRoomEntitySet(currentRoomId);

    // ---- Step 7: Finalize ----
    WL4EditorInterface.PostImportRefresh();
    WL4EditorInterface.log("Room import completed.");
})();
