#include "ChunkManagerTreeView.h"
#include "ROMUtils.h"
#include "WL4EditorWindow.h"
#include "SettingsUtils.h"

extern WL4EditorWindow *singleton;

// ============================================================================
//  ChunkManagerModel
// ============================================================================

ChunkManagerModel::ChunkManagerModel(QObject *parent)
    : QStandardItemModel(parent)
{
    // 4 columns: Address, Type, Size, Issues (no checkbox column)
    QStringList headerLabels({tr("Address"), tr("Type"), tr("Size"), tr("Issues")});
    setHorizontalHeaderLabels(headerLabels);

    // Create category rows for each chunk type
    for (int i = 0; i < CHUNK_TYPE_COUNT; ++i)
    {
        const char *chunkType = ROMUtils::ChunkTypeString[i];
        QStandardItem *typeNameItem = new QStandardItem(chunkType);     // col 0: type name
        QStandardItem *emptyItem = new QStandardItem("");                // col 1: empty
        QStandardItem *totalSizeItem = new QStandardItem("0");           // col 2: total size
        QStandardItem *issuesItem = new QStandardItem("");               // col 3: aggregated issues
        insertRow(i, QList<QStandardItem*>({
            typeNameItem, emptyItem, totalSizeItem, issuesItem
        }));
    }
}

ChunkManagerModel::~ChunkManagerModel()
{
}

QColor ChunkManagerModel::IssueColor(int issues)
{
    int themeId = SettingsUtils::GetKey(SettingsUtils::IniKeys::EditorThemeId).toInt();
    bool isDark = (themeId == 1);

    if (issues & ChunkUtils::Orphan)
        return isDark ? QColor(180, 60, 60)   : QColor(255, 180, 180);
    // BrokenHeader, HasMisalignedPtr, Overlap → unified Corrupted
    if (issues & (ChunkUtils::BrokenHeader | ChunkUtils::HasMisalignedPtr | ChunkUtils::Overlap))
        return isDark ? QColor(180, 120, 40)  : QColor(255, 210, 130);
    if (issues & ChunkUtils::DuplicateRef)
        return isDark ? QColor(40, 100, 160)  : QColor(180, 230, 255);
    return QColor();
}

QString ChunkManagerModel::IssueText(int issues)
{
    QStringList parts;
    if (issues & ChunkUtils::Orphan)
        parts << QString::fromUtf8("\xF0\x9F\x94\xB4");          // 🔴
    if (issues & (ChunkUtils::BrokenHeader | ChunkUtils::HasMisalignedPtr | ChunkUtils::Overlap))
        parts << QString::fromUtf8("\xF0\x9F\x9F\xA0");          // 🟠
    if (issues & ChunkUtils::DuplicateRef)
        parts << QString::fromUtf8("\xF0\x9F\x94\xB5");          // 🔵
    return parts.join(" ");
}

