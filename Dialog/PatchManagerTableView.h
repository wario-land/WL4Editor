#ifndef PATCHMANAGERTABLEVIEW_H
#define PATCHMANAGERTABLEVIEW_H

#include <QTableView>
#include <QStandardItemModel>
#include <PatchUtils.h>

namespace Ui {
    class PatchManagerTableView;
}

class PatchEntryTableModel : public QStandardItemModel
{
    Q_OBJECT

public:
    PatchEntryTableModel(QWidget *_parent);
    ~PatchEntryTableModel() { };
    void AddEntry(struct PatchEntryItem entry) { entries.append(entry); }
    QVector<PatchEntryItem> entries;

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
};

#endif // PATCHMANAGERTABLEVIEW_H
