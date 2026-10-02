#include "GraphicManagerDialog.h"
#include "ui_GraphicManagerDialog.h"

#include <QMessageBox>
#include <QModelIndex>
#include <QSet>

#include "WL4EditorWindow.h"
#include "WL4Constants.h"
#include "FileIOUtils.h"

extern WL4EditorWindow *singleton;

// constexpr declarations for the initializers in the header
constexpr const char *GraphicManagerDialog::AssortedGraphicTileDataTypeNameData[1];
constexpr const char *GraphicManagerDialog::AssortedGraphicMappingDataCompressionTypeNameData[2];

// static variables used by CameraControlDockWidget
static QStringList GraphicTileDataTypeName;
static QStringList GraphicMappingDataCompressionTypeName;

/// <summary>
/// Construct an instance of the GraphicManagerDialog.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
GraphicManagerDialog::GraphicManagerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GraphicManagerDialog)
{
    // Setup GUI
    ui->setupUi(this);
    ui->comboBox_tileDataType->addItems(GraphicTileDataTypeName);
    ui->comboBox_mappingDataType->addItems(GraphicMappingDataCompressionTypeName);
    ui->graphicsView_tile8x8setData->scale(2, 2);
    ui->graphicsView_palettes->scale(2, 2);
    ui->graphicsView_mappingGraphic->scale(1, 1);

    if (graphicEntries.size())
    {
        graphicEntries.clear();
    }
    graphicEntries = AssortedGraphicUtils::GetAssortedGraphicsFromROM();

    // if there is no graphicEntry in the ROM, we generate entries from existing Tilesets and Rooms.
    if (!graphicEntries.size())
    {
        GetVanillaGraphicEntriesFromROM();
    }
    UpdateEntryList();

    // init status of some buttons
    ui->pushButton_RemoveGraphicEntries->setEnabled(false);
    ui->pushButton_validateAndSetMappingData->setEnabled(false);
    ui->pushButton_ImportPaletteData->setEnabled(false);
    ui->pushButton_ImportTile8x8Data->setEnabled(false);
    ui->pushButton_ImportGraphic->setEnabled(false);
    ui->pushButton_duplicateCurrentEntry->setEnabled(false);
    ui->pushButton_SwapPalettes->setEnabled(false);
}

/// <summary>
/// Deconstruct the GraphicManagerDialog and clean up its instance objects on the heap.
/// </summary>
GraphicManagerDialog::~GraphicManagerDialog()
{
    delete ui;
}

/// <summary>
/// Perform static initialization of constant data structures for the dialog.
/// </summary>
void GraphicManagerDialog::StaticInitialization()
{
    // Initialize the selections for the ComboBoxes
    for (unsigned int i = 0;
         i < sizeof(AssortedGraphicTileDataTypeNameData) / sizeof(AssortedGraphicTileDataTypeNameData[0]); ++i)
    {
        GraphicTileDataTypeName << AssortedGraphicTileDataTypeNameData[i];
    }
    for (unsigned int i = 0;
         i < sizeof(AssortedGraphicMappingDataCompressionTypeNameData) / sizeof(AssortedGraphicMappingDataCompressionTypeNameData[0]); ++i)
    {
        GraphicMappingDataCompressionTypeName << AssortedGraphicMappingDataCompressionTypeNameData[i];
    }
}

/// <summary>
/// Create a new default Entry and add it into the graphicEntries
/// </summary>
void GraphicManagerDialog::CreateAndAddDefaultEntry()
{
    struct AssortedGraphicUtils::AssortedGraphicEntryItem testentry;
    testentry.TileDataAddress = 0x4E851C;
    testentry.TileDataSizeInByte = 9376; // unit: Byte
    testentry.TileDataRAMOffsetNum = 0x4DA - 0x200; // unit: per Tile8x8
    testentry.TileDataType = AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg;
    testentry.TileDataName = "vanilla Tileset 0x11 bg tiles";
    testentry.MappingDataAddress = 0x5FA6D0;
    testentry.MappingDataSizeAfterCompressionInByte = 0xC10; // unit: Byte
    testentry.MappingDataCompressType = AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType::RLE_mappingtype_0x20;
    testentry.MappingDataName = "vanilla background mapping data";
    testentry.PaletteAddress = 0x583C7C;
    for (unsigned int i = 0; i < 16; i++)
        testentry.PaletteSlotIDs.push_back(i); // all 16 palette slots
    testentry.optionalGraphicWidth = 0; // overwrite size params when the mapping data include size info
    testentry.optionalGraphicHeight = 0;

    AssortedGraphicUtils::ExtractDataFromEntryInfo_v2(testentry);
    graphicEntries.append(testentry);
}

/// <summary>
/// Update Graphic Entry list accroding to variable "graphicEntries".
/// </summary>
/// <returns>
/// If the number of entry is 0, return false.
/// </returns>
bool GraphicManagerDialog::UpdateEntryList()
{
    // cleanup old listview ui
    bool result = false;
    if (ListViewItemModel)
    {
        ListViewItemModel->clear();
        delete ListViewItemModel;
        ListViewItemModel = nullptr;
    }
    ListViewItemModel = new QStandardItemModel(this);

    if (graphicEntries.size())
    {
        // create new listview ui data
        for(auto &entry: graphicEntries)
        {
            QString text = GenerateEntryTextFromStruct(entry);
            QStandardItem *item = new QStandardItem(text);
            ListViewItemModel->appendRow(item);
        }
        result = true;
    }

    ui->listView_RecordGraphicsList->setModel(ListViewItemModel);
    return result;
}

/// <summary>
/// Extract entry info into GUI.
/// </summary>
/// <param name="entry">
/// The struct data used to extract info of graphic.
/// </param>
void GraphicManagerDialog::ExtractEntryToGUI(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    // Cleanup then Load Tiles
    CleanTilesInstances();
    switch (static_cast<int>(entry.TileDataType))
    {
        case AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg:
        {
            GenerateBGTile8x8Instances(entry);
            break;
        }
    }

    // UI Reset
    SetTilesPanelInfoGUI(entry);
    SetMappingGraphicInfoGUI(entry);
    SetPaletteInfoGUI(entry);

    // graphicviews reset
    UpdatePaletteGraphicView(entry);
    UpdateTilesGraphicView(entry);
    UpdateMappingGraphicView(entry);
}

/// <summary>
/// Get all the palettes' pixmap of an entry.
/// </summary>
/// <param name="entry">
/// The struct data saves the info of a graphic.
/// </param>
/// <returns>
/// The pixmap of the palettes recorded in the entry.
/// </returns>
QPixmap GraphicManagerDialog::RenderAllPalette(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    // draw palette pixmap
    QPixmap PaletteBarpixmap(8 * 16, 8 * 16);
    PaletteBarpixmap.fill(Qt::transparent);
    QPainter PaletteBarPainter(&PaletteBarpixmap);
    for (int j = 0; j < 16; ++j)
    {
        QVector<QRgb> palettetable = entry.palettes[j];
        for (int i = 1; i < 16; ++i) // Ignore the first color
        {
            PaletteBarPainter.fillRect(8 * i, 8 * j, 8, 8, palettetable[i]);
        }
    }
    return PaletteBarpixmap;
}

/// <summary>
/// Get all the tiles' pixmap of an entry.
/// </summary>
/// <param name="entry">
/// The struct data saves the info of a graphic.
/// </param>
/// <returns>
/// The pixmap of the tiles recorded in the entry.
/// </returns>
QPixmap GraphicManagerDialog::RenderAllTiles(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    switch (static_cast<int>(entry.TileDataType))
    {
        case AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg:
        {
            int lineNum = tmpTile8x8array.size() / 16;
            if ((lineNum * 16) < tmpTile8x8array.size())
            {
                lineNum += 1;
            }
            QPixmap pixmap(8 * 16, 8 * lineNum);
            pixmap.fill(Qt::transparent);

            // find a palette can be used to draw Tiles
            int paletteId = -1;
            if (tmpEntry.PaletteSlotIDs.size())
            {
                paletteId = tmpEntry.PaletteSlotIDs[0];
            }
            else
            {
                // find a palette has non-black color(s) in it
                for (int i = 0; i < entry.palettes->size(); i++)
                {
                    for(int j = 1; j < entry.palettes[i].size(); j++) // skip the first color
                    {
                        if (entry.palettes[i][j] != QColor(0, 0, 0, 0xFF).rgba())
                        {
                            paletteId = i;
                            break;
                        }
                    }
                    if (paletteId != -1)
                    {
                        break;
                    }
                }
                if (paletteId == -1)
                {
                    // there is no valid palette exist
                    break;
                }
            }

            // drawing
            for (int i = 0; i < lineNum; ++i)
            {
                for (int j = 0; j < 16; ++j)
                {
                    if (tmpTile8x8array.size() <= (i * 16 + j))
                    {
                        tmpblankTile->DrawTile(&pixmap, j * 8, i * 8);
                        continue;
                    }
                    if (tmpTile8x8array[i * 16 + j] == tmpblankTile) continue;
                    tmpTile8x8array[i * 16 + j]->SetPaletteIndex(paletteId);
                    tmpTile8x8array[i * 16 + j]->DrawTile(&pixmap, j * 8, i * 8);
                }
            }
            return pixmap;
        }
    }

    return QPixmap();
}

