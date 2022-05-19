#ifndef GRAPHICMANAGERDIALOG_H
#define GRAPHICMANAGERDIALOG_H

#include <QDialog>
#include <QModelIndex>
#include <QStandardItem>
#include <QStandardItemModel>
#include "LevelComponents/Tile.h"
#include "ScatteredGraphicUtils.h"

namespace Ui {
class GraphicManagerDialog;
}

class GraphicManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GraphicManagerDialog(QWidget *parent = nullptr);
    ~GraphicManagerDialog();

private slots:
    void on_listView_RecordGraphicsList_clicked(const QModelIndex &index);

private:
    Ui::GraphicManagerDialog *ui;
    QVector<struct ScatteredGraphicUtils::ScatteredGraphicEntryItem> graphicEntries;
    QStandardItemModel *ListViewItemModel = nullptr;
    int SelectedEntryID = -1;

    // functions
    bool UpdateEntryList();
    void ExtractEntryToGUI(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    QPixmap GetPixmap(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
    QString GenerateEntryTextFromStruct(ScatteredGraphicUtils::ScatteredGraphicEntryItem &entry);
};

#endif // GRAPHICMANAGERDIALOG_H
