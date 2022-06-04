#include "GraphicManagerDialog.h"
#include "ui_GraphicManagerDialog.h"

#include <QMessageBox>

#include "WL4EditorWindow.h"
#include "WL4Constants.h"
#include "FileIOUtils.h"

extern WL4EditorWindow *singleton;

// constexpr declarations for the initializers in the header
constexpr const char *GraphicManagerDialog::ScatteredGraphicTileDataTypeNameData[1];
constexpr const char *GraphicManagerDialog::ScatteredGraphicMappingDataCompressionTypeNameData[2];

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
    graphicEntries = ScatteredGraphicUtils::GetScatteredGraphicsFromROM();

    // -------------test code-----------------
    struct ScatteredGraphicUtils::ScatteredGraphicEntryItem testentry;
    testentry.TileDataAddress = 0x4E851C;
    testentry.TileDataSize_Byte = 9376; // unit: Byte
    testentry.TileDataRAMOffsetNum = 0x4DA - 0x200; // unit: per Tile8x8
    testentry.TileDataType = ScatteredGraphicUtils::ScatteredGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg;
    testentry.TileDataName = "Tileset 0x11 bg tiles";
    testentry.MappingDataAddress = 0x5FA6D0;
    testentry.MappingDataSize_Byte = 0xC10; // unit: Byte
    testentry.MappingDataCompressType = ScatteredGraphicUtils::ScatteredGraphicMappingDataCompressionType::RLE_mappingtype_0x20;
    testentry.MappingDataName = "Tileset 0x11 bg";
    testentry.PaletteAddress = 0x583C7C;
    testentry.PaletteNum = 16; // when (optionalPaletteAddress + PaletteNum) > 16, we just discard the latter palettes
    testentry.PaletteRAMOffsetNum = 0; // unit: per palette, 16 color
    testentry.optionalGraphicWidth = 0; // overwrite size params when the mapping data include size info
    testentry.optionalGraphicHeight = 0;

    ScatteredGraphicUtils::ExtractDataFromEntryInfo_v1(testentry);
    graphicEntries.append(testentry);
    // -------------test code end-------------

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
         i < sizeof(ScatteredGraphicTileDataTypeNameData) / sizeof(ScatteredGraphicTileDataTypeNameData[0]); ++i)
    {
        GraphicTileDataTypeName << ScatteredGraphicTileDataTypeNameData[i];
    }
    for (unsigned int i = 0;
         i < sizeof(ScatteredGraphicMappingDataCompressionTypeNameData) / sizeof(ScatteredGraphicMappingDataCompressionTypeNameData[0]); ++i)
    {
        GraphicMappingDataCompressionTypeName << ScatteredGraphicMappingDataCompressionTypeNameData[i];
    }
}

/// <summary>
/// Update Graphic Entry list accroding to variable "graphicEntries".
/// </summary>
/// <returns>
/// If the number of entry is 0, return false.
/// </returns>
bool GraphicManagerDialog::UpdateEntryList()
{
    if (graphicEntries.size())
    {
        // cleanup old listview ui
        if (ListViewItemModel)
        {
            ListViewItemModel->clear();
            delete ListViewItemModel;
            ListViewItemModel = nullptr;
        }

        // create new listview ui data
        ListViewItemModel = new QStandardItemModel(this);
        for(auto &entry: graphicEntries)
        {
            QString text = GenerateEntryTextFromStruct(entry);
            QStandardItem *item = new QStandardItem(text);
            ListViewItemModel->appendRow(item);
        }
        ui->listView_RecordGraphicsList->setModel(ListViewItemModel);
        return true;
    }
    return false;
}

