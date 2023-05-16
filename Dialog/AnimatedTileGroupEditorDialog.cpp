#include "AnimatedTileGroupEditorDialog.h"
#include "ui_AnimatedTileGroupEditorDialog.h"

#include <QScrollBar>
#include <QMessageBox>

#include "ROMUtils.h"
#include "FileIOUtils.h"

/// <summary>
/// Construct an instance of AnimatedTileGroupEditorDialog.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
/// <param name="_currentTileset">
/// To set this->currentTileset for the ref palettes which is needed when import new Tile8x8 data.
/// </param>
AnimatedTileGroupEditorDialog::AnimatedTileGroupEditorDialog(QWidget *parent,
                                                             LevelComponents::Tileset *_currentTileset,
                                                             DialogParams::AnimatedTileGroupsEditParams *param) :
    QDialog(parent),
    ui(new Ui::AnimatedTileGroupEditorDialog),
    currentTileset(_currentTileset),
    animatedTileGroupsEditParam(param)
{
    ui->setupUi(this);

    ui->graphicsView_Tile8x8Array->scale(2, 2);
    ui->graphicsView_RefPalettes->scale(2, 2);

    ui->spinBox_GlobalID->setValue(0);
    ExtractAnimatedTileGroupInfoToUI(0);
    UpdateTileArrayGraphicView(0);
}

/// <summary>
/// Deconstruct an instance of AnimatedTileGroupEditorDialog.
/// </summary>
AnimatedTileGroupEditorDialog::~AnimatedTileGroupEditorDialog()
{
    delete ui;
}

/// <summary>
/// Extract animated Tile group info and tile graphic to UI.
/// </summary>
void AnimatedTileGroupEditorDialog::ExtractAnimatedTileGroupInfoToUI(unsigned int _animatedTileGroup_globalId)
{
    // Set spinboxes
    ui->spinBox_GlobalID->setValue(_animatedTileGroup_globalId);
    auto animatedTileGroupPtr = GetAnimatedTileGroupPtr(_animatedTileGroup_globalId);
    unsigned int type = animatedTileGroupPtr->GetAnimationType();
    ui->spinBox_AnimationType->setValue(type);
    unsigned int countperframe = animatedTileGroupPtr->GetCountPerFrame();
    ui->spinBox_CountPerFrame->setValue(countperframe);
    unsigned int tile16Num = animatedTileGroupPtr->GetTotalFrameCount();
    ui->spinBox_TIle16Num->setValue(tile16Num);

    // Render palettes and TIle8x8 array into graphicviews
    UpdatePaletteGraphicView();
    UpdateTileArrayGraphicView(ui->spinBox_paletteId->value());

    // we should not edit the first entry in the whole list always
    ui->pushButton_ImportAndSetSlot->setEnabled(_animatedTileGroup_globalId);

    // Add Extra Info into the textbox
    ui->textEdit_ExtraInfo->setText("current Animated Tile Group find in Tileset(s): ");
    if (_animatedTileGroup_globalId)
    {
        for (int i = 0; i < (sizeof(ROMUtils::singletonTilesets)) / sizeof(ROMUtils::singletonTilesets[0]); i++)
        {
            unsigned short *_value1_slots = ROMUtils::singletonTilesets[i]->GetAnimatedTileData(0);
            unsigned short *_value2_slots = ROMUtils::singletonTilesets[i]->GetAnimatedTileData(1);
            for (int j = 0; j < 16; j++)
            {
                if (_value1_slots[j] == _animatedTileGroup_globalId || _value2_slots[j] == _animatedTileGroup_globalId)
                {
                    ui->textEdit_ExtraInfo->append("0x" + QString::number(i, 16) + ", ");
                    break;
                }
            }
        }
    }
}

