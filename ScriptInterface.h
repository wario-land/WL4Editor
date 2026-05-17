#ifndef SCRIPTINTERFACE_H
#define SCRIPTINTERFACE_H

#include <QObject>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include <string>
#include <QTextStream>

class ScriptInterface : public QObject
{
    Q_OBJECT
public:
    explicit ScriptInterface(QObject *parent = nullptr);

    // Getter
    Q_INVOKABLE int GetCurRoomLayerWidth(int layerId);
    Q_INVOKABLE int GetCurRoomLayerHeight(int layerId);
    Q_INVOKABLE int GetCurRoomTile16(int layerID, int x, int y);
    Q_INVOKABLE int GetCurRoomTile8(int layerID, int x, int y);
    Q_INVOKABLE int GetRoomNum();
    Q_INVOKABLE int GetCurRoomId();
    Q_INVOKABLE int GetCurTilesetTile16EventId(unsigned short tile16Id);
    Q_INVOKABLE int GetCurTilesetTile16TerrainType(unsigned short tile16Id);
    Q_INVOKABLE QJsonObject GetLayerEventMap(int layerId, int startX = 0, int startY = 0, int width = -1, int height = -1);
    Q_INVOKABLE QJsonObject GetLayerTerrainMap(int layerId, int startX = 0, int startY = 0, int width = -1, int height = -1);
    Q_INVOKABLE QString GetEntityListData(int entitylistid = -1);
    Q_INVOKABLE QString GetEntityListSource();
    Q_INVOKABLE QString GetCurRoomAllDoorsRangeData();
    Q_INVOKABLE void PrintEntityDefaultOAMData(int globalEntityId);
    Q_INVOKABLE QString GetCurRoomEntitySetInfo();
    Q_INVOKABLE int GetCurRoomTilesetId();
    Q_INVOKABLE int GetCurRoomEntitySetId();
    Q_INVOKABLE int GetCurRoomCameraControlType();
    Q_INVOKABLE QString GetCurRoomCameraControlRecords();

    // Test
    Q_INVOKABLE void _UnpackScreen(int address);
    Q_INVOKABLE void _PackScreen(QString inputData, bool skipzeros = true);
    Q_INVOKABLE void _DecompressData(int mappingtype, int address);
    Q_INVOKABLE unsigned int _GetLayerDecomdataPointer(int layerId);
    Q_INVOKABLE void _PrintRoomHeader();
    Q_INVOKABLE void _ExportLayerData(QString filePath = QString(""), int layerid = -1);
    Q_INVOKABLE void _ImportLayerData(QString fileName = QString(""), int layerid = -1);
    Q_INVOKABLE void _GetTilesetGFXInfo(int tilesetId);
    Q_INVOKABLE void _ExtractSpriteOAMPackage(int address);
    Q_INVOKABLE void _ExtractSpriteOAMPackage(QString address);

    // Setter
    Q_INVOKABLE void SetCurrentRoomId(int roomid);
    Q_INVOKABLE int AddNewRoom();
    Q_INVOKABLE void SetCurRoomTile16(int layerID, int TileID, int x, int y);
    Q_INVOKABLE void SetRoomSize(int roomwidth, int roomheight, int layer0width, int layer0height);
    Q_INVOKABLE void SetEntityListData(QString entitylistdata, int entitylistid = -1);
    Q_INVOKABLE int AddEntityByGlobalId(int globalEntityId, int x, int y, int difficulty);

    // Localize JS function
    Q_INVOKABLE void alert(QString message);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void log(QString message);
    Q_INVOKABLE QString prompt(QString message, QString defaultInput);

    // UI
    Q_INVOKABLE void UpdateRoomGFXFull();
    Q_INVOKABLE void DoEvents();

    // File operations
    Q_INVOKABLE void WriteTxtFile(QString filePath = QString(""), QString test = "");
    Q_INVOKABLE void WriteJsonFile(QString filePath = QString(""), QString test = "");
    Q_INVOKABLE QString ReadTxtFile(QString filepath);
    Q_INVOKABLE QString ReadJsonFile(QString filepath);
    Q_INVOKABLE QString ReadJsonFileDialog();

