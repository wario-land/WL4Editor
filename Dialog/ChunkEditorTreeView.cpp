#include "ChunkEditorTreeView.h"
#include "WL4EditorWindow.h"

extern WL4EditorWindow *singleton;

/// <summary>
/// Construct an instance of the ChunkEditorTreeView.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
ChunkEditorTreeView::ChunkEditorTreeView(QWidget *parent) : QTreeView(parent), Model(this)
{
    // Populate model
    QVector<unsigned int> allChunks = FindAllChunksInROM(ROMUtils::CurrentFile, ROMUtils::CurrentFileSize, WL4Constants::AvailableSpaceBeginningInROM, ROMUtils::SaveDataChunkType::InvalidationChunk, true);
    for(unsigned int chunk : allChunks)
    {
        Model.addChunk(chunk);
    }
    Model.refresh();
}

ChunkEditorTreeView::~ChunkEditorTreeView()
{

}

//---------------------------------------------------------------------------------------------------------------------------
// ChunkEditorTreeModel functions
//---------------------------------------------------------------------------------------------------------------------------

/// <summary>
/// Construct an instance of the ChunkEditorTreeModel.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
ChunkEditorTreeModel::ChunkEditorTreeModel(QObject *parent) : QAbstractItemModel(parent)
{
    RootItem = new GenericTreeItem();
    RootItem->ItemData = { tr("Chunk"), tr("Size") };
}

ChunkEditorTreeModel::~ChunkEditorTreeModel()
{
    delete RootItem;
}

void ChunkEditorTreeModel::addChunk(int address)
{
    // Find if there is already an entry in the tree model for this chunk type
    enum ROMUtils::SaveDataChunkType chunkType = static_cast<enum ROMUtils::SaveDataChunkType>(ROMUtils::CurrentFile[address + 8]);
    auto chunkTypeItemItr = std::find_if(RootItem->ChildItems.begin(), RootItem->ChildItems.end(),
        [chunkType](GenericTreeItem* item)
        {
            ChunkTypeItem* chunkTypeItem = dynamic_cast<ChunkTypeItem*>(item);
            return chunkTypeItem->ChunkType == chunkType;
        });

    // Get the chunk type entry to add the chunk item entry
    GenericTreeItem *chunkTypeItem = (chunkTypeItemItr == RootItem->ChildItems.end()) ?
        new ChunkTypeItem(RootItem, chunkType) : *chunkTypeItemItr;

    // Create chunk item entry
    new ChunkEntryItem(chunkTypeItem, address);
}

//---------------------------------------------------------------------------------------------------------------------------
// Item refresh functions
//---------------------------------------------------------------------------------------------------------------------------

void ChunkTypeItem::refresh()
{
    int totalSize = 0;
    for(GenericTreeItem *item : ChildItems)
    {
        auto child = dynamic_cast<ChunkEntryItem*>(item);
        child->refresh();
        totalSize += child->ItemData[1].toUInt();
    }
    QString heading = ROMUtils::ChunkTypeString[ChunkType];
    QString info = QString("%1 (total)").arg(totalSize);
    ItemData = { heading, info };
}

void ChunkEntryItem::refresh()
{
    QString heading = "0x" + QString::number(Address, 16);
    unsigned int chunkLen = *reinterpret_cast<unsigned short*>(ROMUtils::CurrentFile + Address + 4);
    unsigned int extLen = (unsigned int) *reinterpret_cast<unsigned char*>(ROMUtils::CurrentFile + Address + 9) << 16;
    unsigned int size = chunkLen + extLen + 12;
    ItemData = { heading, size };
}

//---------------------------------------------------------------------------------------------------------------------------
// QAbstractItemModel implementation functions
// https://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html
//---------------------------------------------------------------------------------------------------------------------------

int GenericTreeItem::row() const
{
    if (ParentItem)
        return ParentItem->ChildItems.indexOf(const_cast<GenericTreeItem*>(this));

    return 0;
}

int GenericTreeItem::columnCount() const
{
    return ChildItems.count();
}

int GenericTreeItem::childCount() const
{
    return ChildItems.count();
}

QVariant GenericTreeItem::data(int column) const
{
    if (column < 0 || column >= ItemData.size())
        return QVariant();
    return ItemData.at(column);
}

QModelIndex ChunkEditorTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    GenericTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = RootItem;
    else
        parentItem = static_cast<GenericTreeItem*>(parent.internalPointer());

    GenericTreeItem *childItem = parentItem->ChildItems[row];
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex ChunkEditorTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    GenericTreeItem *childItem = static_cast<GenericTreeItem*>(index.internalPointer());
    GenericTreeItem *parentItem = childItem->ParentItem;

    if (parentItem == RootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int ChunkEditorTreeModel::rowCount(const QModelIndex &parent) const
{
    GenericTreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = RootItem;
    else
        parentItem = static_cast<GenericTreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

int ChunkEditorTreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<GenericTreeItem*>(parent.internalPointer())->columnCount();
    return RootItem->columnCount();
}

QVariant ChunkEditorTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    GenericTreeItem *item = static_cast<GenericTreeItem*>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags ChunkEditorTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}

QVariant ChunkEditorTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return RootItem->data(section);

    return QVariant();
}
