#include "ChunkManagerDialog.h"
#include "ui_ChunkManagerDialog.h"

#include "WL4EditorWindow.h"
extern WL4EditorWindow *singleton;

ChunkManagerDialog::ChunkManagerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChunkManagerDialog)
{
    ui->setupUi(this);

    TreeView = ui->treeView_chunkData;
    Info = ui->groupBox_Information;
    Info->SetTreeView(TreeView);

    QObject::connect(TreeView->selectionModel(), &QItemSelectionModel::currentRowChanged, Info, &ChunkManagerInfoGroupBox::UpdateContents);
}

ChunkManagerDialog::~ChunkManagerDialog()
{
    delete ui;
}

void ChunkManagerDialog::on_pushButton_SelectAllOrphanChunks_clicked()
{
    auto chunkRefs = ROMUtils::GetAllChunkReferences();
    auto allChunks = ROMUtils::FindAllChunksInROM(
        ROMUtils::ROMFileMetadata->ROMDataPtr,
        ROMUtils::ROMFileMetadata->Length,
        WL4Constants::AvailableSpaceBeginningInROM,
        ROMUtils::SaveDataChunkType::InvalidationChunk,
        true
    );
    allChunks.erase(std::remove_if(allChunks.begin(),
                                   allChunks.end(),
                                   [&chunkRefs](unsigned int chunk)
                                    {
                                        return chunkRefs.contains(chunk);
                                    }),
                                    allChunks.end());
    TreeView->SelectChunks(allChunks);
}

void ChunkManagerDialog::on_pushButton_DeselectAll_clicked()
{
    TreeView->SelectChunks(QVector<unsigned int>());
}

void ChunkManagerDialog::on_pushButton_SelectAllBrokenChunks_clicked()
{
    // TODO
    auto chunkRefs = ROMUtils::GetAllChunkReferences();
}

void ChunkManagerDialog::on_pushButton_ShowInHexView_clicked()
{
    singleton->ShowHexEditorWindow();

    // TODO: also highlight the last chunk and the next chunk,
    // also show hint if some random pointer point to the data around (use foreground highlight)
    // which is useful to check if data corruption exist
    unsigned int chunkAddress = TreeView->model()->data(TreeView->currentIndex()).toString().mid(2).toUInt(nullptr, 16);
    unsigned char *dataptr = ROMUtils::CurrentROMMetadata.ROMDataPtr;
    unsigned int chunkLength = ((*(unsigned short *)(dataptr + chunkAddress + 4)) & 0xFFFF) |
                               ((dataptr[chunkAddress + 9] << 16) & 0xFF0000);
    HexEditorWindow *hexEditorPtr = singleton->GetHexEditorWindowPtr();
    hexEditorPtr->highlightClear();
    hexEditorPtr->hightlightData_bg(chunkAddress, 12, Qt::blue);
    hexEditorPtr->hightlightData_bg(chunkAddress + 12, chunkLength, Qt::red);
    hexEditorPtr->gotoOffset(chunkAddress);
}

void ChunkManagerDialog::on_buttonBox_accepted()
{
    if(ui->checkBox_DefragNormalChunks->checkState() == Qt::Checked)
    {
        auto chunks = TreeView->GetRemainingChunks();
        singleton->GetOutputWidgetPtr()->PrintString(QString("Defragmenting %1 chunks...").arg(chunks.size()));
        ROMUtils::DefragmentChunks(chunks);
    }
    // TODO: other cases
}