/// <summary>
/// Extract entry info into GUI.
/// </summary>
/// <param name="entry">
/// The struct data used to extract info of graphic.
/// </param>
void GraphicManagerDialog::ExtractEntryToGUI(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
{
    // Cleanup then Load Tiles
    CleanTilesInstances();
    switch (static_cast<int>(entry.TileDataType))
    {
        case ScatteredGraphicUtils::ScatteredGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg:
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
QPixmap GraphicManagerDialog::RenderAllPalette(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
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
QPixmap GraphicManagerDialog::RenderAllTiles(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
{
    switch (static_cast<int>(entry.TileDataType))
    {
        case ScatteredGraphicUtils::ScatteredGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg:
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
//                paletteId = tmpEntry.PaletteRAMOffsetNum;
                paletteId = 0xF;
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
QPixmap GraphicManagerDialog::RenderGraphic(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
{
    int graphicheight = entry.optionalGraphicHeight;
    int graphicwidth = entry.optionalGraphicWidth;
    switch (static_cast<int>(entry.MappingDataCompressType))
    {
        case ScatteredGraphicUtils::ScatteredGraphicMappingDataCompressionType::RLE_mappingtype_0x20:
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
void GraphicManagerDialog::UpdatePaletteGraphicView(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
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
void GraphicManagerDialog::UpdateTilesGraphicView(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
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
void GraphicManagerDialog::UpdateMappingGraphicView(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
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
    ui->lineEdit_tileDataSize_Byte->setText("");
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
    ui->lineEdit_mappingDataSize_Byte->setText("");
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
void GraphicManagerDialog::SetPaletteInfoGUI(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
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
void GraphicManagerDialog::SetTilesPanelInfoGUI(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
{
    ui->lineEdit_tileDataAddress->setText(QString::number(entry.TileDataAddress, 16));
    ui->lineEdit_tileDataSize_Byte->setText(QString::number(entry.TileDataSize_Byte, 16));
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
void GraphicManagerDialog::SetMappingGraphicInfoGUI(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
{
    ui->lineEdit_mappingDataAddress->setText(QString::number(entry.MappingDataAddress, 16));
    ui->lineEdit_mappingDataSize_Byte->setText(QString::number(entry.MappingDataSize_Byte, 16));
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
void GraphicManagerDialog::GenerateBGTile8x8Instances(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
{
    tmpblankTile = LevelComponents::Tile8x8::CreateBlankTile(entry.palettes);
    for (int i = 0; i < entry.TileDataRAMOffsetNum; ++i)
    {
        tmpTile8x8array.push_back(tmpblankTile);
    }
    int GFXcount = entry.TileDataSize_Byte / 32;
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
/// Generate the text of one entry according to a provided struct, to show in the listview.
/// </summary>
/// <param name="entry">
/// The struct data used to generate entry text.
/// </param>
/// <returns>
/// The text of the entry to show in listview.
/// </returns>
QString GraphicManagerDialog::GenerateEntryTextFromStruct(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
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
            // TODO
            QMessageBox::critical(this, tr("Error"), tr("Import palettes from current ROM cannot work yet!"));
            return;
        }

        // UI reset
        UpdatePaletteGraphicView(tmpEntry);
        SetPaletteInfoGUI(tmpEntry);
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
                case ScatteredGraphicUtils::ScatteredGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg:
                {
                    // Ignore the settings from the UI, import tile data directly and see if the data is legal
                    FileIOUtils::ImportTile8x8GfxData(this,
                        tmpEntry.palettes[15], // use the last palette for palette comparison
                        tr("Choose a color to covert to transparent:"),
                        [this] (QByteArray finaldata, QWidget *parentPtr)
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
                                this->tmpEntry.TileDataSize_Byte = finaldata.size();
                                this->tmpEntry.TileDataAddress = 0;
                                this->tmpEntry.TileDataType = ScatteredGraphicUtils::Tile8x8_4bpp_no_comp_Tileset_text_bg;
                            }
                        });
                    tmpEntry.TileDataName = ui->lineEdit_tileDataName->text();

                    // UI reset
                    CleanTilesInstances();
                    GenerateBGTile8x8Instances(tmpEntry);
                    UpdateTilesGraphicView(tmpEntry);
                    SetTilesPanelInfoGUI(tmpEntry);
                }
                break;
            }

        }
        else // we need to import tile data from the current ROM directly
        {
            switch (tiledatatype)
            {
                case ScatteredGraphicUtils::ScatteredGraphicTileDataType::Tile8x8_4bpp_no_comp_Tileset_text_bg:
                {
                    // TODO
//                    int tiledataSize_byte = ui->lineEdit_tileDataSize_Byte->text().toUInt(nullptr, 16);
//                    int tileVRAMoffsetNum = ui->lineEdit_tileDataRAMoffset->text().toUInt(nullptr, 16);
//                    int tile8x8Num = tiledataSize_byte / 32;
//                    if ((tile8x8Num << 5) != tiledataSize_byte)
//                    {
//                        QMessageBox::critical(this, tr("Error"), tr("Illegal tile data size, size has to be multiple of 0x20!"));
//                    }
//                    if ((tile8x8Num + tileVRAMoffsetNum) > 0x3FF)
//                    {
//                        QMessageBox::critical(this, tr("Error"), tr("Tile8x8 index(es) out of bound!\n"
//                                                                    "The last tile8x8 has to be indexed 0x3FE"));
//                    }
                }
                break;
            }
            QMessageBox::critical(this, tr("Error"), tr("Import tiles from current ROM cannot work yet!"));
        }
    }
}

