#include "WallPaintEditorDialog.h"
#include "ui_WallPaintEditorDialog.h"

#include <QScrollBar>
#include <QGraphicsView>

#include "WL4Constants.h"
#include "ROMUtils.h"
#include "FileIOUtils.h"

#include "LevelComponents/Tile.h"

/// <summary>
/// Construct the instance of the WallPaintEditorDialog.
/// Extract all the palettes and graphic data from ROM to the temp u8 array saved in the dialog.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
WallPaintEditorDialog::WallPaintEditorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WallPaintEditorDialog)
{
    ui->setupUi(this);

    // init data
    memcpy(gfxdata, &(ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintGFXAddr]), sizeof(gfxdata));
    memcpy(pal_passage_color, &(ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageColor]), sizeof(pal_passage_color));
    memcpy(pal_passage_gray, &(ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageGray]), sizeof(pal_passage_gray));
    memset(pal_startlevel_color, 0, sizeof(pal_startlevel_color));
    for (int passage = 0; passage < 6; passage++)
    {
        for (int level = 0; level < 4; level++)
        {
            unsigned int startlevel_paladdr = GetGradPalStartAddr(passage, level);
            if (!startlevel_paladdr) continue;
            memcpy(pal_startlevel_color + passage * 4 * (32 * 8) + level * (32 * 8), &(ROMUtils::ROMFileMetadata->ROMDataPtr[startlevel_paladdr]), 32 * 8);
        }
    }

    // init UI
    ui->graphicsView_Palette_passage_color->scale(2, 2);
    ui->graphicsView_Palette_startlevel_color->scale(2, 2);
    ui->graphicsView_Palette_passage_gray->scale(2, 2);
    ui->graphicsView_WallPaintGraphic->scale(2, 2);

    // render graphics
    RenderCurrentWallPaintAndPalette();
}

/// <summary>
/// Deconstruct the instance of the WallPaintEditorDialog.
/// </summary>
WallPaintEditorDialog::~WallPaintEditorDialog()
{
    delete ui;
}

/// <summary>
/// call this after closing the dialog.
/// </summary>
void WallPaintEditorDialog::AcceptChanges()
{
    // save data to the rom instance
    memcpy(&(ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintGFXAddr]), gfxdata, sizeof(gfxdata));
    memcpy(&(ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageColor]), pal_passage_color, sizeof(pal_passage_color));
    memcpy(&(ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageGray]), pal_passage_gray, sizeof(pal_passage_gray));
    memset(pal_startlevel_color, 0, sizeof(pal_startlevel_color));
    for (int passage = 0; passage < 6; passage++)
    {
        for (int level = 0; level < 4; level++)
        {
            unsigned int startlevel_paladdr = GetGradPalStartAddr(passage, level);
            if (!startlevel_paladdr) continue;
            memcpy(&(ROMUtils::ROMFileMetadata->ROMDataPtr[startlevel_paladdr]), pal_startlevel_color + passage * 4 * (32 * 8) + level * (32 * 8), 32 * 8);
        }
    }
}