void ChunkManagerModel::Populate(
    const QVector<unsigned int> &allChunks,
    const QMap<unsigned int, ChunkUtils::ChunkReference> &refs,
    const QMap<unsigned int, int> &chunkIssues)
{
    // Remove all existing child rows (but keep category rows)
    for (int i = 0; i < rowCount(); ++i)
    {
        QStandardItem *categoryRow = item(i);
        categoryRow->removeRows(0, categoryRow->rowCount());
        setItem(i, 2, new QStandardItem("0"));   // col 2: total size
        setItem(i, 3, new QStandardItem(""));    // col 3: issues
    }

    m_chunkIssues = chunkIssues;

    // Add each chunk under its type category
    for (unsigned int chunkAddr : allChunks)
    {
        unsigned char *romData = ROMUtils::ROMFileMetadata->ROMDataPtr;
        unsigned int chunkType = romData[chunkAddr + 8];
        unsigned int chunkSize = ROMUtils::GetChunkDataLength(chunkAddr) + 12;

        if (chunkType >= CHUNK_TYPE_COUNT)
            continue;

        QStandardItem *parent = item(static_cast<int>(chunkType));
        int rowNumber = parent->rowCount();

        // Column 0: address (color-coded)
        QStandardItem *addrItem = new QStandardItem(
            "0x" + QString::number(chunkAddr, 16).toUpper());

        // Column 1: chunk type name
        QStandardItem *typeItem = new QStandardItem(
            ROMUtils::ChunkTypeString[chunkType]);

        // Column 2: size
        QStandardItem *sizeItem = new QStandardItem(
            QString::number(chunkSize));

        // Column 3: issue indicators
        int issues = chunkIssues.value(chunkAddr, ChunkUtils::NoIssue);
        QStandardItem *issuesItem = new QStandardItem(IssueText(issues));

        // Apply color coding
        QColor bgColor = IssueColor(issues);
        if (bgColor.isValid())
        {
            QBrush bg(bgColor);
            addrItem->setBackground(bg);
            typeItem->setBackground(bg);
            sizeItem->setBackground(bg);
        }

        parent->setChild(rowNumber, 0, addrItem);
        parent->setChild(rowNumber, 1, typeItem);
        parent->setChild(rowNumber, 2, sizeItem);
        parent->setChild(rowNumber, 3, issuesItem);

        // Update category total size (column 2)
        QStandardItem *oldSizeItem = item(static_cast<int>(chunkType), 2);
        int newSize = oldSizeItem->text().toInt() + chunkSize;
        setItem(static_cast<int>(chunkType), 2, new QStandardItem(QString::number(newSize)));
    }

    // Aggregate issues for each parent row (column 3)
    for (int i = 0; i < rowCount(); ++i)
    {
        QStandardItem *categoryRow = item(i);
        int aggregatedIssues = ChunkUtils::NoIssue;
        QColor worstColor;
        for (int j = 0; j < categoryRow->rowCount(); ++j)
        {
            QStandardItem *addrItem = categoryRow->child(j, 0);
            if (!addrItem) continue;
            int childIssues = chunkIssues.value(
                addrItem->text().mid(2).toUInt(nullptr, 16),
                ChunkUtils::NoIssue);
            aggregatedIssues |= childIssues;
            QColor childColor = IssueColor(childIssues);
            if (childColor.isValid() && !worstColor.isValid())
                worstColor = childColor;
        }
        if (aggregatedIssues)
        {
            QStandardItem *parentIssues = new QStandardItem(IssueText(aggregatedIssues));
            if (worstColor.isValid())
                parentIssues->setBackground(QBrush(worstColor));
            else
                parentIssues->setBackground(QBrush(IssueColor(aggregatedIssues)));
            setItem(i, 3, parentIssues);
        }
    }
}

QStandardItem *ChunkManagerModel::FindChunk(unsigned int chunkAddr)
{
    unsigned char *romData = ROMUtils::ROMFileMetadata->ROMDataPtr;
    unsigned int chunkType = romData[chunkAddr + 8];

    if (chunkType >= CHUNK_TYPE_COUNT)
        return nullptr;

    QStandardItem *categoryRow = item(static_cast<int>(chunkType));
    int childCount = categoryRow->rowCount();

    int low = 0, high = childCount;
    while (low < high)
    {
        int middle = (low + high) / 2;
        QStandardItem *addrItem = categoryRow->child(middle, 0);
        if (!addrItem)
            return nullptr;
        unsigned int addr = addrItem->text().mid(2).toUInt(nullptr, 16);
        if (chunkAddr == addr)
            return addrItem;
        else if (chunkAddr < addr)
            high = middle;
        else
            low = middle + 1;
    }
    return nullptr;
}

