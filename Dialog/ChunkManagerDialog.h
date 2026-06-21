#ifndef CHUNKMANAGERDIALOG_H
#define CHUNKMANAGERDIALOG_H

#include <QDialog>
#include <QAbstractButton>

#include "ChunkUtils.h"
#include "ChunkManagerTreeView.h"

namespace Ui
{
    class ChunkManagerDialog;
}

class QHexDocument;

class ChunkManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChunkManagerDialog(QWidget *parent = nullptr);
    ~ChunkManagerDialog();

private slots:
    // Left panel
    void on_pushButton_Refresh_clicked();
    void on_pushButton_InvalidateAllOrphans_clicked();

    // Right panel
    void on_pushButton_InvalidateCurrentOrphanedChunk_clicked();
    void on_pushButton_FixBrokenHeader_clicked();
    void on_pushButton_CycleCorruptionDetails_clicked();
    void on_pushButton_CloneAndRelinkDuplicates_clicked();

    // Dialog
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_treeView_clicked(const QModelIndex &index);

private:
    void PopulateTreeView();
    void SyncHexView(unsigned int chunkAddr,
                     unsigned int extraHighlightAddr = 0,
                     unsigned int extraHighlightSize = 0,
                     const QColor &extraColor = QColor());
    void SyncInfoPanel(unsigned int chunkAddr);
    void EnableContextButtons(unsigned int chunkAddr);

    // Corruption helper
    bool HasCorruption(int issues) const;
    int  CorruptionFlags() const;

    // Duplicate helpers
    bool IsCleanDuplicate(unsigned int chunkAddr) const;
    unsigned int FindFreeSpace(unsigned int size) const;

    // Length suggestion helpers
    void AppendLengthSuggestion(unsigned int chunkAddr, unsigned char chunkType,
                                 QString &html) const;
    unsigned int SuggestLength(unsigned int chunkAddr, unsigned char chunkType) const;

    Ui::ChunkManagerDialog *ui;
    ChunkManagerModel *m_model = nullptr;
    QHexDocument *m_hexDocument = nullptr;
    bool m_hasUnsavedChanges = false;

    unsigned char *m_tempROMData = nullptr;
    unsigned int m_tempROMLength = 0;

    QMap<unsigned int, int> m_chunkIssues;
    QVector<ChunkUtils::PointerIssue> m_pointerIssues;
    QVector<ChunkUtils::OverlapIssue> m_overlapIssues;
    QVector<ChunkUtils::RawPointerEntry> m_rawPointers;
    QMap<unsigned int, ChunkUtils::ChunkReference> m_chunkRefs;

    unsigned int m_selectedChunkAddr = 0;

    // Cycle state
    QMap<unsigned int, int> m_cycleIdx_Corruption;  // chunkAddr → substep
    QMap<unsigned int, int> m_cycleIdx_Duplicate;    // chunkAddr → index
};

#endif // CHUNKMANAGERDIALOG_H
