#include "Dialog/CreditEditor_TableView.h"
#include "Dialog/CreditsEditDialog.h"
#include <QTableView>
#include <QStyledItemDelegate>
#include <QMenu>
#include <QPainter>
#include <QKeyEvent>

#define WHITECOLOR "#ffffff"
#define BLACKCOLOR "#000000"
#define DARKBACKGROUNDCOLOR "#3b3c3d"

//Used for describing one tile
#define BLUECOLOR "#8080ff"
//Used for describing uppertile
#define REDCOLOR "#ff8080"
//Used for describing lowertile
#define GREENCOLOR "#80ff80"


/// <summary>
/// The main view who is also a QTableView, there is one for each tab (so one for each credit screen)
/// </summary>
/// <param name="parent">
/// The parent object
/// </param>
CreditEditor_TableView::CreditEditor_TableView(QWidget *parent) :
    QTableView(parent)
{
    menu=new QMenu("Menu", this);
    action_one_tile = new QAction("One Tile",menu);
    action_upper_tile = new QAction("Upper Tile",menu);
    action_lower_tile = new QAction("Lower Tile",menu);
    action_delete = new QAction("Delete",menu);

    menu->addAction(action_one_tile);
    menu->addAction(action_upper_tile);
    menu->addAction(action_lower_tile);
    menu->addAction(action_delete);

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showContextMenu(const QPoint &)));
    connect(menu, SIGNAL(triggered(QAction *)), this, SLOT(doAction(QAction *)));
}

/// <summary>
/// Called each time a cell is changed
/// </summary>
/// <param name="topLeft">
/// The top left QModelIndex
/// <param name="bottomRight">
/// Unused
/// <param name="roles">
/// Unused
/// </param>
void CreditEditor_TableView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    (void) bottomRight;
    (void) roles;

    QString firstLetter=topLeft.data(Qt::DisplayRole).value<QString>();
    QString role=topLeft.data(Qt::BackgroundRole).value<QColor>().name();

    std::string displayStringUppercase = firstLetter.toUpper().toUtf8().constData();

    if (displayStringUppercase.size() > 0) {
        std::string roleString = role.toUtf8().constData();
        std::string firstLetterString(1,displayStringUppercase.at(0));

        //If the tile is not valid
        if (CreditsEditDialog::CreditTileReverseMap[std::make_pair(firstLetterString,roleString)] == 0)
        {

            //If the tile is still not valid with the default background we delete it
            if (CreditsEditDialog::CreditTileReverseMap[std::make_pair(firstLetterString,BLUECOLOR)] == 0)
            {
                deleteFunction(topLeft);
            }
        }
    }

}

/// <summary>
/// Show a popup menu when a right click is made on the QTableView.
/// </summary>
/// <param name="pos">
/// The position of cursor.
/// </param>
void CreditEditor_TableView::showContextMenu(const QPoint &pos)
{
    menu->exec(this->mapToGlobal(pos),action_one_tile);
}

/// <summary>
/// When the popup menu is clicked set selected cells in the described category
/// </summary>
/// <param name="action">
/// Action from the popup menu (One Tile, Upper Tile, Lower Tile, Delete).
/// </param>
void CreditEditor_TableView::doAction(QAction * action)
{
     QString actionText=action->text();

     QModelIndexList index_list=this->selectionModel()->selectedIndexes();
     for (int i=0;i<index_list.size();i++) {
         QModelIndex modelIndex=index_list.at(i);
         int column=modelIndex.column();

         //If we are in the editable zone
         if (column < 30) {
             if (actionText == "One Tile")
             {
                 this->model()->setData(modelIndex,QColor(BLUECOLOR), Qt::BackgroundRole);
             } else if (actionText == "Upper Tile")
             {
                 this->model()->setData(modelIndex,QColor(REDCOLOR), Qt::BackgroundRole);
             } else if (actionText == "Lower Tile")
             {
                 this->model()->setData(modelIndex,QColor(GREENCOLOR), Qt::BackgroundRole);
             } else if (actionText == "Delete")
             {
                deleteFunction(modelIndex);
             }
         }
     }

}

