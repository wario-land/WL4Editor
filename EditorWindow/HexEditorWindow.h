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
    void gotoOffset(unsigned int offset);
    void hightlightData_bg(unsigned int offset, unsigned length, QColor color);
    void hightlightData_fg(unsigned int offset, unsigned length, QColor color);
    void highlightClear();

private slots:
    void on_actionSave_changes_triggered();

private:
    Ui::HexEditorWindow *ui;
    QHexDocument* currentDocument = nullptr;
};

#endif // HEXEDITORWINDOW_H
