#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include "PatchEditDialog.h"
#include "ui_PatchEditDialog.h"
#include "ROMUtils.h"
#include "FileIOUtils.h"

static QStringList PatchTypeNameSet;

/// <summary>
/// Perform static initializtion of constant data structures for the dialog.
/// </summary>
void PatchEditDialog::StaticComboBoxesInitialization()
{
    PatchTypeNameSet << "Binary" << "Assembly" << "C" << "C_dependency";
}

/// <summary>
/// Construct an instance of the PatchEditDialog.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
/// <param name="patchEntry">
/// The patch entry whose fields are used to initialize the dialog.
/// </param>
PatchEditDialog::PatchEditDialog(QWidget *parent, struct PatchEntryItem patchEntry) :
    QDialog(parent),
    ui(new Ui::PatchEditDialog)
{
    ui->setupUi(this);

    // Initialize the items in comboBox_PatchType
    ui->comboBox_PatchType->addItems(PatchTypeNameSet);

    // Set Validator for lineEdit_HookAddress
    ui->lineEdit_HookAddress->setValidator(addressvalidator = new QRegularExpressionValidator(PatchUtils::HookAddressRegex(), this));

    // Set Validator for lineEdit_HookText
    ui->lineEdit_HookText->setValidator(addressvalidator = new QRegularExpressionValidator(PatchUtils::HookStringRegex(), this));

    // Initialize the components with the patch entry item
    InitializeComponents(patchEntry);
}

/// <summary>
/// Format the normalized hook text in a nice, human-readable format.
/// </summary>
/// <param name="hookStr">
/// The normalized hook text.
/// </param>
/// <param name="patchOffset">
/// The offset of the patch address.
/// </param>
/// <returns>
/// The string in a format like: 8F 69 B1 44 A0 89 P 56 BC
/// </returns>
static QString FormatHookText(QString hookStr, int patchOffset)
{
    QString ret;
    for(int i = 0;; ++i)
    {
        if(i == patchOffset) ret += " P";
        if(i == hookStr.length() / 2) break;
        ret += QString(" %1").arg(hookStr.mid(i * 2, 2));
    }
    return ret.length() ? ret.mid(1) : "";
}

/// <summary>
/// Deconstruct the PatchEditDialog and clean up its instance objects on the heap.
/// </summary>
PatchEditDialog::~PatchEditDialog()
{
    delete addressvalidator;
    delete ui;
}

/// <summary>
/// Initialize the components of the PatchEditDialog.
/// </summary>
/// <param name="patchEntry">
/// The patch entry whose fields are used to initialize the dialog.
/// </param>
void PatchEditDialog::InitializeComponents(struct PatchEntryItem patchEntry)
{
    QString displayPath = FileIOUtils::RelativeFilePathToAbsoluteFilePath(patchEntry.FileName);
    ui->lineEdit_FilePath->setText(displayPath);
    ui->comboBox_PatchType->setCurrentIndex(patchEntry.PatchType);
    QString hookAddressText = patchEntry.HookAddress ? QString::number(patchEntry.HookAddress, 16).toUpper() : "";
    ui->lineEdit_HookAddress->setText("0x" + hookAddressText);
    ui->lineEdit_HookText->setText(FormatHookText(patchEntry.HookString, patchEntry.PatchOffsetInHookString));
    ui->textEdit_Description->setText(patchEntry.Description);
}

/// <summary>
/// Create a patch entry struct based on the fields of the PatchEditDialog.
/// </summary>
/// <returns>
/// The patch entry.
/// </returns>
struct PatchEntryItem PatchEditDialog::CreatePatchEntry()
{
    QString hookAddressText = ui->lineEdit_HookAddress->text();
    if(hookAddressText.startsWith("0x")) hookAddressText = hookAddressText.mid(2);
    return
    {
        ui->lineEdit_FilePath->text(),
        static_cast<enum PatchType>(ui->comboBox_PatchType->currentIndex()),
        hookAddressText.toUInt(Q_NULLPTR, 16),
        PatchUtils::NormalizeHookText(ui->lineEdit_HookText->text()),
        PatchUtils::GetPatchOffset(ui->lineEdit_HookText->text()),
        ui->lineEdit_FilePath->text().isEmpty() ? 0 : PatchManagerTableView::DummyPatchAddressValue,
        "",
        ui->textEdit_Description->toPlainText()
    };
}

void PatchEditDialog::ParsePatchFile(QString patchfilepath)
{
    if(patchfilepath.isEmpty()) return;

    ui->lineEdit_FilePath->setText(patchfilepath);

    // Start from the current UI values so that annotations which are absent from the file
    // keep their previous values, matching the behavior of the old parsing code.
    struct PatchEntryItem parsed = CreatePatchEntry();
    parsed.FileName = patchfilepath;

    QString error;
    bool hookStringMissing = false;
    if(!PatchUtils::ParsePatchSourceFile(patchfilepath, parsed, &error, &hookStringMissing))
    {
        QMessageBox::information(this, QT_TR_NOOP("WL4Editor Patch Import Info"), error);
        return;
    }

    ui->comboBox_PatchType->setCurrentIndex(parsed.PatchType);
    ui->lineEdit_HookAddress->setText(parsed.HookAddress ? "0x" + QString::number(parsed.HookAddress, 16).toUpper() : "");

    int patchOffset = parsed.PatchOffsetInHookString == static_cast<unsigned int>(-1)
            ? -1 : static_cast<int>(parsed.PatchOffsetInHookString);
    ui->lineEdit_HookText->setText(FormatHookText(parsed.HookString, patchOffset));
    ui->textEdit_Description->setText(parsed.Description);

    if(hookStringMissing)
    {
        QMessageBox::information(
            this,
            QT_TR_NOOP("WL4Editor Patch Import Info"),
            QT_TR_NOOP("Unable to find HookString from the patch file.\nAutomatically set the patch type to C_dependency."),
            QMessageBox::Ok,
            QMessageBox::Ok
            );
    }
}

/// <summary>
/// This slot function will be triggered when clicking the "Browse" button.
/// </summary>
void PatchEditDialog::on_pushButton_Browse_clicked()
{
    // Prompt the user for the patch file
    QString romFileDir = QFileInfo(ROMUtils::ROMFileMetadata->FilePath).dir().path();
    QString qFilePath = QFileDialog::getOpenFileName(
        this,
        tr("Open patch file"),
        romFileDir,
        tr("C source files (*.c);;ARM assembly files (*.s);;Binary files (*.bin)")
    );
    ParsePatchFile(qFilePath);
}

/// <summary>
/// This function will reload patch infos from file.
/// </summary>
void PatchEditDialog::on_pushButton_Reload_clicked()
{
    ParsePatchFile(ui->lineEdit_FilePath->text());
}

