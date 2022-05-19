#include "GraphicManagerDialog.h"
#include "ui_GraphicManagerDialog.h"
#include "WL4EditorWindow.h"

extern WL4EditorWindow *singleton;

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
    ui->setupUi(this);

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
    testentry.TileDataCompressType = ScatteredGraphicUtils::ScatteredGraphicTileDataType::Tileset_text_bg_4bpp_no_comp;
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
    // Load palette first
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

    // Load Tiles
    switch (static_cast<int>(entry.TileDataCompressType))
    {
        case ScatteredGraphicUtils::ScatteredGraphicTileDataType::Tileset_text_bg_4bpp_no_comp:
        {
            entry.blankTile = LevelComponents::Tile8x8::CreateBlankTile(entry.palettes);
            for (int i = 0; i < entry.TileDataRAMOffsetNum; ++i)
            {
                entry.tile8x8array.push_back(entry.blankTile);
            }
            break;
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
    int graphicheight = entry.optionalGraphicHeight;
    int graphicwidth = entry.optionalGraphicWidth;
    switch (static_cast<int>(entry.MappingDataCompressType))
    {
        case ScatteredGraphicUtils::ScatteredGraphicMappingDataCompressionType::No_mapping_data_comp:
        { // this case never work atm
            for (int i = 0; i < graphicwidth * graphicheight; ++i)
            {
                unsigned short *data = (unsigned short *)(ROMUtils::ROMFileMetadata->ROMDataPtr + entry.MappingDataAddress);
                entry.mappingData.push_back(data[i]);
            }
            break;
        }
        case ScatteredGraphicUtils::ScatteredGraphicMappingDataCompressionType::RLE16_with_sizeheader:
        {
            LevelComponents::Layer BGlayer(entry.MappingDataAddress, LevelComponents::LayerTile8x8);
            graphicheight = BGlayer.GetLayerHeight();
            graphicwidth = BGlayer.GetLayerWidth();
            unsigned short *layerdata = BGlayer.GetLayerData();
            for (int i = 0; i < graphicwidth * graphicheight; ++i)
            {
                entry.mappingData.push_back(layerdata[i]);
            }
            break;
        }
    }

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
QPixmap GraphicManagerDialog::GetPixmap(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry)
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