/// <summary>
/// Get the graphic's pixmap of an entry.
/// </summary>
/// <param name="entry">
/// The struct data saves the info of a graphic.
/// </param>
/// <returns>
/// The pixmap of the graphic recorded in the entry.
/// </returns>
QPixmap GraphicManagerDialog::RenderGraphic(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    int graphicheight = entry.optionalGraphicHeight;
    int graphicwidth = entry.optionalGraphicWidth;
    switch (static_cast<int>(entry.MappingDataCompressType))
    {
        case AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType::RLE_mappingtype_0x20:
        {
            // Initialize the QPixmap with transparency
            int unit = 8;
            QPixmap graphicPixmap(graphicwidth * unit, graphicheight * unit);
            graphicPixmap.fill(Qt::transparent);

            // generate tile instances for rendering
            QVector<LevelComponents::Tile8x8 *> tmptiles;
            for (int i = 0; i < graphicwidth * graphicheight; ++i)
            {
                unsigned short tileData = entry.mappingData[i];
                LevelComponents::Tile8x8 *newTile = new LevelComponents::Tile8x8(tmpTile8x8array[(tileData & 0x3FF)]);
                newTile->SetFlipX((tileData & (1 << 10)) != 0);
                newTile->SetFlipY((tileData & (1 << 11)) != 0);
                newTile->SetPaletteIndex((tileData >> 12) & 0xF);
                tmptiles.push_back(newTile);
            }

            // Draw the tiles to the QPixmap
            for (int i = 0; i < graphicheight; ++i)
            {
                for (int j = 0; j < graphicwidth; ++j)
                {
                    LevelComponents::Tile8x8 *t = tmptiles[j + i * graphicwidth];
                    t->DrawTile(&graphicPixmap, j * unit, i * unit);
                    delete t;
                }
            }
            return graphicPixmap;
        }
    }

    return QPixmap();
}

/// <summary>
/// Update Palette graphicview.
/// </summary>
void GraphicManagerDialog::UpdatePaletteGraphicView(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    if (entry.PaletteSlotIDs.isEmpty())
    { // the entry uses no palette at all, so there is nothing to render
        ClearPalettePanel();
        return;
    }
    if (ui->graphicsView_palettes->scene())
    {
        delete ui->graphicsView_palettes->scene();
    }
    QGraphicsScene *scene = new QGraphicsScene(0, 0, 16 * 8, 16 * 8);
    ui->graphicsView_palettes->setScene(scene);
    ui->graphicsView_palettes->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scene->addPixmap(RenderAllPalette(entry));
    ui->graphicsView_palettes->verticalScrollBar()->setValue(0);
}

/// <summary>
/// Update Tiles graphicview.
/// </summary>
void GraphicManagerDialog::UpdateTilesGraphicView(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    if (!entry.TileDataSizeInByte)
    { // the entry contains no Tile8x8 data, so there is nothing to render
        ClearTilesPanel();
        return;
    }
    int linenum = tmpTile8x8array.size() / 16;
    if ((linenum * 16) < tmpTile8x8array.size())
    {
        linenum += 1;
    }
    if (ui->graphicsView_tile8x8setData->scene())
    {
        delete ui->graphicsView_tile8x8setData->scene();
    }
    QGraphicsScene *scene = new QGraphicsScene(0, 0, 16 * 8, linenum * 8);
    ui->graphicsView_tile8x8setData->setScene(scene);
    ui->graphicsView_tile8x8setData->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scene->addPixmap(RenderAllTiles(entry));
    ui->graphicsView_tile8x8setData->verticalScrollBar()->setValue(0);
}

/// <summary>
/// Update Tiles graphicview.
/// </summary>
void GraphicManagerDialog::UpdateMappingGraphicView(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    if (ui->graphicsView_mappingGraphic->scene())
    {
        delete ui->graphicsView_mappingGraphic->scene();
    }
    QPixmap pixmap = RenderGraphic(entry);
    QGraphicsScene *scene = new QGraphicsScene(0, 0, pixmap.width(), pixmap.height());
    ui->graphicsView_mappingGraphic->setScene(scene);
    ui->graphicsView_mappingGraphic->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scene->addPixmap(pixmap);
    ui->graphicsView_mappingGraphic->verticalScrollBar()->setValue(0);
}

/// <summary>
/// Clear all the GUi things in the palette panel
/// </summary>
void GraphicManagerDialog::ClearPalettePanel()
{
    if (ui->graphicsView_palettes->scene())
    {
        delete ui->graphicsView_palettes->scene();
    }
    ui->graphicsView_palettes->setScene(nullptr);

    ui->lineEdit_paletteAddress->setText("");
    ui->lineEdit_paletteNum->setText("");
}

/// <summary>
/// Clear all the GUi things in the Tile stuff panel
/// </summary>
void GraphicManagerDialog::ClearTilesPanel()
{
    if (ui->graphicsView_tile8x8setData->scene())
    {
        delete ui->graphicsView_tile8x8setData->scene();
    }
    ui->graphicsView_tile8x8setData->setScene(nullptr);

    ui->lineEdit_tileDataAddress->setText("");
    ui->lineEdit_tileDataSizeInByte->setText("");
    ui->lineEdit_tileDataRAMoffset->setText("");
    ui->comboBox_tileDataType->setCurrentIndex(0);
    ui->lineEdit_tileDataName->setText("");
}

/// <summary>
/// Clear all the GUi things in the mapping stuff panel
/// </summary>
void GraphicManagerDialog::ClearMappingPanel()
{
    if (ui->graphicsView_mappingGraphic->scene())
    {
        delete ui->graphicsView_mappingGraphic->scene();
    }
    ui->graphicsView_mappingGraphic->setScene(nullptr);

    ui->lineEdit_mappingDataAddress->setText("");
    ui->lineEdit_mappingDataSizeInByte->setText("");
    ui->comboBox_mappingDataType->setCurrentIndex(0);
    ui->lineEdit_mappingDataName->setText("");
    ui->lineEdit_optionalGraphicHeight->setText("");
    ui->lineEdit_optionalGraphicWidth->setText("");
}

/// <summary>
/// set palette panel info according to the provided entry.
/// </summary>
/// <param name="entry">
/// The struct data of the entry.
/// </param>
void GraphicManagerDialog::SetPaletteInfoGUI(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    ui->lineEdit_paletteAddress->setText(QString::number(entry.PaletteAddress, 16));

    // Build comma-separated hex string of palette slot IDs
    QString slotIDsStr;
    for (int i = 0; i < entry.PaletteSlotIDs.size(); i++)
    {
        if (i > 0) slotIDsStr += ",";
        slotIDsStr += QString::number(entry.PaletteSlotIDs[i], 16);
    }
    if (slotIDsStr.isEmpty()) slotIDsStr = "0";
    ui->lineEdit_paletteNum->setText(slotIDsStr);
}

/// <summary>
/// set Tiles panel info according to the provided entry.
/// </summary>
/// <param name="entry">
/// The struct data of the entry.
/// </param>
void GraphicManagerDialog::SetTilesPanelInfoGUI(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    ui->lineEdit_tileDataAddress->setText(QString::number(entry.TileDataAddress, 16));
    ui->lineEdit_tileDataSizeInByte->setText(QString::number(entry.TileDataSizeInByte, 16));
    ui->lineEdit_tileDataRAMoffset->setText(QString::number(entry.TileDataRAMOffsetNum, 16));
    ui->comboBox_tileDataType->setCurrentIndex(entry.TileDataType);
    ui->lineEdit_tileDataName->setText(entry.TileDataName);
}

/// <summary>
/// set mapping graphic panel info according to the provided entry.
/// </summary>
/// <param name="entry">
/// The struct data of the entry.
/// </param>
void GraphicManagerDialog::SetMappingGraphicInfoGUI(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    ui->lineEdit_mappingDataAddress->setText(QString::number(entry.MappingDataAddress, 16));
    ui->lineEdit_mappingDataSizeInByte->setText(QString::number(entry.MappingDataSizeAfterCompressionInByte, 16));
    ui->comboBox_mappingDataType->setCurrentIndex(entry.MappingDataCompressType);
    ui->lineEdit_mappingDataName->setText(entry.MappingDataName);
    ui->lineEdit_optionalGraphicHeight->setText(QString::number(entry.optionalGraphicHeight, 16));
    ui->lineEdit_optionalGraphicWidth->setText(QString::number(entry.optionalGraphicWidth, 16));
}

/// <summary>
/// Clean the Tile instances in the entry.
/// </summary>
void GraphicManagerDialog::CleanTilesInstances()
{
    if (tmpblankTile != nullptr)
    {
        for(int i = 0; i < tmpTile8x8array.size(); i++)
        {
            if (tmpTile8x8array[i] != tmpblankTile)
            {
                delete tmpTile8x8array[i];
            }
        }
        tmpTile8x8array.clear();
        delete tmpblankTile;
        tmpblankTile = nullptr;
    }
}

/// <summary>
/// Generate the Tile instances according to the entry's tile data.
/// </summary>
/// <param name="entry">
/// The struct data of the entry.
/// </param>
void GraphicManagerDialog::GenerateBGTile8x8Instances(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    tmpblankTile = LevelComponents::Tile8x8::CreateBlankTile(entry.palettes);
    for (int i = 0; i < entry.TileDataRAMOffsetNum; ++i)
    {
        tmpTile8x8array.push_back(tmpblankTile);
    }
    int GFXcount = entry.TileDataSizeInByte / 32;
    for (int i = 0; i < GFXcount; ++i)
    {
        tmpTile8x8array.push_back(new LevelComponents::Tile8x8((unsigned char *)(entry.tileData.data() + i * 32), entry.palettes));
    }
    tmpTile8x8array.push_back(tmpblankTile);
}

/// <summary>
/// Clear and reset palettes in tmpEntry
/// </summary>
void GraphicManagerDialog::ClearAndResettmpEntryPalettes()
{
    for (unsigned int i = 0; i < 16; ++i)
    {
        ResetPaletteRowInTmpEntry(i);
    }
}

