#ifndef SCRIPTINTERFACE_H
#define SCRIPTINTERFACE_H

#include <QObject>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <string>
#include <QTextStream>

class ScriptInterface : public QObject
{
    Q_OBJECT
public:
    explicit ScriptInterface(QObject *parent = nullptr);

    // Getter
    Q_INVOKABLE int GetCurRoomWidth();
    Q_INVOKABLE int GetCurRoomHeight();
    Q_INVOKABLE int GetCurRoomTile16(int layerID, int x, int y);

    // Test
    Q_INVOKABLE void Test_DecompressData(int mappingtype, int address);
    Q_INVOKABLE unsigned int Test_GetLayerDecomdataPointer(int layerId);
    Q_INVOKABLE void Test_ExportLayerData();
    Q_INVOKABLE void Test_ImportLayerData();
    Q_INVOKABLE void Test_ExportEntityListData();
    Q_INVOKABLE void Test_ImportEntityListData();

    // Setter
    Q_INVOKABLE void SetCurRoomTile16(int layerID, int TileID, int x, int y);
    Q_INVOKABLE void SetRoomSize(int roomwidth, int roomheight, int layer0width, int layer0height);

    // Localize JS function
    Q_INVOKABLE void alert(QString message);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void log(QString message);
    Q_INVOKABLE QString prompt(QString message, QString defaultInput);

    // UI
    Q_INVOKABLE void UpdateRoomGFXFull();

    // File operations
    Q_INVOKABLE void WriteTxtFile(QString filepath, QString test);
    Q_INVOKABLE QString ReadTxtFile(QString filepath);

signals:

};

#endif // SCRIPTINTERFACE_H
