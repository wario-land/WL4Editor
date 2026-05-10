// WL4Editor level export script - exports the entire current level to a JSON file

(function () {
    WL4EditorInterface.log("Exporting current level...");

    // Save the original room ID so we can restore it later
    var originalRoomId = WL4EditorInterface.GetCurRoomId();

    // ---------- Helper: parse comma-separated numeric ID lists ----------
    function parseIdList(str) {
        if (!str || str.trim() === "") return [];
        var parts = str.split(",");
        var ids = [];
        for (var i = 0; i < parts.length; i++) {
            var trimmed = parts[i].trim();
            if (trimmed !== "") {
                ids.push(parseInt(trimmed, 10));
            }
        }
        return ids;
    }

    // ---------- Helper: export the currently selected room ----------
    function exportRoomData() {
        var rid = WL4EditorInterface.GetCurRoomId();

        var headerHex = WL4EditorInterface.ExportGetRoomHeaderHex();

        var obj = {
            version: 1,
            roomId: rid,
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

        // Layers
        for (var li = 0; li < 4; li++) {
            var mappingType = (parseInt(headerHex.substr((1 + li) * 2, 2), 16) & 0x3F);
            var w = WL4EditorInterface.GetCurRoomLayerWidth(li);
            var h = WL4EditorInterface.GetCurRoomLayerHeight(li);
            var data = [];

            if (mappingType >= 0x10) {
                if (mappingType < 0x20) {
                    for (var y = 0; y < h; y++) {
                        for (var x = 0; x < w; x++) {
                            data.push(WL4EditorInterface.GetCurRoomTile16(li, x, y));
                        }
                    }
                } else {
                    for (var y = 0; y < h; y++) {
                        for (var x = 0; x < w; x++) {
                            data.push(WL4EditorInterface.GetCurRoomTile8(li, x, y));
                        }
                    }
                }
            }

            obj.layers.push({
                layerId: li,
                mappingType: mappingType,
                width: w,
                height: h,
                data: data
            });
        }

        // Entity lists
        for (var diff = 0; diff < 3; diff++) {
            var entityStr = WL4EditorInterface.GetEntityListData(diff);
            var entities = [];

            if (entityStr && entityStr.trim() !== "") {
                var tokens = entityStr.trim().split(/\s+/);
                for (var t = 0; t + 2 < tokens.length; t += 3) {
                    entities.push({
                        yPos: parseInt(tokens[t], 16),
                        xPos: parseInt(tokens[t + 1], 16),
                        entityId: parseInt(tokens[t + 2], 16)
                    });
                }
            }

            obj.entityLists.push({
                difficulty: diff,
                entities: entities
            });
        }

        // Doors
        var doorsStr = WL4EditorInterface.ExportGetDoorsFullData();
        if (doorsStr && doorsStr.trim() !== "") {
            var doorEntries = doorsStr.split(";");
            for (var di = 0; di < doorEntries.length; di++) {
                var entry = doorEntries[di].trim();
                if (entry !== "") {
                    var df = entry.split(",");
                    obj.doors.push({
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

        // Camera control records
        var camType = WL4EditorInterface.ExportGetCameraControlType();
        var camRecStr = WL4EditorInterface.ExportGetCameraControlRecords();
        var camRecs = [];

        if (camRecStr && camRecStr.trim() !== "") {
            var recEntries = camRecStr.split(";");
            for (var ri = 0; ri < recEntries.length; ri++) {
                var recEntry = recEntries[ri].trim();
                if (recEntry !== "") {
                    var rf = recEntry.split(",");
                    camRecs.push({
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

        obj.cameraControl = {
            type: camType,
            records: camRecs
        };

        return obj;
    }

    // ---------- Build level object ----------
    var levelObj = {
        version: 1,
        levelName: WL4EditorInterface.ExportGetLevelName(),
        levelNameJ: WL4EditorInterface.ExportGetLevelNameJ(),
        levelHeader: {
            passage: WL4EditorInterface.ExportGetLevelPassage(),
            stage: WL4EditorInterface.ExportGetLevelStage(),
            timerHard: WL4EditorInterface.ExportGetLevelTimerSeconds(0),
            timerNormal: WL4EditorInterface.ExportGetLevelTimerSeconds(1),
            timerSHard: WL4EditorInterface.ExportGetLevelTimerSeconds(2)
        },
        roomCount: WL4EditorInterface.GetRoomNum(),
        rooms: [],
        changedGlobals: {}
    };

    // ---------- Export each room ----------
    var roomCount = levelObj.roomCount;
    for (var ri = 0; ri < roomCount; ri++) {
        WL4EditorInterface.SetCurrentRoomId(ri);
        levelObj.rooms.push(exportRoomData());
    }

    // ---------- Export changed tilesets ----------
    var changedTilesetIds = parseIdList(WL4EditorInterface.ExportGetChangedTilesetIds());
    var tilesets = [];

    for (var ti = 0; ti < changedTilesetIds.length; ti++) {
        var tsId = changedTilesetIds[ti];
        var tile8x8Count = WL4EditorInterface.ExportGetTilesetTile8x8Count(tsId);
        var tile8x8Data = [];

        for (var t8 = 0; t8 < tile8x8Count; t8++) {
            tile8x8Data.push(WL4EditorInterface.ExportGetTilesetTile8x8DataHex(tsId, t8));
        }

        var map16Count = WL4EditorInterface.ExportGetTilesetMap16Count(tsId);
        var map16Data = [];

        for (var m16 = 0; m16 < map16Count; m16++) {
            map16Data.push(WL4EditorInterface.ExportGetTilesetMap16DataHex(tsId, m16));
        }

        var allPalHex = WL4EditorInterface.ExportGetTilesetPalettesHex(tsId);
        var paletteData = [];
        for (var pid = 0; pid < 16; pid++) {
            paletteData.push(allPalHex.substr(pid * 64, 64)); // 16 colors * 4 hex chars
        }

        // Animated tile data for switch states 0-1
        var animatedTileData = [];
        for (var ss = 0; ss < 2; ss++) {
            animatedTileData.push(WL4EditorInterface.ExportGetTilesetAnimatedTileDataHex(tsId, ss));
        }

        tilesets.push({
            id: tsId,
            fggfxPtr: WL4EditorInterface.ExportGetTilesetFGGFXPtr(tsId),
            fggfxLen: WL4EditorInterface.ExportGetTilesetFGGFXLen(tsId),
            tile8x8Count: tile8x8Count,
            tile8x8Data: tile8x8Data,
            map16Count: map16Count,
            map16Data: map16Data,
            paletteData: paletteData,
            eventTable: WL4EditorInterface.ExportGetTilesetEventTableHex(tsId),
            terrainTable: WL4EditorInterface.ExportGetTilesetTerrainTableHex(tsId),
            animatedSwitchTable: WL4EditorInterface.ExportGetTilesetAnimatedSwitchTableHex(tsId),
            animatedTileData: animatedTileData
        });
    }

    levelObj.changedGlobals.tilesets = tilesets;

    // ---------- Export changed entities ----------
    var changedEntityIds = parseIdList(WL4EditorInterface.ExportGetChangedEntityIds());
    var entities = [];

    for (var ei = 0; ei < changedEntityIds.length; ei++) {
        var entId = changedEntityIds[ei];
        var entPalCount = WL4EditorInterface.ExportGetEntityPaletteCount(entId);
        var entPalData = [];

        for (var epi = 0; epi < entPalCount; epi++) {
            entPalData.push(WL4EditorInterface.ExportGetEntityPaletteDataHex(entId, epi));
        }

        var entTileCount = WL4EditorInterface.ExportGetEntityTile8x8Count(entId);
        var entTileData = [];

        for (var eti = 0; eti < entTileCount; eti++) {
            entTileData.push(WL4EditorInterface.ExportGetEntityTile8x8DataHex(entId, eti));
        }

        entities.push({
            id: entId,
            paletteCount: entPalCount,
            paletteData: entPalData,
            tile8x8Count: entTileCount,
            tile8x8Data: entTileData
        });
    }

    levelObj.changedGlobals.entities = entities;

    // ---------- Export changed entity sets ----------
    var changedEntitySetIds = parseIdList(WL4EditorInterface.ExportGetChangedEntitySetIds());
    var entitySets = [];

    for (var esi = 0; esi < changedEntitySetIds.length; esi++) {
        var esId = changedEntitySetIds[esi];
        var infoTableSize = WL4EditorInterface.ExportGetEntitySetInfoTableSize(esId);
        var infoTable = [];

        for (var iti = 0; iti < infoTableSize; iti++) {
            infoTable.push(WL4EditorInterface.ExportGetEntitySetInfoEntry(esId, iti));
        }

        entitySets.push({
            id: esId,
            infoTableSize: infoTableSize,
            infoTable: infoTable
        });
    }

    levelObj.changedGlobals.entitySets = entitySets;

    // ---------- Export changed animated tile groups ----------
    var changedAnimatedTileGroupIds = parseIdList(WL4EditorInterface.ExportGetChangedAnimatedTileGroupIds());
    var animatedTileGroups = [];

    for (var agi = 0; agi < changedAnimatedTileGroupIds.length; agi++) {
        var agId = changedAnimatedTileGroupIds[agi];
        var tileCount = WL4EditorInterface.ExportGetAnimatedTileGroupTileCount(agId);
        var tileData = [];

        for (var ati = 0; ati < tileCount; ati++) {
            tileData.push(WL4EditorInterface.ExportGetAnimatedTileGroupTileDataHex(agId, ati));
        }

        animatedTileGroups.push({
            id: agId,
            animType: WL4EditorInterface.ExportGetAnimatedTileGroupAnimType(agId),
            countPerFrame: WL4EditorInterface.ExportGetAnimatedTileGroupCountPerFrame(agId),
            totalFrameCount: WL4EditorInterface.ExportGetAnimatedTileGroupTotalFrameCount(agId),
            tileCount: tileCount,
            tileData: tileData
        });
    }

    levelObj.changedGlobals.animatedTileGroups = animatedTileGroups;

    // ---------- Export wall paint data ----------
    var scatteredBlockCount = WL4EditorInterface.ExportGetWallPaintScatteredBlockCount();
    var scatteredBlocks = [];

    for (var sbi = 0; sbi < scatteredBlockCount; sbi++) {
        scatteredBlocks.push({
            index: sbi,
            address: WL4EditorInterface.ExportGetWallPaintScatteredBlockAddr(sbi),
            size: WL4EditorInterface.ExportGetWallPaintScatteredBlockSize(sbi),
            dataHex: WL4EditorInterface.ExportGetWallPaintScatteredBlockDataHex(sbi)
        });
    }

    levelObj.changedGlobals.wallPaintData = {
        gfxHex: WL4EditorInterface.ExportGetWallPaintGFXHex(),
        passageColorHex: WL4EditorInterface.ExportGetWallPaintPassageColorHex(),
        passageGrayHex: WL4EditorInterface.ExportGetWallPaintPassageGrayHex(),
        scatteredBlockCount: scatteredBlockCount,
        scatteredBlocks: scatteredBlocks
    };

    // ---------- Export credits data ----------
    levelObj.changedGlobals.creditsData = WL4EditorInterface.ExportGetCreditsDataHex();

    // ---------- Restore original room and write file ----------
    WL4EditorInterface.SetCurrentRoomId(originalRoomId);
    WL4EditorInterface.WriteJsonFile("", JSON.stringify(levelObj, null, 2));

    WL4EditorInterface.log("Level export completed.");
})();
