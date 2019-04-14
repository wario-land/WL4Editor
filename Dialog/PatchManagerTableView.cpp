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
    entryTableModel(this)
{
    // Configure the table
    setModel(&entryTableModel);
    setSelectionBehavior(SelectionBehavior::SelectRows);

    // Populate the table
    QVector<struct PatchEntryItem> patches = GetPatchesFromROM();
    foreach(struct PatchEntryItem patch, patches)
    {
        entryTableModel.AddEntry(patch);
    }

    // TEST
    struct PatchEntryItem TEST1 { QString("foo.c"), PatchType::C, 0x1111AF, false, 0x800000 };
    struct PatchEntryItem TEST2 { QString("bar.c"), PatchType::C, 0x2222AF, false, 0x800001 };
    struct PatchEntryItem TEST3 { QString("baz.asm"), PatchType::Assembly, 0x3333AF, true, 0x800002 };
    struct PatchEntryItem TEST4 { QString("file.bin"), PatchType::Binary, 0x4444AF, true, 0x800003 };
    entryTableModel.AddEntry(TEST1);
    entryTableModel.AddEntry(TEST2);
    entryTableModel.AddEntry(TEST3);
    entryTableModel.AddEntry(TEST4);

    UpdateTableView();
}

/// <summary>
/// Deconstruct the PatchManagerTableView and clean up its instance objects on the heap.
/// </summary>
PatchManagerTableView::~PatchManagerTableView()
{
    // TODO
}

/// <summary>
/// Update the underlying table based on the patch entry contents
/// </summary>
void PatchManagerTableView::UpdateTableView()
{
    entryTableModel.clear();
    entryTableModel.setHorizontalHeaderLabels(QStringList() << "File" << "Type" << "Hook Address" << "Patch Address" << "Stub");
    int row = 0;
    foreach(const struct PatchEntryItem patchEntry, entryTableModel.entries)
    {
        entryTableModel.setItem(row, 0, new QStandardItem(patchEntry.FileName));
        const char *typeStrings[3] =
        {
            "Binary",
            "Assembly",
            "C"
        };
        assert(patchEntry.PatchType < sizeof(typeStrings) / sizeof(typeStrings[0]) /* Patch entry type out of range */);
        entryTableModel.setItem(row, 1, new QStandardItem(QString(typeStrings[patchEntry.PatchType])));
        entryTableModel.setItem(row, 2, new QStandardItem(!patchEntry.HookAddress ?
            "none" : "0x" + QString::number(patchEntry.HookAddress, 16).toUpper()));
        entryTableModel.setItem(row, 3, new QStandardItem("0x" + QString::number(patchEntry.PatchAddress, 16).toUpper()));
        entryTableModel.setItem(row++, 4, new QStandardItem(patchEntry.StubFunction ? "yes" : "no"));
    }
}

/// <summary>
/// Remove the selected patch entry from the table.
/// </summary>
void PatchManagerTableView::RemoveSelected()
{
    QItemSelectionModel *select = selectionModel();
    QModelIndexList selectedRows = select->selectedRows();
    entryTableModel.RemoveEntries(selectedRows);
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
    return entryTableModel.entries[selectedRows[0].row()];
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
