#include "ChunkManagerTreeView.h"
#include "WL4EditorWindow.h"

extern WL4EditorWindow *singleton;

/// <summary>
/// Construct an instance of the ChunkEditorTreeView.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
ChunkManagerTreeView::ChunkManagerTreeView(QWidget *parent) : QTreeView(parent), Model(this)
{
    // Configure the tree view
    setModel(&Model);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Populate the model
    QVector<unsigned int> allChunks = FindAllChunksInROM(ROMUtils::CurrentFile, ROMUtils::CurrentFileSize, WL4Constants::AvailableSpaceBeginningInROM, ROMUtils::SaveDataChunkType::InvalidationChunk, true);
    for(unsigned int chunk : allChunks)
    {
        Model.AddChunk(chunk);
    }
    Model.CanUpdateTristate = true;

    // Resize columns to fit the view
    for(int i = 0; i < Model.columnCount(); ++i)
    {
        resizeColumnToContents(i);
    }
    setColumnWidth(1, 155);
}

ChunkManagerTreeView::~ChunkManagerTreeView()
{

}

//---------------------------------------------------------------------------------------------------------------------------
// ChunkManagerModel functions
//---------------------------------------------------------------------------------------------------------------------------

ChunkManagerModel::ChunkManagerModel(QWidget *parent) : QStandardItemModel(parent)
{
    QStringList headerLabels({tr("Selected"), tr("Chunk"), tr("Size")});
    setHorizontalHeaderLabels(headerLabels);
    for(int i = 0; i < CHUNK_TYPE_COUNT; ++i)
    {
        const char *chunkType = ROMUtils::ChunkTypeString[i];
        QStandardItem *checkableItem = new QStandardItem();
        checkableItem->setCheckable(true);
        insertRow(i, QList<QStandardItem*>({checkableItem, new QStandardItem(chunkType), new QStandardItem(0)}));
    }
    QObject::connect(this, &ChunkManagerModel::itemChanged, this, &ChunkManagerModel::UpdateTristate);
}

ChunkManagerModel::~ChunkManagerModel()
{

}

void ChunkManagerModel::AddChunk(unsigned int chunk)
{
    // Get info to add to the model
    unsigned int chunkType = ROMUtils::CurrentFile[chunk + 8];
    unsigned int chunkLen = *reinterpret_cast<unsigned short*>(ROMUtils::CurrentFile + chunk + 4);
    unsigned int extLen = (unsigned int) *reinterpret_cast<unsigned char*>(ROMUtils::CurrentFile + chunk + 9) << 16;
    unsigned int chunkSize = chunkLen + extLen + 12;

    // Add the row to the model under the appropriate chunk type row
    QStandardItem *parent = item(chunkType);
    int rowNumber = parent->rowCount();
    QStandardItem *checkableItem = new QStandardItem();
    checkableItem->setCheckable(true);
    parent->setChild(rowNumber, 0, checkableItem);
    parent->setChild(rowNumber, 1, new QStandardItem("0x" + QString::number(chunk, 16).toUpper()));
    parent->setChild(rowNumber, 2, new QStandardItem(QString::number(chunkSize)));

    // Update info in chunk type row
    QStandardItem *oldSizeItem = item(chunkType, 2);
    int newSize = oldSizeItem->text().toInt() + chunkSize;
    QStandardItem *totalSizeItem = new QStandardItem(QString::number(newSize));
    totalSizeItem->setCheckable(false);
    setItem(chunkType, 2, totalSizeItem);
}

void ChunkManagerModel::RemoveChunk(unsigned int chunk)
{
    for(int i = 0; i < rowCount(); ++i)
    {
        QStandardItem *categoryRow = item(i);
        for(int j = 0; j < categoryRow->rowCount(); ++j)
        {
            QStandardItem *chunkRow = categoryRow->child(j, 0);
            unsigned int chunkAddress = chunkRow->text().mid(2).toUInt(nullptr, 16);
            if(chunk == chunkAddress)
            {
                categoryRow->removeRow(j);
                return;
            }
        }
    }
}

QVector<unsigned int> ChunkManagerModel::GetCheckedChunks()
{
    QVector<unsigned int> checkedChunks;
    for(int i = 0; i < rowCount(); ++i)
    {
        QStandardItem *categoryRow = item(i);
        for(int j = 0; j < categoryRow->rowCount(); ++j)
        {
            QStandardItem *chunkRow = categoryRow->child(j, 0);
            if(chunkRow->checkState() == Qt::Checked)
            {
                unsigned int chunkAddress = chunkRow->text().mid(2).toUInt(nullptr, 16);
                checkedChunks.append(chunkAddress);
            }
        }
    }
    return checkedChunks;
}

void ChunkManagerModel::UpdateTristate(QStandardItem *item)
{
    if(!CanUpdateTristate) return; // disallow cascading updates
    CanUpdateTristate = false;

    QStandardItem *parent = item->parent();
    if(parent)
    {
        // Update tristate of the parent
        int checkedChildren = 0;
        int childCount = parent->rowCount();
        for(int i = 0; i < childCount; ++i)
        {
            QStandardItem *child = parent->child(i);
            if(child->checkState() == Qt::Checked)
            {
                ++checkedChildren;
            }
        }
        Qt::CheckState state =
            (checkedChildren == 0) ?          Qt::Unchecked :
            (checkedChildren == childCount) ? Qt::Checked :
                                              Qt::PartiallyChecked;
        parent->setCheckState(state);
    }
    else
    {
        // Update checked state of the children
        Qt::CheckState state = item->checkState();
        int childCount = item->rowCount();
        if(childCount)
        {
            for(int i = 0; i < childCount; ++i)
            {
                QStandardItem *child = item->child(i);
                child->setCheckState(state);
            }
        }
        else
        {
            item->setCheckState(Qt::Unchecked);
        }
    }

    CanUpdateTristate = true;
}