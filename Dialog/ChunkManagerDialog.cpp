#include "ChunkManagerDialog.h"
#include "ui_ChunkManagerDialog.h"
#include "ROMUtils.h"
#include "WL4EditorWindow.h"
#include "SettingsUtils.h"

#include <QHexView/qhexview.h>
#include <QHexView/model/qhexdocument.h>

#include <QDialogButtonBox>
#include <QFile>
#include <QMessageBox>
#include <cstring>
#include <algorithm>

extern WL4EditorWindow *singleton;

// ============================================================================
//  Constructor / Destructor
// ============================================================================

ChunkManagerDialog::ChunkManagerDialog(ChunkManagerMode mode, QWidget *parent)
    : QDialog(parent), ui(new Ui::ChunkManagerDialog), m_mode(mode)
{
    ui->setupUi(this);

    m_tempROMLength = ROMUtils::CurrentROMMetadata.Length;
    m_tempROMData = new unsigned char[m_tempROMLength];
    memcpy(m_tempROMData, ROMUtils::CurrentROMMetadata.ROMDataPtr, m_tempROMLength);

    ROMUtils::TempROMMetadata.Length = m_tempROMLength;
    ROMUtils::TempROMMetadata.FilePath = ROMUtils::CurrentROMMetadata.FilePath;
    ROMUtils::TempROMMetadata.ROMDataPtr = m_tempROMData;
    ROMUtils::ROMFileMetadata = &ROMUtils::TempROMMetadata;

    // Configure button box based on mode
    switch (m_mode)
    {
    case ChunkManagerMode::Standalone:
    case ChunkManagerMode::LoadGuard:
        ui->buttonBox->setStandardButtons(
            QDialogButtonBox::Save | QDialogButtonBox::Cancel);
        break;
    case ChunkManagerMode::SaveGuard:
        ui->buttonBox->setStandardButtons(
            QDialogButtonBox::Abort | QDialogButtonBox::Save);
        ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Unsafe Save"));
        break;
    }

    m_model = new ChunkManagerModel(this);
    ui->treeView->setModel(m_model);
    ui->treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(ui->treeView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, [this](const QModelIndex &current, const QModelIndex &) {
                m_selectedChunkAddr = m_model->GetChunkAddress(current);
                if (m_selectedChunkAddr)
                {
                    SyncHexView(m_selectedChunkAddr);
                    SyncInfoPanel(m_selectedChunkAddr);
                    EnableContextButtons(m_selectedChunkAddr);
                }
            });

    if (!ROMUtils::CurrentROMMetadata.FilePath.isEmpty())
    {
        m_hexDocument = QHexDocument::fromFile(
            ROMUtils::CurrentROMMetadata.FilePath, this);
        if (m_hexDocument)
        {
            ui->frame->setDocument(m_hexDocument);
            ui->frame->setReadOnly(true);
        }
    }

    PopulateTreeView();
}

ChunkManagerDialog::~ChunkManagerDialog()
{
    ROMUtils::ROMFileMetadata = &ROMUtils::CurrentROMMetadata;
    delete[] m_tempROMData;
    ROMUtils::TempROMMetadata.ROMDataPtr = nullptr;
    ROMUtils::TempROMMetadata.Length = 0;
    delete ui;
}

// ============================================================================
//  PopulateTreeView
// ============================================================================

void ChunkManagerDialog::PopulateTreeView()
{
    using namespace ROMUtils;

    QVector<unsigned int> allChunks = FindAllChunksInROM(
        ROMFileMetadata->ROMDataPtr, ROMFileMetadata->Length,
        WL4Constants::AvailableSpaceBeginningInROM,
        InvalidationChunk, true);

    m_rawPointers.clear();
    m_chunkRefs = ChunkUtils::GetAllChunkReferences(&m_rawPointers);
    m_pointerIssues.clear();
    m_overlapIssues.clear();
    m_chunkIssues = ChunkUtils::ScanChunkIssues(
        allChunks, m_chunkRefs, m_pointerIssues, m_overlapIssues);

    m_cycleIdx_Corruption.clear();

    m_model->Populate(allChunks, m_chunkRefs, m_chunkIssues);

    ui->treeView->resizeColumnToContents(0);
    ui->treeView->setColumnWidth(0, 155);
    ui->treeView->setColumnWidth(3, 60);
}

// ============================================================================
//  Helpers
// ============================================================================

bool ChunkManagerDialog::HasCorruption(int issues) const
{
    return issues & (ChunkUtils::BrokenHeader | ChunkUtils::HasMisalignedPtr | ChunkUtils::Overlap);
}

int ChunkManagerDialog::CorruptionFlags() const
{
    return ChunkUtils::BrokenHeader | ChunkUtils::HasMisalignedPtr | ChunkUtils::Overlap;
}

