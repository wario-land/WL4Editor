#include "SpritesEditorDialog.h"
#include "ui_SpritesEditorDialog.h"

#include <QStringList>
#include <QScrollBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QColorDialog>
#include <QFileDialog>
#include "ROMUtils.h"
#include "FileIOUtils.h"

/// <summary>
/// Constructor of SpritesEditorDialog class.
/// </summary>
SpritesEditorDialog::SpritesEditorDialog(QWidget *parent, DialogParams::EntitiesAndEntitySetsEditParams *entitiesAndEntitySetsEditParams) :
    QDialog(parent),
    ui(new Ui::SpritesEditorDialog),
    entitiesAndEntitySetsEditParam(entitiesAndEntitySetsEditParams)
{
    ui->setupUi(this);

    // Render Setup
    SpriteTileMAPScene = new QGraphicsScene(0, 0, 8 * 32, 8 * 32);
    ui->graphicsView_SpriteTileMap->setScene(SpriteTileMAPScene);
    ui->graphicsView_SpriteTileMap->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->graphicsView_SpriteTileMap->scale(2, 2);
    ui->graphicsView_SpriteTileMap->SetCurrentSpritesEditor(this);
    SpritesetTileMAPScene = new QGraphicsScene(0, 0, 8 * 32, 8 * 32);
    ui->graphicsView_SpritesetTileMap->setScene(SpritesetTileMAPScene);
    ui->graphicsView_SpritesetTileMap->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    PaletteBarScene = new QGraphicsScene(0, 0, 16 * 8, 16 * 8);
    // TODO: PaletteBarScene->setBackgroundBrush();
    ui->graphicsView_SpritePals->setScene(PaletteBarScene);
    ui->graphicsView_SpritePals->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->graphicsView_SpritePals->scale(2, 2);
    ui->graphicsView_SpritePals->SetCurrentSpritesEditor(this);

    // Seems the spinboxes won't trigger the valuechanged event when loading UI
    // We need to load graphics manually
    RenderSpritesTileMap();
    SetSelectedSpriteTile(0);
    RenderSpritesPalette();
    SetSelectedEntityColorId(0);
    RenderSpritesetTileMapAndResetLoadTable();
}

/// <summary>
/// Deconstructor of SpritesEditorDialog class.
/// </summary>
SpritesEditorDialog::~SpritesEditorDialog()
{
    delete ui;
}

void SpritesEditorDialog::SetSelectedEntityColorId(int colorID)
{
    if (colorID > 15) colorID = 15;
    curEntityColorIdInPalette = colorID;
    SelectionBox_Color->setPos(colorID * 8, curEntityPalId * 8);
    SelectionBox_Color->setVisible(true);
    QColor color = GetCurEntityPtr()->GetPalette(curEntityPalId)[colorID];
    ui->label_CurColorValue->setText(QString("RGB888: (") +
                                    QString::number(color.red(), 10) + QString(", ") +
                                    QString::number(color.green(), 10) + QString(", ") +
                                    QString::number(color.blue(), 10) + QString(") RGB555: (") +
                                    QString::number(color.red() >> 3, 10) + QString(", ") +
                                    QString::number(color.green() >> 3, 10) + QString(", ") +
                                    QString::number(color.blue() >> 3, 10) + QString(")"));
}

/// <summary>
/// Reset sprite palette Id
/// </summary>
/// <param name="paletteID">
/// New palette id value.
/// </param>
void SpritesEditorDialog::SetSelectedEntityPaletteId(int paletteID)
{
    if (int palnum = GetCurEntityPtr()->GetPalNum(); paletteID >= palnum) paletteID = palnum - 1;
    curEntityPalId = paletteID;
    ui->label_PalID->setText(QString::number(paletteID));
    RenderSpritesTileMap();
    SetSelectedSpriteTile(0);
}

