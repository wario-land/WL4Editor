#ifndef SCRIPTINTERFACE_H
#define SCRIPTINTERFACE_H

#include <QObject>
#include <QInputDialog>

class ScriptInterface : public QObject
{
    Q_OBJECT
public:
    explicit ScriptInterface(QObject *parent = nullptr);

    // Getter
    Q_INVOKABLE int GetCurRoomWidth();
    Q_INVOKABLE int GetCurRoomHeight();
    Q_INVOKABLE int GetCurRoomTile16(int layerID, int x, int y);

    // Setter
    Q_INVOKABLE void SetCurRoomTile16(int layerID, int TileID, int x, int y);

    // Localize JS function
    Q_INVOKABLE void alert(QString message);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void log(QString message);
    Q_INVOKABLE QString prompt(QString message, QString defaultInput);

    // UI
    Q_INVOKABLE void UpdateRoomGFXFull();

signals:

};

#endif // SCRIPTINTERFACE_H
