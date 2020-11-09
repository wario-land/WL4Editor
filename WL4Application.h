#ifndef WL4APPLICATION_H
#define WL4APPLICATION_H

#include <QApplication>

class WL4Application : public QApplication
{
    Q_OBJECT

private:
    bool notify(QObject *receiver, QEvent *event)
    {
        (void) receiver;
        (void) event;
        // TODO
        return false;
    }

public:
    WL4Application(int argc, char **argv) : QApplication(argc, argv) {}
};

#endif // WL4APPLICATION_H
