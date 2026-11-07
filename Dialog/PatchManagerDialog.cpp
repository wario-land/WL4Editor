#include "PatchManagerDialog.h"
#include "ui_PatchManagerDialog.h"
#include "WL4EditorWindow.h"
#include "PatchIO.h"
#include "FileIOUtils.h"
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QVBoxLayout>
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
    if (num_of_select_rows == 1)
    {
        QVector<struct PatchEntryItem> currentEntries = PatchTable->GetAllEntries();
        struct PatchEntryItem selectedEntry = PatchTable->GetSelectedEntry();
        int selectedIndex = -1;
        std::find_if(currentEntries.begin(), currentEntries.end(),
            [&selectedEntry, &selectedIndex](struct PatchEntryItem e)
            { ++selectedIndex; return selectedEntry.HookAddress == e.HookAddress; });
        ui->textEdit_PatchDescription->setText(tr("Description:\n") + currentEntries[selectedIndex].Description);
    }
    else
    {
        ui->textEdit_PatchDescription->setText("");
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
        return QString(QT_TR_NOOP("Hook overlaps with save chunk area in ROM: 0x%1 - 0x%2")).arg(
            QString::number(newEntry.HookAddress, 16).toUpper(), QString::number(newEntry.HookAddress + newEntry.GetHookLength() - 1, 16).toUpper());
    }

    // Patch on the first 4 bytes of the rom is not allowed
    if(newEntry.HookAddress < 4 && newEntry.PatchType != C_dependency)
    {
        return QT_TR_NOOP("Patch on the first 4 bytes of the rom is not allowed.");
    }

    // special cases for "C_dependency" type patches
    if (newEntry.PatchType == C_dependency)
    {
        // not allowed to set HookAddress
        if(newEntry.HookAddress)
        {
            return QT_TR_NOOP("It is not allowed to set HookAddress for C_dependency type patches.");
        }

        // not allowed to set HookString
        if(!newEntry.HookString.isEmpty())
        {
            return QT_TR_NOOP("It is not allowed to set HookString for C_dependency type patches.");
        }
    }

    // File name may not contain a semicolon
    if(newEntry.FileName.contains(";"))
    {
        return QString(QT_TR_NOOP("File path contains a semicolon, which is not allowed: %1")).arg(newEntry.FileName);
    }

    // Description may not contain a semicolon
    if(newEntry.Description.contains(";"))
    {
        return QString(QT_TR_NOOP("Description contains a semicolon, which is not allowed: %1")).arg(newEntry.Description);
    }

    if(newEntry.FileName.length())
    {
        // If a file name is specified, the file must exist.
        QFile file(newEntry.FileName);
        if(!file.exists())
        {
            return QString(QT_TR_NOOP("File does not exist: %1")).arg(newEntry.FileName);
        }

        // The file must be in a subdirectory relative to the ROM file.
        const QString romDirectory = QFileInfo(ROMUtils::ROMFileMetadata->FilePath).absolutePath();
        bool relativePathOk = false;
        QString relativeFN = FileIOUtils::PathRelativeToDirectory(newEntry.FileName, romDirectory, &relativePathOk);
        if(!relativePathOk || relativeFN.isEmpty())
        {
            return QString(QT_TR_NOOP("File must be within directory subtree of ROM file: %1")).arg(romDirectory);
        }

        // Two entries may not use the same file name. Two entries with blank file names are allowed.
        newEntry.FileName = relativeFN;
        bool fileNameIsValid = !std::any_of(currentEntries.begin(), currentEntries.end(),
            [newEntry](struct PatchEntryItem e){ return e.FileName == newEntry.FileName; });
        if(!fileNameIsValid)
        {
            return QString(QT_TR_NOOP("Another entry already exists with the filename: %1")).arg(newEntry.FileName);
        }

        // It does not make sense to add a save chunk with no link to it in the hook
        if(newEntry.PatchOffsetInHookString == (unsigned int) -1 && newEntry.PatchType != C_dependency)
        {
            return QT_TR_NOOP("A file is sepcified, so a save chunk will be created. But, the hook does not specify the P identifier for the patch code address. The save chunk would be useless, so this is not allowed (please use P in the hook).");
        }
    }

    // If a file is not specified, then it will not create a save chunk. Therefore, the hook may not contain a save chunk address.
    else if(newEntry.PatchOffsetInHookString != (unsigned int) -1)
    {
        return QT_TR_NOOP("Patch does not specify a file so no save chunk will be created. But, the hook text specifies that there should be a pointer to a save chunk. This contradiction is not allowed");
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
        return QString(QT_TR_NOOP("This patch's hook (0x%1 - 0x%2) overlaps with the hook of another patch (0x%3 - 0x%4)")).arg(
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

retry_add_patch:
    if(editDialog.exec() == QDialog::Accepted)
    {
        entry = editDialog.CreatePatchEntry();
        QString result = ValidateNewEntry(currentEntries, entry);
        if(result != "")
        {
            QMessageBox::information(this, tr("About"), result);
            goto retry_add_patch;
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

retry_edit_patch:
        if(editDialog.exec() == QDialog::Accepted)
        {
            entry = editDialog.CreatePatchEntry();
            QString result = ValidateNewEntry(currentEntries, entry);
            if(result != "")
            {
                QMessageBox::information(this, tr("About"), result);
                goto retry_edit_patch;
            }
            PatchTable->UpdateEntry(selectedIndex, entry);
            ui->savePatchButton->setEnabled(true);

            currentEntries = PatchTable->GetAllEntries();
            ui->textEdit_PatchDescription->setText(tr("Description:\n") + currentEntries[selectedIndex].Description);
        }
    }
    else if(!selectedRows.size())
    {
        QMessageBox::information(this, tr("About"), tr("You must select a row to edit."));
    }
    else
    {
        QMessageBox::information(this, tr("About"), tr("You may only select 1 row at a time to edit."));
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
    ui->textEdit_PatchDescription->setText(tr(""));
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
            QString(tr("Invalid EABI folder selected: %1 (required binaries not found)<br>")).arg(invalidDir);
        QString URL = "https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads";
        QMessageBox::information(this, tr("Select EABI installation"),
            msg + tr("You must select the \"bin\" folder of a valid EABI installation.<br>Download:") +
            " <a href=\"" + URL + "\">" + URL + "</a>");

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
        singleton->GetOutputWidgetPtr()->PrintString(tr("Finished saving patch data to ROM. (patches: %1)").arg(QString::number(PatchTable->GetAllEntries().size())));
        this->close();
    }
    else
    {
        QMessageBox::information(this, tr("Error saving patches"), errorStr);
    }
}

/// <summary>
/// Show a dialog which lists all problem lines of one import check.
/// </summary>
/// <param name="parent">
/// The parent widget.
/// </param>
/// <param name="title">
/// The dialog title.
/// </param>
/// <param name="summary">
/// The explanatory text shown above the list.
/// </param>
/// <param name="lines">
/// The problem lines to show in a read-only, scrollable text area.
/// </param>
/// <param name="acceptText">
/// The text of the accept button. When empty, no accept button is shown.
/// </param>
/// <param name="rejectText">
/// The text of the reject button. When empty, no reject button is shown.
/// </param>
/// <returns>
/// True when the accept button was clicked.
/// </returns>
static bool ShowIssueDialog(QWidget *parent, const QString &title, const QString &summary,
                            const QStringList &lines, const QString &acceptText,
                            const QString &rejectText)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.resize(780, 480);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *summaryLabel = new QLabel(summary, &dialog);
    summaryLabel->setWordWrap(true);
    layout->addWidget(summaryLabel);

    QPlainTextEdit *details = new QPlainTextEdit(&dialog);
    details->setReadOnly(true);
    details->setPlainText(lines.join("\n"));
    layout->addWidget(details, 1);

    QDialogButtonBox *buttons = new QDialogButtonBox(&dialog);
    bool accepted = false;

    if(!acceptText.isEmpty())
    {
        QPushButton *acceptButton = buttons->addButton(acceptText, QDialogButtonBox::AcceptRole);
        QObject::connect(acceptButton, &QPushButton::clicked, &dialog,
                         [&dialog, &accepted]() { accepted = true; dialog.accept(); });
    }
    if(!rejectText.isEmpty())
    {
        QPushButton *rejectButton = buttons->addButton(rejectText, QDialogButtonBox::RejectRole);
        QObject::connect(rejectButton, &QPushButton::clicked, &dialog, &QDialog::reject);
        rejectButton->setDefault(true);
    }

    layout->addWidget(buttons);
    dialog.exec();
    return accepted;
}

/// <summary>
/// Export the selected patch entries to a JSON file.
/// </summary>
void PatchManagerDialog::on_ExportPatchButton_clicked()
{
    const QVector<struct PatchEntryItem> selectedEntries = PatchTable->GetSelectedEntries();
    if(selectedEntries.isEmpty())
    {
        QMessageBox::information(this, tr("Export Patch List"),
                                 tr("Select one or more rows in the patch table to export."));
        return;
    }

    const QFileInfo romInfo(ROMUtils::ROMFileMetadata->FilePath);
    const QString romDirectory = romInfo.absolutePath();

    QString initialPath = singleton->GetdDialogInitialPath();
    if(initialPath.isEmpty()) initialPath = romDirectory;
    initialPath = QDir(initialPath).filePath(romInfo.completeBaseName() + "_patchlist.json");

    const QString filePath = QFileDialog::getSaveFileName(
                this, tr("Export Patch List"), initialPath, tr("JSON files (*.json)"));
    if(filePath.isEmpty()) return;

    QStringList warnings;
    const QJsonObject document = PatchIO::BuildDocument(selectedEntries, romDirectory, &warnings);

    QSaveFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, tr("Export Error"), tr("Cannot create the selected file."));
        return;
    }
    file.write(QJsonDocument(document).toJson(QJsonDocument::Indented));
    if(!file.commit())
    {
        QMessageBox::critical(this, tr("Export Error"), tr("Failed to write the selected file."));
        return;
    }

    singleton->SetDialogInitialPath(QFileInfo(filePath).absolutePath());
    singleton->GetOutputWidgetPtr()->PrintString(
                tr("Exported %1 patch entries and %2 file records.")
                .arg(static_cast<int>(selectedEntries.size()))
                .arg(static_cast<int>(document.value("files").toObject().size())));
    for(const QString &warning : warnings)
    {
        singleton->GetOutputWidgetPtr()->PrintString(warning);
    }
}

