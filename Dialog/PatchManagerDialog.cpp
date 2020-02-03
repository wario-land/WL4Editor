#include "PatchManagerDialog.h"
#include "ui_PatchManagerDialog.h"
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <ROMUtils.h>
#include <SettingsUtils.h>
#include <QFileDialog>

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
/// This slot function will be triggered when clicking the TableView.
/// </summary>
void PatchManagerDialog::on_patchManagerTableView_clicked(const QModelIndex &index)
{
    (void) index;
    QItemSelectionModel *select = PatchTable->selectionModel();
    QModelIndexList selectedRows = select->selectedRows();
    int num_of_select_rows = selectedRows.size();
    ui->removePatchButton->setEnabled(num_of_select_rows);
    ui->editPatchButton->setEnabled(num_of_select_rows == 1);
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
        if(
            entry.ThumbMode && (entry.HookAddress & 1) || // Mis-aligned instruction check
            !entry.ThumbMode && (entry.HookAddress & 3)   // 2 bytes for thumb, 4 bytes for arm
        ){
            QMessageBox::information(this, "About", QString("Misaligned hook address: ") + QString("%1").arg(entry.HookAddress, 0, 16));
        }
        QFile file(entry.FileName);
        if(!file.exists()) // Patch file must exist
        {
            QMessageBox::information(this, "About", QString("File does not exist: ") + entry.FileName);
            goto error;
        }
        QString relativeFN = GetPathRelativeToROM(entry.FileName);
        if(relativeFN.isEmpty()) // Patch file must be within the directory tree of the ROM file
        {
            QMessageBox::information(this, "About", QString("File must be within directory subtree of ROM file: ") + ROMUtils::ROMFilePath);
            goto error;
        }
        entry.FileName = relativeFN;
        bool fileNameIsValid = !std::any_of(currentEntries.begin(), currentEntries.end(),
            [entry](struct PatchEntryItem e){ return e.FileName == entry.FileName; });
        if(!fileNameIsValid) // Cannot add two patch entries using the same file
        {
            QMessageBox::information(this, "About", QString("Another entry already exists with the filename: ") + entry.FileName);
            goto error;
        }
        bool hookIsValid = !entry.HookAddress || !std::any_of(currentEntries.begin(), currentEntries.end(),
            [entry](struct PatchEntryItem e){ return e.HookAddress == entry.HookAddress; });
        if(!hookIsValid) // Cannot use two patches on same hook address
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
    // First, ensure that the EABI directory is valid
    QString invalidDir;
    QString oldPath = GetKey(SettingsUtils::IniKeys::eabi_binfile_path);
    PatchUtils::EABI_INSTALLATION = oldPath;
    bool ok = PatchUtils::VerifyEABI(&invalidDir);
    while(!ok)
    {
        // Prompt the user to download EABI
        QString msg = !invalidDir.length() ? QString("") :
            QString("Invalid EABI folder selected: " + invalidDir + " (required binaries not found)<br>");
        QString URL = "https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads";
        QMessageBox::information(this, "Select EABI installation",
            msg + "You must select the \"bin\" folder of a valid EABI installation.<br>" +
            "Download: <a href=\"" + URL + "\">" + URL + "</a>");

        // Choose folder for EABI bin directory
        QString selectedDir = QFileDialog::getExistingDirectory(
            this,
            tr("Select EABI installation"),
            invalidDir,
            QFileDialog::ShowDirsOnly
        );
        if(selectedDir.isEmpty())
        {
            return; // canceled
        }

        PatchUtils::EABI_INSTALLATION = selectedDir;
        ok = PatchUtils::VerifyEABI(&invalidDir);
    }
    if(PatchUtils::EABI_INSTALLATION != oldPath)
    {
        SettingsUtils::SetKey(SettingsUtils::IniKeys::eabi_binfile_path, PatchUtils::EABI_INSTALLATION);
    }

    // Generate the save chunks and write them to the ROM
    QString errorStr = PatchUtils::SavePatchesToROM(PatchTable->GetAllEntries());
    if(errorStr.isEmpty())
    {
        // TODO close the patch manager dialog
    }
    else
    {
        QMessageBox::information(this, "Error saving patches", errorStr);
    }
}
