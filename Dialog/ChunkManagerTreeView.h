#ifndef CHUNKMANAGERTREEVIEW_H
#define CHUNKMANAGERTREEVIEW_H

#include "ChunkUtils.h"

#include <QStandardItemModel>
#include <QTreeView>
#include <QVector>
#include <QMap>

class ChunkManagerModel : public QStandardItemModel
{
    Q_OBJECT

    friend class ChunkManagerTreeView;

public:
    explicit ChunkManagerModel(QObject *parent = nullptr);
    ~ChunkManagerModel();

    /// <summary>
    /// Populate the model with chunks, organized by type and color-coded by issue.
    /// </summary>
    void Populate(const QVector<unsigned int> &allChunks,
                  const QMap<unsigned int, ChunkUtils::ChunkReference> &refs,
                  const QMap<unsigned int, int> &chunkIssues);

    /// <summary>
    /// Find a chunk row by address using binary search within its type category.
    /// Returns the address item (column 0) or nullptr.
    /// </summary>
    QStandardItem *FindChunk(unsigned int chunkAddr);

    /// <summary>
    /// Remove a chunk from the model and update category totals.
    /// </summary>
    void RemoveChunk(unsigned int chunkAddr);

    /// <summary>
    /// Get all chunk addresses that are currently in the model.
    /// </summary>
    QVector<unsigned int> GetAllChunkAddresses();

    /// <summary>
    /// Get the chunk address encoded in a model index (from column 0).
    /// </summary>
    unsigned int GetChunkAddress(const QModelIndex &index) const;

    /// <summary>
    /// Get the issue flags for a chunk.
    /// </summary>
    int GetChunkIssues(unsigned int chunkAddr) const;

private:
    static QColor IssueColor(int issues);
    static QString IssueText(int issues);

    QMap<unsigned int, int> m_chunkIssues;
};

class ChunkManagerTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit ChunkManagerTreeView(QWidget *parent = nullptr);
    ~ChunkManagerTreeView();

    ChunkManagerModel *GetModel() { return &m_model; }

    void SelectChunk(unsigned int chunkAddr);

    void Populate(const QVector<unsigned int> &allChunks,
                  const QMap<unsigned int, ChunkUtils::ChunkReference> &refs,
                  const QMap<unsigned int, int> &chunkIssues);

private:
    ChunkManagerModel m_model;
};

#endif // CHUNKMANAGERTREEVIEW_H
