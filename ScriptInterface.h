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
    Q_INVOKABLE int GetCurRoomTile16(int layerID, int x, int y);

    // Setter
    Q_INVOKABLE void SetCurRoomTile16(int layerID, int TileID, int x, int y);

    // UI
    Q_INVOKABLE void UpdateRoomGFXFull();

signals:

};

#endif // SCRIPTOBJECTSGROUP_H
