#include "EntitySetDockWidget.h"
#include "ui_EntitySetDockWidget.h"

EntitySetDockWidget::EntitySetDockWidget(QWidget *parent) : QDockWidget(parent), ui(new Ui::EntitySetDockWidget)
{
    ui->setupUi(this);
    ui->graphicsView_CurrentEntity->scale(2, 2);
}

EntitySetDockWidget::~EntitySetDockWidget() { delete ui; }

/// <summary>
/// Reset EntitySet in the Dock Widget.
/// </summary>
/// <param name="currentroom">
/// The ptr of the current Room instance, don't delete it in this class.
/// </param>
void EntitySetDockWidget::ResetEntitySet(LevelComponents::Room *currentroom)
{
    currentRoom = currentroom;
    ui->label_EntitySetID->setText("EntitySet ID: 0x" + QString::number(currentroom->GetCurrentEntitySetID(), 16));
    EntityMaxNum = currentroom->GetCurrentEntityListSource().size() - 1;
    currentEntityId = 0;
    RenderEntityAndResetInfo();
    ui->pushButton_PreviousEntity->setEnabled(false);
    ui->pushButton_NextEntity->setEnabled(true);
}

/// <summary>
/// Reset Entity ID and update UI in the Dock Widget.
/// </summary>
/// <param name="entityindex">
/// Provide a new entity id to set current entity in the entityset.
/// </param>
void EntitySetDockWidget::SetCurrentEntity(int entityindex)
{
    // Bad Input
    if (entityindex > EntityMaxNum || entityindex < 0)
        return;

    ui->pushButton_PreviousEntity->setEnabled(entityindex > 0 ? true : false);
    ui->pushButton_NextEntity->setEnabled(entityindex < EntityMaxNum ? true : false);
    currentEntityId = entityindex;
    RenderEntityAndResetInfo();
}

/// <summary>
/// Redraw Entity on graphicsView_CurrentEntity and reset its info.
/// </summary>
void EntitySetDockWidget::RenderEntityAndResetInfo()
{
    // Render Entity
    QGraphicsScene *scene = ui->graphicsView_CurrentEntity->scene();
    if (scene)
    {
        delete scene;
    }
    LevelComponents::Entity *currentEntityPtr = currentRoom->GetCurrentEntityListSource()[currentEntityId];
    QImage EntityImage = currentEntityPtr->Render();
    int Entitywidth, Entityheight;
    Entitywidth = EntityImage.width();
    Entityheight = EntityImage.height();
    scene = new QGraphicsScene(0, 0, Entitywidth, Entityheight);
    QPixmap pixmap(Entitywidth, Entityheight);
    pixmap.fill(Qt::transparent);
    QPainter Entitypainter(&pixmap);
    QPoint drawDestination(0, 0);
    Entitypainter.drawImage(drawDestination, EntityImage);
    QPixmap pixmap2(Entitywidth, Entityheight);
    pixmap2.fill(Qt::transparent);
    QPainter EntityBoxPainter(&pixmap2);
    QPen EntityBoxPen = QPen(QBrush(QColor(0xFF, 0xFF, 0, 0xFF)), 2);
    EntityBoxPen.setJoinStyle(Qt::MiterJoin);
    EntityBoxPainter.setPen(EntityBoxPen);
    LevelComponents::EntityPositionalOffset position =
        LevelComponents::Entity::GetEntityPositionalOffset(currentEntityPtr->GetEntityGlobalID());
    EntityBoxPainter.drawRect(-((position.XOffset + 98) / 4 + currentEntityPtr->GetXOffset() + 8),
                              -((position.YOffset + 66) / 4 + currentEntityPtr->GetYOffset() + 16), 16, 16);
    scene->addPixmap(pixmap);
    scene->addPixmap(pixmap2);
    ui->graphicsView_CurrentEntity->setScene(scene);
    ui->graphicsView_CurrentEntity->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // Reset Info
    ui->textEdit_EntityInfo->clear();
    ui->textEdit_EntityInfo->append("Entity local Id: 0x" + QString::number(currentEntityId, 16));
    ui->textEdit_EntityInfo->append("Entity global Id: 0x" + QString::number(currentEntityPtr->GetEntityGlobalID(), 16));
}

/// <summary>
/// Decrease the index of the current Entity and render it.
/// </summary>
/// <remarks>
/// If the current Entity is index 1, then this button will become disabled.
/// The NextEntity button will be enabled.
/// </remarks>
void EntitySetDockWidget::on_pushButton_PreviousEntity_clicked()
{
    ui->pushButton_NextEntity->setEnabled(true);
    if (currentEntityId == 0)
        return;
    --currentEntityId;
    RenderEntityAndResetInfo();
    if (currentEntityId == 0)
        ui->pushButton_PreviousEntity->setEnabled(false);
}

/// <summary>
/// Increase the index of the current Entity and render it.
/// </summary>
/// <remarks>
/// If the current Entity has the max index, then this button will become disabled.
/// The PreviousEntity button will be enabled.
/// </remarks>
void EntitySetDockWidget::on_pushButton_NextEntity_clicked()
{
    ui->pushButton_PreviousEntity->setEnabled(true);
    if (EntityMaxNum == currentEntityId)
        return;
    ++currentEntityId;
    RenderEntityAndResetInfo();
    if (EntityMaxNum == currentEntityId)
        ui->pushButton_NextEntity->setEnabled(false);
}