/// <summary>
/// Render all the palettes and graphic according to the temp u8 array saved in the dialog.
/// </summary>
void WallPaintEditorDialog::RenderCurrentWallPaintAndPalette()
{
    // get passage id and local level id, then calculate offset
    int passageid = ui->spinBox_PassageID->value();
    if (passageid < 0) passageid = 0;
    int locallevelid = ui->spinBox_LocalLevelID->value();
    if (locallevelid < 0) locallevelid = 0;
    int palette_offset = 32 * 5 * passageid + 32 * locallevelid;
    int usedpal = ui->spinBox_CurrentPalette->value();
    if (usedpal < 0) usedpal = 0;

    // draw palette pixmap
    QVector<QRgb> palettes[2 + 8];
    unsigned short *tmpptr;
    QPixmap PaletteBarpixmap(8 * 16, 8);
    PaletteBarpixmap.fill(Qt::transparent);
    QPainter PaletteBarPainter(&PaletteBarpixmap);
    QGraphicsScene *scene;
    unsigned char* ptrarray[2] = {pal_passage_color, pal_passage_gray};
    QGraphicsView* viewarray[2] = {ui->graphicsView_Palette_passage_color,
                                   ui->graphicsView_Palette_passage_gray};
    for (int j = 0; j < 2; j++)
    {
        tmpptr = (unsigned short*) (ptrarray[j] + palette_offset);
        ROMUtils::LoadPalette(&(palettes[j]), tmpptr, true);
        PaletteBarpixmap.fill(Qt::transparent);
        for (int i = 0; i < 16; ++i) // Ignore the first color
        {
            PaletteBarPainter.fillRect(8 * i, 0, 8, 8, palettes[j][i]);
        }

        if (viewarray[j]->scene())
        {
            delete viewarray[j]->scene();
        }
        scene = new QGraphicsScene(0, 0, 16 * 8, 8);
        viewarray[j]->setScene(scene);
        viewarray[j]->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        scene->addPixmap(PaletteBarpixmap);
        viewarray[j]->verticalScrollBar()->setValue(0);
    }

    // load and render start level palettes for regular levels
    unsigned int startlevel_paladdr = GetGradPalStartAddr(passageid, locallevelid);
    if (ui->graphicsView_Palette_startlevel_color->scene())
    {
        delete ui->graphicsView_Palette_startlevel_color->scene();
        ui->graphicsView_Palette_startlevel_color->setScene(nullptr);
    }
    if (startlevel_paladdr && locallevelid < 4)
    {
        QPixmap Palettespixmap(8 * 16, 8 * 8);
        Palettespixmap.fill(Qt::transparent);
        QPainter PalettesPainter(&Palettespixmap);
        int grad_palette_offset = (32 * 8) * 4 * passageid + (32 * 8) * locallevelid;
        for (int j = 0; j < 8; j++)
        {
            tmpptr = (unsigned short*) (pal_startlevel_color + grad_palette_offset + j * 32);
            ROMUtils::LoadPalette(&(palettes[j + 2]), tmpptr, true);
            for (int i = 0; i < 16; ++i) // Ignore the first color
            {
                PalettesPainter.fillRect(8 * i, 8 * j, 8, 8, palettes[j + 2][i]);
            }
        }
        scene = new QGraphicsScene(0, 0, 16 * 8, 8 * 8);
        ui->graphicsView_Palette_startlevel_color->setScene(scene);
        ui->graphicsView_Palette_startlevel_color->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        scene->addPixmap(Palettespixmap);
        ui->graphicsView_Palette_startlevel_color->verticalScrollBar()->setValue(0);

        // selected palette max value changes
        ui->spinBox_CurrentPalette->setMaximum(9);
    }
    else
    {
        // selected palette max value changes
        if (usedpal > 1)
        {
            usedpal = 0;
            ui->spinBox_CurrentPalette->setMaximum(1);
            ui->spinBox_CurrentPalette->setValue(0);
        }
    }

    // draw wall paint
    int gfx_offset = (1024 * 5) * passageid + (5 * 32) * locallevelid;
    QPixmap pixmap(8 * 5, 8 * 5);
    pixmap.fill(Qt::transparent);
    for (int c = 0; c < 5; ++c)
    {
        for (int r = 0; r < 5; ++r)
        {
            LevelComponents::Tile8x8 *tmptile = new LevelComponents::Tile8x8(gfxdata + r * 32 + c * 1024 + gfx_offset, palettes);
            tmptile->SetPaletteIndex(usedpal);
            tmptile->DrawTile(&pixmap, r * 8, c * 8);
            delete tmptile;
        }
    }
    if (ui->graphicsView_WallPaintGraphic->scene())
    {
        delete ui->graphicsView_WallPaintGraphic->scene();
    }
    scene = new QGraphicsScene(0, 0, 8 * 5, 8 * 5);
    ui->graphicsView_WallPaintGraphic->setScene(scene);
    ui->graphicsView_WallPaintGraphic->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scene->addPixmap(pixmap);
    ui->graphicsView_WallPaintGraphic->verticalScrollBar()->setValue(0);
}

