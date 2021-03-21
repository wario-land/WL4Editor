#include "PatchManagerDialog.h"
#include "ui_PatchManagerDialog.h"
#include "WL4EditorWindow.h"
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <ROMUtils.h>
#include <SettingsUtils.h>
#include <QFileDialog>

extern WL4EditorWindow *singleton;

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
    initialEntries = PatchTable->GetAllEntries().size();
    ui->savePatchButton->setEnabled(initialEntries);
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
/// Validate that some new patch entry can be added to the current set of patches in the dialog.
/// </summary>
/// <param name="currentEntries">
/// The current patch entries in the list.
/// </param>
/// <param name="newEntry">
/// The new patch entry to add to the list.
/// </param>
/// <returns>
/// If the entry is valid, an empty QString. Otherwise, the error message.
/// </returns>
static QString ValidateNewEntry(QVector<struct PatchEntryItem> currentEntries, struct PatchEntryItem &newEntry)
{
    // The hook may not fall outside the area of the vanilla rom
    if(newEntry.HookAddress + newEntry.GetHookLength() > WL4Constants::AvailableSpaceBeginningInROM)
    {
        return QString("Hook overlaps with save chunk area in ROM: 0x%1 - 0x%2").arg(
            QString::number(newEntry.HookAddress, 16).toUpper(), QString::number(newEntry.HookAddress + newEntry.GetHookLength() - 1, 16).toUpper());
    }

    // Patch on the first 4 bytes of the rom is not allowed
    if(newEntry.HookAddress < 4)
    {
        return QString("Patch on the first 4 bytes of the rom is not allowed.");
    }

    // File name may not contain a semicolon
    if(newEntry.FileName.contains(";"))
    {
        return QString("File path contains a semicolon, which is not allowed: ") + newEntry.FileName;
    }

    // Description may not contain a semicolon
    if(newEntry.Description.contains(";"))
    {
        return QString("Description contains a semicolon, which is not allowed: ") + newEntry.Description;
    }

    if(newEntry.FileName.length())
    {
        // If a file name is specified, the file must exist.
        QFile file(newEntry.FileName);
        if(!file.exists())
        {
            return QString("File does not exist: ") + newEntry.FileName;
        }

        // The file must be in a subdirectory relative to the ROM file.
        QString relativeFN = GetPathRelativeToROM(newEntry.FileName);
        if(relativeFN.isEmpty())
        {
            return QString("File must be within directory subtree of ROM file: ") + QFileInfo(ROMUtils::ROMFilePath).dir().path();
        }

        // Two entries may not use the same file name. Two entries with blank file names are allowed.
        newEntry.FileName = relativeFN;
        bool fileNameIsValid = !std::any_of(currentEntries.begin(), currentEntries.end(),
            [newEntry](struct PatchEntryItem e){ return e.FileName == newEntry.FileName; });
        if(!fileNameIsValid)
        {
            return QString("Another entry already exists with the filename: ") + newEntry.FileName;
        }

        // It does not make sense to add a save chunk with no link to it in the hook
        if(newEntry.PatchOffsetInHookString == (unsigned int) -1)
        {
            return "A file is sepcified, so a save chunk will be created. But, the hook does not specify the P identifier for the patch code address. The save chunk would be useless, so this is not allowed (please use P in the hook).";
        }
    }

    // If a file is not specified, then it will not create a save chunk. Therefore, the hook may not contain a save chunk address.
    else if(newEntry.PatchOffsetInHookString != (unsigned int) -1)
    {
        return "Patch does not specify a file so no save chunk will be created. But, the hook text specifies that there should be a pointer to a save chunk. This contradiction is not allowed";
    }

    // Patch's hook may not overlap with another patch's hook
    struct PatchEntryItem curr;
    bool hasHookAddressConflict = std::any_of(currentEntries.begin(), currentEntries.end(), [newEntry, &curr](struct PatchEntryItem e)
    {
        curr = e;
        int s1 = newEntry.HookAddress, s2 = e.HookAddress, e1 = newEntry.GetHookLength() + s1 - 1, e2 = e.GetHookLength() + s2 - 1;
        return (e1 >= s2 && e1 <= e2) || (e2 >= s1 && e2 <= e1);
    });
    if(hasHookAddressConflict)
    {
        return QString("This patch's hook (0x%1 - 0x%2) overlaps with the hook of another patch (0x%3 - 0x%4)").arg(
            QString::number(newEntry.HookAddress, 16).toUpper(), QString::number(newEntry.HookAddress + newEntry.GetHookLength() - 1, 16).toUpper(),
            QString::number(curr.HookAddress, 16).toUpper(), QString::number(curr.HookAddress + curr.GetHookLength() - 1, 16).toUpper());
    }

    return "";
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
    PatchEditDialog editDialog(this);

retry:
    if(editDialog.exec() == QDialog::Accepted)
    {
        entry = editDialog.CreatePatchEntry();
        QString result = ValidateNewEntry(currentEntries, entry);
        if(result != "")
        {
            QMessageBox::information(this, "About", result);
            goto retry;
        }
        PatchTable->AddEntry(entry);
        ui->savePatchButton->setEnabled(true);
    }
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
            [selectedEntry, &selectedIndex](struct PatchEntryItem e){ ++selectedIndex; return selectedEntry.HookAddress == e.HookAddress; });
        currentEntries.remove(selectedIndex);

        // Execute the edit dialog
        PatchEditDialog editDialog(this, selectedEntry);

retry:
        if(editDialog.exec() == QDialog::Accepted)
        {
            entry = editDialog.CreatePatchEntry();
            QString result = ValidateNewEntry(currentEntries, entry);
            if(result != "")
            {
                QMessageBox::information(this, "About", result);
                goto retry;
            }
            PatchTable->UpdateEntry(selectedIndex, entry);
            ui->savePatchButton->setEnabled(true);
        }
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
    if(PatchTable->GetAllEntries().size())
    {
        ui->savePatchButton->setEnabled(true);
    }
    else
    {
        ui->savePatchButton->setEnabled(initialEntries);
    }
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
        singleton->GetOutputWidgetPtr()->PrintString("Finished saving patch data to ROM. (patches: " + QString::number(PatchTable->GetAllEntries().size()) + ")");
        this->close();
    }
    else
    {
        QMessageBox::information(this, "Error saving patches", errorStr);
    }
}
