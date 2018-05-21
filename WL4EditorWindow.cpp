#include "WL4EditorWindow.h"
#include "ui_WL4EditorWindow.h"

WL4EditorWindow::WL4EditorWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WL4EditorWindow)
{
    ui->setupUi(this);
}

WL4EditorWindow::~WL4EditorWindow()
{
    delete ui;
}
