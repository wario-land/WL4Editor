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

private:
    Ui::GraphicManagerDialog *ui;
    QVector<struct ScatteredGraphicUtils::ScatteredGraphicEntryItem> graphicEntries;
    QStandardItemModel *ListViewItemModel = nullptr;
    int SelectedEntryID = -1;

    // functions
    bool UpdateEntryList();
    QString GenerateEntryTextFromStruct(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    void ExtractEntryToGUI(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    QPixmap RenderAllPalette(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    QPixmap RenderAllTiles(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    QPixmap RenderGraphic(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    void UpdatePaletteGraphicView(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    void UpdateTilesGraphicView(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    void UpdateMappingGraphicView(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);

public:
    // clang-format off
    static constexpr const char *ScatteredGraphicTileDataTypeNameData[1] =
    {
        "0: Tileset_text_bg_4bpp_no_comp"
    };

    static constexpr const char *ScatteredGraphicMappingDataCompressionTypeNameData[2] =
    {
        "0: No_mapping_data_comp",
        "1: RLE16_with_sizeheader"
    };
    // clang-format on
};

#endif // GRAPHICMANAGERDIALOG_H