bool ChunkManagerDialog::IsCleanDuplicate(unsigned int chunkAddr) const
{
    int issues = m_chunkIssues.value(chunkAddr, ChunkUtils::NoIssue);
    if (!(issues & ChunkUtils::DuplicateRef))
        return false;
    if (issues & ~ChunkUtils::DuplicateRef)  // any other issue
    {
        // Allow Corrupted flags only if the chunk is ALSO DuplicateRef
        // But clean means NO other issues at all
        return false;
    }
    if (m_chunkRefs.contains(chunkAddr) &&
        !m_chunkRefs[chunkAddr].ChildrenChunkLocalOffset.isEmpty())
        return false;
    return true;
}

unsigned int ChunkManagerDialog::FindFreeSpace(unsigned int size) const
{
    using namespace ROMUtils;
    unsigned char *ROMData = ROMFileMetadata->ROMDataPtr;
    unsigned int ROMLength = ROMFileMetadata->Length;

    QVector<unsigned int> chunks = FindAllChunksInROM(
        ROMData, ROMLength,
        WL4Constants::AvailableSpaceBeginningInROM,
        InvalidationChunk, true);

    unsigned int cursor = WL4Constants::AvailableSpaceBeginningInROM;
    for (unsigned int addr : chunks)
    {
        if (addr > cursor + size + 11)
            return cursor;
        unsigned int cSize = GetChunkDataLength(addr);
        cursor = addr + 12 + cSize;
    }
    if (cursor + 12 + size <= ROMLength)
        return cursor;
    return 0;
}

// ============================================================================
//  SyncHexView
// ============================================================================

void ChunkManagerDialog::SyncHexView(unsigned int chunkAddr,
                                     unsigned int extraHighlightAddr,
                                     unsigned int extraHighlightSize,
                                     const QColor &extraColor)
{
    if (!m_hexDocument) return;

    ui->frame->clearMetadata();

    int themeId = SettingsUtils::GetKey(SettingsUtils::IniKeys::EditorThemeId).toInt();
    bool isDark = (themeId == 1);

    unsigned int chunkSize = ROMUtils::GetChunkDataLength(chunkAddr);

    ui->frame->setBackground(chunkAddr, chunkAddr + 12,
        QBrush(isDark ? QColor(50, 70, 160) : QColor(180, 200, 255)));
    ui->frame->setBackground(chunkAddr + 12, chunkAddr + 12 + chunkSize,
        QBrush(isDark ? QColor(140, 50, 50) : QColor(255, 200, 200)));

    if (extraHighlightSize > 0 && extraColor.isValid())
    {
        ui->frame->setBackground(extraHighlightAddr,
                                  extraHighlightAddr + extraHighlightSize,
                                  QBrush(extraColor));
    }

    ui->frame->hexCursor()->move(static_cast<qint64>(chunkAddr));
}

// ============================================================================
//  AppendLengthSuggestion — type-specific repair hints
// ============================================================================

unsigned int ChunkManagerDialog::SuggestLength(unsigned int chunkAddr,
                                                unsigned char chunkType) const
{
    using namespace ROMUtils;
    unsigned char *ROMData = ROMFileMetadata->ROMDataPtr;
    unsigned int storedLen = GetChunkDataLength(chunkAddr);

    switch (chunkType)
    {
    case RoomHeaderChunkType:
        // Each room header is 40 bytes. If stored length is misaligned,
        // suggest the floor multiple as the most likely correct value.
        if (storedLen % 40 != 0)
            return (storedLen / 40) * 40;
        return storedLen;  // already valid multiple — no change needed

    case LevelNameChunkType:
        return 34;  // fixed: 17 UTF-16LE characters × 2 bytes

    case EntityListChunk:
    {
        unsigned int dataStart = chunkAddr + 12;
        for (unsigned int p = dataStart; p + 2 < dataStart + storedLen; p += 3)
        {
            if (ROMData[p]==0xFF && ROMData[p+1]==0xFF && ROMData[p+2]==0xFF)
                return (p + 3) - dataStart;
        }
        return 0;  // terminator not found
    }

    case CameraPointerTableType:
    {
        unsigned int dataStart = chunkAddr + 12;
        for (unsigned int p = dataStart; p + 3 < dataStart + storedLen; p += 4)
        {
            unsigned int ptr = *reinterpret_cast<unsigned int *>(ROMData + p) & 0x7FFFFFF;
            if (ptr == WL4Constants::CameraRecordSentinel)
                return (p + 4) - dataStart;
        }
        return 0;  // sentinel not found
    }

    case DoorChunkType:
    {
        unsigned int dataStart = chunkAddr + 12;
        for (unsigned int p = dataStart; p + 11 < dataStart + storedLen; p += 12)
        {
            bool allZero = true;
            for (int b = 0; b < 12; ++b)
                if (ROMData[p + b] != 0) { allZero = false; break; }
            if (allZero)
                return (p + 12) - dataStart;  // includes terminator
        }
        return 0;  // terminator not found
    }

    default:
        return 0;  // not inferable
    }
}

