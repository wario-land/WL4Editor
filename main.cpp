#include "WL4EditorWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WL4EditorWindow w;
    w.show();

    return a.exec();
}