/// <summary>
/// Reset sprite color in one specified palette.
/// </summary>
/// <param name="paletteId">
/// palette id value to find color to set.
/// </param>
/// <param name="colorId">
/// color id value to find color to set.
/// </param>
void SpritesEditorDialog::SetColor(int paletteId, int colorId)
{
    if (int palnum = GetCurEntityPtr()->GetPalNum(); paletteId >= palnum) paletteId = palnum - 1;
    if (colorId > 15) colorId = 15;
    QColor color = QColorDialog::getColor(Qt::black, this);
    color.setAlpha(0xFF);
    if(color.isValid())
    {
        // Find if new entity data exist
        LevelComponents::Entity *curEntity = GetCurEntityPtr(true);
        curEntity->SetColor(paletteId, colorId, color.rgba());

        // Update Palette Graphicview
        RenderSpritesPalette();
    }
}

/// <summary>
/// Reset sprite color in one specified palette.
/// </summary>
/// <param name="paletteId">
/// palette id value to find color to set.
/// </param>
void SpritesEditorDialog::SetSelectedSpriteTile(const int tileID)
{
    if (int tilenum = GetCurEntityPtr()->GetTilesNum(); tileID >= tilenum)
    {
        curEntityTileId = (((tilenum >> 5) - 1) << 5) + (tileID & 31);
    }
    else
    {
        curEntityTileId = tileID;
    }
    SelectionBox_Sprite->setPos((curEntityTileId & 31) << 3, curEntityTileId >> 5 << 3);
    SelectionBox_Sprite->setVisible(true);
    ui->label_spriteTileID->setText(QString::number(curEntityTileId, 16));
}

/// <summary>
/// Reset sprite graphicview and sprite palette graphiview when spinBox_GlobalSpriteId has a value change event
/// </summary>
/// <param name="arg1">
/// New spinBox_GlobalSpriteId value.
/// </param>
void SpritesEditorDialog::on_spinBox_GlobalSpriteId_valueChanged(int arg1)
{
    currentEntityID = arg1;
    RenderSpritesPalette();
    SetSelectedEntityColorId(0);
    RenderSpritesTileMap();
    SetSelectedSpriteTile(0);
}

/// <summary>
/// Reset sprite graphicview based on the current spinBox_GlobalSpriteId value
/// </summary>
void SpritesEditorDialog::RenderSpritesTileMap()
{
    // Find if new entity data exist
    LevelComponents::Entity *curEntity = GetCurEntityPtr(); // init

    // Calculate size
    int tilenum = curEntity->GetTilesNum();
    int rownum = tilenum >> 5; // tilenum / 32

    // draw pixmaps
    QPixmap SpriteTilePixmap(8 * 32, rownum * 8);
    SpriteTilePixmap.fill(Qt::transparent);
    QPainter SpriteTilePixmapPainter(&SpriteTilePixmap);
    SpriteTilePixmapPainter.drawImage(0, 0, curEntity->GetTileMap(curEntityPalId));

    // Set up scenes
    SpriteTileMAPScene->clear();
    SpriteTilemapping = SpriteTileMAPScene->addPixmap(SpriteTilePixmap);
    ui->graphicsView_SpriteTileMap->verticalScrollBar()->setValue(0);
    ui->graphicsView_SpriteTileMap->horizontalScrollBar()->setValue(0);

    // Add the highlighted tile rectangle
    QPixmap selectionPixmap(8, 8);
    const QColor highlightColor(0xFF, 0, 0, 0x7F);
    selectionPixmap.fill(highlightColor);
    SelectionBox_Sprite = SpriteTileMAPScene->addPixmap(selectionPixmap);
    SelectionBox_Sprite->setVisible(false);

    bool enability = true;
    if (GetCurEntityPtr()->GetEntityGlobalID() < 0x11) enability = false;
    ui->pushButton_SpriteTilesImport->setEnabled(enability);
    ui->pushButton_AddPal->setEnabled(enability);
    ui->pushButton_DeletePal->setEnabled(enability);
    ui->pushButton_SpritePaletteImport->setEnabled(enability);
    ui->pushButton_SwapPal->setEnabled(enability);
}

