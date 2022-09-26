#include "SpritesEditorDialog.h"
#include "ui_SpritesEditorDialog.h"

#include <QStringList>
#include <QScrollBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QColorDialog>
#include <QFileDialog>
#include <QTextStream>
#include "ROMUtils.h"
#include "FileIOUtils.h"

// constexpr declarations for the initializers in the header
constexpr const char *SpritesEditorDialog::OAMShapeTypeNameData[12];
static QStringList OAMShapeTypeNameSet;

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
    ui->graphicsView_SpritePals->setScene(PaletteBarScene);
    ui->graphicsView_SpritePals->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->graphicsView_SpritePals->scale(2, 2);
    ui->graphicsView_SpritePals->SetCurrentSpritesEditor(this);
    OAMDesignerMAPScene = new QGraphicsScene(0, 0, 1024, 512);
    ui->graphicsView_OamDesignView->setScene(OAMDesignerMAPScene);
    ui->graphicsView_OamDesignView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->graphicsView_OamDesignView->scale(2, 2);

    // Seems the spinboxes won't trigger the valuechanged event when loading UI
    // We need to load graphics manually
    RenderSpritesTileMap();
    SetSelectedSpriteTile(0);
    RenderSpritesPalette();
    SetSelectedEntityColorId(0);
    RenderSpritesetTileMapAndResetLoadTable();

    // Add ToolTip Text
    ui->lineEdit_SpritesetLoadTable->setToolTip(tr("Non-universal Entities can only be loaded starting from tile line 16 and palette line 8.\n"
                                                   "Every entity uses 2 bytes - the first byte is the global entity id,\n"
                                                   "and the second one is the offset used to calculate the address to copy the palettes and tiles data into the ram.\n"
                                                   "In the vanilla game engine, the previously loaded palettes and tiles can be overwritten by new ones,\n"
                                                   "so set the load table carefully.\n"
                                                   "The editor uses different palettes for every entity. However, in-game, some of them share the same palettes and tiles.\n"
                                                   "You may find that the editor acts differently from real game play.\n"
                                                   "In this case, you need to modify all of the entities that share the same tiles and palettes."));

    ui->comboBox_OamShapeType->addItems(OAMShapeTypeNameSet);

    // ListView Init
    ListViewItemModel = new QStandardItemModel(this);
    ui->listView_OamDataList->setModel(ListViewItemModel);
}

/// <summary>
/// Deconstructor of SpritesEditorDialog class.
/// </summary>
SpritesEditorDialog::~SpritesEditorDialog()
{
    delete ui;
}

/// <summary>
/// Perform static initialization of constant data structures for the dialog.
/// </summary>
void SpritesEditorDialog::StaticInitialization()
{
    // Init ComboBox
    for (unsigned int i = 0; i < sizeof(OAMShapeTypeNameData) / sizeof(OAMShapeTypeNameData[0]); ++i)
    {
        OAMShapeTypeNameSet << OAMShapeTypeNameData[i];
    }
}

