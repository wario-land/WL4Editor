#include "Dialog/CustomQTableView.h"
#include <QTableView>
#include <QStyledItemDelegate>
#include <iostream>

CustomQTableView::CustomQTableView(QWidget *parent) :
    QTableView(parent)
{
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showContextMenu(const QPoint &)));
}

void CustomQTableView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
    QString qs=topLeft.data().value<QString>();
    std::string utf8_text = qs.toUtf8().constData();
    std::cout << "Changed here : " << utf8_text << std::endl;
}

void CustomQTableView::showContextMenu(const QPoint &) {
    std::cout << "Menu here" << std::endl;
}
