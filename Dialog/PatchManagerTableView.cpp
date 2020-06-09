#include "PatchManagerTableView.h"
#include <cassert>
#include <QModelIndex>

/// <summary>
/// Construct an instance of the PatchManagerTableView.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
PatchManagerTableView::PatchManagerTableView(QWidget *param) : QTableView(param),
    EntryTableModel(this)
{
    // Configure the table
    setModel(&EntryTableModel);
    setSelectionBehavior(SelectionBehavior::SelectRows);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Populate the table
    QVector<struct PatchEntryItem> patches = PatchUtils::GetPatchesFromROM();
    foreach(struct PatchEntryItem patch, patches)
    {
        EntryTableModel.AddEntry(patch);
    }

    // TODO delete this once the patch manager is complete
    // TEST this is here for quick auto-population of the patch list while testing

    struct PatchEntryItem TEST1 { QString("PatchCode/testfunc.c"), PatchType::C, 0x1F628, "8F69B144A08956BC", 6, 0, "" };
    struct PatchEntryItem TEST2 { QString("PatchCode/UnlimitedRockBouncing.c"), PatchType::C, 4, "8F69B144A08956BC", (unsigned int)-1, 0, "" };
    EntryTableModel.AddEntry(TEST1);
    EntryTableModel.AddEntry(TEST2);

    UpdateTableView();
}

/// <summary>
/// Update the underlying table based on the patch entry contents
/// </summary>
void PatchManagerTableView::UpdateTableView()
{
    EntryTableModel.clear();
    EntryTableModel.setHorizontalHeaderLabels(QStringList() <<
        "File" << "Type" << "Hook Address" << "Patch Address" << "Hook string" << "Patch addr offset");
    int row = 0;
    for(const struct PatchEntryItem patchEntry : EntryTableModel.entries)
    {
        EntryTableModel.setItem(row, 0, new QStandardItem(patchEntry.FileName));
        const char *typeStrings[3] =
        {
            "Binary",
            "Assembly",
            "C"
        };
        assert(patchEntry.PatchType < sizeof(typeStrings) / sizeof(typeStrings[0]) /* Patch entry type out of range */);
        EntryTableModel.setItem(row, 1, new QStandardItem(QString(typeStrings[patchEntry.PatchType])));
        EntryTableModel.setItem(row, 2, new QStandardItem(!patchEntry.HookAddress ?
            "none" : "0x" + QString::number(patchEntry.HookAddress, 16).toUpper()));
        EntryTableModel.setItem(row, 3, new QStandardItem(!patchEntry.PatchAddress ?
            "pending" : "0x" + QString::number(patchEntry.PatchAddress, 16).toUpper()));
        EntryTableModel.setItem(row, 4, new QStandardItem(patchEntry.HookString));
        EntryTableModel.setItem(row, 5, new QStandardItem(patchEntry.PatchOffsetInHookString == (unsigned int) -1 ?
            "no patch addr" : QString::number(patchEntry.PatchOffsetInHookString, 10).toUpper()));
    }
}

/// <summary>
/// Remove the selected patch entry from the table.
/// </summary>
void PatchManagerTableView::RemoveSelected()
{
    QItemSelectionModel *select = selectionModel();
    QModelIndexList selectedRows = select->selectedRows();
    EntryTableModel.RemoveEntries(selectedRows);
    UpdateTableView();
}

/// <summary>
/// Get the selected patch entry from the table.
/// </summary>
/// <returns>
/// The selected patch entry.
/// </returns>
struct PatchEntryItem PatchManagerTableView::GetSelectedEntry()
{
    QItemSelectionModel *select = selectionModel();
    QModelIndexList selectedRows = select->selectedRows();
    assert(selectedRows.size() == 1 /* PatchManagerTableView::GetSelectedEntry called when a single row is not selected */);
    return EntryTableModel.entries[selectedRows[0].row()];
}

/// <summary>
/// Add an entry to the table.
/// </summary>
/// <param name="entry">
/// The entry to add to the table.
/// </param>
void PatchManagerTableView::AddEntry(struct PatchEntryItem entry)
{
    EntryTableModel.AddEntry(entry);
    UpdateTableView();
}

/// <summary>
/// Replace an entry in the table with an updated patch entry struct.
/// </summary>
/// <param name="index">
/// The index to update.
/// </param>
/// <param name="entry">
/// The entry to replace at the index.
/// </param>
void PatchManagerTableView::UpdateEntry(int index, struct PatchEntryItem entry)
{
    EntryTableModel.entries[index] = entry;
    UpdateTableView();
    selectRow(index);
}

//---------------------------------------------------------------------------------------------------------------------------
// PatchEntryTableModel functions
//---------------------------------------------------------------------------------------------------------------------------

/// <summary>
/// Construct an instance of the PatchEntryTableModel.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
PatchEntryTableModel::PatchEntryTableModel(QWidget *_parent) : QStandardItemModel(_parent), parent(_parent)
{
    // Configure the model
    setColumnCount(3);
}

/// <summary>
/// Remove entries from the table model.
/// </summary>
/// <param name="entries">
/// The entries to remove.
/// </param>
void PatchEntryTableModel::RemoveEntries(QModelIndexList entryList)
{
    qSort(entryList.begin(), entryList.end(), qGreater<QModelIndex>()); // so that rows are removed from highest index
    foreach (QModelIndex index, entryList)
    {
        entries.removeAt(index.row());
    }
}
