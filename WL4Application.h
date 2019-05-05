#ifndef WL4APPLICATION_H
#define WL4APPLICATION_H

#include <QApplication>

class WL4Application : public QApplication
{
    Q_OBJECT

private:
    bool notify(QObject *receiver, QEvent *event)
    {
        // TODO
    }

public:
    WL4Application(int argc, char **argv) : QApplication(argc, argv) {}
};

#endif // WL4APPLICATION_H
