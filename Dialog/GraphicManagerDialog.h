#ifndef GRAPHICMANAGERDIALOG_H
#define GRAPHICMANAGERDIALOG_H

#include <QDialog>
#include <QModelIndex>
#include <QStandardItem>
#include <QStandardItemModel>
#include "LevelComponents/Tile.h"
#include "ScatteredGraphicUtils.h"

namespace Ui {
class GraphicManagerDialog;
}

class GraphicManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GraphicManagerDialog(QWidget *parent = nullptr);
    ~GraphicManagerDialog();
    static void StaticInitialization();

private slots:
    void on_listView_RecordGraphicsList_clicked(const QModelIndex &index);
    void on_pushButton_ClearTile8x8Data_clicked();
    void on_pushButton_ClearPaletteData_clicked();
    void on_pushButton_ClearMappingData_clicked();
    void on_pushButton_ImportPaletteData_clicked();
    void on_pushButton_ImportTile8x8Data_clicked();
    void on_pushButton_ImportGraphic_clicked();
    void on_pushButton_AddGraphicEntry_clicked();
    void on_pushButton_RemoveGraphicEntries_clicked();
    void on_pushButton_validateAndSetMappingData_clicked();
    void on_pushButton_saveAllGraphicEntries_clicked();
    void on_pushButton_cancelEditing_clicked();
    void on_pushButton_ReduceTiles_clicked();
    void on_lineEdit_tileDataName_textChanged(const QString &arg1);
    void on_lineEdit_mappingDataName_textChanged(const QString &arg1);

private:
    Ui::GraphicManagerDialog *ui;
    QVector<struct ScatteredGraphicUtils::ScatteredGraphicEntryItem> graphicEntries;
    QStandardItemModel *ListViewItemModel = nullptr;
    int SelectedEntryID = -1;
    struct ScatteredGraphicUtils::ScatteredGraphicEntryItem tmpEntry;
    QVector<LevelComponents::Tile8x8 *> tmpTile8x8array;
    LevelComponents::Tile8x8 *tmpblankTile = nullptr;

    // functions
    void CreateAndAddDefaultEntry();
    bool UpdateEntryList();
    QString GenerateEntryTextFromStruct(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    void ExtractEntryToGUI(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    QPixmap RenderAllPalette(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    QPixmap RenderAllTiles(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    QPixmap RenderGraphic(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    void UpdatePaletteGraphicView(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    void UpdateTilesGraphicView(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    void UpdateMappingGraphicView(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    void ClearPalettePanel();
    void ClearTilesPanel();
    void ClearMappingPanel();
    void SetPaletteInfoGUI(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    void SetTilesPanelInfoGUI(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    void SetMappingGraphicInfoGUI(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);

    void CleanTilesInstances();
    void GenerateBGTile8x8Instances(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    void CleanMappingDataInEntry(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry) { entry.mappingData.clear(); }
    void ClearAndResettmpEntryPalettes();

    void DeltmpEntryTile(int tileId);

    // helper functions
    bool CheckEditability(int entryId);
    bool FindbgGFXptrInAllTilesets(unsigned int address, unsigned int *tilesetId_find);
    bool FindLayerptrInAllRooms(unsigned int address, unsigned int *levelId_found, unsigned int *roomId_found);
    void GetVanillaGraphicEntriesFromROM();

public:
    // clang-format off
    static constexpr const char *ScatteredGraphicTileDataTypeNameData[1] =
    {
        "0: Tile8x8_4bpp_no_comp_Tileset_text_bg"
    };

    static constexpr const char *ScatteredGraphicMappingDataCompressionTypeNameData[2] =
    {
        "0: No_mapping_data_comp",
        "1: RLE_mappingtype_0x20"
    };
    // clang-format on
};

#endif // GRAPHICMANAGERDIALOG_H
