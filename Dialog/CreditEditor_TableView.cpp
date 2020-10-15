#include "Dialog/CreditEditor_TableView.h"
#include <QTableView>
#include <QStyledItemDelegate>
#include <QMenu>
#include <iostream>
#include <QPainter>
#include <QKeyEvent>

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
void CreditEditor_TableView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
    QString qs=topLeft.data().value<QString>();
    std::string utf8_text = qs.toUtf8().constData();
    //std::cout << "Changed here : " << utf8_text << std::endl;
}


/// <summary>
/// Show a popup menu when a right click is made on the QTableView.
/// </summary>
/// <param name="pos">
/// The position of cursor.
/// </param>
void CreditEditor_TableView::showContextMenu(const QPoint &pos) {
    menu->exec(this->mapToGlobal(pos),action_one_tile);
}


/// <summary>
/// When the popup menu is clicked set selected cells in the described category
/// </summary>
/// <param name="action">
/// Action from the popup menu (One Tile, Upper Tile, Lower Tile, Delete).
/// </param>
void CreditEditor_TableView::doAction(QAction * action) {
     QString action_text=action->text();

     QModelIndexList index_list=this->selectionModel()->selectedIndexes();
     for (int i=0;i<index_list.size();i++) {
         QModelIndex model_index=index_list.at(i);
         int column=model_index.column();

         //If we are in the editable zone
         if (column < 30) {
             if (action_text.compare("One Tile") == 0) {
                 this->model()->setData(model_index,QColor("#8080FF"), Qt::BackgroundRole);
             } else if (action_text.compare("Upper Tile") == 0) {
                 this->model()->setData(model_index,QColor("#FF8080"), Qt::BackgroundRole);
             } else if (action_text.compare("Lower Tile") == 0) {
                 this->model()->setData(model_index,QColor("#80FF80"), Qt::BackgroundRole);
             } else if (action_text.compare("Delete") == 0) {
                 if(int light = 0; light != SettingsUtils::GetKey(static_cast<SettingsUtils::IniKeys>(6)).toInt()) {
                     this->model()->setData(model_index, QColor(53, 54, 55), Qt::BackgroundRole); // perhaps inconsistant, idk, but looks not bad
                 } else {
                     this->model()->setData(model_index, QColor("#FFFFFF"), Qt::BackgroundRole);
                 }
                 this->model()->setData(model_index, "", Qt::DisplayRole);
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
void CreditEditor_TableView::keyPressEvent(QKeyEvent *event) {
    int keycode=event->key();
    if (keycode == Qt::Key_Delete) {
        QModelIndexList index_list=this->selectionModel()->selectedIndexes();
        for (int i=0;i<index_list.size();i++) {
            QModelIndex model_index=index_list.at(i);
            int column=model_index.column();

            //If we are in the editable zone
            if (column < 30) {
                this->model()->setData(model_index,QColor("#FFFFFF"), Qt::BackgroundRole);
                this->model()->setData(model_index,"", Qt::DisplayRole);
            }
        }
    } else if (keycode >= Qt::Key_A && keycode <=Qt::Key_Z) {
        QModelIndexList index_list=this->selectionModel()->selectedIndexes();
        for (int i=0;i<index_list.size();i++) {
            QModelIndex model_index=index_list.at(i);
            int column=model_index.column();

            //If we are in the editable zone
            if (column < 30) {
                this->model()->setData(model_index,event->text().toUpper(), Qt::DisplayRole);
                if (index_list.size() == 1) {
                    QModelIndex model_index=index_list.at(0);
                    int column=model_index.column();
                    int row=model_index.row();
                    column++;

                    QModelIndex newindex=this->model()->index(row,column);
                    this->selectionModel()->select(newindex,QItemSelectionModel::SelectCurrent);
                }
            }
        }
    } else if (keycode == Qt::Key_Left) {
        QModelIndexList index_list=this->selectionModel()->selectedIndexes();
        if (index_list.size() == 1) {
            QModelIndex model_index=index_list.at(0);
            int column=model_index.column();
            int row=model_index.row();
            column--;

            if (column >= 0) {
                QModelIndex newindex=this->model()->index(row,column);
                this->selectionModel()->select(newindex,QItemSelectionModel::SelectCurrent);
            }
        }
    } else if (keycode == Qt::Key_Right) {
        QModelIndexList index_list=this->selectionModel()->selectedIndexes();
        if (index_list.size() == 1) {
            QModelIndex model_index=index_list.at(0);
            int column=model_index.column();
            int row=model_index.row();
            column++;

            if (column < 32) {
                QModelIndex newindex=this->model()->index(row,column);
                this->selectionModel()->select(newindex,QItemSelectionModel::SelectCurrent);
            }
        }
    } else if (keycode == Qt::Key_Up) {
        QModelIndexList index_list=this->selectionModel()->selectedIndexes();
        if (index_list.size() == 1) {
            QModelIndex model_index=index_list.at(0);
            int column=model_index.column();
            int row=model_index.row();
            row--;

            if (row >= 0) {
                QModelIndex newindex=this->model()->index(row,column);
                this->selectionModel()->select(newindex,QItemSelectionModel::SelectCurrent);
            }
        }
    } else if (keycode == Qt::Key_Down) {
        QModelIndexList index_list=this->selectionModel()->selectedIndexes();
        if (index_list.size() == 1) {
            QModelIndex model_index=index_list.at(0);
            int column=model_index.column();
            int row=model_index.row();
            row++;

            if (row < 20) {
                QModelIndex newindex=this->model()->index(row,column);
                this->selectionModel()->select(newindex,QItemSelectionModel::SelectCurrent);
            }
        }
    }
}




