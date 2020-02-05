#ifndef SCRIPTOBJECTSGROUP_H
#define SCRIPTOBJECTSGROUP_H

#include <QObject>

class ScriptInterface : public QObject
{
    Q_OBJECT
public:
    explicit ScriptInterface(QObject *parent = nullptr);

    // Getter
    Q_INVOKABLE int GetCurRoomWidth();
    Q_INVOKABLE int GetCurRoomHeight();

signals:

};

#endif // SCRIPTOBJECTSGROUP_H
