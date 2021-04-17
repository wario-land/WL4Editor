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
    // Configure the tree view
    setModel(&Model);
    setSelectionMode(QAbstractItemView::NoSelection);

    // Populate model
    QVector<unsigned int> allChunks = FindAllChunksInROM(ROMUtils::CurrentFile, ROMUtils::CurrentFileSize, WL4Constants::AvailableSpaceBeginningInROM, ROMUtils::SaveDataChunkType::InvalidationChunk, true);
    for(unsigned int chunk : allChunks)
    {
        Model.addChunk(chunk);
    }
    Model.refresh();
    for(int i = 0; i < Model.columnCount(); ++i)
    {
        resizeColumnToContents(i);
    }
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
    RootItem->ItemData = { tr("Sel"), tr("Chunk"), tr("Size") };
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
    QString info = QString("%1 (%2 chunks)").arg(totalSize).arg(0);
    ItemData = { QString("a"), heading, info };
}

void ChunkEntryItem::refresh()
{
    QString heading = "0x" + QString::number(Address, 16);
    unsigned int chunkLen = *reinterpret_cast<unsigned short*>(ROMUtils::CurrentFile + Address + 4);
    unsigned int extLen = (unsigned int) *reinterpret_cast<unsigned char*>(ROMUtils::CurrentFile + Address + 9) << 16;
    unsigned int size = chunkLen + extLen + 12;
    ItemData = { QString("b"), heading, size };
}

//---------------------------------------------------------------------------------------------------------------------------
// QAbstractItemModel implementation functions
// https://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html
// https://stackoverflow.com/questions/8175122/qtreeview-checkboxes
// https://forum.qt.io/topic/66080/qtreeview-with-checkbox/5
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

    if (role == Qt::CheckStateRole && index.column() == 0)
        return static_cast<int>(item->Checked ? Qt::Checked : Qt::Unchecked);

    return item->data(index.column());
}

bool ChunkEditorTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    GenericTreeItem *item = static_cast<GenericTreeItem*>(index.internalPointer());

        if (index.column() == 0) {
            if (role == Qt::EditRole)
            {
                return false;
            }
            if (role == Qt::CheckStateRole)
            {
                item->Checked = value.toBool();
                QVector<int> tmpvec;  tmpvec << role;
                emit dataChanged(index, index, tmpvec);
                return true;
            }
        }

        return QAbstractItemModel::setData(index, value, role);
}

Qt::ItemFlags ChunkEditorTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;;

    if (index.column() == 0)
        flags |= Qt::ItemIsUserCheckable;

    return flags;
}

QVariant ChunkEditorTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return RootItem->data(section);

    return QVariant();
}
