#include "PatchManagerDialog.h"
#include "ui_PatchManagerDialog.h"
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <ROMUtils.h>

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
/// Obtain the part of a file path which is relative to the currently opened ROM file.
/// </summary>
/// <param name="filePath">
/// The absolute path to the file.
/// </param>
static QString GetPathRelativeToROM(const QString &filePath)
{
    QDir ROMdir(ROMUtils::ROMFilePath);
    ROMdir.cdUp();
    if(filePath.startsWith(ROMdir.absolutePath()))
    {
        return filePath.right(filePath.length() - ROMdir.absolutePath().length() - 1);
    }
    else
    {
        return "";
    }
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
        QString relativeFN = GetPathRelativeToROM(entry.FileName);
        if(relativeFN == "")
        {
            QMessageBox::information(this, "About", QString("File must be within directory subtree of ROM file: ") + ROMUtils::ROMFilePath);
            goto error;
        }
        entry.FileName = relativeFN;
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
            QString relativeFN = GetPathRelativeToROM(entry.FileName);
            if(relativeFN == "")
            {
                QMessageBox::information(this, "About", QString("File must be within directory subtree of ROM file: ") + ROMUtils::ROMFilePath);
                goto error;
            }
            entry.FileName = relativeFN;
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
    // TODO attempt to obtain this setting from WL4Editor.ini once it is implemented
    QString EABIpath;
retry:
    bool ok = PatchUtils::VerifyEABI(&EABIpath);
    if(!ok)
    {

    }
}

//---------------------------------------------------------------------------------------------------------------------------
// EABIPrompt functions
//---------------------------------------------------------------------------------------------------------------------------

/// <summary>
/// Construct an instance of the EABIPrompt.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
EABIPrompt::EABIPrompt(QWidget *parent) : QDialog(parent)
{
    // TODO setup prompt
}