void ChunkManagerDialog::AppendLengthSuggestion(unsigned int chunkAddr,
                                                 unsigned char chunkType,
                                                 QString &html) const
{
    using namespace ROMUtils;
    unsigned char *ROMData = ROMFileMetadata->ROMDataPtr;
    unsigned int storedLen = GetChunkDataLength(chunkAddr);

    html += "<br><b>Repair analysis:</b><br>";

    switch (chunkType)
    {
    case RoomHeaderChunkType:
    {
        // Room header = array of __RoomHeader (40 bytes each).
        // roomCount is at levelHeaderAddr + 1, but we need to find which level.
        // The source description gives us passage/stage hints.
        // Since we can't easily map back to levelHeaderAddr from chunkAddr alone,
        // we provide a bounds-based check: data length should be a multiple of 40.
        if (storedLen % 40 == 0)
        {
            int rooms = storedLen / 40;
            html += QString("&nbsp;&nbsp;Stored length %1 = %2 × 40 bytes → %3 room(s).<br>")
                .arg(storedLen).arg(rooms).arg(rooms);
            html += QString("&nbsp;&nbsp;<span style='color:green'>Length is valid</span> "
                            "(multiple of 40). Verify room count in level config.<br>");
        }
        else
        {
            // Suggest the nearest valid length
            int roomsLo = storedLen / 40;
            int roomsHi = roomsLo + 1;
            html += QString("&nbsp;&nbsp;<span style='color:red'>Stored length %1 is not a multiple of 40.</span><br>"
                            "&nbsp;&nbsp;Nearest valid: %2 bytes (%3 rooms) or %4 bytes (%5 rooms).<br>")
                .arg(storedLen)
                .arg(roomsLo * 40).arg(roomsLo)
                .arg(roomsHi * 40).arg(roomsHi);
        }
        break;
    }
    case DoorChunkType:
    {
        unsigned int expected = SuggestLength(chunkAddr, chunkType);
        if (expected > 0)
        {
            int doorCount = (expected / 12) - 1;  // subtract terminator
            html += QString("&nbsp;&nbsp;Found all-zero terminator → %1 door(s) + terminator = %2 bytes.<br>"
                            "&nbsp;&nbsp;Stored length: %3 bytes%4<br>")
                .arg(doorCount).arg(expected).arg(storedLen)
                .arg(storedLen != expected
                     ? QString(" <span style='color:red'>(mismatch)</span>")
                     : " <span style='color:green'>(correct)</span>");
        }
        else
        {
            html += QString("&nbsp;&nbsp;No all-zero terminator found. "
                            "Stored length: %1 bytes. Check for corruption.<br>")
                .arg(storedLen);
        }
        break;
    }
    case LevelNameChunkType:
    {
        unsigned int expected = SuggestLength(chunkAddr, chunkType);
        html += QString("&nbsp;&nbsp;Level names are fixed at %1 bytes (17 chars × 2 bytes UTF-16LE).<br>"
                        "&nbsp;&nbsp;Stored length: %2 bytes%3<br>")
            .arg(expected).arg(storedLen)
            .arg(storedLen != expected
                 ? QString(" <span style='color:red'>(should be %1)</span>").arg(expected)
                 : " <span style='color:green'>(correct)</span>");
        break;
    }
    case EntityListChunk:
    {
        unsigned int expected = SuggestLength(chunkAddr, chunkType);
        if (expected > 0)
        {
            int entries = expected / 3;
            html += QString("&nbsp;&nbsp;Found FF FF FF terminator → %1 entries (%2 bytes).<br>"
                            "&nbsp;&nbsp;Stored length: %3 bytes%4<br>")
                .arg(entries).arg(expected).arg(storedLen)
                .arg(storedLen != expected
                     ? QString(" <span style='color:red'>(mismatch)</span>")
                     : " <span style='color:green'>(correct)</span>");
        }
        else
        {
            html += QString("&nbsp;&nbsp;No FF FF FF terminator found. "
                            "Stored length: %1 bytes. Check for corruption.<br>")
                .arg(storedLen);
        }
        break;
    }
    case CameraPointerTableType:
    {
        unsigned int expected = SuggestLength(chunkAddr, chunkType);
        if (expected > 0)
        {
            int entries = expected / 4;
            html += QString("&nbsp;&nbsp;Found sentinel pointer → %1 entries (%2 bytes).<br>"
                            "&nbsp;&nbsp;Stored length: %3 bytes%4<br>")
                .arg(entries).arg(expected).arg(storedLen)
                .arg(storedLen != expected
                     ? QString(" <span style='color:red'>(mismatch)</span>")
                     : " <span style='color:green'>(correct)</span>");
        }
        else
        {
            html += QString("&nbsp;&nbsp;Camera sentinel not found in stored range. "
                            "Stored length: %1 bytes. Check for corruption.<br>")
                .arg(storedLen);
        }
        break;
    }
    default:
        html += "&nbsp;&nbsp;No automatic length inference available for this chunk type.<br>";
        break;
    }

    // General header guidance
    html += "<br><b>Header repair:</b> if the RATS tag or checksum is corrupt, "
            "use [Fix Broken Header] to restore them from the stored length.<br>"
            "⚠ Only do this if you trust the stored length value.<br>";
}

