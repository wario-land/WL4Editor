#include "EditModeDockWidget.h"
#include "ui_EditModeDockWidget.h"

EditModeDockWidget::EditModeDockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::EditModeDockWidget)
{
    ui->setupUi(this);
}

EditModeDockWidget::~EditModeDockWidget()
{
    delete ui;
}
