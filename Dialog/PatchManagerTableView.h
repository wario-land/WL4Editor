#ifndef PATCHMANAGERTABLEVIEW_H
#define PATCHMANAGERTABLEVIEW_H

#include <QTableView>
#include <QStandardItemModel>
#include <PatchUtils.h>

class PatchEntryTableModel : public QStandardItemModel
{
    Q_OBJECT

public:
    PatchEntryTableModel(QWidget *_parent);
    ~PatchEntryTableModel() { };
    void AddEntry(struct PatchEntryItem entry) { entries.append(entry); }
    QVector<struct PatchEntryItem> entries;
    void RemoveEntries(QModelIndexList entries);

private:
    QWidget *parent;
};

class PatchManagerTableView : public QTableView
{
    Q_OBJECT

private:
    PatchEntryTableModel entryTableModel;

public:
    PatchManagerTableView(QWidget *param);
    ~PatchManagerTableView();
    void UpdateTableView();
    void RemoveSelected();
    struct PatchEntryItem GetSelectedEntry();
    QVector<struct PatchEntryItem> GetAllEntries() { return entryTableModel.entries; }
    void AddEntry(struct PatchEntryItem entry) { entryTableModel.AddEntry(entry); UpdateTableView(); }
    void UpdateEntry(int index, struct PatchEntryItem entry)
    {
        entryTableModel.entries[index] = entry;
        UpdateTableView();
        selectRow(index);
    }
};

#endif // PATCHMANAGERTABLEVIEW_H
