#ifndef HEXEDITORWINDOW_H
#define HEXEDITORWINDOW_H

#include <QMainWindow>

#include "document/buffer/qmemorybuffer.h"
#include "qhexview.h"

namespace Ui {
class HexEditorWindow;
}

class HexEditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit HexEditorWindow(QWidget *parent = nullptr);
    ~HexEditorWindow();
    void ReLoadFile(QString filePath = "");

private slots:
    void on_actionSave_changes_triggered();

private:
    Ui::HexEditorWindow *ui;
    QHexDocument* currentDocument = nullptr;
};

#endif // HEXEDITORWINDOW_H