// ============================================================================
//  SyncInfoPanel — structured corruption analysis
// ============================================================================

void ChunkManagerDialog::SyncInfoPanel(unsigned int chunkAddr)
{
    using namespace ROMUtils;
    unsigned char *ROMData = ROMFileMetadata->ROMDataPtr;

    unsigned int chunkSize = GetChunkDataLength(chunkAddr);
    unsigned char chunkType = ROMData[chunkAddr + 8];
    int issues = m_chunkIssues.value(chunkAddr, ChunkUtils::NoIssue);
    bool hasRef = m_chunkRefs.contains(chunkAddr);

    QString html;
    html += "<html><body style='font-size:11pt;'>";

    // --- Basic identification ---
    html += QString("<b>Address:</b> 0x%1&nbsp;&nbsp;|&nbsp;&nbsp;"
                    "<b>Total:</b> %2 bytes (hdr 12 + data %3)<br>")
        .arg(QString::number(chunkAddr, 16).toUpper())
        .arg(chunkSize + 12).arg(chunkSize);

    if (chunkType < CHUNK_TYPE_COUNT)
        html += QString("<b>Type:</b> %1 (0x%2)")
            .arg(ChunkTypeString[chunkType])
            .arg(QString::number(chunkType, 16).toUpper());
    else
        html += QString("<b>Type:</b> <span style='color:red'>UNKNOWN (0x%1)</span>")
            .arg(QString::number(chunkType, 16).toUpper());

    if (hasRef && !m_chunkRefs[chunkAddr].SourceDescription.isEmpty())
        html += QString("&nbsp;&nbsp;|&nbsp;&nbsp;<b>Source:</b> %1")
            .arg(m_chunkRefs[chunkAddr].SourceDescription);
    html += "<br>";

    // --- RATS header details ---
    unsigned short storedTag = *reinterpret_cast<unsigned short *>(ROMData + chunkAddr);
    bool tagST = (ROMData[chunkAddr+0]=='S' && ROMData[chunkAddr+1]=='T');
    bool tagAR = (ROMData[chunkAddr+2]=='A' && ROMData[chunkAddr+3]=='R');
    unsigned short storedLen16 = *reinterpret_cast<unsigned short *>(ROMData + chunkAddr + 4);
    unsigned short storedChecksum = *reinterpret_cast<unsigned short *>(ROMData + chunkAddr + 6);
    unsigned short expectedChecksum = static_cast<unsigned short>(~storedLen16);
    unsigned char extLen = ROMData[chunkAddr + 9];
    unsigned int fullLen = storedLen16 | (static_cast<unsigned int>(extLen) << 16);

    html += QString("<b>RATS:</b> tag=%1%2 checksum=0x%3 (expect 0x%4) %5<br>")
        .arg(tagST ? "ST" : QString("<span style='color:red'>%1%2</span>")
             .arg(QChar(ROMData[chunkAddr+0])).arg(QChar(ROMData[chunkAddr+1])))
        .arg(tagAR ? "AR" : QString("<span style='color:red'>%1%2</span>")
             .arg(QChar(ROMData[chunkAddr+2])).arg(QChar(ROMData[chunkAddr+3])))
        .arg(storedChecksum, 4, 16, QChar('0'))
        .arg(expectedChecksum, 4, 16, QChar('0'))
        .arg(storedChecksum == expectedChecksum
             ? "<span style='color:green'>OK</span>"
             : "<span style='color:red'>MISMATCH</span>");
    html += QString("<b>Stored length:</b> %1 (0x%2 low + 0x%3 ext)&nbsp;&nbsp;"
                    "<b>Data range:</b> [0x%4, 0x%5)<br>")
        .arg(fullLen)
        .arg(storedLen16, 4, 16, QChar('0'))
        .arg(extLen, 2, 16, QChar('0'))
        .arg(QString::number(chunkAddr + 12, 16).toUpper())
        .arg(QString::number(chunkAddr + 12 + chunkSize, 16).toUpper());

    // --- Overlap analysis ---
    if (issues & ChunkUtils::Overlap)
    {
        html += "<br><b>🟠 Overlap detected:</b><br>";
        for (const auto &oi : m_overlapIssues)
        {
            if (oi.chunkA == chunkAddr)
            {
                html += QString("&nbsp;&nbsp;Chunk <b>0x%1</b> intrudes into this chunk's tail:<br>"
                                "&nbsp;&nbsp;&nbsp;&nbsp;Overlap region: 0x%2 – 0x%3 (%4 bytes)<br>"
                                "&nbsp;&nbsp;&nbsp;&nbsp;This chunk's data ends at 0x%5; "
                                "intruder starts at 0x%6<br>")
                    .arg(QString::number(oi.chunkB, 16).toUpper())
                    .arg(QString::number(oi.overlapStart, 16).toUpper())
                    .arg(QString::number(oi.overlapEnd, 16).toUpper())
                    .arg(oi.overlapEnd - oi.overlapStart)
                    .arg(QString::number(chunkAddr + 12 + chunkSize, 16).toUpper())
                    .arg(QString::number(oi.chunkB, 16).toUpper());
            }
            else if (oi.chunkB == chunkAddr)
            {
                html += QString("&nbsp;&nbsp;This chunk intrudes into chunk <b>0x%1</b>'s tail:<br>"
                                "&nbsp;&nbsp;&nbsp;&nbsp;Overlap region: 0x%2 – 0x%3 (%4 bytes)<br>"
                                "&nbsp;&nbsp;&nbsp;&nbsp;This chunk starts at 0x%5; "
                                "victim ends at 0x%6<br>")
                    .arg(QString::number(oi.chunkA, 16).toUpper())
                    .arg(QString::number(oi.overlapStart, 16).toUpper())
                    .arg(QString::number(oi.overlapEnd, 16).toUpper())
                    .arg(oi.overlapEnd - oi.overlapStart)
                    .arg(QString::number(chunkAddr, 16).toUpper())
                    .arg(QString::number(chunkAddr + 12 + chunkSize, 16).toUpper());
            }
        }
    }

    // --- Misaligned pointer analysis ---
    if (issues & ChunkUtils::HasMisalignedPtr)
    {
        html += "<br><b>🟠 Misaligned pointer(s):</b><br>";
        for (const auto &pi : m_pointerIssues)
        {
            if (pi.affectedChunkAddr == chunkAddr)
            {
                html += QString("&nbsp;&nbsp;ROM 0x%1 → 0x%2 (offset +%3; expected 0x%4)<br>")
                    .arg(QString::number(pi.pointerSourceAddr, 16).toUpper())
                    .arg(QString::number(pi.pointerTargetAddr, 16).toUpper())
                    .arg(pi.pointerTargetAddr - (chunkAddr + 12))
                    .arg(QString::number(chunkAddr + 12, 16).toUpper());
            }
        }
    }

    // --- Length suggestion (type-specific, only for Corrupted chunks) ---
    if (issues & (ChunkUtils::BrokenHeader | ChunkUtils::HasMisalignedPtr | ChunkUtils::Overlap))
    {
        AppendLengthSuggestion(chunkAddr, chunkType, html);
    }

    // --- Duplicate reference info ---
    if (issues & ChunkUtils::DuplicateRef)
    {
        QVector<unsigned int> sources;
        unsigned int target = chunkAddr + 12;
        for (const auto &rp : m_rawPointers)
            if (rp.targetAddr == target && !sources.contains(rp.sourceAddr))
                sources.append(rp.sourceAddr);

        html += QString("<br><b>🔵 Duplicate reference — %1 pointer(s) → 0x%2:</b><br>")
            .arg(sources.size())
            .arg(QString::number(target, 16).toUpper());
        for (unsigned int src : sources)
            html += QString("&nbsp;&nbsp;ROM 0x%1<br>")
                .arg(QString::number(src, 16).toUpper());
    }

    // --- Orphan ---
    if (issues & ChunkUtils::Orphan)
    {
        html += "<br><b>🔴 Orphan:</b> no pointer references this chunk.<br>";
    }

    // --- Parent / children structural info (if healthy enough to show) ---
    if (hasRef)
    {
        const auto &ref = m_chunkRefs[chunkAddr];
        if (ref.ParentChunkAddress)
        {
            html += QString("<br><b>Parent:</b> 0x%1<br>")
                .arg(QString::number(ref.ParentChunkAddress, 16).toUpper());
        }
        if (!ref.ChildrenChunkLocalOffset.isEmpty())
        {
            html += "<b>Children:</b><br>";
            for (unsigned int off : ref.ChildrenChunkLocalOffset)
            {
                unsigned int ptr = PointerFromData(chunkAddr + 12 + off);
                bool broken = ref.BrokenChildrenChunkLocalOffset.contains(off);
                html += QString("&nbsp;&nbsp;+0x%1 → 0x%2%3<br>")
                    .arg(QString::number(off, 16).toUpper())
                    .arg(ptr ? "0x" + QString::number(ptr, 16).toUpper() : "(null)")
                    .arg(broken ? " <span style='color:red'>(broken)</span>" : "");
            }
        }
    }

    html += "</body></html>";
    ui->textEdit->setHtml(html);
}