    // MCP Server control
    Q_INVOKABLE void StartMCPServer();
    Q_INVOKABLE void StartMCPServerTcp(int port = 9876);
    Q_INVOKABLE void StopMCPServer();
    Q_INVOKABLE void StopMCPServerTcp();

    // Headless mode — suppresses all modal dialogs (QInputDialog, QMessageBox, QFileDialog)
    // so MCP tools can call ScriptInterface methods without blocking the main thread.
    Q_INVOKABLE void SetHeadlessMode(bool on) { m_headlessMode = on; }
    Q_INVOKABLE bool IsHeadlessMode() { return m_headlessMode; }

    // Log buffer — in headless mode, log() accumulates messages here instead of
    // writing to the OutputDockWidget. MCP handlers can retrieve and clear.
    Q_INVOKABLE QString GetAndClearLogBuffer() { QString tmp = m_logBuffer; m_logBuffer.clear(); return tmp; }

    // Safe variants that never show dialogs (for MCP / headless use)
    Q_INVOKABLE QString GetEntityListDataSafe(int entitylistid);
    Q_INVOKABLE void SetEntityListDataSafe(QString entitylistdata, int entitylistid);

    // helper functions
    Q_INVOKABLE void ShowSaveDataAnalysis();

    // Export/Import APIs

    // Room Export Getters
    Q_INVOKABLE int ExportGetLayerDataPtr(int layerId);
    Q_INVOKABLE int ExportGetTilesetId();
    Q_INVOKABLE int ExportGetEntitySetId();
    Q_INVOKABLE int ExportGetCameraControlType();
    Q_INVOKABLE QString ExportGetCameraControlRecords();
    Q_INVOKABLE QString ExportGetDoorsFullData();
    Q_INVOKABLE QString ExportGetRoomHeaderHex();

    // Level Export Getters
    Q_INVOKABLE QString ExportGetLevelName();
    Q_INVOKABLE QString ExportGetLevelNameJ();
    Q_INVOKABLE int ExportGetLevelTimerSeconds(int difficulty);
    Q_INVOKABLE int ExportGetLevelPassage();
    Q_INVOKABLE int ExportGetLevelStage();

    // Changed Globals Detection
    Q_INVOKABLE QString ExportGetChangedTilesetIds();
    Q_INVOKABLE QString ExportGetChangedEntityIds();
    Q_INVOKABLE QString ExportGetChangedEntitySetIds();
    Q_INVOKABLE QString ExportGetChangedAnimatedTileGroupIds();

    // Tileset Export Getters
    Q_INVOKABLE int ExportGetTilesetFGGFXPtr(int id);
    Q_INVOKABLE int ExportGetTilesetFGGFXLen(int id);
    Q_INVOKABLE int ExportGetTilesetBGGFXPtr(int id);
    Q_INVOKABLE int ExportGetTilesetBGGFXLen(int id);
    Q_INVOKABLE int ExportGetTilesetMap16Ptr(int id);
    Q_INVOKABLE QString ExportGetTilesetTile8x8DataHex(int id);
    Q_INVOKABLE QString ExportGetTilesetMap16DataHex(int id);
    Q_INVOKABLE QString ExportGetTilesetPalettesHex(int id);
    Q_INVOKABLE QString ExportGetTilesetEventTableHex(int id);
    Q_INVOKABLE QString ExportGetTilesetTerrainTableHex(int id);
    Q_INVOKABLE QString ExportGetTilesetAnimatedSwitchTableHex(int id);
    Q_INVOKABLE QString ExportGetTilesetAnimatedTileData0Hex(int id);
    Q_INVOKABLE QString ExportGetTilesetAnimatedTileData1Hex(int id);

    // Entity Export Getters
    Q_INVOKABLE int ExportGetEntityPaletteCount(int id);
    Q_INVOKABLE QString ExportGetEntityPaletteDataHex(int id, int paletteId);
    Q_INVOKABLE int ExportGetEntityTile8x8Count(int id);
    Q_INVOKABLE QString ExportGetEntityTile8x8DataHex(int id, int index);

    // EntitySet Export Getters
    Q_INVOKABLE int ExportGetEntitySetInfoTableSize(int id);
    Q_INVOKABLE QString ExportGetEntitySetInfoEntry(int id, int index);