/// <summary>
/// Update Palette graphicview.
/// </summary>
void AnimatedTileGroupEditorDialog::UpdatePaletteGraphicView()
{
    if (ui->graphicsView_RefPalettes->scene())
    {
        delete ui->graphicsView_RefPalettes->scene();
    }
    QGraphicsScene *scene = new QGraphicsScene(0, 0, 16 * 8, 16 * 8);
    ui->graphicsView_RefPalettes->setScene(scene);
    ui->graphicsView_RefPalettes->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // draw palette pixmap
    QPixmap PaletteBarpixmap(8 * 16, 8 * 16);
    PaletteBarpixmap.fill(Qt::transparent);
    QPainter PaletteBarPainter(&PaletteBarpixmap);
    for (int j = 0; j < 16; ++j)
    {
        QVector<QRgb> palettetable = currentTileset->GetPalettes()[j];
        for (int i = 1; i < 16; ++i) // Ignore the first color
        {
            PaletteBarPainter.fillRect(8 * i, 8 * j, 8, 8, palettetable[i]);
        }
    }
    scene->addPixmap(PaletteBarpixmap);
    ui->graphicsView_RefPalettes->verticalScrollBar()->setValue(0);
}

void AnimatedTileGroupEditorDialog::UpdateTileArrayGraphicView(unsigned int paletteId)
{
    // draw TIle8x8 array pixmap
    LevelComponents::AnimatedTile8x8Group *curAnimatedTilegroup = GetAnimatedTileGroupPtr();
    unsigned int tile8x8num = curAnimatedTilegroup->GetTotalFrameCount();

    if (ui->graphicsView_Tile8x8Array->scene())
    {
        delete ui->graphicsView_Tile8x8Array->scene();
    }
    QGraphicsScene *scene = new QGraphicsScene(0, 0, 8 * 4 * tile8x8num, 8);
    ui->graphicsView_Tile8x8Array->setScene(scene);
    ui->graphicsView_Tile8x8Array->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scene->addPixmap(curAnimatedTilegroup->RenderWholeAnimatedTileGroup(currentTileset->GetPalettes(), paletteId));
}

/// <summary>
/// Find and return a suitable AnimatedTile8x8Group pointer points to the AnimatedTile8x8Group with the current id
/// </summary>
/// <return>
/// current AnimatedTile8x8Group pointer
/// </return>
LevelComponents::AnimatedTile8x8Group *AnimatedTileGroupEditorDialog::GetAnimatedTileGroupPtr(bool createNewAnimatedTileGroup)
{
    // Find if new entity data exist
    unsigned int curGlobalId = ui->spinBox_GlobalID->value();
    LevelComponents::AnimatedTile8x8Group *oldanimatedTileGroup = ROMUtils::animatedTileGroups[curGlobalId];
    LevelComponents::AnimatedTile8x8Group *curanimatedTileGroup = oldanimatedTileGroup; // init

    auto animatedTileGroupFound = std::find_if(animatedTileGroupsEditParam->animatedTileGroups.begin(),
                                               animatedTileGroupsEditParam->animatedTileGroups.end(),
        [&oldanimatedTileGroup](LevelComponents::AnimatedTile8x8Group *tmpgroup) {return tmpgroup->GetGlobalID() == oldanimatedTileGroup->GetGlobalID();});
    int IdInChangedlist = std::distance(animatedTileGroupsEditParam->animatedTileGroups.begin(), animatedTileGroupFound);

    // If the current entity has a new unsaved instance in the dialog
    if(animatedTileGroupFound != animatedTileGroupsEditParam->animatedTileGroups.end())
    {
        curanimatedTileGroup = animatedTileGroupsEditParam->animatedTileGroups[IdInChangedlist];
    }
    else
    {
        if (createNewAnimatedTileGroup)
        {
            curanimatedTileGroup = new LevelComponents::AnimatedTile8x8Group(*oldanimatedTileGroup);
            animatedTileGroupsEditParam->animatedTileGroups.push_back(curanimatedTileGroup);
        }
    }
    return curanimatedTileGroup;
}