/// <summary>
/// Reset sprite palettes graphicview
/// </summary>
void SpritesEditorDialog::RenderSpritesPalette()
{
    // Data reset
    curEntityPalId = 0;
    curEntityColorIdInPalette = 0;

    // Find if new entity data exist
    LevelComponents::Entity *curEntity = GetCurEntityPtr(); // init

    // Render palettes
    int palnum = curEntity->GetPalNum();
    QPixmap PaletteBarpixmap(8 * 16, palnum * 8);
    PaletteBarpixmap.fill(Qt::transparent);
    QPainter PaletteBarPainter(&PaletteBarpixmap);
    for (int j = 0; j < palnum; ++j)
    {
        QVector<QRgb> palettetable = curEntity->GetPalette(j);
        for (int i = 1; i < 16; ++i) // Ignore the first color
        {
            PaletteBarPainter.fillRect(8 * i, 8 * j, 8, 8, palettetable[i]);
        }
    }
    PaletteBarScene->clear();
    Palettemapping = PaletteBarScene->addPixmap(PaletteBarpixmap);
    ui->graphicsView_SpritePals->verticalScrollBar()->setValue(0);

    // Add Color selection box
    QPixmap selectionPixmap3(8, 8);
    selectionPixmap3.fill(Qt::transparent);
    QPainter SelectionBoxRectPainter(&selectionPixmap3);
    SelectionBoxRectPainter.setPen(QPen(QBrush(Qt::blue), 2));
    SelectionBoxRectPainter.drawRect(1, 1, 7, 7);
    SelectionBox_Color = PaletteBarScene->addPixmap(selectionPixmap3);
    SelectionBox_Color->setVisible(false);
}

/// <summary>
/// Reset spriteset graphicview based on the current spinBox_SpritesetID value and reset Load table
/// </summary>
void SpritesEditorDialog::RenderSpritesetTileMapAndResetLoadTable()
{
    // Find if new entityset data exist
    LevelComponents::EntitySet *curEntityset = GetCurEntitySetPtr(); // init

    // Add Extra entities instances
    curEntityset->SetExtraEntities(entitiesAndEntitySetsEditParam->entities);

    // draw pixmaps
    QPixmap SpriteSetTilePixmap(8 * 32, 8 * 32);
    SpriteSetTilePixmap.fill(Qt::transparent);
    QPainter SpriteSetTilePixmapPainter(&SpriteSetTilePixmap);
    SpriteSetTilePixmapPainter.drawImage(0, 0, curEntityset->GetPixmap(ui->spinBox_SpritesetPaletteID->value()).toImage());

    // Set up scenes
    SpritesetTileMAPScene->clear();
    SpritesetTilemapping = SpritesetTileMAPScene->addPixmap(SpriteSetTilePixmap);

    // Clear Extra entities instances vector table
    curEntityset->ClearExtraEntities();

    // Set Load Table
    QVector<LevelComponents::EntitySetinfoTableElement> loadtable = curEntityset->GetEntityTable();
    QString outputstr;
    for (auto element: loadtable)
    {
        outputstr += QString("%1 %2 ").arg(element.Global_EntityID, 2, 16, QChar('0')).arg(element.paletteOffset, 2, 16, QChar('0'));
    }
    ui->lineEdit_SpritesetLoadTable->setText(outputstr);
}

