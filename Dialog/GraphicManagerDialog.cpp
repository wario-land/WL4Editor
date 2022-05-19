#include "GraphicManagerDialog.h"
#include "ui_GraphicManagerDialog.h"
#include "WL4EditorWindow.h"

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
    testentry.TileDataType = ScatteredGraphicUtils::ScatteredGraphicTileDataType::Tileset_text_bg_4bpp_no_comp;
    testentry.TileDataName = "Tileset 0x11 bg tiles";
    testentry.MappingDataAddress = 0x5FA6D0;
    testentry.MappingDataSize_Byte = 0xC10; // unit: Byte
    testentry.MappingDataCompressType = ScatteredGraphicUtils::ScatteredGraphicMappingDataCompressionType::RLE16_with_sizeheader;
    testentry.MappingDataName = "Tileset 0x11 bg";
    testentry.PaletteAddress = 0x583C7C;
    testentry.PaletteNum = 16; // when (optionalPaletteAddress + PaletteNum) > 16, we just discard the latter palettes
    testentry.PaletteRAMOffsetNum = 0; // unit: per palette, 16 color
    testentry.optionalGraphicWidth = 0; // overwrite size params when the mapping data include size info
    testentry.optionalGraphicHeight = 0;
    graphicEntries.append(testentry);
    // -------------test code end-------------

    UpdateEntryList();
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
    // Cleanup then Load palette first
    int paletteAddress = entry.PaletteAddress;
    unsigned char *curFilePtr = ROMUtils::ROMFileMetadata->ROMDataPtr;
    for (int i = 0; i < 16; ++i)
    {
        entry.palettes[i].clear();
    }
    for (int i = entry.PaletteRAMOffsetNum; i < qMin((entry.PaletteRAMOffsetNum + entry.PaletteNum), (unsigned int)16); ++i)
    {
        int subPalettePtr = paletteAddress + i * 32;
        unsigned short *tmpptr = (unsigned short*) (curFilePtr + subPalettePtr);
        ROMUtils::LoadPalette(&(entry.palettes[i]), tmpptr);
    }
    for (int i = 0; i < 16; ++i)
    {
        if (!(entry.palettes[i].size()))
        {
            for (int j = 0; j < 16; ++j)
                entry.palettes[i].push_back(QColor(0, 0, 0, 0xFF).rgba());
        }
    }

    // Cleanup then Load Tiles
    if (entry.blankTile != nullptr)
    {
        for(int i = 0; i < entry.tile8x8array.size(); i++)
        {
            if (entry.tile8x8array[i] != entry.blankTile)
            {
                delete entry.tile8x8array[i];
            }
        }
        entry.tile8x8array.clear();
        delete entry.blankTile;
        entry.blankTile = nullptr;
    }
    switch (static_cast<int>(entry.TileDataType))
    {
        case ScatteredGraphicUtils::ScatteredGraphicTileDataType::Tileset_text_bg_4bpp_no_comp:
        {
            entry.blankTile = LevelComponents::Tile8x8::CreateBlankTile(entry.palettes);
            for (int i = 0; i < entry.TileDataRAMOffsetNum; ++i)
            {
                entry.tile8x8array.push_back(entry.blankTile);
            }
            int GFXcount = entry.TileDataSize_Byte / 32;
            for (int i = 0; i < GFXcount; ++i)
            {
                entry.tile8x8array.push_back(new LevelComponents::Tile8x8(entry.TileDataAddress + i * 32, entry.palettes));
            }
            entry.tile8x8array.push_back(entry.blankTile);
            break;
        }
    }

    // Load mapping data
    entry.mappingData.clear();
    switch (static_cast<int>(entry.MappingDataCompressType))
    {
        case ScatteredGraphicUtils::ScatteredGraphicMappingDataCompressionType::No_mapping_data_comp:
        { // this case never work atm
            for (int i = 0; i < entry.optionalGraphicWidth * entry.optionalGraphicHeight; ++i)
            {
                unsigned short *data = (unsigned short *)(ROMUtils::ROMFileMetadata->ROMDataPtr + entry.MappingDataAddress);
                entry.mappingData.push_back(data[i]);
            }
            break;
        }
        case ScatteredGraphicUtils::ScatteredGraphicMappingDataCompressionType::RLE16_with_sizeheader:
        {
            LevelComponents::Layer BGlayer(entry.MappingDataAddress, LevelComponents::LayerTile8x8);
            entry.optionalGraphicHeight = BGlayer.GetLayerHeight();
            entry.optionalGraphicWidth = BGlayer.GetLayerWidth();
            unsigned short *layerdata = BGlayer.GetLayerData();
            for (int i = 0; i < entry.optionalGraphicWidth * entry.optionalGraphicHeight; ++i)
            {
                entry.mappingData.push_back(layerdata[i]);
            }
            break;
        }
    }

    // UI Reset
    ui->lineEdit_tileDataAddress->setText(QString::number(entry.TileDataAddress, 16));
    ui->lineEdit_tileDataSize_Byte->setText(QString::number(entry.TileDataSize_Byte, 16));
    ui->lineEdit_tileDataRAMoffset->setText(QString::number(entry.TileDataRAMOffsetNum, 16));
    ui->comboBox_tileDataType->setCurrentIndex(entry.TileDataType);
    ui->lineEdit_tileDataName->setText(entry.TileDataName);
    ui->lineEdit_mappingDataAddress->setText(QString::number(entry.MappingDataAddress, 16));
    ui->lineEdit_mappingDataSize_Byte->setText(QString::number(entry.MappingDataSize_Byte, 16));
    ui->comboBox_mappingDataType->setCurrentIndex(entry.MappingDataCompressType);
    ui->lineEdit_mappingDataName->setText(entry.MappingDataName);
    ui->lineEdit_optionalGraphicHeight->setText(QString::number(entry.optionalGraphicHeight, 16));
    ui->lineEdit_optionalGraphicWidth->setText(QString::number(entry.optionalGraphicWidth, 16));
    ui->lineEdit_paletteAddress->setText(QString::number(entry.PaletteAddress, 16));
    ui->lineEdit_paletteNum->setText(QString::number(entry.PaletteNum, 16));
    ui->lineEdit_paletteRAMOffset->setText(QString::number(entry.PaletteRAMOffsetNum, 16));

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
        case ScatteredGraphicUtils::ScatteredGraphicTileDataType::Tileset_text_bg_4bpp_no_comp:
        {
            int lineNum = entry.tile8x8array.size() / 16;
            if ((lineNum * 16) < entry.tile8x8array.size())
            {
                lineNum += 1;
            }
            QPixmap pixmap(8 * 16, 8 * lineNum);
            pixmap.fill(Qt::transparent);

            // find a palette has non-black color(s) in it
            int paletteId = -1;
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

            // drawing
            for (int i = 0; i < lineNum; ++i)
            {
                for (int j = 0; j < 16; ++j)
                {
                    if (entry.tile8x8array.size() <= (i * 16 + j))
                    {
                        entry.blankTile->DrawTile(&pixmap, j * 8, i * 8);
                        continue;
                    }
                    if (entry.tile8x8array[i * 16 + j] == entry.blankTile) continue;
                    entry.tile8x8array[i * 16 + j]->SetPaletteIndex(paletteId);
                    entry.tile8x8array[i * 16 + j]->DrawTile(&pixmap, j * 8, i * 8);
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
        case ScatteredGraphicUtils::ScatteredGraphicMappingDataCompressionType::RLE16_with_sizeheader:
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
                LevelComponents::Tile8x8 *newTile = new LevelComponents::Tile8x8(entry.tile8x8array[(tileData & 0x3FF)]);
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
    ui->graphicsView_palettes->scale(2, 2);
    scene->addPixmap(RenderAllPalette(entry));
    ui->graphicsView_palettes->verticalScrollBar()->setValue(0);
}

/// <summary>
/// Update Tiles graphicview.
/// </summary>
void GraphicManagerDialog::UpdateTilesGraphicView(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
{
    int linenum = entry.tile8x8array.size() / 16;
    if ((linenum * 16) < entry.tile8x8array.size())
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
    ui->graphicsView_tile8x8setData->scale(2, 2);
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
    ui->graphicsView_mappingGraphic->scale(1, 1);
    scene->addPixmap(pixmap);
    ui->graphicsView_mappingGraphic->verticalScrollBar()->setValue(0);
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
    int linenum = index.row();
    ExtractEntryToGUI(graphicEntries[linenum]);

    ui->listView_RecordGraphicsList->setEnabled(true);
}