/// <summary>
/// A helper function to check if the currnet wall paint has grad palettes in the ROM
/// </summary>
unsigned int WallPaintEditorDialog::GetGradPalStartAddr(int passage_Id, int local_level_id)
{
    return ROMUtils::PointerFromData(WL4Constants::WallPaintPalStartLevelPointerTable + passage_Id * 16 + local_level_id * 4);
}

void WallPaintEditorDialog::on_spinBox_PassageID_valueChanged(int arg1)
{
    // update all graphicviews
    (void) arg1;
    RenderCurrentWallPaintAndPalette();
}


void WallPaintEditorDialog::on_spinBox_LocalLevelID_valueChanged(int arg1)
{
    // update all graphicviews
    (void) arg1;
    RenderCurrentWallPaintAndPalette();
}

void WallPaintEditorDialog::on_spinBox_CurrentPalette_valueChanged(int arg1)
{
    // update all graphicviews
    (void) arg1;
    RenderCurrentWallPaintAndPalette();
}

/// <summary>
/// only support the import of the colored palette, then generate all the others by the editor
/// </summary>
void WallPaintEditorDialog::on_pushButton_ImportColoredPalette_clicked()
{
    QVector<QRgb> palettes[2 + 8];
    int passageid = ui->spinBox_PassageID->value();
    if (passageid < 0) passageid = 0;
    int locallevelid = ui->spinBox_LocalLevelID->value();
    if (locallevelid < 0) locallevelid = 0;
    int palette_offset = 32 * 5 * passageid + 32 * locallevelid;
    unsigned short *tmpptr;
    tmpptr = (unsigned short*) (pal_passage_color + palette_offset);
    for (int i = 0; i < 10; i++)
    {
        ROMUtils::LoadPalette(&palettes[i], tmpptr, true);
    }
    bool check_apply = FileIOUtils::ImportPalette(this,
    [&palettes] (int selectedPalId, int colorId, QRgb newColor)
    {
        palettes[0][colorId] = newColor;
        palettes[9][colorId] = newColor;
    },
    0);
    if (!check_apply) return;

    // write new data to u8 array
    memset(pal_passage_color + palette_offset + 6 * 2, 0, 10 * 2);
    memset(pal_passage_gray + palette_offset + 6 * 2, 0, 10 * 2);
    for(int j = 6; j < 16; ++j) // The first 6 colors should not be changed
    {
        ((unsigned short*)(pal_passage_color + palette_offset))[j] = ROMUtils::QRgbToData(palettes[0][j]);
        palettes[1][j] = ROMUtils::QRgbGrayScale(palettes[0][j]);
        palettes[2][j] = palettes[1][j];
        ((unsigned short*)(pal_passage_gray + palette_offset))[j] = ROMUtils::QRgbToData(palettes[1][j]);
    }

    // extra grad palette generation
    unsigned int startlevel_paladdr = GetGradPalStartAddr(passageid, locallevelid);
    if (startlevel_paladdr)
    {
        for(int j = 6; j < 16; ++j) // The first 6 colors should not be changed
        {
            QColor tmpcolor, tmpgray;
            tmpcolor.setRgb(palettes[0][j]);
            int r8_start = tmpcolor.red();
            int g8_start = tmpcolor.green();
            int b8_start = tmpcolor.blue();
            tmpgray.setRgb(palettes[1][j]);
            int r8_end = tmpgray.red();
            int g8_end = tmpgray.green();
            int b8_end = tmpgray.blue();
            int delta_r8 = r8_end - r8_start;
            int delta_g8 = g8_end - g8_start;
            int delta_b8 = b8_end - b8_start;

            for (int i = 5; i >= 0; i--)
            {
                int part_delta_r8 = (delta_r8 * (i + 1)) / 7.0;
                int part_delta_g8 = (delta_g8 * (i + 1)) / 7.0;
                int part_delta_b8 = (delta_b8 * (i + 1)) / 7.0;
                int final_r8 = r8_start + part_delta_r8;
                int final_g8 = g8_start + part_delta_g8;
                int final_b8 = b8_start + part_delta_b8;
                if (final_r8 < 0) final_r8 = 0; if (final_r8 > 255) final_r8 = 255;
                if (final_g8 < 0) final_g8 = 0; if (final_g8 > 255) final_g8 = 255;
                if (final_b8 < 0) final_b8 = 0; if (final_b8 > 255) final_b8 = 255;
                QRgb result = final_r8 << 16 | final_g8 << 8 | final_b8;
                palettes[8 - i][j] = result;
            }
        }

        // save data to the u8 array
        int grad_palette_offset = (32 * 8) * 4 * passageid + (32 * 8) * locallevelid;
        for (int p = 0; p < 8; p++)
        {
            tmpptr = (unsigned short*) (pal_startlevel_color + grad_palette_offset + p * 32);
            for(int j = 6; j < 16; ++j) // The first 6 colors should not be changed
            {
                tmpptr[j] = ROMUtils::QRgbToData(palettes[p + 2][j]);
            }
        }
    }

    // update all graphicviews
    RenderCurrentWallPaintAndPalette();
}