void SpritesEditorDialog::SetSelectedEntityColorId(int colorID)
{
    LevelComponents::Entity *curEntity = GetCurEntityPtr(); // init
    int palnum = curEntity->GetPalNum();
    if (palnum < 1)
    {
        curEntityColorIdInPalette = 0;
        SelectionBox_Color->setPos(curEntityColorIdInPalette * 8, curEntityPalId * 8);
        SelectionBox_Color->setVisible(true);
        ui->label_CurColorValue->setText(tr("Palette does not exist !"));
        return;
    }
    colorID = qMin(colorID, 15);
    curEntityColorIdInPalette = colorID;
    SelectionBox_Color->setPos(colorID * 8, curEntityPalId * 8);
    SelectionBox_Color->setVisible(true);
    QColor color = GetCurEntityPtr()->GetPalette(curEntityPalId)[colorID];
    ui->label_CurColorValue->setText(QString("RGB888: (") +
                                    QString::number(color.red(), 10) + QString(", ") +
                                    QString::number(color.green(), 10) + QString(", ") +
                                    QString::number(color.blue(), 10) + QString(") RGBHEX: 0x") + QString("%1%2%3")
                                    .arg((int)color.red(), (int)2, (int)16, QChar('0'))
                                    .arg((int)color.green(), (int)2, (int)16, QChar('0'))
                                    .arg((int)color.blue(), (int)2, (int)16, QChar('0')).toUpper() +
                                    QString("  RGB555: (") +
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
    int palnum = GetCurEntityPtr()->GetPalNum();
    if (palnum > 0)
    {
        paletteID = qMin(paletteID, palnum - 1);
    }
    else
    {
        paletteID = 0;
    }
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
    if (colorId > 15) colorId = 15;
    int palnum = GetCurEntityPtr()->GetPalNum();
    if (palnum > 0)
    {
        paletteId = qMin(paletteId, palnum - 1);
    }
    else
    {
        paletteId = 0;
        colorId = 0;
    }
    QColor color = QColorDialog::getColor(Qt::black, this);
    color.setAlpha(0xFF);
    if(color.isValid())
    {
        // Find if new entity data exist
        LevelComponents::Entity *curEntity = GetCurEntityPtr(true);
        curEntity->SetColor(paletteId, colorId, color.rgba());

        // Update Palette Graphicview
        RenderSpritesPalette();
        SetSelectedEntityPaletteId(paletteId);
        SetSelectedEntityColorId(colorId);
        RenderSpritesTileMap();
        SetSelectedSpriteTile(0);
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
    int tilenum = GetCurEntityPtr()->GetTilesNum();
    if (tileID >= tilenum)
    {
        curEntityTileId = (((tilenum >> 5) - 1) << 5) + (tileID & 31);
    }
    else
    {
        curEntityTileId = tileID;
    }
    if (tilenum > 0)
    {
        ui->label_spriteTileID->setText(QString::number(curEntityTileId, 16));
    }
    else
    {
        curEntityTileId = 0;
        ui->label_spriteTileID->setText(tr("Sprite's tile does not exist !"));
    }
    SelectionBox_Sprite->setPos((curEntityTileId & 31) << 3, curEntityTileId >> 5 << 3);
    SelectionBox_Sprite->setVisible(true);
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
    int palnum = curEntity->GetPalNum();
    if (palnum > 0)
    {
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
    }
    else
    {
        // Set up scenes
        SpriteTileMAPScene->clear();
        ui->graphicsView_SpriteTileMap->verticalScrollBar()->setValue(0);
        ui->graphicsView_SpriteTileMap->horizontalScrollBar()->setValue(0);
    }

    // Add the highlighted tile rectangle
    QPixmap selectionPixmap(8, 8);
    const QColor highlightColor(0xFF, 0, 0, 0x7F);
    selectionPixmap.fill(highlightColor);
    SelectionBox_Sprite = SpriteTileMAPScene->addPixmap(selectionPixmap);
    SelectionBox_Sprite->setVisible(false);

    bool enabled = true;
    if (GetCurEntityPtr()->GetEntityGlobalID() < 0x11) enabled = false;
    ui->pushButton_SpriteTilesImport->setEnabled(enabled);
    ui->pushButton_AddPal->setEnabled(enabled);
    ui->pushButton_DeletePal->setEnabled(enabled);
    ui->pushButton_SpritePaletteImport->setEnabled(enabled);
    ui->pushButton_SwapPal->setEnabled(enabled);
    ui->pushButton_SpritePaletteExport->setEnabled(palnum);
    ui->pushButton_SpriteTilesExport->setEnabled(palnum);
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
    if (palnum > 0)
    {
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
    }
    else
    {
        PaletteBarScene->clear();
    }

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
/// Render OAM Set based on the ListView data
/// </summary>
/// <param name="selectrow">
/// select a row after re-render oam graphic, leave it blank to set default value 0
/// </param>
void SpritesEditorDialog::RenderOamSet(int selectrow)
{
    int rowcount = ListViewItemModel->rowCount();
    if (!rowcount) return;
    QVector<unsigned short> nakedOAMData;
    for (int i = 0; i < rowcount; ++i)
    {
        QStringList CurloadtableStrData = ListViewItemModel->item(i)->text().split(QChar(' '), Qt::SkipEmptyParts);
        for (int j = 0; j < 3; ++j)
        {
            nakedOAMData.push_back(CurloadtableStrData.at(j).toUInt(nullptr, 16));
        }
    }
    QPixmap oampixmap(1024, 512);
    QPainter oampainter(&oampixmap);
    oampainter.drawImage(0, 0, GetCurEntityPtr()->RenderOAMPreview(rowcount, nakedOAMData, ui->checkBox_NoReferBox->isChecked()));
    OAMDesignerMAPScene->clear();
    OAMmapping = OAMDesignerMAPScene->addPixmap(oampixmap);

    // try to select a row in the listview
    QModelIndex index = ListViewItemModel->index(selectrow, 0);
    if (index.isValid())
        ui->listView_OamDataList->selectionModel()->select(index, QItemSelectionModel::Select);
    SelectedRow_ListView = selectrow;
    handleSelectionChanged();
}

/// <summary>
/// Helper function used to generate OAM String based on the oam param widgets
/// </summary>
/// <return>
/// single OAM data QString
/// </return>
QString SpritesEditorDialog::GenerateOAMString()
{
    unsigned short tmpOAMData[3] = {0, 0, 0};
    int shapesizedata = ui->comboBox_OamShapeType->currentIndex();
    int yvalue = (ui->spinBox_OamY->value() >= 0) ? ui->spinBox_OamY->value() : 0x100 + ui->spinBox_OamY->value();
    tmpOAMData[0] = yvalue |
            ((shapesizedata & 0xC) >> 2) << 14 |
            (ui->checkBox_OAMSemiTransparency->isChecked() ? 1 : 0) << 0xA;
    int xvalue = (ui->spinBox_OamX->value() >= 0) ? ui->spinBox_OamX->value() : 0x200 + ui->spinBox_OamX->value();
    tmpOAMData[1] = xvalue |
            (ui->checkBox_OAMXFlip->isChecked() ? 1 : 0) << 12 |
            (ui->checkBox_OAMYFlip->isChecked() ? 1 : 0) << 13 |
            ((shapesizedata & 3) << 14);
    tmpOAMData[2] = ui->spinBox_OAMCharId->value() | ui->spinBox_OAMPriority->value() << 10 | ui->spinBox_OamPalID->value() << 12;
    QString result = QString::number(tmpOAMData[0], 16).toUpper() + " " +
                     QString::number(tmpOAMData[1], 16).toUpper() + " " +
                     QString::number(tmpOAMData[2], 16).toUpper();
    return result;
}

/// <summary>
/// Helper function used to get the complete OAM array
/// </summary>
/// <return>
/// OAM array QString
/// </return>
QString SpritesEditorDialog::GetOAMArray()
{
    int rowcount = ListViewItemModel->rowCount();
    QString result;
    if (!rowcount) return result;
    for (int i = 0; i < rowcount; ++i)
    {
        result += ListViewItemModel->item(i)->text() + " ";
    }
    return result;
}

/// <summary>
/// Helper function used to handle selection changed in ListView
/// </summary>
void SpritesEditorDialog::handleSelectionChanged()
{
    QVector<unsigned short> nakedOAMData;
    QStringList CurloadtableStrData = ListViewItemModel->item(SelectedRow_ListView)->text().split(QChar(' '), Qt::SkipEmptyParts);
    for (int i = 0; i < 3; ++i)
    {
        nakedOAMData.push_back(CurloadtableStrData.at(i).toUInt(nullptr, 16));
    }

    ui->spinBox_OamX->setValue((nakedOAMData[1] & 0xFF) - (nakedOAMData[1] & 0x100));
    ui->spinBox_OamY->setValue((nakedOAMData[0] & 0x7F) - (nakedOAMData[0] & 0x80));
    ui->checkBox_OAMXFlip->setChecked((nakedOAMData[1] & (1 << 0xC)) != 0);
    ui->checkBox_OAMYFlip->setChecked((nakedOAMData[1] & (1 << 0xD)) != 0);
    int SZ = (nakedOAMData[1] >> 0xE) & 3;                         // object size
    int SH = (nakedOAMData[0] >> 0xE) & 3;                         // object shape
    ui->comboBox_OamShapeType->setCurrentIndex(SH << 2 | SZ);
    ui->spinBox_OAMCharId->setValue(nakedOAMData[2] & 0x3FF);
    ui->spinBox_OamPalID->setValue((nakedOAMData[2] >> 0xC) & 0xF);
    ui->spinBox_OAMPriority->setValue((nakedOAMData[2] >> 0xA) & 3);
    ui->checkBox_OAMSemiTransparency->setChecked(((nakedOAMData[0] >> 0xA) & 3) == 1);
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

    // Find if new entityset data exist
    LevelComponents::EntitySet *curEntityset = GetCurEntitySetPtr(true);
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
    QString romFileDir = QFileInfo(ROMUtils::ROMFileMetadata->FilePath).dir().path();
    QString qFilePath = QFileDialog::getSaveFileName(this, tr("Save current Tile8x8 map to a file"),
                                                     romFileDir, tr("PNG file (*.png)"));
    if (!qFilePath.isEmpty())
    {
        LevelComponents::Entity *curEntity = GetCurEntityPtr();
        int tileNum = curEntity->GetTilesNum();
        QPixmap SpriteTilesPixmap(8 * 32, (tileNum / 32) * 8);
        QPainter SpriteTilemapPixmapPainter(&SpriteTilesPixmap);
        SpriteTilemapPixmapPainter.drawImage(0, 0, FileIOUtils::RenderBGColor(curEntity->GetTileMap(curEntityPalId), this));
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
        tr("Choose a color to covert to transparent:"),
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
    int result_1 = QInputDialog::getInt(this, tr("InputBox"),
                                         tr("Input the palette Id you want to delete.\nRelevant tiles will be deleted too."),
                                        0, 0, maxval, 1, &ok);
    if (ok && (result_1 < 0 || result_1 > maxval))
    {
        QMessageBox::critical(this, tr("Error"), QString(tr("Illegal input!")));
        return;
    }
    int result_2 = QInputDialog::getInt(this, tr("InputBox"),
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

/// <summary>
/// Reset Oam designer with new OAM data string when clicking pushButton_ResetAllOamData
/// </summary>
void SpritesEditorDialog::on_pushButton_ResetAllOamData_clicked()
{
    QString tmplist = QInputDialog::getText(this, tr("InputBox"),
                                            tr("Input OAM Data Hex String:"),
                                            QLineEdit::Normal,
                                            GetOAMArray());
    tmplist.replace(",", " ");
    tmplist.replace("0x", " ");
    QStringList listStrs = tmplist.split(QChar(' '), Qt::SkipEmptyParts);

    if (!listStrs.size()) return;
    if (listStrs.size() % 3)
    {
        QMessageBox::critical(this, tr("Error"), QString(tr("Data number should be multiple of 3 !")));
        return;
    }
    if (ListViewItemModel)
    {
        ListViewItemModel->clear();
        delete ListViewItemModel;
    }

    ListViewItemModel = new QStandardItemModel(this);

    for (int i = 0; i < (listStrs.size() / 3); i++)
    {
        QString string0 = static_cast<QString>(listStrs.at(3 * i));
        QString string1 = static_cast<QString>(listStrs.at(3 * i + 1));
        QString string2 = static_cast<QString>(listStrs.at(3 * i + 2));
        string0 = QString::number(string0.toUInt(nullptr, 16), 16).toUpper();
        string1 = QString::number(string1.toUInt(nullptr, 16), 16).toUpper();
        string2 = QString::number(string2.toUInt(nullptr, 16), 16).toUpper();
        QStandardItem *item = new QStandardItem(string0 + " " + string1 + " " + string2);
        ListViewItemModel->appendRow(item);
    }
    ui->listView_OamDataList->setModel(ListViewItemModel);

    // Render graphic based on list view
    RenderOamSet();
}

/// <summary>
/// Be called the listview is clicked and an oam data is selected.
/// </summary>
/// <param name="index">
/// Reference of the selected QModelIndex from the listview.
/// </param>
void SpritesEditorDialog::on_listView_OamDataList_clicked(const QModelIndex &index)
{
    SelectedRow_ListView = index.row();
    handleSelectionChanged();
}

/// <summary>
/// Add a new OAM data string when clicking pushButton_AddOAM
/// </summary>
void SpritesEditorDialog::on_pushButton_AddOAM_clicked()
{
    QStandardItem *item = new QStandardItem(GenerateOAMString());
    ListViewItemModel->appendRow(item);

    // Render graphic based on list view
    RenderOamSet(ListViewItemModel->rowCount() - 1);
}

/// <summary>
/// Reset new OAM data string to the selected row when clicking pushButton_ResetCurOAM
/// </summary>
void SpritesEditorDialog::on_pushButton_ResetCurOAM_clicked()
{
    ListViewItemModel->item(SelectedRow_ListView, 0)->setText(GenerateOAMString());

    // Render graphic based on list view
    RenderOamSet(SelectedRow_ListView);
}

/// <summary>
/// Reset new OAM data string to the selected row when clicking pushButton_ResetCurOAM
/// </summary>
void SpritesEditorDialog::on_pushButton_ExportOAMData_clicked()
{
    QString romFileDir = QFileInfo(ROMUtils::ROMFileMetadata->FilePath).dir().path();
    QString selectedfilter;
    QString txtFilter(QObject::tr("text file") + " (*.txt)");
    QString qFilePath = QFileDialog::getSaveFileName(this,
                                                     QObject::tr("Save OAM data to txt file"),
                                                     romFileDir,
                                                     txtFilter,
                                                     &selectedfilter);
    if(qFilePath.isEmpty()) return;

    QFile palfile(qFilePath);
    if(palfile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // Stream text to the file
        QTextStream out(&palfile);
        out << GetOAMArray();
        palfile.close();
    }
}

/// <summary>
/// Delete one row of OAM data from the ListView when clicking pushButton_DeleteCurOam
/// </summary>
void SpritesEditorDialog::on_pushButton_DeleteCurOam_clicked()
{
    if (!(ListViewItemModel->rowCount() - 1)) return; // we don't want to delete the last oam
    ListViewItemModel->removeRows(SelectedRow_ListView, 1);

    // Render graphic based on list view
    RenderOamSet();
}