/// <summary>
/// Reset one palette row of tmpEntry into a color table full of transparent black colors.
/// </summary>
/// <param name="paletteId">
/// The palette slot ID of the palette row to reset.
/// </param>
void GraphicManagerDialog::ResetPaletteRowInTmpEntry(unsigned int paletteId)
{
    if (paletteId >= 16)
    {
        return;
    }
    if (tmpEntry.palettes[paletteId].size())
    {
        tmpEntry.palettes[paletteId].clear();
    }
    for (int i = 0; i < 16; ++i)
    {
        tmpEntry.palettes[paletteId].push_back(QColor(0, 0, 0, 0xFF).rgba());
    }
}

/// <summary>
/// Delete a Tile from the tmpEntry and from instances too.
/// </summary>
/// <param name="entry">
/// The struct data used to generate entry text.
/// </param>
void GraphicManagerDialog::DeltmpEntryTile(int tileId)
{
    switch (tmpEntry.TileDataType)
    {
        case AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg:
        {
            // change tmpEntry instance
            int startid = tmpEntry.TileDataRAMOffsetNum;
            if (tmpEntry.TileDataSizeInByte != tmpEntry.tileData.size())
            { // something went wrong in the code
                return;
            }
            QByteArray data;
            int old_tilenum = tmpEntry.TileDataSizeInByte / 32;
            data = tmpEntry.tileData.left(32 * (tileId - startid)) + tmpEntry.tileData.right(32 * (old_tilenum + startid - tileId - 1));
            tmpEntry.tileData = data;

            tmpEntry.TileDataRAMOffsetNum += 1;
            tmpEntry.TileDataSizeInByte -= 32;

            // update mapping data
            bool find_bug = false;
            for (int w = 0; w < tmpEntry.mappingData.size(); w++)
            {
                unsigned short data = tmpEntry.mappingData[w];
                unsigned short id = data & 0x3FF;
                if (id < tileId)
                {
                    tmpEntry.mappingData[w] = (data & 0xFC00) | ((id + 1) & 0x3FF);
                }
                else if (id == tileId)
                {
                    // something went wrong in the previous code if this part of code gets executed
                    // there should be no existance of the current tile when calling this function
                    // we just set it to use the default 0x3FF Tile8x8 and use the palette 0xF
                    tmpEntry.mappingData[w] = 0xF000 | 0x3FF;
                    find_bug = true;
                }
            }
            if (find_bug)
            {
                QMessageBox::critical(this, tr("Warning"), tr("Something went wrong when reducing tiles.\n"
                                                              "Contact developers for more details."));
            }

        }
        case AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp:
        {
            // TODO
        }
    }
}

/// <summary>
/// Check if all the data of an entry is loaded from vanilla ROM addresses.
/// </summary>
/// <remarks>
/// Such an entry only describes vanilla ROM data, none of its data is stored in a chunk generated
/// by the editor. Removing or modifying it would either lose the only description of that vanilla
/// data or need a new ROM data chunk for data the editor does not own, so the user is not allowed
/// to do that. Use the duplicate button to get an editable copy of the entry instead.
/// </remarks>
/// <param name="entry">
/// The entry to check.
/// </param>
/// <returns>
/// Return true if all the data of the entry is loaded from vanilla ROM addresses.
/// </returns>
static bool IsVanillaReferencingEntry(const struct AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    return (entry.TileDataAddress < WL4Constants::AvailableSpaceBeginningInROM)
            && (entry.PaletteAddress < WL4Constants::AvailableSpaceBeginningInROM)
            && (entry.MappingDataAddress < WL4Constants::AvailableSpaceBeginningInROM);
}

/// <summary>
/// Check if an entry can be edited or deleted
/// </summary>
/// <param name="entryId">
/// The id of the entry in the entrylist.
/// </param>
/// <returns>
/// Return true if the data is never used in the Tilesets or Rooms.
/// </returns>
bool GraphicManagerDialog::CheckEditability(int entryId)
{
    if (IsVanillaReferencingEntry(graphicEntries[entryId]))
    {
        QMessageBox::critical(this, tr("Error"), tr("Cannot delete or edit entry: ") + QString::number(entryId) + ",\n" +
                              tr("all of its Tile8x8 data, palette data and mapping data are vanilla ROM data.\n"
                                 "Duplicate it to get an editable copy of the entry."));
        return false;
    }

    unsigned int find_level = -1;
    unsigned int find_room = -1;
    unsigned int find_tileset = -1;
    if (AssortedGraphicUtils::CheckEditability(graphicEntries[entryId], find_level, find_room, find_tileset))
    {
        return true;
    }
    if (find_level != -1)
    {
        QMessageBox::critical(this, tr("Error"), tr("Cannot delete or edit entry: ") + QString::number(entryId) + ",\n" +
                              tr("its mapping data is found used in Level: ") + QString::number(find_level) +
                              tr(", Room: ") + QString::number(find_room));
    }
    if (find_tileset != -1)
    {
        QMessageBox::critical(this, tr("Error"), tr("Cannot delete or edit entry: ") + QString::number(entryId) + ",\n" +
                              tr("its Tile data is found used in Tileset: 0x") + QString::number(find_tileset, 16));
    }
    return false;
}

/// <summary>
/// Generate entries from the current ROM's Tileset and Rooms data.
/// </summary>
void GraphicManagerDialog::GetVanillaGraphicEntriesFromROM()
{
    // don't run this function if there is some graphic Entry exist(s).
    if (graphicEntries.size())
    {
        return;
    }

    // Generate a graphic entry from a Room header layer mapping data when it is not in the list yet
    auto addGraphicEntryFromRoomLayer = [this] (const LevelComponents::__RoomHeader &header,
                                                unsigned int mappingDataAddr, const QString &layerName,
                                                unsigned int levelId, unsigned int roomId, int roomIndex)
    {
        mappingDataAddr &= 0x7FF'FFFF;

        // don't add an entry twice when multiple Rooms share one mapping data chunk
        for (int n = 0; n < graphicEntries.size(); ++n)
        {
            if (graphicEntries[n].MappingDataAddress == mappingDataAddr)
            {
                return;
            }
        }

        // not found, so we add a new entry
        struct AssortedGraphicUtils::AssortedGraphicEntryItem newentry;
        LevelComponents::Tileset *roomtileset = ROMUtils::singletonTilesets[header.TilesetID];
        newentry.TileDataAddress = roomtileset->GetbgGFXptr();
        newentry.TileDataSizeInByte = roomtileset->GetbgGFXlen();
        int tilenum = newentry.TileDataSizeInByte / 32;
        newentry.TileDataRAMOffsetNum = 0x3FF - tilenum;
        newentry.TileDataType = AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg;
        newentry.TileDataName = "vanilla Tileset 0x" + QString::number(header.TilesetID, 16) + " bg tiles";
        newentry.MappingDataAddress = mappingDataAddr;
        newentry.MappingDataSizeAfterCompressionInByte = 0x1000; // a big number (0x40 x 0x40), since it won't cause problems for vanilla data
        newentry.MappingDataCompressType = AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType::RLE_mappingtype_0x20;
        newentry.MappingDataName = layerName + " found in: " + QString::number(levelId) + "-" +
                                   QString::number(roomId) + "-" + QString::number(roomIndex);
        newentry.PaletteAddress = roomtileset->GetPaletteAddr();
        for (unsigned int k = 0; k < 16; k++)
            newentry.PaletteSlotIDs.push_back(k); // initially all 16 slots
        newentry.optionalGraphicWidth = 0; // overwrite size params when the mapping data include size info
        newentry.optionalGraphicHeight = 0;

        AssortedGraphicUtils::ExtractDataFromEntryInfo_v2(newentry);

        // Collect all distinct palette IDs actually used by the mapping data
        QSet<unsigned int> usedPaletteIDs;
        for (int m = 0; m < newentry.mappingData.size(); m++)
        {
            int tileid = (newentry.mappingData[m] & 0x3FF);
            if (tileid != 0x3FF)
            {
                unsigned int palId = (newentry.mappingData[m] & 0xF000) >> 12;
                usedPaletteIDs.insert(palId);
            }
        }

        // Build PaletteSlotIDs from the actually-used palette IDs (sorted)
        newentry.PaletteSlotIDs.clear();
        for (unsigned int id : usedPaletteIDs)
            newentry.PaletteSlotIDs.push_back(id);
        std::sort(newentry.PaletteSlotIDs.begin(), newentry.PaletteSlotIDs.end());
        if (newentry.PaletteSlotIDs.isEmpty())
            newentry.PaletteSlotIDs.push_back(0); // fallback

        // PaletteAddress stays at the tileset's palette base (palette 0).
        // ExtractDataFromEntryInfo_v2 uses tmpPalId*32 offset for vanilla ROM
        // addresses, so keeping the base address is correct.

        // Clear unused palettes
        for (int i = 0; i < 16; ++i)
        {
            if (!usedPaletteIDs.contains(i))
            {
                newentry.palettes[i].clear();
                for (int j = 0; j < 16; ++j) // (re-)initialization
                {
                    newentry.palettes[i].push_back(QColor(0, 0, 0, 0xFF).rgba());
                }
            }
        }
        graphicEntries.append(newentry);
    };

    // loop through all the Rooms
    QVector<unsigned int> levelid_array = {0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5};
    QVector<unsigned int> roomid_array = {0, 2, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 4};
    for (int i = 0; i < levelid_array.size(); i++)
    {
        LevelComponents::Level *tmpLevel = new LevelComponents::Level(static_cast<LevelComponents::__passage>(levelid_array[i]),
                                                                      static_cast<LevelComponents::__stage>(roomid_array[i]));
        for (int j = 0; j < tmpLevel->GetRooms().size(); j++)
        {
            LevelComponents::__RoomHeader header = tmpLevel->GetRooms()[j]->GetRoomHeader();

            // Layer 0 generated from Tile8x8 data uses the background Tile8x8 set of its Tileset,
            // so its mapping data is an usable graphic like the Layer 3 mapping data
            if ((header.Layer0MappingType & 0x30) == 0x20)
            {
                addGraphicEntryFromRoomLayer(header, header.Layer0Data, "Layer 0",
                                             levelid_array[i], roomid_array[i], j);
            }

            if ((header.Layer3MappingType & 0x30) == 0x20)
            {
                addGraphicEntryFromRoomLayer(header, header.Layer3Data, "Layer 3",
                                             levelid_array[i], roomid_array[i], j);
            }
        }

        delete tmpLevel;
    }
}