/// <summary>
/// Find and return entity pointer points to current entity
/// </summary>
/// <return>
/// current entity pointer
/// </return>
LevelComponents::Entity *SpritesEditorDialog::GetCurEntityPtr(bool createNewEntity)
{
    // Find if new entity data exist
    LevelComponents::Entity *oldEntity = ROMUtils::entities[currentEntityID];
    LevelComponents::Entity *curEntity = oldEntity; // init

    auto entityFound = std::find_if(entitiesAndEntitySetsEditParam->entities.begin(),
                                    entitiesAndEntitySetsEditParam->entities.end(),
        [oldEntity](LevelComponents::Entity *entity) {return entity->GetEntityGlobalID() == oldEntity->GetEntityGlobalID();});
    int spriteIdInChangelist = std::distance(entitiesAndEntitySetsEditParam->entities.begin(), entityFound);

    // If the current entity has a new unsaved instance in the dialog
    if(entityFound != entitiesAndEntitySetsEditParam->entities.end())
    {
        curEntity = entitiesAndEntitySetsEditParam->entities[spriteIdInChangelist];
    }
    else
    {
        if (createNewEntity)
        {
            curEntity = new LevelComponents::Entity(*oldEntity);
            entitiesAndEntitySetsEditParam->entities.push_back(curEntity);
        }
    }
    return curEntity;
}

/// <summary>
/// Find and return entityset pointer points to current entityset
/// </summary>
/// <return>
/// current entityset pointer
/// </return>
LevelComponents::EntitySet *SpritesEditorDialog::GetCurEntitySetPtr(bool createNewEntitySet)
{
    LevelComponents::EntitySet *oldEntityset = ROMUtils::entitiessets[currentEntitySetID];
    LevelComponents::EntitySet *curEntityset = oldEntityset; // init

    auto entitySetFound = std::find_if(entitiesAndEntitySetsEditParam->entitySets.begin(),
                                    entitiesAndEntitySetsEditParam->entitySets.end(),
        [oldEntityset](LevelComponents::EntitySet *entityset) {return entityset->GetEntitySetId() == oldEntityset->GetEntitySetId();});
    int spritesetIdInChangelist = std::distance(entitiesAndEntitySetsEditParam->entitySets.begin(), entitySetFound);

    // If the current entity has a new unsaved instance in the dialog
    if(entitySetFound != entitiesAndEntitySetsEditParam->entitySets.end())
    {
        curEntityset = entitiesAndEntitySetsEditParam->entitySets[spritesetIdInChangelist];
    }
    else
    {
        if (createNewEntitySet)
        {
            curEntityset = new LevelComponents::EntitySet(*oldEntityset);
            entitiesAndEntitySetsEditParam->entitySets.push_back(curEntityset);
        }
    }
    return curEntityset;
}

/// <summary>
/// Reset spriteset graphicview and spriteset loadtable lineedit when spinBox_SpritesetID has a value change event
/// </summary>
/// <param name="arg1">
/// New spinBox_SpritesetID value.
/// </param>
void SpritesEditorDialog::on_spinBox_SpritesetID_valueChanged(int arg1)
{
    currentEntitySetID = arg1;
    RenderSpritesetTileMapAndResetLoadTable();
}

/// <summary>
/// Reset Spriteset Palette Id
/// </summary>
/// <param name="arg1">
/// New spinBox_SpritesetPaletteID value.
/// </param>
void SpritesEditorDialog::on_spinBox_SpritesetPaletteID_valueChanged(int arg1)
{
    (void) arg1;
    RenderSpritesetTileMapAndResetLoadTable();
}

