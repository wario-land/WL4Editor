#ifndef GRAPHICMANAGERDIALOG_H
#define GRAPHICMANAGERDIALOG_H

#include <QDialog>
#include <QModelIndex>
#include <QStandardItem>
#include <QStandardItemModel>
#include "LevelComponents/Tile.h"
#include "AssortedGraphicUtils.h"

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
    QVector<struct AssortedGraphicUtils::AssortedGraphicEntryItem> graphicEntries;
    QStandardItemModel *ListViewItemModel = nullptr;
    int SelectedEntryID = -1;
    struct AssortedGraphicUtils::AssortedGraphicEntryItem tmpEntry;
    QVector<LevelComponents::Tile8x8 *> tmpTile8x8array;
    LevelComponents::Tile8x8 *tmpblankTile = nullptr;

    // functions
    void CreateAndAddDefaultEntry();
    bool UpdateEntryList();
    QString GenerateEntryTextFromStruct(AssortedGraphicUtils::AssortedGraphicEntryItem &entry);
    void ExtractEntryToGUI(AssortedGraphicUtils::AssortedGraphicEntryItem &entry);
    QPixmap RenderAllPalette(AssortedGraphicUtils::AssortedGraphicEntryItem &entry);
    QPixmap RenderAllTiles(AssortedGraphicUtils::AssortedGraphicEntryItem &entry);
    QPixmap RenderGraphic(AssortedGraphicUtils::AssortedGraphicEntryItem &entry);
    void UpdatePaletteGraphicView(AssortedGraphicUtils::AssortedGraphicEntryItem &entry);
    void UpdateTilesGraphicView(AssortedGraphicUtils::AssortedGraphicEntryItem &entry);
    void UpdateMappingGraphicView(AssortedGraphicUtils::AssortedGraphicEntryItem &entry);
    void ClearPalettePanel();
    void ClearTilesPanel();
    void ClearMappingPanel();
    void SetPaletteInfoGUI(AssortedGraphicUtils::AssortedGraphicEntryItem &entry);
    void SetTilesPanelInfoGUI(AssortedGraphicUtils::AssortedGraphicEntryItem &entry);
    void SetMappingGraphicInfoGUI(AssortedGraphicUtils::AssortedGraphicEntryItem &entry);

    void CleanTilesInstances();
    void GenerateBGTile8x8Instances(AssortedGraphicUtils::AssortedGraphicEntryItem &entry);
    void CleanMappingDataInEntry(AssortedGraphicUtils::AssortedGraphicEntryItem &entry) { entry.mappingData.clear(); }
    void ClearAndResettmpEntryPalettes();

    void DeltmpEntryTile(int tileId);

    // helper functions
    bool CheckEditability(int entryId);
    void GetVanillaGraphicEntriesFromROM();

public:
    // clang-format off
    static constexpr const char *AssortedGraphicTileDataTypeNameData[1] =
    {
        "0: Tile8x8_4bpp_no_comp_Tileset_text_bg"
    };

    static constexpr const char *AssortedGraphicMappingDataCompressionTypeNameData[2] =
    {
        "0: No_mapping_data_comp",
        "1: RLE_mappingtype_0x20"
    };
    // clang-format on
};

#endif // GRAPHICMANAGERDIALOG_H
