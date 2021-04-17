#ifndef CHUNKEDITORTREEVIEW_H
#define CHUNKEDITORTREEVIEW_H

#include "ROMUtils.h"

#include <QAbstractItemModel>
#include <QTreeView>
#include <QVector>



class GenericTreeItem
{
public:
    // Functions
    GenericTreeItem(GenericTreeItem *parentItem = nullptr) :
        ParentItem(parentItem) { if(ParentItem) ParentItem->ChildItems.append(this); }
    ~GenericTreeItem() { qDeleteAll(ChildItems); }
    GenericTreeItem operator[](int i) { return ChildItems[i]; }
    int size() { return ChildItems.size(); }
    QVector<QVariant> data() { return ItemData; }
    virtual void refresh() { for(GenericTreeItem *item : ChildItems) { item->refresh(); }}

    // Members
    QVector<QVariant> ItemData;
    QVector<GenericTreeItem*> ChildItems;
    GenericTreeItem *ParentItem;
    bool Checked = false;

    // QAbstractItemModel implementation functions
    int row() const;
    int columnCount() const;
    int childCount() const;
    QVariant data(int column) const;
};



class ChunkTypeItem : public GenericTreeItem
{
public:
    ChunkTypeItem(GenericTreeItem *parentItem, const enum ROMUtils::SaveDataChunkType &chunkType) :
        GenericTreeItem(parentItem), ChunkType(chunkType) { }
    void refresh() override;
    enum ROMUtils::SaveDataChunkType ChunkType;
};



class ChunkEntryItem : public GenericTreeItem
{
public:
    ChunkEntryItem(GenericTreeItem *parentItem, unsigned int address) :
        GenericTreeItem(parentItem), Address(address) { }
    void refresh() override;
private:
    unsigned int Address;
};



class ChunkEditorTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    ChunkEditorTreeModel(QObject *parent = nullptr);
    ~ChunkEditorTreeModel();
    void refresh() { RootItem->refresh(); }
    void addChunk(int address);

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
private:
    GenericTreeItem *RootItem;
};



class ChunkEditorTreeView : public QTreeView
{
    Q_OBJECT
public:
    ChunkEditorTreeView(QWidget *parent = nullptr);
    ~ChunkEditorTreeView();
private:
    ChunkEditorTreeModel Model;
};



#endif // CHUNKEDITORTREEVIEW_H
