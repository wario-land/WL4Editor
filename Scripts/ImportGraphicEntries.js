// WL4Editor graphic entry import script
// Imports the Graphic Manager entries of a JSON file and optionally applies them to the current
// Room or Level. Entries which already exist in the ROM are reused; only missing entries are added.
//
// The file may be:
//   - the output of ExportGraphicEntries.js (an object with a "graphics" section),
//   - a Room or Level JSON file (the "graphics" section is used),
//   - an object with an "entries" array,
//   - a bare array of entry objects.

(function () {
    // Set to false when you only want to import the entries into the Graphic Manager list and
    // apply them manually later.
    var APPLY_TO_ROOM_OR_LEVEL = true;

    var fileContents = WL4EditorInterface.ReadJsonFileDialog();
    if (!fileContents || fileContents.trim() === "") {
        WL4EditorInterface.log("Graphic entry import cancelled: no file selected or file is empty.");
        return;
    }

    var parsed;
    try {
        parsed = JSON.parse(fileContents);
    } catch (e) {
        WL4EditorInterface.alert("Failed to parse JSON: " + e.message);
        return;
    }

    var graphics = parsed.graphics;
    if (!graphics) {
        if (parsed.entries) {
            graphics = parsed;
        } else if (Array.isArray(parsed)) {
            graphics = { version: 1, entries: parsed };
        }
    }
    if (!graphics || !graphics.entries || !graphics.entries.length) {
        WL4EditorInterface.alert("The file does not contain any graphic entries.");
        return;
    }

    if (!WL4EditorInterface.EnsureLevelSavedForGraphicImport()) {
        WL4EditorInterface.log("Graphic entry import cancelled: the Level was not saved.");
        return;
    }

    var importResult = JSON.parse(
        WL4EditorInterface.ImportGraphicEntries(JSON.stringify(graphics.entries))
    );
    if (!importResult.success) {
        WL4EditorInterface.alert("ImportGraphicEntries failed: " + importResult.error);
        return;
    }

    WL4EditorInterface.log("Graphic entry import completed: " +
                           importResult.createdCount + " created, " +
                           importResult.reusedCount + " reused.");

    if (!APPLY_TO_ROOM_OR_LEVEL) {
        WL4EditorInterface.log("Entries were imported into the ROM. Room/Level application was skipped.");
        return;
    }

    var resolvedEntries = importResult.entries;
    if (graphics.room && graphics.tileset) {
        var roomId = WL4EditorInterface.GetCurRoomId();
        if (!WL4EditorInterface.ApplyRoomGraphics(roomId, JSON.stringify(graphics),
                                                  JSON.stringify(resolvedEntries), true)) {
            WL4EditorInterface.alert("ApplyRoomGraphics failed.");
            return;
        }
        WL4EditorInterface.log("Graphic entries were applied to the current Room.");
    } else if (graphics.rooms && graphics.tilesets) {
        if (!WL4EditorInterface.ApplyLevelGraphics(JSON.stringify(graphics),
                                                    JSON.stringify(resolvedEntries), true)) {
            WL4EditorInterface.alert("ApplyLevelGraphics failed.");
            return;
        }
        WL4EditorInterface.log("Graphic entries were applied to the current Level.");
    } else {
        WL4EditorInterface.log("The file has no Room/Level usage information, so the entries were only imported.");
    }

    WL4EditorInterface.log("Save the Level to persist the Room/Tileset changes.");
})();
