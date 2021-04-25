#ifndef CHUNKEDITORTREEVIEW_H
#define CHUNKEDITORTREEVIEW_H

#include "ROMUtils.h"

#include <QStandardItemModel>
#include <QTreeView>
#include <QVector>

class ChunkManagerModel : public QStandardItemModel
{
    Q_OBJECT
public:
    ChunkManagerModel(QWidget *parent = nullptr);
    ~ChunkManagerModel();
    void AddChunk(unsigned int chunk);
    void RemoveChunk(unsigned int chunk);
    QVector<unsigned int> GetCheckedChunks();
    bool CanUpdateTristate = false;
public slots:
    void UpdateTristate(QStandardItem *item);
};

class ChunkManagerTreeView : public QTreeView
{
    Q_OBJECT
public:
    ChunkManagerTreeView(QWidget *parent = nullptr);
    ~ChunkManagerTreeView();
private:
    ChunkManagerModel Model;
};

#endif // CHUNKEDITORTREEVIEW_H
