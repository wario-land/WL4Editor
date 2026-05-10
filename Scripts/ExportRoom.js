// WL4Editor room export script - exports the currently selected room to a JSON file

(function () {
    WL4EditorInterface.log("Exporting current room...");

    var roomId = WL4EditorInterface.GetCurRoomId();

    // Get the room header hex string (44 bytes = 88 hex chars, captures all header fields)
    var headerHex = WL4EditorInterface.ExportGetRoomHeaderHex();

    // Build the room JSON object
    var roomObj = {
        version: 1,
        roomId: roomId,
        roomConfig: {
            roomWidth: WL4EditorInterface.GetCurRoomLayerWidth(1),
            roomHeight: WL4EditorInterface.GetCurRoomLayerHeight(1),
            layer0Width: WL4EditorInterface.GetCurRoomLayerWidth(0),
            layer0Height: WL4EditorInterface.GetCurRoomLayerHeight(0)
        },
        roomHeaderHex: headerHex,
        layers: [],
        entityLists: [],
        entitySetId: WL4EditorInterface.ExportGetEntitySetId(),
        doors: [],
        cameraControl: {},
        nonOriginalLayerPointers: {
            layer0: WL4EditorInterface.ExportGetLayerDataPtr(0),
            layer3: WL4EditorInterface.ExportGetLayerDataPtr(3)
        }
    };

    // Build layers (4 layers: 0 through 3)
    for (var i = 0; i < 4; i++) {
        var mappingType = (parseInt(headerHex.substr((1 + i) * 2, 2), 16) & 0x3F);
        var width = WL4EditorInterface.GetCurRoomLayerWidth(i);
        var height = WL4EditorInterface.GetCurRoomLayerHeight(i);
        var layerData = [];

        if (mappingType >= 0x10) {
            if (mappingType < 0x20) {
                // Map16 layer — each row is a compact hex string (4 chars per tile)
                for (var y = 0; y < height; y++) {
                    var rowHex = "";
                    for (var x = 0; x < width; x++) {
                        var h = WL4EditorInterface.GetCurRoomTile16(i, x, y).toString(16);
                        while (h.length < 4) h = "0" + h;
                        rowHex += h;
                    }
                    layerData.push(rowHex);
                }
            } else {
                // Tile8x8 layer
                for (var y = 0; y < height; y++) {
                    var rowHex = "";
                    for (var x = 0; x < width; x++) {
                        var h = WL4EditorInterface.GetCurRoomTile8(i, x, y).toString(16);
                        while (h.length < 4) h = "0" + h;
                        rowHex += h;
                    }
                    layerData.push(rowHex);
                }
            }
        }

        roomObj.layers.push({
            layerId: i,
            mappingType: mappingType,
            width: width,
            height: height,
            data: layerData
        });
    }

    // Build entity lists (difficulty 0 = Hard, 1 = Normal, 2 = SHard)
    for (var d = 0; d < 3; d++) {
        var entityDataStr = WL4EditorInterface.GetEntityListData(d);
        var entities = [];

        if (entityDataStr && entityDataStr.trim() !== "") {
            var tokens = entityDataStr.trim().split(/\s+/);
            for (var t = 0; t + 2 < tokens.length; t += 3) {
                entities.push({
                    yPos: parseInt(tokens[t], 16),
                    xPos: parseInt(tokens[t + 1], 16),
                    entityId: parseInt(tokens[t + 2], 16)
                });
            }
        }

        roomObj.entityLists.push({
            difficulty: d,
            entities: entities
        });
    }

    // Parse doors (semicolon-separated entries, comma-separated fields)
    var doorsStr = WL4EditorInterface.ExportGetDoorsFullData();
    if (doorsStr && doorsStr.trim() !== "") {
        var doorEntries = doorsStr.split(";");
        for (var di = 0; di < doorEntries.length; di++) {
            var doorEntry = doorEntries[di].trim();
            if (doorEntry !== "") {
                var df = doorEntry.split(",");
                roomObj.doors.push({
                    type: parseInt(df[0], 10),
                    roomID: parseInt(df[1], 10),
                    x1: parseInt(df[2], 10),
                    x2: parseInt(df[3], 10),
                    y1: parseInt(df[4], 10),
                    y2: parseInt(df[5], 10),
                    destID: parseInt(df[6], 10),
                    dx: parseInt(df[7], 10),
                    dy: parseInt(df[8], 10),
                    entitySetID: parseInt(df[9], 10),
                    bgm: parseInt(df[10], 10)
                });
            }
        }
    }

    // Parse camera control records (semicolon-separated entries, comma-separated fields)
    var cameraControlType = WL4EditorInterface.ExportGetCameraControlType();
    var cameraRecordsStr = WL4EditorInterface.ExportGetCameraControlRecords();
    var cameraRecords = [];

    if (cameraRecordsStr && cameraRecordsStr.trim() !== "") {
        var recordEntries = cameraRecordsStr.split(";");
        for (var ri = 0; ri < recordEntries.length; ri++) {
            var recordEntry = recordEntries[ri].trim();
            if (recordEntry !== "") {
                var rf = recordEntry.split(",");
                cameraRecords.push({
                    trans: parseInt(rf[0], 10),
                    x1: parseInt(rf[1], 10),
                    x2: parseInt(rf[2], 10),
                    y1: parseInt(rf[3], 10),
                    y2: parseInt(rf[4], 10),
                    x3: parseInt(rf[5], 10),
                    y3: parseInt(rf[6], 10),
                    offset: parseInt(rf[7], 10),
                    value: parseInt(rf[8], 10)
                });
            }
        }
    }

    roomObj.cameraControl = {
        type: cameraControlType,
        records: cameraRecords
    };

    // Write to file (empty path triggers save dialog)
    WL4EditorInterface.WriteJsonFile("", JSON.stringify(roomObj, null, 2));

    WL4EditorInterface.log("Room export completed.");
})();
