#include "PatchManagerTableView.h"
#include "WL4EditorWindow.h"
#include <cassert>
#include <QModelIndex>

extern WL4EditorWindow *singleton;

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
    for(struct PatchEntryItem patch : patches)
    {
        EntryTableModel.AddEntry(patch);
    }

    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    setVerticalHeader(new PersistentHeader(Qt::Vertical, this));
    UpdateTableView();
}

/// <summary>
/// Update the underlying table based on the patch entry contents
/// </summary>
void PatchManagerTableView::UpdateTableView()
{
    // Populate the table header
    EntryTableModel.clear();
    EntryTableModel.setHorizontalHeaderLabels(QStringList() <<
        "File" << "Type" << "Hook address" << "Patch address" << "Hook string" << "Hook length" << "Address offset");
    int row = 0;

    // Populate the table items
    for(const struct PatchEntryItem patchEntry : EntryTableModel.entries)
    {
        EntryTableModel.setItem(row, 0, new QStandardItem(patchEntry.FileName.length() ? patchEntry.FileName : "(no file)"));
        const char *typeStrings[3] =
        {
            "Binary",
            "Assembly",
            "C"
        };
        if(patchEntry.PatchType >= sizeof(typeStrings) / sizeof(typeStrings[0]))
        {
            singleton->GetOutputWidgetPtr()->PrintString("Internal error: Patch entry type out of range: " + QString::number(patchEntry.PatchType));
            continue;
        }
        EntryTableModel.setItem(row, 1, new QStandardItem(QString(typeStrings[patchEntry.PatchType])));
        EntryTableModel.setItem(row, 2, new QStandardItem("0x" + QString::number(patchEntry.HookAddress, 16).toUpper()));
        EntryTableModel.setItem(row, 3, new QStandardItem(!patchEntry.PatchAddress ?
            "N/A" : "0x" + QString::number(patchEntry.PatchAddress, 16).toUpper()));
        EntryTableModel.setItem(row, 4, new QStandardItem(patchEntry.HookString));
        EntryTableModel.setItem(row, 5, new QStandardItem(QString::number(patchEntry.GetHookLength(), 10).toUpper()));
        EntryTableModel.setItem(row, 6, new QStandardItem(patchEntry.PatchOffsetInHookString == (unsigned int) -1 ?
            "no patch addr" : QString::number(patchEntry.PatchOffsetInHookString, 10).toUpper()));
        ++row;
    }

    // Resize the table columns
    int remain = width(), colCount = EntryTableModel.columnCount();
    for(int i = 0; i < colCount; ++i)
    {
        if(i == colCount - 1)
        {
            setColumnWidth(i, remain);
        }
        else
        {
            int w = width() / colCount;
            setColumnWidth(i, w);
            remain -= w;
        }
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
    if(selectedRows.size() != 1)
    {
        singleton->GetOutputWidgetPtr()->PrintString("Internal error: PatchManagerTableView::GetSelectedEntry called when a single row is not selected");
        if(!selectedRows.size()) selectedRows.append(QModelIndex());
    }
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