/// <summary>
/// always import Tile8x8 data using the colored palette as ref palette.
/// </summary>
void WallPaintEditorDialog::on_pushButton_ImportGraphics_clicked()
{
    QVector<QRgb> palette;
    int passageid = ui->spinBox_PassageID->value();
    if (passageid < 0) passageid = 0;
    int locallevelid = ui->spinBox_LocalLevelID->value();
    if (locallevelid < 0) locallevelid = 0;
    int palette_offset = 32 * 5 * passageid + 32 * locallevelid;
    unsigned short *tmpptr;
    tmpptr = (unsigned short*) (pal_passage_color + palette_offset);
    ROMUtils::LoadPalette(&palette, tmpptr, true);
    unsigned char *gfxdataptr = gfxdata + (1024 * 5) * passageid + (5 * 32) * locallevelid;

    FileIOUtils::ImportTile8x8GfxData(this, palette,
    tr("Choose a color to covert to transparent:"),
    [gfxdataptr] (QByteArray finaldata, QWidget *parentPtr)
    {
        // Assume the file is fully filled with tiles
        int newtile8x8num = finaldata.size() / 32;
        if (newtile8x8num != 25)
        {
            QMessageBox::critical(parentPtr, tr("Load Error"), tr("Illegal Tile8x8 number!\nYou need to import 25 Tile8x8s in data files!"));
            return;
        }

        // Generate tile8x8 data for all the existing foreground Tile8x8s
        for (int c = 0; c < 5; ++c)
        {
            memcpy(gfxdataptr + c * 1024, finaldata.data() + (32 * 5) * c, 32 * 5);
        }
    });

    // update all graphicviews
    RenderCurrentWallPaintAndPalette();
}

/// <summary>
/// Export the colored palette.
/// </summary>
void WallPaintEditorDialog::on_pushButton_ExportColoredPalette_clicked()
{
    QVector<QRgb> palette;
    int passageid = ui->spinBox_PassageID->value();
    if (passageid < 0) passageid = 0;
    int locallevelid = ui->spinBox_LocalLevelID->value();
    if (locallevelid < 0) locallevelid = 0;
    int palette_offset = 32 * 5 * passageid + 32 * locallevelid;
    unsigned short *tmpptr;
    tmpptr = (unsigned short*) (pal_passage_color + palette_offset);
    ROMUtils::LoadPalette(&palette, tmpptr, true);
    FileIOUtils::ExportPalette(this, palette);
}

