#ifndef CHUNKEDITORINFOGROUPBOX_H
#define CHUNKEDITORINFOGROUPBOX_H

#include "ROMUtils.h"
#include "ChunkManagerTreeView.h"

#include <QMap>
#include <QGroupBox>
#include <QBoxLayout>

class ChunkEntryHighlightAction : public QObject
{
    Q_OBJECT
public:
    ChunkEntryHighlightAction(ChunkManagerTreeView *treeView, unsigned int chunk) :
        TreeView(treeView), Chunk(chunk) {}
    ~ChunkEntryHighlightAction() {}
private:
    ChunkManagerTreeView *TreeView;
    unsigned int Chunk;
public slots:
    void HighlightChunkConnector() {TreeView->HighlightChunk(Chunk);}
};

class ChunkManagerInfoGroupBox : public QGroupBox
{
    Q_OBJECT
public:
    ChunkManagerInfoGroupBox(QWidget *parent = nullptr);
    ~ChunkManagerInfoGroupBox();
    void SetTreeView(ChunkManagerTreeView *t) {TreeView = t;}
private:
    QVector<QWidget*> GetInfoFromChunk(unsigned int chunk);
    QWidget *GetOverallChunkInfo();
    void ClearLayout(QLayout *layout);
    QBoxLayout Layout;
    QMap<unsigned int, struct ROMUtils::ChunkReference> ChunkReferences = ROMUtils::GetAllChunkReferences();
    struct ROMUtils::ChunkReference GetChunkReference(unsigned int chunk)
    {
        struct ROMUtils::ChunkReference defaultRef = {ROMUtils::InvalidationChunk};
        return ChunkReferences.contains(chunk) ? ChunkReferences[chunk] : defaultRef;
    }
    ChunkManagerTreeView *TreeView;
    QVector<ChunkEntryHighlightAction*> Actions;
public slots:
    void UpdateContents(const QModelIndex &current, const QModelIndex &previous);
};

#endif // CHUNKEDITORINFOGROUPBOX_H
