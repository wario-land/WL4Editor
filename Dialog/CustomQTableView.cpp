#include "Dialog/CustomQTableView.h"
#include <QTableView>
#include <QStyledItemDelegate>
#include <QMenu>
#include <iostream>
#include <QPainter>
#include <QKeyEvent>

CustomQTableView::CustomQTableView(QWidget *parent) :
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

void CustomQTableView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
    QString qs=topLeft.data().value<QString>();
    std::string utf8_text = qs.toUtf8().constData();
    std::cout << "Changed here : " << utf8_text << std::endl;
}

void CustomQTableView::showContextMenu(const QPoint &pos) {
    menu->exec(this->mapToGlobal(pos),action_one_tile);
}

void CustomQTableView::doAction(QAction * action) {
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
                 this->model()->setData(model_index,QColor("#FFFFFF"), Qt::BackgroundRole);
                 this->model()->setData(model_index,"", Qt::DisplayRole);
             }
         }
     }

}

void CustomQTableView::keyPressEvent(QKeyEvent *event) {
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
    }
}




