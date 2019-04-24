#include "PatchEditDialog.h"
#include "ui_PatchEditDialog.h"
#include <QFileDialog>
#include <ROMUtils.h>

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
    QStringList PatchTypeNameSet;
    PatchTypeNameSet << "Binary" << "Assembly" << "C";
    ui->comboBox_PatchType->addItems(PatchTypeNameSet);

    // Set Validator for lineEdit_HookAddress
    QRegExp regExp("[a-fA-F0-9]{6}");
    ui->lineEdit_HookAddress->setValidator(addressvalidator = new QRegExpValidator(regExp, this));

    // Initialize the components with the patch entry item
    InitializeComponents(patchEntry);
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
    QString hookText = patchEntry.HookAddress ? QString::number(patchEntry.HookAddress, 16) : "";
    ui->lineEdit_HookAddress->setText(hookText);
    ui->checkBox_StubCallingFunction->setChecked(patchEntry.StubFunction);
    (patchEntry.ThumbMode ? ui->radioButton_CompileInThumbMode : ui->radioButton_CompileInARMMode)->setChecked(true);
}

/// <summary>
/// Create a patch entry struct based on the fields of the PatchEditDialog.
/// </summary>
/// <returns>
/// The patch entry.
/// </returns>
struct PatchEntryItem PatchEditDialog::CreatePatchEntry()
{
    return
    {
        ui->lineEdit_FilePath->text(),
        static_cast<enum PatchType>(ui->comboBox_PatchType->currentIndex()),
        static_cast<unsigned int>(ui->lineEdit_HookAddress->text().toInt(Q_NULLPTR, 16)),
        ui->checkBox_StubCallingFunction->isChecked(),
        ui->radioButton_CompileInThumbMode->isChecked(),
        // These should be calculated later by saving
        0, ""
    };
}

/// <summary>
/// This slot function will be triggered when clicking the "Browse" button.
/// </summary>
void PatchEditDialog::on_pushButton_Browse_clicked()
{
    // Promt the user for the patch file
    QString qFilePath = QFileDialog::getOpenFileName(
        this,
        tr("Open patch file"),
        QString(""),
        tr("C source files (*.c);;ARM assembly files (*.s);;Binary files (*.bin)")
    );
    if(qFilePath != "")
    {
        ui->lineEdit_FilePath->setText(qFilePath);

        // Infer the type based on the extension
        if(qFilePath.endsWith(".c", Qt::CaseInsensitive))
        {
            ui->comboBox_PatchType->setCurrentIndex(PatchType::C);
        }
        else if(qFilePath.endsWith(".s", Qt::CaseInsensitive))
        {
            ui->comboBox_PatchType->setCurrentIndex(PatchType::Assembly);
        }
        else
        {
            ui->comboBox_PatchType->setCurrentIndex(PatchType::Binary);
        }
    }
}

/// <summary>
/// This slot function will be triggered when text changed in lineEdit_HookAddress.
/// </summary>
void PatchEditDialog::on_lineEdit_HookAddress_textChanged(const QString &arg1)
{
    int hookValue = 0;
    if(arg1.length())
    {
        hookValue = arg1.toInt(Q_NULLPTR, 16);
    }
    ui->groupBox_HookConfig->setEnabled(hookValue);
}