void ChunkManagerModel::RemoveChunk(unsigned int chunkAddr)
{
    QStandardItem *addrItem = FindChunk(chunkAddr);
    if (!addrItem)
        return;

    QStandardItem *categoryRow = addrItem->parent();
    int parentIndex = categoryRow->index().row();
    int childIndex = addrItem->row();

    // Update total size (column 2)
    int chunkSize = categoryRow->child(childIndex, 2)->text().toInt();
    QStandardItem *oldSizeItem = item(parentIndex, 2);
    int oldSize = oldSizeItem->text().toInt();
    setItem(parentIndex, 2, new QStandardItem(QString::number(oldSize - chunkSize)));

    categoryRow->removeRow(childIndex);
    m_chunkIssues.remove(chunkAddr);

    // Recompute parent's aggregated issues (column 3)
    int aggregatedIssues = ChunkUtils::NoIssue;
    QColor worstColor;
    for (int j = 0; j < categoryRow->rowCount(); ++j)
    {
        QStandardItem *childAddrItem = categoryRow->child(j, 0);
        if (!childAddrItem) continue;
        unsigned int addr = childAddrItem->text().mid(2).toUInt(nullptr, 16);
        int childIssues = m_chunkIssues.value(addr, ChunkUtils::NoIssue);
        aggregatedIssues |= childIssues;
        QColor childColor = IssueColor(childIssues);
        if (childColor.isValid() && !worstColor.isValid())
            worstColor = childColor;
    }
    QStandardItem *parentIssues = new QStandardItem(IssueText(aggregatedIssues));
    if (worstColor.isValid())
        parentIssues->setBackground(QBrush(worstColor));
    else if (aggregatedIssues)
        parentIssues->setBackground(QBrush(IssueColor(aggregatedIssues)));
    setItem(parentIndex, 3, parentIssues);
}

QVector<unsigned int> ChunkManagerModel::GetAllChunkAddresses()
{
    QVector<unsigned int> result;
    for (int i = 0; i < rowCount(); ++i)
    {
        QStandardItem *categoryRow = item(i);
        for (int j = 0; j < categoryRow->rowCount(); ++j)
        {
            QStandardItem *addrItem = categoryRow->child(j, 0);
            if (addrItem)
                result.append(addrItem->text().mid(2).toUInt(nullptr, 16));
        }
    }
    return result;
}

unsigned int ChunkManagerModel::GetChunkAddress(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    QStandardItem *modelItem = this->itemFromIndex(index);
    if (!modelItem)
        return 0;

    QStandardItem *parent = modelItem->parent();
    if (parent)
    {
        // Child row — column 0 holds address
        QStandardItem *addrItem = parent->child(modelItem->row(), 0);
        if (addrItem)
            return addrItem->text().mid(2).toUInt(nullptr, 16);
    }

    return 0;
}

QModelIndex ChunkManagerModel::IndexOfChunk(unsigned int chunkAddr) const
{
    for (int i = 0; i < rowCount(); ++i)
    {
        QStandardItem *categoryRow = item(i);
        for (int j = 0; j < categoryRow->rowCount(); ++j)
        {
            QStandardItem *addrItem = categoryRow->child(j, 0);
            if (addrItem && addrItem->text().mid(2).toUInt(nullptr, 16) == chunkAddr)
                return addrItem->index();
        }
    }
    return QModelIndex();
}

int ChunkManagerModel::GetChunkIssues(unsigned int chunkAddr) const
{
    return m_chunkIssues.value(chunkAddr, ChunkUtils::NoIssue);
}

// ============================================================================
//  ChunkManagerTreeView
// ============================================================================

ChunkManagerTreeView::ChunkManagerTreeView(QWidget *parent)
    : QTreeView(parent), m_model(this)
{
    setModel(&m_model);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    resizeColumnToContents(0);
    setColumnWidth(0, 155);
}

ChunkManagerTreeView::~ChunkManagerTreeView()
{
}

void ChunkManagerTreeView::SelectChunk(unsigned int chunkAddr)
{
    QStandardItem *item = m_model.FindChunk(chunkAddr);
    if (item)
    {
        QModelIndex idx = m_model.indexFromItem(item);
        selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}

void ChunkManagerTreeView::Populate(
    const QVector<unsigned int> &allChunks,
    const QMap<unsigned int, ChunkUtils::ChunkReference> &refs,
    const QMap<unsigned int, int> &chunkIssues)
{
    m_model.Populate(allChunks, refs, chunkIssues);

    for (int i = 0; i < m_model.columnCount(); ++i)
        resizeColumnToContents(i);
    setColumnWidth(0, 155);

    collapseAll();
    for (int i = 0; i < m_model.rowCount(); ++i)
    {
        QStandardItem *categoryRow = m_model.item(i);
        if (categoryRow && categoryRow->rowCount() > 0)
            expand(categoryRow->index());
    }
}