/// <summary>
/// Generate the text of one entry according to a provided struct, to show in the listview.
/// </summary>
/// <param name="entry">
/// The struct data used to generate entry text.
/// </param>
/// <returns>
/// The text of the entry to show in listview.
/// </returns>
QString GraphicManagerDialog::GenerateEntryTextFromStruct(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    QString ret = entry.TileDataName + " - " + entry.MappingDataName;
    return ret;
}

/// <summary>
/// Be called when the listview is clicked and an entry is selected.
/// </summary>
/// <param name="index">
/// Reference of the selected QModelIndex from the listview.
/// </param>
void GraphicManagerDialog::on_listView_RecordGraphicsList_clicked(const QModelIndex &index)
{
    ui->listView_RecordGraphicsList->setEnabled(false);
    QItemSelectionModel *select = ui->listView_RecordGraphicsList->selectionModel();
    QModelIndexList selectedRows = select->selectedRows();
    int num_of_select_rows = selectedRows.size();
    ui->pushButton_RemoveGraphicEntries->setEnabled(num_of_select_rows);
    if (num_of_select_rows == 1)
    {
        ClearMappingPanel();
        ClearPalettePanel();
        ClearTilesPanel();
        int linenum = index.row();
        SelectedEntryID = linenum;

        // clean up instances if the tmpEntry was used
        CleanTilesInstances();
        CleanMappingDataInEntry(tmpEntry);

        tmpEntry = graphicEntries[linenum]; // always use tmpentry before validation
        ExtractEntryToGUI(tmpEntry);

        // enable current entry editing
        ui->pushButton_ImportGraphic->setEnabled(true);
        ui->pushButton_ImportPaletteData->setEnabled(true);
        ui->pushButton_ImportTile8x8Data->setEnabled(true);
        ui->pushButton_validateAndSetMappingData->setEnabled(true);
        ui->pushButton_duplicateCurrentEntry->setEnabled(true);
        ui->pushButton_SwapPalettes->setEnabled(true);
    }
    else
    {
        SelectedEntryID = -1;
        ClearMappingPanel();
        ClearPalettePanel();
        ClearTilesPanel();

        // disable current entry editing
        ui->pushButton_ImportGraphic->setEnabled(false);
        ui->pushButton_ImportPaletteData->setEnabled(false);
        ui->pushButton_ImportTile8x8Data->setEnabled(false);
        ui->pushButton_validateAndSetMappingData->setEnabled(false);
        ui->pushButton_duplicateCurrentEntry->setEnabled(false);
        ui->pushButton_SwapPalettes->setEnabled(false);
    }

    ui->listView_RecordGraphicsList->setEnabled(true);
}

/// <summary>
/// Click the button to clear Tile panel UI stuff and the tile data in tmpEntry.
/// The tile data of the entry is not touched until "Validate and Set" is clicked, so
/// re-selecting the entry discards the clearing.
/// </summary>
void GraphicManagerDialog::on_pushButton_ClearTile8x8Data_clicked()
{
    // Reset the tile data in tmpEntry
    tmpEntry.tileData.clear();
    tmpEntry.TileDataAddress = 0;
    tmpEntry.TileDataSizeInByte = 0;
    tmpEntry.TileDataRAMOffsetNum = 0x3FF; // no Tile8x8 is used by the entry yet
    tmpEntry.TileDataType = AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg;
    tmpEntry.TileDataName.clear();

    ClearTilesPanel();

    // Regenerate the Tile8x8 instances for the empty tile data, so that the mapping
    // graphic can still be rendered (with blank Tile8x8s) without crashing
    CleanTilesInstances();
    GenerateBGTile8x8Instances(tmpEntry);
    UpdateTilesGraphicView(tmpEntry);
}

/// <summary>
/// Click the button to clear palette panel UI stuff and the palette data in tmpEntry.
/// The palette data of the entry is not touched until "Validate and Set" is clicked, so
/// re-selecting the entry discards the clearing.
/// </summary>
void GraphicManagerDialog::on_pushButton_ClearPaletteData_clicked()
{
    // Reset the palette data in tmpEntry.
    // Note: an entry without any palette slot ID cannot be stored in the chunk data, the
    // palettes are written with the default slot ID 0 (transparent colors) when such an
    // entry is saved and loaded again.
    tmpEntry.PaletteAddress = 0;
    tmpEntry.PaletteSlotIDs.clear();
    ClearAndResettmpEntryPalettes();

    ClearPalettePanel();

    // The mapping graphic uses palette data too, so it needs to be re-rendered
    UpdateTilesGraphicView(tmpEntry);
    UpdateMappingGraphicView(tmpEntry);
}

/// <summary>
/// Click the button to clear mapping panel UI stuff and the mapping data in tmpEntry.
/// The mapping data of the entry is not touched until "Validate and Set" is clicked, so
/// re-selecting the entry discards the clearing.
/// </summary>
void GraphicManagerDialog::on_pushButton_ClearMappingData_clicked()
{
    // Reset the mapping data in tmpEntry
    CleanMappingDataInEntry(tmpEntry);
    tmpEntry.MappingDataAddress = 0;
    tmpEntry.MappingDataSizeAfterCompressionInByte = 0;
    tmpEntry.optionalGraphicWidth = 0;
    tmpEntry.optionalGraphicHeight = 0;
    tmpEntry.MappingDataName.clear();
    // Match the mapping data type combo box, which is reset in ClearMappingPanel()
    tmpEntry.MappingDataCompressType = AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType::No_mapping_data_comp;

    ClearMappingPanel();
}

/// <summary>
/// Click the button to load palette from the ROM or from pal file
/// </summary>
void GraphicManagerDialog::on_pushButton_ImportPaletteData_clicked()
{
    if (SelectedEntryID != -1)
    {
        // try to use the settings from the UI to import palette
        int palAddress = ui->lineEdit_paletteAddress->text().toUInt(nullptr, 16);

        // Parse palette slot IDs from the comma-separated hex field
        QVector<unsigned int> slotIDs;
        if (!ParseCommaSeparatedHexValues(ui->lineEdit_paletteNum->text(), slotIDs, 0xF, tr("palette slot ID")))
        {
            return;
        }

        // palAddress is not a vanilla rom address, so we need to import palette from file
        if (!palAddress || palAddress >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            if (slotIDs.isEmpty())
            {
                QMessageBox::critical(this, tr("Error"), tr("No valid palette slot IDs specified!"));
                return;
            }
            if (slotIDs.size() == 1) // we only import one 16-color palette
            {
                // Add the imported palette into the entry, the palettes imported before are kept
                if (!FileIOUtils::ImportPalette(this,
                    [this] (int selectedPalId, int colorId, QRgb newColor)
                    {
                        this->tmpEntry.SetColor(selectedPalId, colorId, newColor);
                    },
                    slotIDs[0]))
                {
                    return; // the user cancelled the importing
                }

                // set tmpEntry if everything looks correct
                tmpEntry.PaletteAddress = 0;
                if (!tmpEntry.PaletteSlotIDs.contains(slotIDs[0]))
                {
                    tmpEntry.PaletteSlotIDs.push_back(slotIDs[0]);
                }
            }
            else
            {
                // TDOO
                QMessageBox::critical(this, tr("Error"), tr("Import multiple palettes from one file cannot work yet!"));
                return;
            }
        }
        else // we need to import palette from the current ROM directly
        {
            // Sanity check
            if ((palAddress & 3) || (palAddress < 0))
            {
                QMessageBox::critical(this, tr("Error"), tr("The address has to be multiple of 4!"));
                return;
            }
            if (slotIDs.isEmpty())
            {
                QMessageBox::critical(this, tr("Error"), tr("No valid palette slot IDs specified!"));
                return;
            }

            // Clean all the palettes before importing the new palette set from the ROM
            ClearAndResettmpEntryPalettes();

            // Load palette(s) from the ROM — one per slot ID, consecutive in ROM
            for (int i = 0; i < slotIDs.size(); ++i)
            {
                unsigned int slotId = slotIDs[i];
                if (tmpEntry.palettes[slotId].size())
                    tmpEntry.palettes[slotId].clear();
                // First color is transparent
                ROMUtils::LoadPalette(&(tmpEntry.palettes[slotId]), (unsigned short *) (ROMUtils::ROMFileMetadata->ROMDataPtr + palAddress + i * 32));
            }

            // set tmpEntry if everything looks correct
            tmpEntry.PaletteAddress = palAddress;
            tmpEntry.PaletteSlotIDs = slotIDs;
        }

        // UI reset
        UpdatePaletteGraphicView(tmpEntry);
        SetPaletteInfoGUI(tmpEntry);

        // UI reset on other panels
        CleanTilesInstances();
        GenerateBGTile8x8Instances(tmpEntry);
        UpdateTilesGraphicView(tmpEntry);
        UpdateMappingGraphicView(tmpEntry);
    }
}