/// <summary>
/// Set type hint label text.
/// </summary>
void AnimatedTileGroupEditorDialog::on_spinBox_AnimationType_valueChanged(int arg1)
{
    switch (arg1)
    {
    case 0: {ui->label_AnimationTypeHint->setText("No Animation"); break; }
    case 1: {ui->label_AnimationTypeHint->setText("Loop"); break; }
    case 2: {ui->label_AnimationTypeHint->setText("Stop At Min"); break; }
    case 3: {ui->label_AnimationTypeHint->setText("Min To Max Then Stop"); break; }
    case 4: {ui->label_AnimationTypeHint->setText("Back And Forth"); break; }
    case 5: {ui->label_AnimationTypeHint->setText("Max To Min Then Stop"); break; }
    case 6: {ui->label_AnimationTypeHint->setText("Reverse Loop"); break; }
    default: ui->label_AnimationTypeHint->setText("");
    }
    if (ui->spinBox_GlobalID->value() > 0 && arg1 >= 0 && arg1 <= 6)
    {
        LevelComponents::AnimatedTile8x8Group *curAnimatedTileGroup = GetAnimatedTileGroupPtr(true);
        curAnimatedTileGroup->SetAnimationType(ui->spinBox_AnimationType->value());
    }
}

/// <summary>
/// Show all the info using the changed global id.
/// </summary>
void AnimatedTileGroupEditorDialog::on_spinBox_GlobalID_valueChanged(int arg1)
{
    ExtractAnimatedTileGroupInfoToUI(arg1);
}

/// <summary>
/// Render all the Tile8x8s in the current animated tile8x8 group using the reference palette.
/// </summary>
void AnimatedTileGroupEditorDialog::on_spinBox_paletteId_valueChanged(int arg1)
{
    UpdateTileArrayGraphicView(arg1);
}

/// <summary>
/// Import and reset the current animated Tile group entry's Tile data according to its settings in the UI if suitable.
/// </summary>
void AnimatedTileGroupEditorDialog::on_pushButton_ImportAndSetSlot_clicked()
{

    LevelComponents::AnimatedTile8x8Group *curAnimatedTileGroup = GetAnimatedTileGroupPtr(true);
    unsigned int paletteId = ui->spinBox_paletteId->value();
    unsigned int equivalentTile16Num = ui->spinBox_TIle16Num->value();
    bool success = false;
    FileIOUtils::ImportTile8x8GfxData(this,
        currentTileset->GetPalettes()[paletteId],
        tr("Choose a color to covert to transparent:"),
        [&curAnimatedTileGroup, &equivalentTile16Num, &success] (QByteArray finaldata, QWidget *parentPtr)
        {
            // Assume the file is fully filled with tiles
            int newtilenum = finaldata.size() / 32;
            if(newtilenum != (equivalentTile16Num * 4))
            {
                QMessageBox::critical(parentPtr, tr("Load Error"), tr("Incorrect Tile8x8 data length,\n") +
                                                                   tr("the data needs to contains 0x") +
                                      QString::number(equivalentTile16Num * 4, 16) + tr(" Tile8x8s."));
                return;
            }

            // Set tile data for the current animated Tile Group
            curAnimatedTileGroup->SetTileData(finaldata);
            success = true;
        });

    // update and global singleton and the graphicviews
    if (success)
    {
        curAnimatedTileGroup->SetAnimationType(ui->spinBox_AnimationType->value());
        curAnimatedTileGroup->SetCountPerFrame(ui->spinBox_CountPerFrame->value());
        UpdateTileArrayGraphicView(ui->spinBox_paletteId->value());
    }
}


void AnimatedTileGroupEditorDialog::on_spinBox_CountPerFrame_valueChanged(int arg1)
{
    if (ui->spinBox_GlobalID->value() > 0)
    {
        LevelComponents::AnimatedTile8x8Group *curAnimatedTileGroup = GetAnimatedTileGroupPtr(true);
        curAnimatedTileGroup->SetCountPerFrame(arg1);
    }
}

