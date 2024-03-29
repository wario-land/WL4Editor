﻿#include "GraphicManagerDialog.h"
#include "ui_GraphicManagerDialog.h"

#include <QMessageBox>
#include <QModelIndex>

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
    testentry.MappingDataName = "Layer 3";
    testentry.PaletteAddress = 0x583C7C;
    testentry.PaletteNum = 16; // when (optionalPaletteAddress + PaletteNum) > 16, we just discard the latter palettes
    testentry.PaletteRAMOffsetNum = 0; // unit: per palette, 16 color
    testentry.optionalGraphicWidth = 0; // overwrite size params when the mapping data include size info
    testentry.optionalGraphicHeight = 0;

    AssortedGraphicUtils::ExtractDataFromEntryInfo_v1(testentry);
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
            if (tmpEntry.PaletteNum)
            {
                paletteId = tmpEntry.PaletteRAMOffsetNum;
//                paletteId = 0xF;
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
    ui->lineEdit_paletteRAMOffset->setText("");
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
    ui->lineEdit_paletteNum->setText(QString::number(entry.PaletteNum, 16));
    ui->lineEdit_paletteRAMOffset->setText(QString::number(entry.PaletteRAMOffsetNum, 16));
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
    for (int i = 0; i < 16; ++i)
    {
        if (tmpEntry.palettes[i].size())
        {
            tmpEntry.palettes[i].clear();
        }
        for (int j = 0; j < 16; ++j)
            tmpEntry.palettes[i].push_back(QColor(0, 0, 0, 0xFF).rgba());
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
            // don't generate entry for mappingtype0x20 Layer0 atm, we need a new graphic tile type for them
/*
            if ((header.Layer0MappingType & 0x30) == 0x20)
            {
                unsigned int cur_entry_num = graphicEntries.size();
                bool dontadd = false;
                if (cur_entry_num)
                {
                    for (int n = 0; n < cur_entry_num; n++)
                    {
                        if (graphicEntries[n].MappingDataAddress == (header.Layer0Data & 0x7FFFFFF))
                        {
                            dontadd = true;
                            break;
                        }
                    }
                }

                if (!dontadd)
                {
                    // not found, so we add new entry
                    struct AssortedGraphicUtils::AssortedGraphicEntryItem newentry;
                    LevelComponents::Tileset *roomtileset = ROMUtils::singletonTilesets[header.TilesetID];
                    newentry.TileDataAddress = roomtileset->GetbgGFXptr();
                    newentry.TileDataSizeInByte = roomtileset->GetbgGFXlen();
                    int tilenum = newentry.TileDataSizeInByte / 32;
                    newentry.TileDataRAMOffsetNum = 0x3FF - tilenum;
                    newentry.TileDataType = AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg;
                    newentry.TileDataName = "vanilla Tileset 0x"+ QString::number(header.TilesetID, 16) +" bg tiles";
                    newentry.MappingDataAddress = header.Layer0Data & 0x7FF'FFFF;
                    newentry.MappingDataSizeAfterCompressionInByte = 0x1000; // a big number (0x40 x 0x40), since it won't cause problems for vanilla data
                    newentry.MappingDataCompressType = AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType::RLE_mappingtype_0x20;
                    newentry.MappingDataName = "Layer 0 found in: " + QString::number(levelid_array[i]) + "-" +
                                                QString::number(roomid_array[i]) + "-" + QString::number(j);
                    newentry.PaletteAddress = roomtileset->GetPaletteAddr();
                    newentry.PaletteNum = 16;
                    newentry.PaletteRAMOffsetNum = 0; // unit: per palette, 16 color
                    newentry.optionalGraphicWidth = 0; // overwrite size params when the mapping data include size info
                    newentry.optionalGraphicHeight = 0;

                    AssortedGraphicUtils::ExtractDataFromEntryInfo_v1(newentry);

                    // reset a part of palette settings for bg graphic entries
                    int usingpal = 15;
                    for (int m = 0; m < newentry.mappingData.size(); m++)
                    {
                        int tileid = (newentry.mappingData[m] & 0x3FF);
                        if (tileid != 0x3FF)
                        {
                            usingpal = (newentry.mappingData[m] & 0xF000) >> 12;
                            newentry.PaletteAddress += usingpal * 32;
                            newentry.PaletteNum = 1;
                            newentry.PaletteRAMOffsetNum = usingpal;
                            break;
                        }
                    }
                    for (int i = 0; i < 16; ++i)
                    {
                        if (i != usingpal)
                        {
                            newentry.palettes[i].clear();
                            for (int j = 0; j < 16; ++j) // (re-)initialization
                            {
                                newentry.palettes[i].push_back(QColor(0, 0, 0, 0xFF).rgba());
                            }
                        }
                    }
                    graphicEntries.append(newentry);
                }
            }
*/
            if ((header.Layer3MappingType & 0x30) == 0x20)
            {
                unsigned int cur_entry_num = graphicEntries.size();
                bool dontadd = false;
                if (cur_entry_num)
                {
                    for (int n = 0; n < cur_entry_num; n++)
                    {
                        if (graphicEntries[n].MappingDataAddress == (header.Layer3Data & 0x7FFFFFF))
                        {
                            dontadd = true;
                            break;
                        }
                    }
                }

                if (!dontadd)
                {
                    // not found, so we add new entry
                    struct AssortedGraphicUtils::AssortedGraphicEntryItem newentry;
                    LevelComponents::Tileset *roomtileset = ROMUtils::singletonTilesets[header.TilesetID];
                    newentry.TileDataAddress = roomtileset->GetbgGFXptr();
                    newentry.TileDataSizeInByte = roomtileset->GetbgGFXlen();
                    int tilenum = newentry.TileDataSizeInByte / 32;
                    newentry.TileDataRAMOffsetNum = 0x3FF - tilenum;
                    newentry.TileDataType = AssortedGraphicUtils::AssortedGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg;
                    newentry.TileDataName = "vanilla Tileset 0x"+ QString::number(header.TilesetID, 16) +" bg tiles";
                    newentry.MappingDataAddress = header.Layer3Data & 0x7FF'FFFF;
                    newentry.MappingDataSizeAfterCompressionInByte = 0x1000; // a big number (0x40 x 0x40), since it won't cause problems for vanilla data
                    newentry.MappingDataCompressType = AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType::RLE_mappingtype_0x20;
                    newentry.MappingDataName = "Layer 3 found in: " + QString::number(levelid_array[i]) + "-" +
                                                QString::number(roomid_array[i]) + "-" + QString::number(j);
                    newentry.PaletteAddress = roomtileset->GetPaletteAddr();
                    newentry.PaletteNum = 16;
                    newentry.PaletteRAMOffsetNum = 0; // unit: per palette, 16 color
                    newentry.optionalGraphicWidth = 0; // overwrite size params when the mapping data include size info
                    newentry.optionalGraphicHeight = 0;

                    AssortedGraphicUtils::ExtractDataFromEntryInfo_v1(newentry);

                    // reset a part of palette settings for bg graphic entries
                    int usingpal = 15;
                    int tmpusingpal = -1;
                    int palnum = 1;
                    for (int m = 0; m < newentry.mappingData.size(); m++)
                    {
                        int tileid = (newentry.mappingData[m] & 0x3FF);
                        if (tileid != 0x3FF)
                        {
                            usingpal = (newentry.mappingData[m] & 0xF000) >> 12;
                            if (tmpusingpal == -1)
                            {
                                tmpusingpal = usingpal;
                            }
                            else if (tmpusingpal != -1)
                            {
                                if (tmpusingpal == (usingpal - 1))
                                {
                                    palnum = 2;
                                    usingpal = tmpusingpal;
                                    break;
                                }
                                else if ((tmpusingpal - 1) == usingpal)
                                {
                                    palnum = 2;
                                    break;
                                }
                            }
                        }
                    }
                    newentry.PaletteAddress += usingpal * 32;
                    newentry.PaletteNum = palnum;
                    newentry.PaletteRAMOffsetNum = usingpal;
                    for (int i = 0; i < 16; ++i)
                    {
                        if (palnum == 1 ? (i != usingpal) : ((i != usingpal && (i != (usingpal + 1)))))
                        {
                            newentry.palettes[i].clear();
                            for (int j = 0; j < 16; ++j) // (re-)initialization
                            {
                                newentry.palettes[i].push_back(QColor(0, 0, 0, 0xFF).rgba());
                            }
                        }
                    }
                    graphicEntries.append(newentry);
                }

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
    }

    ui->listView_RecordGraphicsList->setEnabled(true);
}

/// <summary>
/// Click the button to clear Tile panel UI stuff
/// </summary>
void GraphicManagerDialog::on_pushButton_ClearTile8x8Data_clicked()
{
    ClearTilesPanel();
    CleanTilesInstances();
}

/// <summary>
/// Click the button to clear palette panel UI stuff
/// </summary>
void GraphicManagerDialog::on_pushButton_ClearPaletteData_clicked()
{
    ClearPalettePanel();
}

/// <summary>
/// Click the button to clear mapping panel UI stuff
/// </summary>
void GraphicManagerDialog::on_pushButton_ClearMappingData_clicked()
{
    ClearMappingPanel();
}

/// <summary>
/// Click the button to load palette from the ROM or from pal file
/// </summary>
void GraphicManagerDialog::on_pushButton_ImportPaletteData_clicked()
{
    if (SelectedEntryID != -1)
    {
        ClearAndResettmpEntryPalettes();

        // try to use the settings from the UI to import palette
        int palAddress = ui->lineEdit_paletteAddress->text().toUInt(nullptr, 16);
        int palnum = ui->lineEdit_paletteNum->text().toUInt(nullptr, 16);
        int paloffset = ui->lineEdit_paletteRAMOffset->text().toUInt(nullptr, 16);

        // palAddress is not a vanilla rom address, so we need to import palette from file
        if (!palAddress || palAddress >= WL4Constants::AvailableSpaceBeginningInROM)
        {
            if (palnum == 1) // we only import one 16-color palette
            {
                FileIOUtils::ImportPalette(this,
                    [this] (int selectedPalId, int colorId, QRgb newColor)
                    {
                        this->tmpEntry.SetColor(selectedPalId, colorId, newColor);
                    },
                    paloffset);

                // set tmpEntry if everything looks correct
                tmpEntry.PaletteAddress = 0;
                tmpEntry.PaletteNum = palnum;
                tmpEntry.PaletteRAMOffsetNum = paloffset;
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
            if ((palnum + paloffset) > 16)
            {
                QMessageBox::critical(this, tr("Error"), tr("The value of palnum + paloffset goes out of bound!"));
                return;
            }
            if (palnum < 0)
            {
                QMessageBox::critical(this, tr("Error"), tr("Illegal palnum!"));
                return;
            }
            if (paloffset < 0)
            {
                QMessageBox::critical(this, tr("Error"), tr("Illegal palnum!"));
                return;
            }

            // Load palette from the ROM
            for (int i = 0; i < palnum; ++i)
            {
                if (tmpEntry.palettes[i + paloffset].size())
                    tmpEntry.palettes[i + paloffset].clear();
                // First color is transparent
                ROMUtils::LoadPalette(&(tmpEntry.palettes[i + paloffset]), (unsigned short *) (ROMUtils::ROMFileMetadata->ROMDataPtr + palAddress + i * 32));
            }

            // set tmpEntry if everything looks correct
            tmpEntry.PaletteAddress = palAddress;
            tmpEntry.PaletteNum = palnum;
            tmpEntry.PaletteRAMOffsetNum = paloffset;
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
                            if(newtilenum > 0x3FE)
                            {
                                QMessageBox::critical(parentPtr, tr("Load Error"), tr("You can only use 0x3FF background tiles at most!"));
                                return;
                            }
                            else
                            {
                                this->tmpEntry.tileData.resize(finaldata.size());
                                for(int i = 0; i < finaldata.size(); ++i)
                                {
                                    this->tmpEntry.tileData[i] = finaldata[i];
                                }

                                // set tmpEntry if everything looks correct
                                int startid = 0x3FF - newtilenum;
                                this->tmpEntry.TileDataRAMOffsetNum = startid;
                                this->tmpEntry.TileDataSizeInByte = finaldata.size();
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
                                                                              "it has to be 0x40 or 0x40!"));
                        return;
                    }
                    if (optionalgraphicHeight != 0x20 && optionalgraphicHeight != 0x40)
                    {
                        QMessageBox::critical(this, tr("Load Error"), tr("Wrong graphic Height to import graphic for RLE_mappingtype_0x20,\n"
                                                                              "it has to be 0x40 or 0x40!"));
                        return;
                    }
                    if (optionalgraphicWidth == 0x20 && optionalgraphicHeight == 0x40)
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
                                    memcpy(&tmp_current_tile8x8_data[(j - startId) * 32], tile8x8array[j]->CreateGraphicsData().data(), 32);
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