/// <summary>
/// Reset Spriteset Load Table when click pushButton_ResetLoadTable
/// </summary>
void SpritesEditorDialog::on_pushButton_ResetLoadTable_clicked()
{
    // Find if new entityset data exist
    LevelComponents::EntitySet *curEntityset = GetCurEntitySetPtr(true);

    // Generate Entityset Load Table and update them into the new entityset instance
    QStringList loadtableStrData = ui->lineEdit_SpritesetLoadTable->text().split(QChar(' '), Qt::SkipEmptyParts);
    if (loadtableStrData.size() & 1)
    {
        QMessageBox::critical(this,
                              QString(QObject::tr("Error")),
                              QString(QObject::tr("Illegal LoadTable element size!\nThere should be an even number of elements.")));
        return;
    }
    if (loadtableStrData.size() > (0x1F * 2))
    {
        QMessageBox::critical(this,
                              QString(QObject::tr("Error")),
                              QString(QObject::tr("Too many elements. Max number of elements: 62."))); // 0x1F * 2 = 62
        return;
    }
    for (int i = 0; i < loadtableStrData.size(); i += 2)
    {
        if (loadtableStrData[i].toUInt(nullptr, 16) > 0x80)
        {
            QMessageBox::critical(this,
                                  QString(QObject::tr("Error")),
                                  QString(QObject::tr("Illegal entity Id at element %1, it should be less than 0x81.")).arg(i));
            return;
        }
        if (loadtableStrData[i + 1].toUInt(nullptr, 16) > 7)
        {
            QMessageBox::critical(this,
                                  QString(QObject::tr("Error")),
                                  QString(QObject::tr("Illegal palette offset Id at element %1, it should be less than 8.")).arg(i + 1));
            return;
        }
    }
    curEntityset->ClearEntityLoadTable();
    for (int i = 0; i < loadtableStrData.size(); i += 2)
    {
        LevelComponents::EntitySetinfoTableElement newelement;
        newelement.Global_EntityID = loadtableStrData[i].toUInt(nullptr, 16);
        newelement.paletteOffset = loadtableStrData[i + 1].toUInt(nullptr, 16);
        curEntityset->EntityLoadTablePushBack(newelement);
    }

    // UI update
    RenderSpritesetTileMapAndResetLoadTable();
}

/// <summary>
/// Export sprite tiles map to file when hit pushButton_SpriteTilesExport
/// </summary>
void SpritesEditorDialog::on_pushButton_SpriteTilesExport_clicked()
{
    QString qFilePath = QFileDialog::getSaveFileName(this, tr("Save current Tile8x8 map to a file"),
                                                     QString(""), tr("PNG file (*.png)"));
    if (qFilePath.compare(""))
    {
        LevelComponents::Entity *curEntity = GetCurEntityPtr();
        int tileNum = curEntity->GetTilesNum();
        QPixmap SpriteTilesPixmap(8 * 32, (tileNum / 32) * 8);
        QPainter SpriteTilemapPixmapPainter(&SpriteTilesPixmap);
        SpriteTilemapPixmapPainter.drawImage(0, 0, FileIOUtils::SetBGColor(curEntity->GetTileMap(curEntityPalId), this));
        SpriteTilesPixmap.save(qFilePath, "PNG", 100);
    }
}

/// <summary>
/// Import sprite tiles map from file when hit pushButton_SpriteTilesImport
/// </summary>
void SpritesEditorDialog::on_pushButton_SpriteTilesImport_clicked()
{
    LevelComponents::Entity *curEntity = GetCurEntityPtr(true);
    LevelComponents::Tile8x8 *blanktileref = curEntity->GetBlankTile();
    FileIOUtils::ImportTile8x8GfxData(this,
        curEntity->GetPalette(curEntityPalId),
        [curEntity, blanktileref] (QByteArray finaldata, QWidget *parentPtr)
        {
            // Assume the file is fully filled with tiles
            int newtilenum = finaldata.size() / 32;
            unsigned char newtmpdata[32];
            if(newtilenum > curEntity->GetTilesNum())
            {
                QMessageBox::critical(parentPtr, tr("Load Error"), QString(tr("You can only import %1 tiles at most!")).arg(newtilenum));
                return;
            }
            QVector<LevelComponents::Tile8x8 *> curTilearray = curEntity->GetSpriteTiles();
            for(int i = 0; i < newtilenum; ++i)
            {
                memcpy(newtmpdata, finaldata.data() + 32 * i, 32);
                LevelComponents::Tile8x8* tile = curTilearray.at(i);
                if (tile != blanktileref) delete tile;
                tile = new LevelComponents::Tile8x8((unsigned char *)(finaldata.data() + 32 * i), curEntity->GetPalettes());
                curEntity->SetTile8x8(tile, i);
            }
        });

    // update all the graphicviews
    RenderSpritesTileMap();
    SetSelectedSpriteTile(0);
    curEntity->ExtractSpritesTiles();
    RenderSpritesetTileMapAndResetLoadTable();
}