    // AnimatedTile8x8Group Export Getters
    Q_INVOKABLE int ExportGetAnimatedTileGroupAnimType(int id);
    Q_INVOKABLE int ExportGetAnimatedTileGroupCountPerFrame(int id);
    Q_INVOKABLE int ExportGetAnimatedTileGroupTotalFrameCount(int id);
    Q_INVOKABLE int ExportGetAnimatedTileGroupTileCount(int id);
    Q_INVOKABLE QString ExportGetAnimatedTileGroupTileDataHex(int id, int index);

    // WallPaint/Credits Export
    Q_INVOKABLE QString ExportGetWallPaintGFXHex();
    Q_INVOKABLE QString ExportGetWallPaintPassageColorHex();
    Q_INVOKABLE QString ExportGetWallPaintPassageGrayHex();
    Q_INVOKABLE QString ExportGetCreditsDataHex();

    // Import APIs
    // Room import APIs (called by JS in order)
    Q_INVOKABLE bool ImportRoomConfig(int roomWidth, int roomHeight, int layer0Width, int layer0Height,
                                       QString roomHeaderHex);
    Q_INVOKABLE bool ImportLayerTiles(int layerId, int width, int height, QString tileDataHex);
    Q_INVOKABLE bool ImportDoorsDisableDest(QString doorsData);
    Q_INVOKABLE bool ImportEntityList(int difficulty, QString entityDataHex);
    Q_INVOKABLE bool ImportCameraControl(int camType, QString recordsData);
    Q_INVOKABLE QString GetCurRoomDoorGlobalIds();
    Q_INVOKABLE bool DeleteDoorByGlobalId(int globalId);
    Q_INVOKABLE bool SetDoorDestination(int globalId, int destGlobalId);
    Q_INVOKABLE bool ImportDoorVecString(QString doorVecString);
    Q_INVOKABLE void ResetRoomEntitySet(int roomId);
    Q_INVOKABLE void PostImportRefresh();

    // Global Import APIs (per-element create ExecuteOperation for Undo/Redo)
    Q_INVOKABLE bool ImportTileset(int tilesetId, int fggfxPtr, int fggfxLen,
                                     QString tile8x8Data, QString map16Data, QString paletteData,
                                     QString eventTable, QString terrainTable,
                                     QString animatedSwitchTable,
                                     QString animatedTileData0, QString animatedTileData1);
    Q_INVOKABLE bool ImportEntity(int entityId, QString paletteData, QString tile8x8Data);
    Q_INVOKABLE bool ImportEntitySet(int entitySetId, QString infoTable);
    Q_INVOKABLE bool ImportAnimatedTileGroup(int groupId, int animType, int countPerFrame,
                                               int totalFrameCount, QString tileData);
    Q_INVOKABLE bool ImportGlobalWallPaint(QString gfxHex, QString passageColorHex,
                                             QString passageGrayHex);
    Q_INVOKABLE bool ImportGlobalCredits(QString creditsHex);
    Q_INVOKABLE bool ImportLevelConfig(QString levelName, QString levelNameJ,
                                         int timerHard, int timerNormal, int timerSHard);

private:
    bool m_headlessMode = false;
    QString m_logBuffer;
};

class HintLayer : public QObject
{
    Q_OBJECT
public:
    explicit HintLayer(QObject *parent = nullptr) : QObject(parent) {}

    // Init fucntions
    Q_INVOKABLE void GetAutoGeneratedHintLayer();
    Q_INVOKABLE void GetblankHintLayer();

    // drawing
    Q_INVOKABLE void drawRect(int x, int y, int width, int height, int line_width, int red, int green, int blue, int alpha);
    Q_INVOKABLE void drawLine(int x1, int y1, int x2, int y2, int line_width, int red, int green, int blue, int alpha);
    Q_INVOKABLE void drawText(int x, int y, QString show_text, int red, int green, int blue, int alpha);

    // Setter
    Q_INVOKABLE void SubmitHintLayer();
private:
    // this class works like, we operate the QPixmap object in the class using APIs, the send the pixmap back to the Room class
    QPixmap tmpHintLayerPixmap;

};

#endif // SCRIPTINTERFACE_H