/// <summary>
/// Click the button to load tile data from the ROM or from bin file
/// </summary>
void GraphicManagerDialog::on_pushButton_ImportTile8x8Data_clicked()
{
    if (SelectedEntryID != -1)
    {
        // try to use the settings from the UI to import palette
        int tiledataAddress = ui->lineEdit_tileDataAddress->text().toUInt(nullptr, 16);
        int tiledatatype = ui->comboBox_tileDataType->currentIndex();

        // tiledataAddress is not a vanilla rom address, so we need to import tile data from file
        if (!tiledataAddress || tiledataAddress >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            switch (tiledatatype)
            {
                case AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg:
                {
                    QString tmpname = ui->lineEdit_tileDataName->text();

                    // Let user to choose a palette for reference when import graphic by bin files
                    int refPalette = QInputDialog::getText(this,
                                                           tr("WL4Editor"),
                                                           tr("Choose a ref palette to import Tile8x8 data:\n"
                                                              "(Use Hex Id)"), QLineEdit::Normal,
                                                           "0xF").toUInt(nullptr, 16);
                    refPalette = qMin(refPalette, 0xF);
                    refPalette = qMax(refPalette, 0);

                    // Ignore the settings from the UI, import tile data directly and see if the data is legal
                    FileIOUtils::ImportTile8x8GfxData(this,
                        tmpEntry.palettes[refPalette],
                        tr("Choose a color to covert to transparent:"),
                        [this, &tmpname] (QByteArray finaldata, QWidget *parentPtr)
                        {
                            // Assume the file is fully filled with tiles
                            int newtilenum = finaldata.size() / 32;
                            int existingtilenum = this->tmpEntry.tileData.size() / 32;
                            if((newtilenum + existingtilenum) > 0x3FE)
                            {
                                QMessageBox::critical(parentPtr, tr("Load Error"), tr("You can only use 0x3FF background tiles at most!"));
                                return;
                            }
                            else
                            {
                                // Stack the new Tile8x8 data in front of the data imported before, so the
                                // Tile8x8 indexes used by the existing mapping data keep unchanged
                                this->tmpEntry.tileData = finaldata + this->tmpEntry.tileData;

                                // set tmpEntry if everything looks correct
                                int startid = 0x3FF - (newtilenum + existingtilenum);
                                this->tmpEntry.TileDataRAMOffsetNum = startid;
                                this->tmpEntry.TileDataSizeInByte = this->tmpEntry.tileData.size();
                                this->tmpEntry.TileDataAddress = 0;
                                this->tmpEntry.TileDataType = AssortedGraphicUtils::Tile8x8_4bpp_no_comp_Tileset_text_bg;
                                this->tmpEntry.TileDataName = tmpname;
                            }
                        });

                    // UI reset
                    CleanTilesInstances();
                    GenerateBGTile8x8Instances(tmpEntry);
                    UpdateTilesGraphicView(tmpEntry);
                    SetTilesPanelInfoGUI(tmpEntry);

                    // UI reset on other panels
                    UpdateMappingGraphicView(tmpEntry);

                    break;
                }
                case AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp:
                {
                    QMessageBox::critical(this, tr("Error"), tr("Import tiles for Tile8x8_4bpp_no_comp cannot work yet!"));
                    break;
                }
            }

        }
        else // we need to import tile data from the current ROM directly
        {
            switch (tiledatatype)
            {
                case AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg:
                {
                    // Sanity check
                    unsigned int tiledataSizeInByte = ui->lineEdit_tileDataSizeInByte->text().toUInt(nullptr, 16);
                    unsigned int tileVRAMoffsetNum = ui->lineEdit_tileDataRAMoffset->text().toUInt(nullptr, 16);
                    unsigned int tile8x8Num = tiledataSizeInByte / 32;
                    unsigned int tiledataaddr = ui->lineEdit_tileDataAddress->text().toUInt(nullptr, 16);
                    if ((tile8x8Num << 5) != tiledataSizeInByte)
                    {
                        QMessageBox::critical(this, tr("Error"), tr("Illegal tile data size, size has to be multiple of 0x20!"));
                        return;
                    }
                    if ((tile8x8Num + tileVRAMoffsetNum) > 0x3FF)
                    {
                        QMessageBox::critical(this, tr("Error"), tr("Tile8x8 index(es) out of bound!\n"
                                                                    "The last tile8x8 has to be indexed 0x3FE"));
                        return;
                    }
                    if ((tile8x8Num + tileVRAMoffsetNum) < 0x3FF)
                    {
                        QMessageBox::critical(this, tr("Error"), tr("The index of the last Tile8x8 isn't 0x2FE,\n"
                                                                    "which is a rule for Tileset background tiles."));
                        return;
                    }
                    if (tiledataaddr & 3)
                    {
                        QMessageBox::critical(this, tr("Error"), tr("The address has to be multiple of 4!"));
                        return;
                    }

                    // Load Tile data
                    tmpEntry.tileData.resize(tiledataSizeInByte);
                    for (int j = 0; j < tiledataSizeInByte; ++j)
                    {
                        tmpEntry.tileData[j] = *(ROMUtils::ROMFileMetadata->ROMDataPtr + tiledataaddr + j);
                    }

                    // set tmpEntry
                    tmpEntry.TileDataSizeInByte = tiledataSizeInByte;
                    tmpEntry.TileDataAddress = tiledataaddr;
                    tmpEntry.TileDataType = AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg;
                    tmpEntry.TileDataRAMOffsetNum = tileVRAMoffsetNum;
                    tmpEntry.TileDataName = ui->lineEdit_tileDataName->text();

                    // UI reset
                    CleanTilesInstances();
                    GenerateBGTile8x8Instances(tmpEntry);
                    UpdateTilesGraphicView(tmpEntry);
                    SetTilesPanelInfoGUI(tmpEntry);

                    // UI reset on other panels
                    UpdateMappingGraphicView(tmpEntry);

                    break;
                }
                case AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp:
                {
                    QMessageBox::critical(this, tr("Error"), tr("Import tiles for Tile8x8_4bpp_no_comp cannot work yet!"));
                    break;
                }
            }

        }
    }
}