/// <summary>
/// Name of a patch type, used by the hook information diagnostics of the import.
/// </summary>
static QString PatchTypeName(enum PatchType type)
{
    switch(type)
    {
        case PatchType::Binary: return "Binary";
        case PatchType::Assembly: return "Assembly";
        case PatchType::C: return "C";
        case PatchType::C_dependency: return "C_dependency";
    }
    return QString();
}

/// <summary>
/// Import patch entries from a JSON file into the patch table.
/// </summary>
void PatchManagerDialog::on_ImportPatchButton_clicked()
{
    const QFileInfo romInfo(ROMUtils::ROMFileMetadata->FilePath);
    const QString romDirectory = romInfo.absolutePath();

    QString initialPath = singleton->GetdDialogInitialPath();
    if(initialPath.isEmpty()) initialPath = romDirectory;

    const QString filePath = QFileDialog::getOpenFileName(
                this, tr("Import Patch List"), initialPath, tr("JSON files (*.json)"));
    if(filePath.isEmpty()) return;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, tr("Import Error"), tr("Cannot open the selected file."));
        return;
    }
    const QByteArray contents = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(contents, &parseError);
    if(parseError.error != QJsonParseError::NoError)
    {
        QMessageBox::critical(this, tr("Import Error"),
                              tr("Failed to parse the JSON file: ") + parseError.errorString());
        return;
    }

    PatchIO::ImportDocument importDocument;
    QString error;
    if(!PatchIO::ParseDocument(document, importDocument, error))
    {
        QMessageBox::critical(this, tr("Import Error"), error);
        return;
    }

    for(const QString &warning : importDocument.Warnings)
    {
        singleton->GetOutputWidgetPtr()->PrintString(warning);
    }

    if(!importDocument.EntryErrors.isEmpty())
    {
        ShowIssueDialog(this, tr("Patch list import canceled"),
                        tr("The JSON file contains invalid patch entries. No entries were imported."),
                        importDocument.EntryErrors, QString(), tr("Close"));
        return;
    }

    if(importDocument.Entries.isEmpty())
    {
        QMessageBox::information(this, tr("Import Patch List"),
                                 tr("The JSON file does not contain any patch entries."));
        return;
    }

    singleton->SetDialogInitialPath(QFileInfo(filePath).absolutePath());

    const QVector<struct PatchEntryItem> currentEntries = PatchTable->GetAllEntries();

    // A missing or unusable code file is a hard failure: the whole import ends here, before
    // any other check is reported.
    const QVector<PatchIO::FileCheckIssue> fileIssues =
            PatchIO::CheckEntryFiles(importDocument.Entries, importDocument.Files, romDirectory);

    QStringList missingFileLines;
    QStringList softIssueLines;
    for(const PatchIO::FileCheckIssue &issue : fileIssues)
    {
        const QString line = tr("entry %1 (%2): %3")
                .arg(issue.EntryIndex)
                .arg(issue.FileName.isEmpty() ? tr("(no file)") : issue.FileName)
                .arg(issue.Reason);
        if(issue.MissingFile) missingFileLines.append(line);
        else softIssueLines.append(line);
    }

    if(!missingFileLines.isEmpty())
    {
        ShowIssueDialog(this, tr("Patch list import canceled"),
                        tr("The following code files required by the JSON could not be found or used in the target ROM directory. No entries were imported."),
                        missingFileLines, QString(), tr("Close"));
        return;
    }

    if(!softIssueLines.isEmpty())
    {
        const bool importAnyway = ShowIssueDialog(
                    this, tr("Patch file verification"),
                    tr("The following code files do not match the JSON size/SHA-1 manifest. Import anyway?"),
                    softIssueLines, tr("Import Anyway"), tr("Abandon Import"));
        if(!importAnyway) return;
    }

    // When a code file changed, check whether its declared hook information still matches the
    // JSON entry. The comparison must cover the patch type and HookAddress as well as the hook
    // bytes and the P offset: SavePatchesToROM writes the hook to the entry's HookAddress, so
    // keeping a stale address while compiling the updated source would overwrite unrelated ROM
    // instructions.
    QVector<int> inconsistentEntryIndices;
    QStringList inconsistentLines;
    const int importedCount = static_cast<int>(importDocument.Entries.size());
    for(const PatchIO::FileCheckIssue &issue : fileIssues)
    {
        if(!issue.RecordsSizeOrHashMismatch) continue;
        const int index = issue.EntryIndex;
        if(index < 0 || index >= importedCount) continue;

        const struct PatchEntryItem &entry = importDocument.Entries.at(index);
        // Binary files carry no metadata comments, so there is nothing to compare for them.
        // C_dependency files are included on purpose: a dependency which gained @HookString and
        // @HookAddress must be offered a re-parse instead of silently staying dependency-only.
        if(entry.PatchType == PatchType::Binary) continue;
        if(entry.FileName.isEmpty()) continue;

        const QString absolutePath = QDir(romDirectory).absoluteFilePath(entry.FileName);
        struct PatchEntryItem probe{};
        probe.FileName = entry.FileName;
        probe.PatchOffsetInHookString = static_cast<unsigned int>(-1);
        QString parseError;
        if(!PatchUtils::ParsePatchSourceFile(absolutePath, probe, &parseError, nullptr))
        {
            QMessageBox::critical(this, tr("Import Error"), parseError);
            return;
        }

        const bool entryHasHook = !entry.HookString.isEmpty() ||
                entry.PatchOffsetInHookString != static_cast<unsigned int>(-1);
        const bool fileHasHook = !probe.HookString.isEmpty() ||
                probe.PatchOffsetInHookString != static_cast<unsigned int>(-1);
        const bool differs = (entry.PatchType != probe.PatchType) ||
                (entryHasHook != fileHasHook) ||
                (entryHasHook && fileHasHook &&
                 (entry.HookAddress != probe.HookAddress ||
                  entry.HookString.compare(probe.HookString, Qt::CaseInsensitive) != 0 ||
                  entry.PatchOffsetInHookString != probe.PatchOffsetInHookString));
        if(!differs) continue;

        const QString entryHookText = !entryHasHook ? tr("(none)")
                : (entry.HookString.isEmpty() ? tr("(P only)") : entry.HookString);
        const QString fileHookText = !fileHasHook ? tr("(none)")
                : (probe.HookString.isEmpty() ? tr("(P only)") : probe.HookString);

        inconsistentEntryIndices.append(index);
        inconsistentLines.append(
            tr("entry %1 (%2): JSON declares %3, hook address 0x%4, hook %5 (P offset %6)")
                    .arg(index)
                    .arg(entry.FileName)
                    .arg(PatchTypeName(entry.PatchType))
                    .arg(QString::number(entry.HookAddress, 16).toUpper())
                    .arg(entryHookText)
                    .arg(entry.PatchOffsetInHookString == static_cast<unsigned int>(-1)
                         ? QString("-1") : QString::number(entry.PatchOffsetInHookString)));
        inconsistentLines.append(
            QStringLiteral("    ") +
            tr("code file declares %1, hook address 0x%2, hook %3 (P offset %4)")
                    .arg(PatchTypeName(probe.PatchType))
                    .arg(QString::number(probe.HookAddress, 16).toUpper())
                    .arg(fileHookText)
                    .arg(probe.PatchOffsetInHookString == static_cast<unsigned int>(-1)
                         ? QString("-1") : QString::number(probe.PatchOffsetInHookString)));
    }

    if(!inconsistentEntryIndices.isEmpty())
    {
        const bool reparse = ShowIssueDialog(
                    this, tr("Patch hookstring verification"),
                    tr("The following code files declare different hook information than the JSON. Re-parse the hook information from the code files, or abandon the import?"),
                    inconsistentLines, tr("Re-parse from Code Files"), tr("Abandon Import"));
        if(!reparse) return;

        for(int index : inconsistentEntryIndices)
        {
            struct PatchEntryItem &entry = importDocument.Entries[index];
            const QString absolutePath = QDir(romDirectory).absoluteFilePath(entry.FileName);
            struct PatchEntryItem parsed{};
            parsed.PatchOffsetInHookString = static_cast<unsigned int>(-1);
            QString parseError;
            if(!PatchUtils::ParsePatchSourceFile(absolutePath, parsed, &parseError, nullptr))
            {
                QMessageBox::critical(this, tr("Import Error"), parseError);
                return;
            }
            entry.PatchType = parsed.PatchType;
            entry.HookAddress = parsed.HookAddress;
            entry.HookString = parsed.HookString;
            entry.PatchOffsetInHookString = parsed.PatchOffsetInHookString;
            entry.Description = parsed.Description;
            entry.PatchAddress = 0;
            entry.SubstitutedBytes.clear();
            singleton->GetOutputWidgetPtr()->PrintString(
                        tr("Re-parsed patch entry %1 from %2.").arg(index).arg(entry.FileName));
        }
    }

    // Validate the final metadata. This must happen after the hook information has been
    // synchronized with the code files: re-parsing can change the patch type, HookAddress,
    // HookString and the P offset, and validating the stale JSON metadata instead would reject
    // the import with hook ranges that no longer apply, before the user is even offered the
    // re-parse. Hook conflicts and structural problems cancel the whole import.
    const PatchIO::ImportValidation validation =
            PatchIO::ValidateForImport(currentEntries, importDocument.Entries, romDirectory);
    if(!validation.Errors.isEmpty() || !validation.HookConflicts.isEmpty())
    {
        QStringList lines;
        if(!validation.Errors.isEmpty())
        {
            lines.append(tr("Entry errors:"));
            lines += validation.Errors;
        }
        if(!validation.HookConflicts.isEmpty())
        {
            if(!lines.isEmpty()) lines.append(QString());
            lines.append(tr("Hook conflicts:"));
            lines += validation.HookConflicts;
        }
        const QString summary = inconsistentEntryIndices.isEmpty()
                ? tr("The patch list was not imported because of the following problems.")
                : tr("The patch list was not imported because of the following problems after re-parsing the code files.");
        ShowIssueDialog(this, tr("Patch list import canceled"), summary,
                        lines, QString(), tr("Close"));
        return;
    }

    // All checks passed: append every entry at once.
    for(struct PatchEntryItem &entry : importDocument.Entries)
    {
        entry.PatchAddress = entry.FileName.isEmpty() ? 0 : PatchManagerTableView::DummyPatchAddressValue;
        entry.SubstitutedBytes.clear();
    }
    PatchTable->AddEntries(importDocument.Entries);
    ui->savePatchButton->setEnabled(true);
    singleton->GetOutputWidgetPtr()->PrintString(
                tr("Imported %1 patch entries. Click Save to compile and write them to the ROM.")
                .arg(static_cast<int>(importDocument.Entries.size())));
}