// ============================================================================
//  EnableContextButtons
// ============================================================================

void ChunkManagerDialog::EnableContextButtons(unsigned int chunkAddr)
{
    int issues = m_chunkIssues.value(chunkAddr, ChunkUtils::NoIssue);

    ui->pushButton_InvalidateCurrentOrphanedChunk->setEnabled(
        issues & ChunkUtils::Orphan);

    ui->pushButton_FixBrokenHeader->setEnabled(
        issues & ChunkUtils::BrokenHeader);

    ui->pushButton_CycleCorruptionDetails->setEnabled(
        HasCorruption(issues));

    ui->pushButton_CloneAndRelinkDuplicates->setEnabled(
        IsCleanDuplicate(chunkAddr));
}

// ============================================================================
//  Left: Refresh
// ============================================================================

void ChunkManagerDialog::on_pushButton_Refresh_clicked()
{
    PopulateTreeView();
}

// ============================================================================
//  Left: Invalidate All Orphans
// ============================================================================

void ChunkManagerDialog::on_pushButton_InvalidateAllOrphans_clicked()
{
    QVector<unsigned int> orphans;
    for (auto it = m_chunkIssues.constBegin(); it != m_chunkIssues.constEnd(); ++it)
        if (it.value() & ChunkUtils::Orphan)
            orphans.append(it.key());

    if (orphans.isEmpty())
    {
        QMessageBox::information(this, tr("No Orphans"),
                                 tr("No orphaned chunks found."));
        return;
    }

    if (QMessageBox::question(this, tr("Confirm"),
            tr("Invalidate %1 orphaned chunk(s)?").arg(orphans.size()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    unsigned char *romData = ROMUtils::ROMFileMetadata->ROMDataPtr;
    for (unsigned int addr : orphans)
    {
        strncpy(reinterpret_cast<char *>(romData + addr), "STAR_INV", 8);
        m_model->RemoveChunk(addr);
        m_chunkIssues.remove(addr);
        m_chunkRefs.remove(addr);
    }
    m_hasUnsavedChanges = true;
}

// ============================================================================
//  Right: Invalidate This Orphan
// ============================================================================

void ChunkManagerDialog::on_pushButton_InvalidateCurrentOrphanedChunk_clicked()
{
    if (!m_selectedChunkAddr) return;
    if (!(m_chunkIssues.value(m_selectedChunkAddr, 0) & ChunkUtils::Orphan)) return;

    if (QMessageBox::question(this, tr("Confirm"),
            tr("Invalidate orphan at 0x%1?").arg(m_selectedChunkAddr, 0, 16),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    strncpy(reinterpret_cast<char *>(ROMUtils::ROMFileMetadata->ROMDataPtr + m_selectedChunkAddr),
            "STAR_INV", 8);
    m_model->RemoveChunk(m_selectedChunkAddr);
    m_chunkIssues.remove(m_selectedChunkAddr);
    m_chunkRefs.remove(m_selectedChunkAddr);
    m_hasUnsavedChanges = true;
    m_selectedChunkAddr = 0;
}

// ============================================================================
//  Right: Fix Broken Header
// ============================================================================

void ChunkManagerDialog::on_pushButton_FixBrokenHeader_clicked()
{
    if (!m_selectedChunkAddr) return;
    if (!(m_chunkIssues.value(m_selectedChunkAddr, 0) & ChunkUtils::BrokenHeader)) return;

    unsigned char *romData = ROMUtils::ROMFileMetadata->ROMDataPtr;
    unsigned int storedLen = ROMUtils::GetChunkDataLength(m_selectedChunkAddr);
    unsigned char chunkType = romData[m_selectedChunkAddr + 8];

    // Check if length inference is available for this chunk type
    unsigned int suggestedLen = 0;
    if (chunkType < CHUNK_TYPE_COUNT)
        suggestedLen = SuggestLength(m_selectedChunkAddr, chunkType);

    bool canFixLength = (suggestedLen > 0 && suggestedLen != storedLen);

    QString msg = tr("Fix RATS header at 0x%1?\n\n"
                     "Will rewrite \"STAR\" tag and recompute checksum.")
                     .arg(m_selectedChunkAddr, 0, 16);

    if (canFixLength)
    {
        msg += tr("\n\nStored data length: %1 bytes\n"
                  "Suggested length: %2 bytes\n\n"
                  "Also correct the length field?")
                   .arg(storedLen).arg(suggestedLen);
    }
    else
    {
        msg += tr("\n\nStored data length: %1 bytes.\n\n"
                  "⚠ Only do this if you trust the stored length.\n"
                  "Check 'Repair analysis' in the info panel first.")
                   .arg(storedLen);
    }

    if (QMessageBox::question(this, tr("Confirm"), msg,
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    // Fix RATS tag
    romData[m_selectedChunkAddr + 0] = 'S';
    romData[m_selectedChunkAddr + 1] = 'T';
    romData[m_selectedChunkAddr + 2] = 'A';
    romData[m_selectedChunkAddr + 3] = 'R';

    // Apply suggested length if available and different
    if (canFixLength)
    {
        storedLen = suggestedLen;
        *reinterpret_cast<unsigned short *>(romData + m_selectedChunkAddr + 4) =
            static_cast<unsigned short>(suggestedLen & 0xFFFF);
        romData[m_selectedChunkAddr + 9] =
            static_cast<unsigned char>((suggestedLen >> 16) & 0xFF);
    }

    // Recompute checksum from (possibly updated) stored length
    unsigned short len = *reinterpret_cast<unsigned short *>(romData + m_selectedChunkAddr + 4);
    *reinterpret_cast<unsigned short *>(romData + m_selectedChunkAddr + 6) =
        static_cast<unsigned short>(~len);

    // Fix type byte if unknown
    if (romData[m_selectedChunkAddr + 8] >= CHUNK_TYPE_COUNT
        && m_chunkRefs.contains(m_selectedChunkAddr))
        romData[m_selectedChunkAddr + 8] =
            static_cast<unsigned char>(m_chunkRefs[m_selectedChunkAddr].ChunkType);

    PopulateTreeView();
    m_hasUnsavedChanges = true;
}

// ============================================================================
//  Right: Cycle Corruption Details
//  Substeps: 0=header, 1..N=overlaps, (N+1)..(N+M)=misaligned pointers
// ============================================================================

void ChunkManagerDialog::on_pushButton_CycleCorruptionDetails_clicked()
{
    if (!m_selectedChunkAddr) return;

    int issues = m_chunkIssues.value(m_selectedChunkAddr, 0);
    if (!HasCorruption(issues)) return;

    int themeId = SettingsUtils::GetKey(SettingsUtils::IniKeys::EditorThemeId).toInt();
    bool isDark = (themeId == 1);

    // Count corruption instances
    int overlapCount = 0, misalignedCount = 0;
    for (const auto &oi : m_overlapIssues)
        if (oi.chunkA == m_selectedChunkAddr || oi.chunkB == m_selectedChunkAddr)
            ++overlapCount;
    for (const auto &pi : m_pointerIssues)
        if (pi.affectedChunkAddr == m_selectedChunkAddr)
            ++misalignedCount;

    int totalSteps = 0;
    if (issues & ChunkUtils::BrokenHeader) totalSteps++;      // step 0
    totalSteps += overlapCount;
    totalSteps += misalignedCount;
    if (totalSteps == 0) return;

    int &step = m_cycleIdx_Corruption[m_selectedChunkAddr];
    step = (step + 1) % totalSteps;

    // Map step to what to highlight
    if (issues & ChunkUtils::BrokenHeader)
    {
        if (step == 0)
        {
            // Show header
            SyncHexView(m_selectedChunkAddr,
                        m_selectedChunkAddr, 12,
                        isDark ? QColor(180, 100, 0) : QColor(255, 180, 80));
            ui->frame->hexCursor()->move(static_cast<qint64>(m_selectedChunkAddr));
            return;
        }
    }

    int idx = (issues & ChunkUtils::BrokenHeader) ? 1 : 0;

    // Overlaps
    for (const auto &oi : m_overlapIssues)
    {
        if (oi.chunkA == m_selectedChunkAddr || oi.chunkB == m_selectedChunkAddr)
        {
            if (step == idx)
            {
                unsigned int sz = oi.overlapEnd - oi.overlapStart;
                SyncHexView(m_selectedChunkAddr,
                            oi.overlapStart, sz,
                            isDark ? QColor(80, 80, 80) : QColor(180, 180, 180));
                ui->frame->hexCursor()->move(static_cast<qint64>(oi.overlapStart));
                return;
            }
            ++idx;
        }
    }

    // Misaligned pointers
    for (const auto &pi : m_pointerIssues)
    {
        if (pi.affectedChunkAddr == m_selectedChunkAddr)
        {
            if (step == idx)
            {
                SyncHexView(m_selectedChunkAddr,
                            pi.pointerSourceAddr, 4,
                            isDark ? QColor(40, 120, 40) : QColor(180, 255, 180));
                ui->frame->hexCursor()->move(static_cast<qint64>(pi.pointerSourceAddr));
                return;
            }
            ++idx;
        }
    }
}

// ============================================================================
//  Right: Clone & Relink Duplicates
// ============================================================================

void ChunkManagerDialog::on_pushButton_CloneAndRelinkDuplicates_clicked()
{
    if (!m_selectedChunkAddr) return;
    if (!IsCleanDuplicate(m_selectedChunkAddr)) return;
    if (!m_chunkRefs.contains(m_selectedChunkAddr)) return;

    unsigned int chunkSize = ROMUtils::GetChunkDataLength(m_selectedChunkAddr);
    unsigned int totalPerClone = 12 + chunkSize;
    unsigned int targetDataAddr = m_selectedChunkAddr + 12;

    QVector<unsigned int> pointerSources;
    for (const auto &rp : m_rawPointers)
        if (rp.targetAddr == targetDataAddr && !pointerSources.contains(rp.sourceAddr))
            pointerSources.append(rp.sourceAddr);

    int dupCount = pointerSources.size();
    if (dupCount < 2) return;
    int cloneCount = dupCount - 1;

    if (QMessageBox::question(this, tr("Confirm Clone & Relink"),
            tr("Chunk 0x%1 has %2 duplicate pointers.\n"
               "Create %3 clone(s)? Need %4 bytes free space.\n"
               "First pointer (at 0x%5) stays; others get own clones.")
                .arg(m_selectedChunkAddr, 0, 16).arg(dupCount).arg(cloneCount)
                .arg(cloneCount * totalPerClone)
                .arg(pointerSources[0], 0, 16),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    unsigned char *romData = ROMUtils::ROMFileMetadata->ROMDataPtr;
    unsigned char typeByte = romData[m_selectedChunkAddr + 8];
    QVector<unsigned int> cloneAddrs;
    int relinked = 0;

    for (int i = 1; i < dupCount; ++i)
    {
        unsigned int freeAddr = FindFreeSpace(chunkSize);
        if (!freeAddr)
        {
            QMessageBox::warning(this, tr("Out of Space"),
                tr("Only %1/%2 clones created. Remaining duplicates untouched.")
                    .arg(relinked).arg(cloneCount));
            break;
        }

        unsigned short lowLen = static_cast<unsigned short>(chunkSize & 0xFFFF);
        unsigned char extLen = static_cast<unsigned char>((chunkSize >> 16) & 0xFF);

        romData[freeAddr + 0] = 'S';
        romData[freeAddr + 1] = 'T';
        romData[freeAddr + 2] = 'A';
        romData[freeAddr + 3] = 'R';
        *reinterpret_cast<unsigned short *>(romData + freeAddr + 4) = lowLen;
        *reinterpret_cast<unsigned short *>(romData + freeAddr + 6) = static_cast<unsigned short>(~lowLen);
        romData[freeAddr + 8] = typeByte;
        romData[freeAddr + 9] = extLen;
        memset(romData + freeAddr + 10, 0, 2);

        memcpy(romData + freeAddr + 12,
               romData + m_selectedChunkAddr + 12, chunkSize);

        *reinterpret_cast<unsigned int *>(romData + pointerSources[i]) =
            (freeAddr + 12) | 0x08000000;

        cloneAddrs.append(freeAddr);
        relinked++;
    }

    m_hasUnsavedChanges = true;
    PopulateTreeView();

    QStringList addrStrs;
    for (unsigned int a : cloneAddrs)
        addrStrs << QString("0x%1").arg(a, 0, 16);
    singleton->GetOutputWidgetPtr()->PrintString(
        QString("Cloned 0x%1 → %2 clone(s) [%3], relinked %4 ptrs")
            .arg(m_selectedChunkAddr, 0, 16).arg(cloneAddrs.size())
            .arg(addrStrs.join(", ")).arg(relinked));
}

// ============================================================================
//  Button Box: Save / Cancel
// ============================================================================

void ChunkManagerDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    switch (ui->buttonBox->standardButton(button))
    {
    case QDialogButtonBox::Save:
    {
        // Common: copy temp data back to CurrentROMMetadata
        memcpy(ROMUtils::CurrentROMMetadata.ROMDataPtr,
               ROMUtils::ROMFileMetadata->ROMDataPtr,
               ROMUtils::ROMFileMetadata->Length);
        ROMUtils::CurrentROMMetadata.Length = ROMUtils::ROMFileMetadata->Length;

        if (m_hasUnsavedChanges)
        {
            switch (m_mode)
            {
            case ChunkManagerMode::Standalone:
            case ChunkManagerMode::LoadGuard:
            {
                // Write to disk and accept
                QFile file(ROMUtils::ROMFileMetadata->FilePath);
                if (file.open(QIODevice::WriteOnly))
                {
                    file.write(
                        reinterpret_cast<const char *>(ROMUtils::ROMFileMetadata->ROMDataPtr),
                        static_cast<qint64>(ROMUtils::ROMFileMetadata->Length));
                    file.close();
                    m_hasUnsavedChanges = false;
                    m_wasSaved = true;
                    singleton->GetOutputWidgetPtr()->PrintString(
                        "Chunk Manager: Changes saved.");
                }
                else
                {
                    QMessageBox::warning(this, tr("Save Error"),
                        tr("Failed to open ROM file for writing."));
                    return;
                }
                break;
            }
            case ChunkManagerMode::SaveGuard:
                // Do NOT write to disk — SaveFile() will do that after PostProcessing.
                // Just mark as saved and accept, so SaveFile() continues.
                m_hasUnsavedChanges = false;
                m_wasSaved = true;
                singleton->GetOutputWidgetPtr()->PrintString(
                    "Chunk Manager: Changes applied (Unsafe Save).");
                break;
            }
        }
        accept();
        break;
    }
    case QDialogButtonBox::Cancel:
    case QDialogButtonBox::Abort:
        reject();
        break;
    default:
        break;
    }
}

// ============================================================================
//  Tree View Click
// ============================================================================

void ChunkManagerDialog::on_treeView_clicked(const QModelIndex &index)
{
    m_selectedChunkAddr = m_model->GetChunkAddress(index);
    if (m_selectedChunkAddr)
    {
        SyncHexView(m_selectedChunkAddr);
        SyncInfoPanel(m_selectedChunkAddr);
        EnableContextButtons(m_selectedChunkAddr);
    }
}
