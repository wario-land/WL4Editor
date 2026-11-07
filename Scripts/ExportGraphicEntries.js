// WL4Editor graphic entry export script
// Exports the Graphic Manager entries used by the current Room, the current Level, or all entries.
//
// The top-level wrapper is defined by this script. The entry objects use the documented schema in
// Scripts/README.md, which is produced by the C++ API. Add metadata or change the wrapper here if
// you need a different file layout for your own tools.

(function () {
    // "room" | "level" | "all"
    var SCOPE = "room";

    // -1 means the current Room. Only used when SCOPE is "room".
    var ROOM_ID = -1;

    WL4EditorInterface.log("Exporting Graphic Manager entries (" + SCOPE + ")...");

    var result;
    if (SCOPE === "all") {
        result = JSON.parse(WL4EditorInterface.ExportGetAllGraphicEntries());
    } else if (SCOPE === "level") {
        result = JSON.parse(WL4EditorInterface.ExportGetLevelGraphicEntries());
    } else {
        result = JSON.parse(WL4EditorInterface.ExportGetRoomGraphicEntries(ROOM_ID));
    }

    if (result.error) {
        WL4EditorInterface.alert("Graphic entry export failed: " + result.error);
        return;
    }

    // The API already returns the documented key names. This wrapper is the single place where
    // the exported object is assembled, so a user can add metadata or rename the top-level keys.
    var output = {
        version: 1,
        scope: SCOPE,
        graphics: result
    };

    WL4EditorInterface.WriteJsonFile("", JSON.stringify(output, null, 2));
    WL4EditorInterface.log("Graphic entry export completed.");
})();
