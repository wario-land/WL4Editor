#include "SpritesEditorDialog.h"
#include "ui_SpritesEditorDialog.h"

#include <QStringList>
#include <QScrollBar>
#include <QMessageBox>
#include <QColorDialog>
#include "ROMUtils.h"

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
    entityColorIdInPalette = colorID;
    SelectionBox_Color->setPos(colorID * 8, entityPalId * 8);
    SelectionBox_Color->setVisible(true);
    QColor color = FindCurEntity()->GetPalette(entityPalId)[colorID];
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
    entityPalId = paletteID;
    RenderSpritesTileMap();
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
    QColor color = QColorDialog::getColor(Qt::black, this);
    color.setAlpha(0xFF);
    if(color.isValid())
    {
        // Find if new entity data exist
        LevelComponents::Entity *curEntity = FindCurEntity(); // init
        curEntity->SetColor(paletteId, colorId, color.rgba());

        // Update Palette Graphicview
        RenderSpritesPalette();
    }
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
    RenderSpritesTileMap();
    RenderSpritesPalette();
}

/// <summary>
/// Reset sprite graphicview based on the current spinBox_GlobalSpriteId value
/// </summary>
void SpritesEditorDialog::RenderSpritesTileMap()
{
    // Find if new entity data exist
    LevelComponents::Entity *curEntity = FindCurEntity(); // init

    // Calculate size
    int tilenum = curEntity->GetTilesNum();
    int rownum = tilenum >> 5; // tilenum / 32

    // draw pixmaps
    QPixmap SpriteTilePixmap(8 * 32, rownum * 8);
    SpriteTilePixmap.fill(Qt::transparent);
    QPainter SpriteTilePixmapPainter(&SpriteTilePixmap);
    SpriteTilePixmapPainter.drawImage(0, 0, curEntity->GetTileMap(entityPalId));

    // Set up scenes
    SpriteTileMAPScene->clear();
    SpriteTilemapping = SpriteTileMAPScene->addPixmap(SpriteTilePixmap);
    ui->graphicsView_SpriteTileMap->verticalScrollBar()->setValue(0);

    // Add the highlighted tile rectangle
    QPixmap selectionPixmap(8, 8);
    const QColor highlightColor(0xFF, 0, 0, 0x7F);
    selectionPixmap.fill(highlightColor);
    SelectionBox_Sprite = SpriteTileMAPScene->addPixmap(selectionPixmap);
    SelectionBox_Sprite->setVisible(false);
}

/// <summary>
/// Reset sprite palettes graphicview
/// </summary>
void SpritesEditorDialog::RenderSpritesPalette()
{
    // Find if new entity data exist
    LevelComponents::Entity *curEntity = FindCurEntity(); // init

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
    LevelComponents::EntitySet *curEntityset = FindCurEntitySet(); // init

    // draw pixmaps
    QPixmap SpriteSetTilePixmap(8 * 32, 8 * 32);
    SpriteSetTilePixmap.fill(Qt::transparent);
    QPainter SpriteSetTilePixmapPainter(&SpriteSetTilePixmap);
    SpriteSetTilePixmapPainter.drawImage(0, 0, curEntityset->GetPixmap(ui->spinBox_SpritesetPaletteID->value()).toImage());

    // Set up scenes
    SpritesetTileMAPScene->clear();
    SpritesetTilemapping = SpritesetTileMAPScene->addPixmap(SpriteSetTilePixmap);

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
LevelComponents::Entity *SpritesEditorDialog::FindCurEntity()
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
    return curEntity;
}

/// <summary>
/// Find and return entityset pointer points to current entityset
/// </summary>
/// <return>
/// current entityset pointer
/// </return>
LevelComponents::EntitySet *SpritesEditorDialog::FindCurEntitySet()
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

void SpritesEditorDialog::on_pushButton_ResetLoadTable_clicked()
{
    // Find if new entityset data exist
    LevelComponents::EntitySet *curEntityset = nullptr;
    int entitysetId = currentEntitySetID;

    auto entitySetFound = std::find_if(entitiesAndEntitySetsEditParam->entitySets.begin(),
                                    entitiesAndEntitySetsEditParam->entitySets.end(),
        [entitysetId](LevelComponents::EntitySet *entityset) {return entityset->GetEntitySetId() == entitysetId;});
    int spritesetIdInChangelist = std::distance(entitiesAndEntitySetsEditParam->entitySets.begin(), entitySetFound);

    // If the current entity has no new unsaved instance in the dialog
    if(entitySetFound == entitiesAndEntitySetsEditParam->entitySets.end())
    {
        curEntityset = new LevelComponents::EntitySet(*ROMUtils::entitiessets[currentEntitySetID]); // create new instance
        entitiesAndEntitySetsEditParam->entitySets.push_back(curEntityset);
    }
    else
    {
        curEntityset = entitiesAndEntitySetsEditParam->entitySets[spritesetIdInChangelist];
    }

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
