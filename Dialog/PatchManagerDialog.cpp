#include "PatchManagerDialog.h"
#include "ui_PatchManagerDialog.h"
#include <QMessageBox>
#include <QFile>

/// <summary>
/// Construct an instance of the PatchManagerDialog.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
PatchManagerDialog::PatchManagerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PatchManagerDialog)
{
    ui->setupUi(this);

    setWindowTitle("Patch Manager");
    PatchTable = ui->patchManagerTableView;
}

/// <summary>
/// Deconstruct the PatchManagerDialog and clean up its instance objects on the heap.
/// </summary>
PatchManagerDialog::~PatchManagerDialog()
{
    delete ui;
}

/// <summary>
/// Deconstruct the PatchManagerDialog and clean up its instance objects on the heap.
/// </summary>
void PatchManagerDialog::SetButtonsEnabled(bool enable)
{
    ui->removePatchButton->setEnabled(enable);
    ui->editPatchButton->setEnabled(enable);
    ui->savePatchButton->setEnabled(enable);
}

/// <summary>
/// This slot function will be triggered when clicking the TableView.
/// </summary>
void PatchManagerDialog::on_patchManagerTableView_clicked(const QModelIndex &index)
{
    (void) index;
    QItemSelectionModel *select = PatchTable->selectionModel();
    QModelIndexList selectedRows = select->selectedRows();
    bool selected = selectedRows.size();
    SetButtonsEnabled(selected);
}

/// <summary>
/// This slot function will be triggered when clicking the "Add" button.
/// </summary>
void PatchManagerDialog::on_addPatchButton_clicked()
{
    // Get a list of entries for validation later
    QVector<struct PatchEntryItem> currentEntries = PatchTable->GetAllEntries();
    struct PatchEntryItem entry;

    // Execute the edit dialog
    PatchEditDialog *editDialog = new PatchEditDialog(this);
retry:
    if(editDialog->exec() == QDialog::Accepted)
    {
        entry = editDialog->CreatePatchEntry();

        // Validate that the entry can be added
        QFile file(entry.FileName);
        if(!file.exists())
        {
            QMessageBox::information(this, "About", QString("File does not exist: ") + entry.FileName);
            goto error;
        }
        bool fileNameIsValid = !std::any_of(currentEntries.begin(), currentEntries.end(),
            [entry](struct PatchEntryItem e){ return e.FileName == entry.FileName; });
        if(!fileNameIsValid)
        {
            QMessageBox::information(this, "About", QString("Another entry already exists with the filename: ") + entry.FileName);
            goto error;
        }
        bool hookIsValid = !entry.HookAddress || !std::any_of(currentEntries.begin(), currentEntries.end(),
            [entry](struct PatchEntryItem e){ return e.HookAddress == entry.HookAddress; });
        if(!hookIsValid)
        {
            QMessageBox::information(this, "About", QString("Another entry already exists with the hook address: ") + QString::number(entry.HookAddress));
            goto error;
        }

        PatchTable->AddEntry(entry);
    }
    delete editDialog;
    return;

error:
    // Re-run the edit dialog
    delete editDialog;
    editDialog = new PatchEditDialog(this, entry);
    goto retry;
}

/// <summary>
/// This slot function will be triggered when clicking the "Edit" button.
/// </summary>
void PatchManagerDialog::on_editPatchButton_clicked()
{
    // Validate that only one row is selected in the table
    QItemSelectionModel *select = PatchTable->selectionModel();
    QModelIndexList selectedRows = select->selectedRows();
    if(selectedRows.size() == 1)
    {
        // Get list of entries, which does NOT include the selected entry, for validation later
        QVector<struct PatchEntryItem> currentEntries = PatchTable->GetAllEntries();
        struct PatchEntryItem selectedEntry = PatchTable->GetSelectedEntry();
        struct PatchEntryItem entry;
        int selectedIndex = -1;
        std::find_if(currentEntries.begin(), currentEntries.end(),
            [selectedEntry, &selectedIndex](struct PatchEntryItem e){ ++selectedIndex; return selectedEntry.FileName == e.FileName; });
        currentEntries.remove(selectedIndex);

        // Execute the edit dialog
        PatchEditDialog *editDialog = new PatchEditDialog(this, selectedEntry);
retry:
        if(editDialog->exec() == QDialog::Accepted)
        {
            entry = editDialog->CreatePatchEntry();

            // Validate that the entry can be added
            QFile file(entry.FileName);
            if(!file.exists())
            {
                QMessageBox::information(this, "About", QString("File does not exist: ") + entry.FileName);
                goto error;
            }
            bool fileNameIsValid = !std::any_of(currentEntries.begin(), currentEntries.end(),
                [entry](struct PatchEntryItem e){ return e.FileName == entry.FileName; });
            if(!fileNameIsValid)
            {
                QMessageBox::information(this, "About", QString("Another entry already exists with the filename: ") + entry.FileName);
                goto error;
            }
            bool hookIsValid = !entry.HookAddress || !std::any_of(currentEntries.begin(), currentEntries.end(),
                [entry](struct PatchEntryItem e){ return e.HookAddress == entry.HookAddress; });
            if(!hookIsValid)
            {
                QMessageBox::information(this, "About", QString("Another entry already exists with the hook address: ") + QString::number(entry.HookAddress));
                goto error;
            }

            PatchTable->UpdateEntry(selectedIndex, entry);
        }
        delete editDialog;
        return;

error:
        // Re-run the edit dialog
        delete editDialog;
        editDialog = new PatchEditDialog(this, entry);
        goto retry;
    }
    else if(!selectedRows.size())
    {
        QMessageBox::information(this, "About", "You must select a row to edit.");
    }
    else
    {
        QMessageBox::information(this, "About", "You may only select 1 row at a time to edit.");
    }
}

/// <summary>
/// This slot function will be triggered when clicking the "Remove" button.
/// </summary>
void PatchManagerDialog::on_removePatchButton_clicked()
{
    PatchTable->RemoveSelected();
    ui->removePatchButton->setEnabled(false);
    ui->editPatchButton->setEnabled(false);
}

/// <summary>
/// This slot function will be triggered when clicking the "Save" button.
/// </summary>
void PatchManagerDialog::on_savePatchButton_clicked()
{
    // TODO
}
