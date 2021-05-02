#include "ChunkManagerDialog.h"
#include "ui_ChunkManagerDialog.h"

ChunkManagerDialog::ChunkManagerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChunkManagerDialog)
{
    ui->setupUi(this);

    setWindowTitle("Chunk Manager");
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