/// <summary>
/// Click the button to load mapping data from the ROM or from bin file
/// </summary>
void GraphicManagerDialog::on_pushButton_ImportGraphic_clicked()
{
    if (SelectedEntryID != -1)
    {
        // try to use the settings from the UI to import mapping data
        int mappingdataAddress = ui->lineEdit_mappingDataAddress->text().toUInt(nullptr, 16);
        int mappingdatatype = ui->comboBox_mappingDataType->currentIndex();
        int mappingdataSizeInByte = ui->lineEdit_mappingDataSizeInByte->text().toUInt(nullptr, 16);
        int optionalgraphicWidth = ui->lineEdit_optionalGraphicWidth->text().toUInt(nullptr, 16);
        int optionalgraphicHeight = ui->lineEdit_optionalGraphicHeight->text().toUInt(nullptr, 16);

        // tiledataAddress is not a vanilla rom address, so we need to import tile data from file
        if (!mappingdataAddress || mappingdataAddress >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            switch (mappingdatatype)
            {
                case AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType::RLE_mappingtype_0x20:
                {
                    // check if optionalgraphicWidth or optionalgraphicHeight looks correct
                    if (optionalgraphicWidth != 0x20 && optionalgraphicWidth != 0x40)
                    {
                        QMessageBox::critical(this, tr("Load Error"), tr("Wrong graphic Width to import graphic for RLE_mappingtype_0x20,\n"
                                                                              "it has to be 0x20 or 0x40!"));
                        return;
                    }
                    if (optionalgraphicHeight != 0x20 && optionalgraphicHeight != 0x40)
                    {
                        QMessageBox::critical(this, tr("Load Error"), tr("Wrong graphic Height to import graphic for RLE_mappingtype_0x20,\n"
                                                                              "it has to be 0x20 or 0x40!"));
                        return;
                    }
                    if (optionalgraphicWidth == 0x40 && optionalgraphicHeight == 0x40)
                    {
                        QMessageBox::critical(this, tr("Load Error"), tr("vanilla Layer 3 cannot be 0x40 by 0x40 in size!"));
                        return;
                    }

                    // Let user to choose a palette for reference when import graphic by bin files
                    int refPalette = QInputDialog::getText(this,
                                                           tr("WL4Editor"),
                                                           tr("Choose a ref palette to import mapping data of graphics:\n"
                                                              "(Use Hex Id)"), QLineEdit::Normal,
                                                           "0xF").toUInt(nullptr, 16);
                    refPalette = qMin(refPalette, 0xF);
                    refPalette = qMax(refPalette, 0);

                    // Use the optional width and the imported tile data to check if the width and height are legal
                    FileIOUtils::ImportTile8x8GfxData(this,
                        tmpEntry.palettes[refPalette], // use a palette for palette comparison
                        tr("Choose a color to covert to transparent:"),
                        [this, &optionalgraphicWidth, &optionalgraphicHeight, &refPalette] (QByteArray finaldata, QWidget *parentPtr)
                        {
                            // Assume the file is fully filled with tiles
                            int newtilenum = finaldata.size() / 32;
                            if(newtilenum != 0x400 && newtilenum != 0x800 && newtilenum != 0x1000)
                            {
                                QMessageBox::critical(parentPtr, tr("Load Error"), tr("The pic size has to be 0x20 by 0x20,\n"
                                                                                      "0x20 by 0x40 or 0x40 by 0x20!"));
                                return;
                            }
                            else
                            {
                                // Get existing bg tile data
                                int existingTile8x8Num = this->tmpEntry.TileDataSizeInByte / 32;
                                int existingTilesdatasize = (existingTile8x8Num + 1) * 32;
                                unsigned char *tmp_current_tile8x8_data = new unsigned char[existingTilesdatasize];
                                memset(&tmp_current_tile8x8_data[0], 0, existingTilesdatasize);
                                auto tile8x8array = this->tmpTile8x8array;
                                int startId = this->tmpEntry.TileDataRAMOffsetNum;
                                for (int j = startId; j < 0x400; j++)
                                {
                                    memcpy(&tmp_current_tile8x8_data[(j - startId) * 32], tile8x8array[j]->GetRawPixelData().data(), 32);
                                }

                                // Reset optionalgraphicWidth and optionalgraphicHeight if needed
                                if (newtilenum == 0x400)
                                {
                                    optionalgraphicWidth = 0x20;
                                    optionalgraphicHeight = 0x20;
                                }
                                else if (newtilenum == 0x1000)
                                {
                                    optionalgraphicWidth = 0x40;
                                    optionalgraphicHeight = 0x40;
                                }
                                else if (newtilenum == 0x800)
                                {
                                    // assume the optionalgraphicWidth is correct
                                    optionalgraphicHeight = 0x800 / optionalgraphicWidth;
                                }

                                // Generate mapping data
                                QVector<unsigned short> tmpMappingData;
                                for(int i = 0; i < newtilenum; ++i)
                                {
                                    unsigned char newtmpdata[32];
                                    unsigned char newtmpXFlipdata[32];
                                    unsigned char newtmpYFlipdata[32];
                                    unsigned char newtmpXYFlipdata[32];

                                    memcpy(newtmpdata, finaldata.data() + 32 * i, 32);
                                    ROMUtils::Tile8x8DataXFlip(newtmpdata, newtmpXFlipdata);
                                    ROMUtils::Tile8x8DataYFlip(newtmpdata, newtmpYFlipdata);
                                    ROMUtils::Tile8x8DataYFlip(newtmpXFlipdata, newtmpXYFlipdata);

                                    bool find_eqaul = false;
                                    unsigned short mappingdata;
                                    // loop from the first blank tile, excluding those animated tiles
                                    for (int j = startId; j < 0x400; j++)
                                    {
                                        int result0 = memcmp(newtmpdata, &tmp_current_tile8x8_data[(j - startId) * 32], 32);
                                        int result1 = memcmp(newtmpXFlipdata, &tmp_current_tile8x8_data[(j - startId) * 32], 32);
                                        int result2 = memcmp(newtmpYFlipdata, &tmp_current_tile8x8_data[(j - startId) * 32], 32);
                                        int result3 = memcmp(newtmpXYFlipdata, &tmp_current_tile8x8_data[(j - startId) * 32], 32);
                                        int tileid = j;
                                        int paletteId = refPalette;

                                        if (!result0)
                                        {
                                            mappingdata = (paletteId & 0xF) << 12 | (0 << 11) | (0 << 10) | (tileid & 0x3FF);
                                            find_eqaul = true;
                                            break;
                                        }
                                        else if (!result1)
                                        {
                                            mappingdata = (paletteId & 0xF) << 12 | (0 << 11) | (1 << 10) | (tileid & 0x3FF);
                                            find_eqaul = true;
                                            break;
                                        }
                                        else if (!result2)
                                        {
                                            mappingdata = (paletteId & 0xF) << 12 | (1 << 11) | (0 << 10) | (tileid & 0x3FF);
                                            find_eqaul = true;
                                            break;
                                        }
                                        else if (!result3)
                                        {
                                            mappingdata = (paletteId & 0xF) << 12 | (1 << 11) | (1 << 10) | (tileid & 0x3FF);
                                            find_eqaul = true;
                                            break;
                                        }
                                    }
                                    if (!find_eqaul)
                                    {// not find any existing tile8x8 eqaul to the current tile8x8
                                        QMessageBox::critical(parentPtr, tr("Load Error"),
                                                              tr("Detect a Tile8x8 cannot be found in the current Tile8x8 set!"));
                                        delete[] tmp_current_tile8x8_data;
                                        return;
                                    }
                                    tmpMappingData.push_back(mappingdata);
                                }

                                // set tmpEntry if everything looks correct
                                this->tmpEntry.MappingDataAddress = 0;
                                this->tmpEntry.MappingDataCompressType = AssortedGraphicUtils::RLE_mappingtype_0x20;
                                this->tmpEntry.MappingDataSizeAfterCompressionInByte = 0; // the save logic should set this
                                this->tmpEntry.mappingData = tmpMappingData;
                                this->tmpEntry.optionalGraphicWidth = optionalgraphicWidth;
                                this->tmpEntry.optionalGraphicHeight = optionalgraphicHeight;
                                delete[] tmp_current_tile8x8_data;
                            }
                        });
                    tmpEntry.MappingDataName = ui->lineEdit_mappingDataName->text();

                    // UI reset
                    UpdateMappingGraphicView(tmpEntry);
                    SetMappingGraphicInfoGUI(tmpEntry);

                    break;
                }
                case AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType::No_mapping_data_comp:
                { // this case never work atm
                    QMessageBox::critical(this, tr("Error"), tr("Import No_mapping_data_comp graphic from file cannot work yet!"));
                    break;
                }
            }

        }
        else // we need to import tile data from the current ROM directly
        {
            switch (mappingdatatype)
            {
                case AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType::No_mapping_data_comp:
                { // this case never work atm
                    QMessageBox::critical(this, tr("Error"), tr("Import No_mapping_data_comp graphic from ROM cannot work yet!"));
                    break;

//                    for (int i = 0; i < optionalgraphicWidth * optionalgraphicHeight; ++i)
//                    {
//                        unsigned short *data = (unsigned short *)(ROMUtils::ROMFileMetadata->ROMDataPtr + mappingdataAddress);
//                        tmpEntry.mappingData.push_back(data[i]);
//                    }
//                    break;
                }
                case AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType::RLE_mappingtype_0x20:
                {
                    // Clean the old mapping data first, otherwise the data imported before will
                    // be accumulated, which can overflow the buffer in the saving logic
                    CleanMappingDataInEntry(tmpEntry);

                    LevelComponents::Layer BGlayer(mappingdataAddress, LevelComponents::LayerTile8x8);
                    optionalgraphicHeight = BGlayer.GetLayerHeight();
                    optionalgraphicWidth = BGlayer.GetLayerWidth();
                    unsigned short *layerdata = BGlayer.GetLayerData();
                    for (int i = 0; i < optionalgraphicWidth * optionalgraphicHeight; ++i)
                    {
                        tmpEntry.mappingData.push_back(layerdata[i]);
                    }

                    // set tmpEntry if everything looks correct
                    tmpEntry.MappingDataCompressType = AssortedGraphicUtils::RLE_mappingtype_0x20;
                    tmpEntry.MappingDataSizeAfterCompressionInByte = 0; // the save logic should set this
                    tmpEntry.MappingDataName = ui->lineEdit_mappingDataName->text();
                    tmpEntry.optionalGraphicWidth = optionalgraphicWidth;
                    tmpEntry.optionalGraphicHeight = optionalgraphicHeight;

                    // UI reset
                    UpdateMappingGraphicView(tmpEntry);
                    SetMappingGraphicInfoGUI(tmpEntry);

                    break;
                }
            }
        }
    }
}

/// <summary>
/// Add a new default Entry into the Listview.
/// </summary>
void GraphicManagerDialog::on_pushButton_AddGraphicEntry_clicked()
{
    CreateAndAddDefaultEntry();
    UpdateEntryList();

    // clean up instances if the tmpEntry was used
    CleanTilesInstances();
    CleanMappingDataInEntry(tmpEntry);
    SelectedEntryID = -1;

    // UI update
    ClearMappingPanel();
    ClearPalettePanel();
    ClearTilesPanel();
    UpdateEntryList();

    // disable current entry editing
    ui->pushButton_ImportGraphic->setEnabled(false);
    ui->pushButton_ImportPaletteData->setEnabled(false);
    ui->pushButton_ImportTile8x8Data->setEnabled(false);
    ui->pushButton_validateAndSetMappingData->setEnabled(false);
    ui->pushButton_duplicateCurrentEntry->setEnabled(false);
    ui->pushButton_SwapPalettes->setEnabled(false);
    ui->pushButton_RemoveGraphicEntries->setEnabled(false); // should always be false since on selected row any more
}

/// <summary>
/// Delete the selected Entries from the Listview.
/// </summary>
void GraphicManagerDialog::on_pushButton_RemoveGraphicEntries_clicked()
{
    ui->listView_RecordGraphicsList->setEnabled(false);
    QItemSelectionModel *select = ui->listView_RecordGraphicsList->selectionModel();
    QModelIndexList selectedRows = select->selectedRows();
    int num_of_select_rows = selectedRows.size();
    bool no_removal = true;

    if (num_of_select_rows > 0)
    {
        // clean up list and delete entry and reset tmpEntry
        int lastid = graphicEntries.size() - 1;
        std::sort(selectedRows.begin(), selectedRows.end(), [] ( auto &a, auto &b) { return a.row() > b.row(); }); // so that rows are removed from highest index
        foreach (QModelIndex index, selectedRows)
        {
            int id = index.row();
            if (!CheckEditability(id))
            {
                continue;
            }

            // remove entry
            graphicEntries.removeAt(id);
            no_removal = false;
            lastid = id;
        }

        if (no_removal)
        {
            ui->listView_RecordGraphicsList->setEnabled(true);
            return;
        }

        // clean up instances if the tmpEntry was used
        CleanTilesInstances();
        CleanMappingDataInEntry(tmpEntry);
        SelectedEntryID = lastid - 1;

        // UI update
        ClearMappingPanel();
        ClearPalettePanel();
        ClearTilesPanel();
        UpdateEntryList();

        if (lastid > 0)
        {
            tmpEntry = graphicEntries[lastid - 1];
            ExtractEntryToGUI(tmpEntry);
        }

        // enable or disable current entry editing
        ui->pushButton_ImportGraphic->setEnabled(lastid);
        ui->pushButton_ImportPaletteData->setEnabled(lastid);
        ui->pushButton_ImportTile8x8Data->setEnabled(lastid);
        ui->pushButton_validateAndSetMappingData->setEnabled(lastid);
        ui->pushButton_duplicateCurrentEntry->setEnabled(lastid);
        ui->pushButton_SwapPalettes->setEnabled(lastid);
        ui->pushButton_RemoveGraphicEntries->setEnabled(lastid);
    }

    ui->listView_RecordGraphicsList->setEnabled(true);
    ui->listView_RecordGraphicsList->setCurrentIndex(ui->listView_RecordGraphicsList->selectionModel()->model()->index(SelectedEntryID, 0));
}