/// <summary>
/// Triggered when a key is pressed, makes the delete key remove selected cells or
/// insert letters or use directionnal keys to move
/// </summary>
/// <param name="event">
/// An event used to know the key value
/// </param>
void CreditEditor_TableView::keyPressEvent(QKeyEvent *event)
{
    int keycode=event->key();
    if (keycode == Qt::Key_Delete)
    {
        QModelIndexList indexList=this->selectionModel()->selectedIndexes();
        for (int i=0;i<indexList.size();i++)
        {
            QModelIndex modelIndex=indexList.at(i);
            int column=modelIndex.column();

            //If we are in the editable zone
            if (column < 30)
            {
                deleteFunction(modelIndex);
            }
        }
    } else if (keycode >= Qt::Key_A && keycode <=Qt::Key_Z)
    {
        QModelIndexList indexList=this->selectionModel()->selectedIndexes();
        for (int i=0;i<indexList.size();i++)
        {
            QModelIndex modelIndex=indexList.at(i);
            int column=modelIndex.column();

            //If we are in the editable zone
            if (column < 30)
            {

                //Getting the background string
                QString role=modelIndex.data(Qt::BackgroundRole).value<QColor>().name();
                std::string role_string = role.toUtf8().constData();

                //Write the letter in the cell
                this->model()->setData(modelIndex,event->text().toUpper(), Qt::DisplayRole);

                //If there is no backgroud, we take the one tile layout by default
                if (role_string == WHITECOLOR || role_string == DARKBACKGROUNDCOLOR || role_string == BLACKCOLOR)
                {
                    this->model()->setData(modelIndex,QColor(BLUECOLOR), Qt::BackgroundRole);
                }

                //Selecting next index
                if (indexList.size() == 1)
                {
                    QModelIndex modelIndex=indexList.at(0);
                    int column=modelIndex.column();
                    int row=modelIndex.row();
                    column++;

                    //Column 30 and 31 can be selected but not modified
                    QModelIndex newindex=this->model()->index(row,column);
                    this->selectionModel()->select(newindex,QItemSelectionModel::SelectCurrent);

                }
            }
        }
    } else if (keycode == Qt::Key_Left)
    {
        QModelIndexList indexList=this->selectionModel()->selectedIndexes();
        if (indexList.size() == 1)
        {
            QModelIndex modelIndex=indexList.at(0);
            int column=modelIndex.column();
            int row=modelIndex.row();
            column--;

            if (column >= 0)
            {
                QModelIndex newindex=this->model()->index(row,column);
                this->selectionModel()->select(newindex,QItemSelectionModel::SelectCurrent);
            }
        }
    } else if (keycode == Qt::Key_Right || keycode == Qt::Key_Space)
    {
        QModelIndexList indexList=this->selectionModel()->selectedIndexes();
        if (indexList.size() == 1)
        {
            QModelIndex modelIndex=indexList.at(0);
            int column=modelIndex.column();
            int row=modelIndex.row();
            column++;

            if (column < 32)
            {
                QModelIndex newindex=this->model()->index(row,column);
                this->selectionModel()->select(newindex,QItemSelectionModel::SelectCurrent);
            }
        }
    } else if (keycode == Qt::Key_Up)
    {
        QModelIndexList indexList=this->selectionModel()->selectedIndexes();
        if (indexList.size() == 1)
        {
            QModelIndex modelIndex=indexList.at(0);
            int column=modelIndex.column();
            int row=modelIndex.row();
            row--;

            if (row >= 0)
            {
                QModelIndex newindex=this->model()->index(row,column);
                this->selectionModel()->select(newindex,QItemSelectionModel::SelectCurrent);
            }
        }
    } else if (keycode == Qt::Key_Down)
    {
        QModelIndexList indexList=this->selectionModel()->selectedIndexes();
        if (indexList.size() == 1)
        {
            QModelIndex modelIndex=indexList.at(0);
            int column=modelIndex.column();
            int row=modelIndex.row();
            row++;

            if (row < 20)
            {
                QModelIndex newindex=this->model()->index(row,column);
                this->selectionModel()->select(newindex,QItemSelectionModel::SelectCurrent);
            }
        }
    }
}

/// <summary>
/// Delete function used (called by delete key or right click->delete)
/// Delete is consistent across theme
/// </summary>
/// <param name="indexToDelete">
/// An index of the cell to delete
/// </param>
void CreditEditor_TableView::deleteFunction(QModelIndex indexToDelete)
{
    int light = 0;
    if(light != SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(6)).toInt())
    {
        this->model()->setData(indexToDelete, QColor(DARKBACKGROUNDCOLOR), Qt::BackgroundRole); // perhaps inconsistant, idk, but looks not bad
    } else
    {
        this->model()->setData(indexToDelete, QColor(WHITECOLOR), Qt::BackgroundRole);
    }
    this->model()->setData(indexToDelete, "", Qt::DisplayRole);
}