/// <summary>
/// Import sprite palette on the current row when clicking pushButton_SpritePaletteImport
/// </summary>
void SpritesEditorDialog::on_pushButton_SpritePaletteImport_clicked()
{
    SpritesEditorDialog *curEditor = this;
    LevelComponents::Entity *curEntity = nullptr; // have to create instance after checking file format successful
    FileIOUtils::ImportPalette(this,
        [curEditor, curEntity] (int selectedPalId, int colorId, QRgb newColor) mutable
        {
            curEntity = curEditor->GetCurEntityPtr(true);
            curEntity->SetColor(selectedPalId, colorId, newColor);
        },
        curEntityPalId);
    RenderSpritesPalette();
    SetSelectedEntityColorId(0);
    RenderSpritesTileMap();
    SetSelectedSpriteTile(0);
    RenderSpritesetTileMapAndResetLoadTable();
}

/// <summary>
/// Export sprite palette on the current row when clicking pushButton_SpritePaletteImport
/// </summary>
void SpritesEditorDialog::on_pushButton_SpritePaletteExport_clicked()
{
    FileIOUtils::ExportPalette(this, GetCurEntityPtr()->GetPalette(curEntityPalId));
}

/// <summary>
/// Delete sprite's palette and tiles when clicking pushButton_DeletePal
/// </summary>
void SpritesEditorDialog::on_pushButton_DeletePal_clicked()
{
    GetCurEntityPtr(true)->DeleteTilesAndPaletteByOneRow(curEntityPalId);
    RenderSpritesPalette();
    SetSelectedEntityColorId(0);
    RenderSpritesTileMap();
    SetSelectedSpriteTile(0);
    RenderSpritesetTileMapAndResetLoadTable();
}

/// <summary>
/// Add sprite's palette and tiles when clicking pushButton_AddPal
/// </summary>
void SpritesEditorDialog::on_pushButton_AddPal_clicked()
{
    GetCurEntityPtr(true)->AddTilesAndPaletteByOneRow();
    RenderSpritesPalette();
    SetSelectedEntityColorId(0);
    RenderSpritesTileMap();
    SetSelectedSpriteTile(0);
    RenderSpritesetTileMapAndResetLoadTable();
}

/// <summary>
/// Swap 2 sprite's palettes when clicking pushButton_SwapPal
/// </summary>
void SpritesEditorDialog::on_pushButton_SwapPal_clicked()
{
    bool ok;
    int maxval = GetCurEntityPtr()->GetPalNum() - 1;
    int result_1 = QInputDialog::getInt(nullptr, tr("InputBox"),
                                         tr("Input the palette Id you want to delete.\nRelevant tiles will be deleted too."),
                                        0, 0, maxval, 1, &ok);
    if (ok && (result_1 < 0 || result_1 > maxval))
    {
        QMessageBox::critical(this, tr("Error"), QString(tr("Illegal input!")));
        return;
    }
    int result_2 = QInputDialog::getInt(nullptr, tr("InputBox"),
                                         tr("Input the palette Id you want to delete.\nRelevant tiles will be deleted too."),
                                        0, 0, maxval, 1, &ok);
    if (ok && (result_2 < 0 || result_2 > maxval))
    {
        QMessageBox::critical(this, tr("Error"), QString(tr("Illegal input!")));
        return;
    }
    GetCurEntityPtr(true)->SwapPalettes(result_1, result_2);
    RenderSpritesPalette();
    SetSelectedEntityColorId(0);
    RenderSpritesTileMap();
    SetSelectedSpriteTile(0);
    RenderSpritesetTileMapAndResetLoadTable();
}
