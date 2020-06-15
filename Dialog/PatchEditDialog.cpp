#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include "PatchEditDialog.h"
#include "ui_PatchEditDialog.h"
#include "ROMUtils.h"

static QStringList PatchTypeNameSet;
static QRegExp hookAddressRegex("^ *(0x)?[a-fA-F0-9]{1,6} *$");
static QRegExp hookStringRegex("^( *[a-fA-F0-9] *[a-fA-F0-9])*( *[pP])?( *[a-fA-F0-9] *[a-fA-F0-9])* *$");
static QRegExp descriptionRegex("^[^;]+$");

#define HOOK_ADDR_IDENTIFIER "@HookAddress"
#define HOOK_STRING_IDENTIFIER "@HookString"
#define DESCRIPTION_IDENTIFIER "@Description"

/// <summary>
/// Perform static initializtion of constant data structures for the dialog.
/// </summary>
void PatchEditDialog::StaticComboBoxesInitialization()
{
    PatchTypeNameSet << "Binary" << "Assembly" << "C";
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
    ui->lineEdit_HookAddress->setValidator(addressvalidator = new QRegExpValidator(hookAddressRegex, this));

    // Set Validator for lineEdit_HookText
    ui->lineEdit_HookText->setValidator(addressvalidator = new QRegExpValidator(hookStringRegex, this));

    // Initialize the components with the patch entry item
    InitializeComponents(patchEntry);
}

/// <summary>
/// Normalize the hook text to a standard format for the patch saving.
/// </summary>
/// <remarks>
/// Spaces and "P" identifier are removed, all hex digits are made uppercase.
/// </remarks>
/// <param name="str">
/// The string whose contents to normalize.
/// </param>
/// <returns>
/// The string in a format like: 8F69B144A08956BC
/// </returns>
static QString NormalizeHookText(QString str)
{
    str = str.replace(" ", ""); // remove whitespace
    str = str.toUpper();        // uppercase
    str = str.replace("P", ""); // placeholder bytes for patch address
    return str;
}

/// <summary>
/// Find the index of the patch address identifier in hook text.
/// </summary>
/// <param name="str">
/// The string whose contents to analyze.
/// </param>
/// <returns>
/// The index (in bytes, not characters) of the patch address identifier.
/// </returns>
static unsigned int GetPatchOffset(QString str)
{
    str = str.replace(" ", "").toUpper();
    int index = str.indexOf("P");
    return index > 0 ? index / 2 : -1;
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

static QString InferFromIdentifier(QString filePath, QString identifier, QRegExp validator)
{
    QFile file(filePath);
    file.open(QIODevice::ReadOnly);
    QTextStream in(&file);
    QString line;
    do {
        line = in.readLine();
        if (line.contains(identifier, Qt::CaseSensitive)) {
            QString contents = line.mid(line.indexOf(identifier) + identifier.length());
            return validator.indexIn(contents) ? "" : contents.trimmed();
        }
    } while (!line.isNull());
    return "";
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
    ui->lineEdit_FilePath->setText(patchEntry.FileName);
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
        NormalizeHookText(ui->lineEdit_HookText->text()),
        GetPatchOffset(ui->lineEdit_HookText->text()),
        0,
        "",
        ui->textEdit_Description->toPlainText()
    };
}

/// <summary>
/// This slot function will be triggered when clicking the "Browse" button.
/// </summary>
void PatchEditDialog::on_pushButton_Browse_clicked()
{
    // Prompt the user for the patch file
    QString romFileDir = QFileInfo(ROMUtils::ROMFilePath).dir().path();
    QString qFilePath = QFileDialog::getOpenFileName(
        this,
        tr("Open patch file"),
        romFileDir,
        tr("C source files (*.c);;ARM assembly files (*.s);;Binary files (*.bin)")
    );
    if(!qFilePath.isEmpty())
    {
        ui->lineEdit_FilePath->setText(qFilePath);

        // Infer the type based on the extension
        enum PatchType fileType;
        if(qFilePath.endsWith(".c", Qt::CaseInsensitive))
        {
            fileType = PatchType::C;
        }
        else if(qFilePath.endsWith(".s", Qt::CaseInsensitive))
        {
            fileType = PatchType::Assembly;
        }
        else
        {
            fileType = PatchType::Binary;
        }
        ui->comboBox_PatchType->setCurrentIndex(fileType);

        // Infer fields from file comments
        if(fileType != PatchType::Binary)
        {
            QString hookAddress = InferFromIdentifier(qFilePath, HOOK_ADDR_IDENTIFIER, hookAddressRegex);
            if(hookAddress != "")
            {
                ui->lineEdit_HookAddress->setText(hookAddress);
            }

            QString hookString = InferFromIdentifier(qFilePath, HOOK_STRING_IDENTIFIER, hookStringRegex);
            if(hookString != "")
            {
                ui->lineEdit_HookText->setText(hookString);
            }

            QString descString = InferFromIdentifier(qFilePath, DESCRIPTION_IDENTIFIER, descriptionRegex);
            if(descString != "")
            {
                ui->textEdit_Description->setText(descString);
            }
        }
    }
}