/// <summary>
/// only after import graphic, we can save tmpEntry into entrieslist.
/// if the user touch import palette or import tiles again, save tmpEntry into entrieslist is not allowed
/// </summary>
void GraphicManagerDialog::on_pushButton_validateAndSetMappingData_clicked()
{
    if (SelectedEntryID > -1)
    {
        for (int i = 0; i < graphicEntries.size(); i++)
        {
            if (i == SelectedEntryID) continue;
            if (graphicEntries[i].MappingDataName == tmpEntry.MappingDataName &&
                    graphicEntries[i].TileDataName == tmpEntry.TileDataName)
            {
                QMessageBox::information(this, tr("Error"), tr("Find the same MappingDataName and TileDataName from another entry,\n"
                                                               "you need to make at least one of the names different."));
                return;
            }
        }

        // permit set entry as long as tmpEntry's tiles can work with the current mapping data
        if (tmpEntry.TileDataType == AssortedGraphicUtils::Tile8x8_4bpp_no_comp_Tileset_text_bg &&
                tmpEntry.MappingDataCompressType == AssortedGraphicUtils::RLE_mappingtype_0x20)
        {
            if (CheckEditability(SelectedEntryID))
            {
                graphicEntries[SelectedEntryID] = tmpEntry;
            }
        }

        // reset ListView Item name
        ListViewItemModel->item(SelectedEntryID, 0)->setText(GenerateEntryTextFromStruct(tmpEntry));

        // UI reset
        UpdatePaletteGraphicView(tmpEntry);
        SetPaletteInfoGUI(tmpEntry);
        UpdateTilesGraphicView(tmpEntry);
        SetTilesPanelInfoGUI(tmpEntry);
        UpdateMappingGraphicView(tmpEntry);
        SetMappingGraphicInfoGUI(tmpEntry);
    }
}

/// <summary>
/// Save all the entries into the ROM the close the dialog.
/// </summary>
void GraphicManagerDialog::on_pushButton_saveAllGraphicEntries_clicked()
{
    // Generate the save chunks and write them to the ROM
    QString errorStr = AssortedGraphicUtils::SaveAssortedGraphicsToROM(graphicEntries);
    if(errorStr.isEmpty())
    {
        singleton->GetOutputWidgetPtr()->PrintString(tr("Finished saving graphics to ROM. (entry number: %1)").arg(QString::number(graphicEntries.size())));
        this->close();
    }
    else
    {
        QMessageBox::information(this, tr("Error saving graphics"), errorStr);
    }
}

/// <summary>
/// Close the dialog.
/// </summary>
void GraphicManagerDialog::on_pushButton_cancelEditing_clicked()
{
    // TODO
    CleanTilesInstances();
    this->close();
}

/// <summary>
/// Reduce Tile usage in graphic.
/// </summary>
void GraphicManagerDialog::on_pushButton_ReduceTiles_clicked()
{
    switch (tmpEntry.TileDataType)
    {
        case AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg:
        {
            // ask user if eliminate similar tiles to reduce more tiles
            bool ok;
            int diff_upbound = QInputDialog::getInt(this,
                                                    tr("WL4Editor"),
                                                    tr("Input a tolerance value for the number of different pixels between 2 Tile8x8s.\n"
                                                       "The editor will merge similar Tile8x8s to reduce tile count aggressively.\n"
                                                       "Use a bigger value to reduce more tiles. However, the quality of the combined tiles will drop.\n"
                                                       "To perform regular tile reduction, use a value of 0."),
                                                      0, 0, 64, 1, &ok);
            if (!ok) return;

            // tile reduce
            int existingTile8x8Num = tmpEntry.tileData.size() / 32;
            unsigned char newtmpdata[32];
            unsigned char newtmpXFlipdata[32];
            unsigned char newtmpYFlipdata[32];
            unsigned char newtmpXYFlipdata[32];

            int data_size = tmpEntry.tileData.size();
            unsigned char *tmp_current_tile8x8_data = new unsigned char[data_size];
            memcpy(tmp_current_tile8x8_data, tmpEntry.tileData.data(), data_size);
            int constoffset = tmpEntry.TileDataRAMOffsetNum;

            // Compare through all the existing foreground Tile8x8s
            for(int i = 0; i < existingTile8x8Num; i++)
            {
                // Generate 4 possible existing Tile8x8 graphic data for comparison
                memcpy(newtmpdata, &tmp_current_tile8x8_data[32 * i], 32);
                ROMUtils::Tile8x8DataXFlip(newtmpdata, newtmpXFlipdata);
                ROMUtils::Tile8x8DataYFlip(newtmpdata, newtmpYFlipdata);
                ROMUtils::Tile8x8DataYFlip(newtmpXFlipdata, newtmpXYFlipdata);
                int old_tileid = i + constoffset;

                // loop from the last tile to the first tile
                for (int j = existingTile8x8Num - 1; j > i; j--)
                {
                    unsigned char tile_data[32];
                    memcpy(tile_data, &tmp_current_tile8x8_data[32 * j], 32);

                    int result0 = FileIOUtils::quasi_memcmp(newtmpdata, tile_data, 32);
                    int result1 = FileIOUtils::quasi_memcmp(newtmpXFlipdata, tile_data, 32);
                    int result2 = FileIOUtils::quasi_memcmp(newtmpYFlipdata, tile_data, 32);
                    int result3 = FileIOUtils::quasi_memcmp(newtmpXYFlipdata, tile_data, 32);
                    bool find_eqaul = false;
                    int find_tileid = j + constoffset;
                    int reserved_tileid = find_tileid;

                    int x_flip = 0;
                    int y_flip = 0;

                    unsigned short mappingdata;
                    if (result0 <= diff_upbound)
                    {
                        find_eqaul = true;
                        if (newtmpdata == FileIOUtils::find_less_feature_buff(newtmpdata, tile_data, 32))
                        {
                            reserved_tileid = old_tileid;
                        }
                    }
                    else if (result1 <= diff_upbound)
                    {
                        find_eqaul = true;
                        if (newtmpXFlipdata == FileIOUtils::find_less_feature_buff(newtmpXFlipdata, tile_data, 32))
                        {
                            reserved_tileid = old_tileid;
                        }
                        x_flip = 1 << 10;
                    }
                    else if (result2 <= diff_upbound)
                    {
                        find_eqaul = true;
                        if (newtmpYFlipdata == FileIOUtils::find_less_feature_buff(newtmpYFlipdata, tile_data, 32))
                        {
                            reserved_tileid = old_tileid;
                        }
                        y_flip = 1 << 11;
                    }
                    else if (result3 <= diff_upbound)
                    {
                        find_eqaul = true;
                        if (newtmpXYFlipdata == FileIOUtils::find_less_feature_buff(newtmpXYFlipdata, tile_data, 32))
                        {
                            reserved_tileid = old_tileid;
                        }
                        x_flip = 1 << 10;
                        y_flip = 1 << 11;
                    }

                    if (find_eqaul)
                    {
                        // always replace the old_tileid instance with the find_tileid instance
                        for (int w = 0; w < tmpEntry.mappingData.size(); w++)
                        {
                            if ((tmpEntry.mappingData[w] & 0x3FF) == old_tileid)
                            {
                                int old_x_flip_state = tmpEntry.mappingData[w] & (1 << 10);
                                int old_y_flip_state = tmpEntry.mappingData[w] & (1 << 11);
                                mappingdata = 0xF << 12 |
                                             (y_flip ^ old_y_flip_state) |
                                             (x_flip ^ old_x_flip_state) |
                                             (find_tileid & 0x3FF);
                                tmpEntry.mappingData[w] = mappingdata;
                            }
                        }

                        // we need to change the tmp_current_tile8x8_data and tmpEntry.tileData
                        // if the old_tileid needs to be reserved
                        if (reserved_tileid == old_tileid)
                        {
                            memcpy(&tmp_current_tile8x8_data[32 * j], newtmpdata, 32);

                            int startid = tmpEntry.TileDataRAMOffsetNum;
                            for (int k = 0; k < 32; k++)
                            {
                                tmpEntry.tileData[32 * (find_tileid - startid) + k] = newtmpdata[k];
                            }
                        }

                        // delete the Tile8x8 from the tmpEntry's Tile8x8 set
                        DeltmpEntryTile(old_tileid);
                        break;
                    }
                }
            }

            delete[] tmp_current_tile8x8_data;

            // set tmpEntry if everything looks correct
            tmpEntry.TileDataAddress = 0;
            tmpEntry.MappingDataAddress = 0;
            tmpEntry.MappingDataSizeAfterCompressionInByte = 0;

            // UI reset
            CleanTilesInstances();
            GenerateBGTile8x8Instances(tmpEntry);
            UpdateTilesGraphicView(tmpEntry);
            SetTilesPanelInfoGUI(tmpEntry);

            // UI reset on panels
            UpdateMappingGraphicView(tmpEntry);
            SetMappingGraphicInfoGUI(tmpEntry);

            break;
        }
        case AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp:
        {
            // TODO
        }
    }
}

/// <summary>
/// disallow user to input ';' into the box
/// </summary>
void GraphicManagerDialog::on_lineEdit_tileDataName_textChanged(const QString &arg1)
{
    QString tmp = arg1;
    tmp.remove(';');
    ui->lineEdit_tileDataName->setText(tmp);
}

/// <summary>
/// disallow user to input ';' into the box
/// </summary>
void GraphicManagerDialog::on_lineEdit_mappingDataName_textChanged(const QString &arg1)
{
    QString tmp = arg1;
    tmp.remove(';');
    ui->lineEdit_mappingDataName->setText(tmp);
}


/// <summary>
/// Click the button to duplicate the selected graphic entry. The new entry is inserted right
/// below the source entry, so the source entry is the K-th entry and the new one is the (K+1)-th.
/// </summary>
void GraphicManagerDialog::on_pushButton_duplicateCurrentEntry_clicked()
{
    QItemSelectionModel *select = ui->listView_RecordGraphicsList->selectionModel();
    QModelIndexList selectedRows = select->selectedRows();
    if (selectedRows.size() != 1)
    {
        QMessageBox::information(this, tr("Error"), tr("Select one graphic entry before duplicating it."));
        return;
    }

    int sourceEntryID = selectedRows[0].row();

    // Use the data in the panels if the selected entry is the entry being edited, so the
    // not-yet-validated changes can be duplicated as well
    struct AssortedGraphicUtils::AssortedGraphicEntryItem newEntry;
    if (sourceEntryID == SelectedEntryID)
    {
        newEntry = tmpEntry;
    }
    else
    {
        newEntry = graphicEntries[sourceEntryID];
    }

    // Set a new mapping data name to distinguish the new entry from the source entry
    newEntry.MappingDataName = GenerateUniqueMappingDataName(newEntry.MappingDataName);

    // Detach all the 3 data parts from the ROM addresses they were loaded from, so the saving
    // logic writes them as new chunks of the duplicated entry. The duplicate then owns its Tile8x8
    // data, palette data and mapping data, so it can be edited without touching the data of the
    // source entry (and without modifying the vanilla ROM data the source entry references).
    newEntry.TileDataAddress = 0;
    newEntry.PaletteAddress = 0;
    newEntry.MappingDataAddress = 0;
    newEntry.MappingDataSizeAfterCompressionInByte = 0; // the save logic should set this

    // Insert the new entry right below the source entry, all the entries after the source
    // entry are shifted down by one position
    graphicEntries.insert(sourceEntryID + 1, newEntry);

    // Rebuild the entry list, then select the new entry and load it into the panels
    UpdateEntryList();
    SelectedEntryID = sourceEntryID + 1;
    tmpEntry = graphicEntries[SelectedEntryID];
    ExtractEntryToGUI(tmpEntry);
    ui->listView_RecordGraphicsList->setCurrentIndex(ListViewItemModel->index(SelectedEntryID, 0));
    ui->listView_RecordGraphicsList->scrollTo(ListViewItemModel->index(SelectedEntryID, 0));
}

/// <summary>
/// Click the button to swap 2 palette rows of the current entry, or to move one palette row to
/// another palette slot ID by swapping it with an unused palette row.
/// </summary>
void GraphicManagerDialog::on_pushButton_SwapPalettes_clicked()
{
    if (SelectedEntryID == -1)
    {
        return;
    }

    // Ask the user for the 2 palette rows to swap
    bool ok;
    QString input = QInputDialog::getText(this,
                                          tr("WL4Editor"),
                                          tr("Input the indexes of the 2 palette rows to swap:\n"
                                             "Use hexadecimal palette slot IDs without the \"0x\" prefix,\n"
                                             "and split the 2 row indexes with an English comma, such as \"3,5\".\n"
                                             "If only one of the 2 rows has a palette, that palette is moved to the\n"
                                             "other row, and its palette slot ID is reset to the other row."),
                                          QLineEdit::Normal, "", &ok);
    if (!ok)
    {
        return;
    }

    QVector<unsigned int> paletteRows;
    if (!ParseCommaSeparatedHexValues(input, paletteRows, 0xF, tr("palette row index")))
    {
        return;
    }
    if (paletteRows.size() != 2)
    {
        QMessageBox::warning(this, tr("Warning"), tr("2 palette rows have to be input to perform the swapping!"));
        return;
    }
    if (paletteRows[0] == paletteRows[1])
    {
        QMessageBox::warning(this, tr("Warning"), tr("The 2 palette rows to swap cannot be the same one!"));
        return;
    }

    unsigned int firstRow = paletteRows[0];
    unsigned int secondRow = paletteRows[1];
    bool firstRowUsed = tmpEntry.PaletteSlotIDs.contains(firstRow);
    bool secondRowUsed = tmpEntry.PaletteSlotIDs.contains(secondRow);

    if (firstRowUsed && secondRowUsed)
    {
        // Both of the 2 rows are used by the current entry, so swap their color tables
        QVector<QRgb> tmpPalette = tmpEntry.palettes[firstRow];
        tmpEntry.palettes[firstRow] = tmpEntry.palettes[secondRow];
        tmpEntry.palettes[secondRow] = tmpPalette;
    }
    else if (firstRowUsed || secondRowUsed)
    {
        // Only one of the 2 rows is used by the current entry, so move the existing palette to
        // the unused row, which resets the palette slot ID of the existing palette
        unsigned int usedRow = firstRowUsed ? firstRow : secondRow;
        unsigned int unusedRow = firstRowUsed ? secondRow : firstRow;
        tmpEntry.palettes[unusedRow] = tmpEntry.palettes[usedRow];
        ResetPaletteRowInTmpEntry(usedRow);
        for (int i = 0; i < tmpEntry.PaletteSlotIDs.size(); ++i)
        {
            if (tmpEntry.PaletteSlotIDs[i] == usedRow)
            {
                tmpEntry.PaletteSlotIDs[i] = unusedRow;
            }
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Warning"), tr("Neither of the 2 palette rows has a palette in the current entry, nothing to swap!"));
        return;
    }

    // Swap the palette IDs used by the mapping data as well, so each Tile8x8 keeps using the
    // same colors and the rendered graphic does not change
    bool mappingDataChanged = false;
    for (int i = 0; i < tmpEntry.mappingData.size(); ++i)
    {
        unsigned int paletteId = (tmpEntry.mappingData[i] >> 12) & 0xF;
        if (paletteId == firstRow)
        {
            tmpEntry.mappingData[i] = static_cast<unsigned short>((tmpEntry.mappingData[i] & 0x0FFF) | (secondRow << 12));
            mappingDataChanged = true;
        }
        else if (paletteId == secondRow)
        {
            tmpEntry.mappingData[i] = static_cast<unsigned short>((tmpEntry.mappingData[i] & 0x0FFF) | (firstRow << 12));
            mappingDataChanged = true;
        }
    }

    // The palettes are not the palette data at the old address any more, so they need to be
    // saved as the entry's own palette chunk
    tmpEntry.PaletteAddress = 0;

    // The mapping data of a vanilla ROM address is not saved by the entry, so the swapped
    // palette IDs would be lost. Detach the mapping data from the vanilla ROM address to keep
    // the rendered graphic unchanged after saving and loading again.
    if (mappingDataChanged && tmpEntry.MappingDataAddress
            && tmpEntry.MappingDataAddress < WL4Constants::AvailableSpaceBeginningInROM)
    {
        tmpEntry.MappingDataAddress = 0;
        tmpEntry.MappingDataSizeAfterCompressionInByte = 0; // the save logic should set this
    }

    // Keep the palette slot IDs sorted, the palette data is saved and loaded in this order
    std::sort(tmpEntry.PaletteSlotIDs.begin(), tmpEntry.PaletteSlotIDs.end());

    // UI reset
    CleanTilesInstances();
    GenerateBGTile8x8Instances(tmpEntry);
    UpdatePaletteGraphicView(tmpEntry);
    SetPaletteInfoGUI(tmpEntry);
    UpdateTilesGraphicView(tmpEntry);
    UpdateMappingGraphicView(tmpEntry);
    SetMappingGraphicInfoGUI(tmpEntry);
}

/// <summary>
/// Parse a list of hex numbers separated by English commas.
/// </summary>
/// <param name="text">
/// The text to be parsed.
/// </param>
/// <param name="result">
/// The parsed values.
/// </param>
/// <param name="maxValue">
/// The biggest value allowed.
/// </param>
/// <param name="valueName">
/// The name of the values, which is used in the error messages.
/// </param>
/// <returns>
/// Return true if all the values in the text are parsed successfully.
/// </returns>
bool GraphicManagerDialog::ParseCommaSeparatedHexValues(const QString &text, QVector<unsigned int> &result,
                                                        unsigned int maxValue, const QString &valueName)
{
    result.clear();
    QStringList valueStrings = text.split(",", Qt::SkipEmptyParts);
    for (const QString &s : valueStrings)
    {
        bool ok;
        unsigned int value = s.trimmed().toUInt(&ok, 16);
        if (!ok)
        {
            QMessageBox::warning(this, tr("Warning"), tr("A part of text cannot be converted to ") + valueName + tr(": ") + s);
            result.clear();
            return false;
        }
        else if (value > maxValue)
        {
            QMessageBox::warning(this, tr("Warning"),
                                 valueName + tr(" must be between 0 and ") + QString::number(maxValue) + tr(": ") + s);
            result.clear();
            return false;
        }
        else
        {
            result.push_back(value);
        }
    }
    return true;
}

/// <summary>
/// Generate a mapping data name which is different from all the mapping data names in use.
/// </summary>
/// <param name="baseName">
/// The original mapping data name.
/// </param>
/// <returns>
/// The new mapping data name.
/// </returns>
QString GraphicManagerDialog::GenerateUniqueMappingDataName(const QString &baseName)
{
    QString newName = baseName + " (copy)";
    for (int copyId = 2; ; ++copyId)
    {
        bool nameUsed = false;
        for (const auto &entry : graphicEntries)
        {
            if (entry.MappingDataName == newName)
            {
                nameUsed = true;
                break;
            }
        }
        if (!nameUsed)
        {
            break;
        }
        newName = baseName + " (copy " + QString::number(copyId) + ")";
    }
    return newName;
}

