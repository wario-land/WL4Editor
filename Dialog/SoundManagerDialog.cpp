#include "SoundManagerDialog.h"
#include "ui_SoundManagerDialog.h"

#include "Sound/SoundExport.h"
#include "Sound/SoundImporter.h"
#include "Sound/SoundParser.h"
#include "Sound/SoundSave.h"
#include "ROMUtils.h"
#include "WL4EditorWindow.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QFormLayout>
#include <QGroupBox>
#include <QHash>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLayout>
#include <QLayoutItem>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

extern WL4EditorWindow *singleton;

namespace
{
    constexpr int RoleEntryType = Qt::UserRole;
    constexpr int RoleEntryId = Qt::UserRole + 1;
    constexpr int RoleParentId = Qt::UserRole + 2;

    QString hexAddress(quint32 address)
    {
        return QStringLiteral("0x") + QString::number(address, 16).toUpper();
    }

    QString yesNo(bool value)
    {
        return value ? QStringLiteral("Yes") : QStringLiteral("No");
    }
} // namespace

SoundManagerDialog::SoundManagerDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SoundManagerDialog)
{
    ui->setupUi(this);

    setWindowTitle(tr("Sound Manager"));
    ui->progressBar->setValue(0);
    ui->progressBar->setRange(0, 100);
    ui->labelStatus->setText(tr("Changed: 0"));
    ui->pushButtonExportAll->setEnabled(true);
    ui->pushButtonImportAll->setEnabled(true);
    ui->pushButtonExportAll->setToolTip(tr("Export all songs, banks and samples to the sound project folder."));
    ui->pushButtonImportAll->setToolTip(tr("Import edited .s files from the sound project folder."));

    entryModel = new QStandardItemModel(this);
    entryModel->setHorizontalHeaderLabels({ tr("Name"), tr("ID"), tr("Type"), tr("Status") });
    ui->treeViewEntries->setModel(entryModel);
    ui->treeViewEntries->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->treeViewEntries->setUniformRowHeights(true);
    ui->treeViewEntries->setAlternatingRowColors(true);
    ui->treeViewEntries->header()->setStretchLastSection(false);
    ui->treeViewEntries->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeViewEntries->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->treeViewEntries->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->treeViewEntries->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    ui->verticalLayout_Details->setAlignment(Qt::AlignTop);

    // QTreeView has no currentChanged signal; the signal lives on the
    // selection model. This connection must be manual.
    connect(ui->treeViewEntries->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this, &SoundManagerDialog::onEntryCurrentChanged);

    ParseSoundData();
    PopulateTree();
    UpdateStatusLabel();
}

SoundManagerDialog::~SoundManagerDialog()
{
    delete ui;
}

void SoundManagerDialog::ParseSoundData()
{
    soundData = Sound::ParseResult{};

    if (!ROMUtils::ROMFileMetadata->ROMDataPtr)
    {
        QMessageBox::warning(this, tr("Sound Manager"), tr("No ROM is loaded."));
        return;
    }

    soundData = Sound::SoundParser::parse(ROMUtils::ROMFileMetadata->ROMDataPtr,
                                          ROMUtils::ROMFileMetadata->Length);
    ui->progressBar->setValue(0);

    if (!soundData.error.isEmpty())
    {
        QMessageBox::warning(this, tr("Sound Manager"), soundData.error);
    }
}

QList<QStandardItem *> SoundManagerDialog::CreateEntryRow(const QString &name, int id,
                                                          EntryType type, const QString &typeText,
                                                          const QString &status, int parentId) const
{
    auto *nameItem = new QStandardItem(name);
    nameItem->setEditable(false);
    nameItem->setData(static_cast<int>(type), RoleEntryType);
    nameItem->setData(id, RoleEntryId);
    nameItem->setData(parentId, RoleParentId);

    auto *idItem = new QStandardItem(QString::number(id));
    idItem->setEditable(false);
    auto *typeItem = new QStandardItem(typeText);
    typeItem->setEditable(false);
    auto *statusItem = new QStandardItem(status);
    statusItem->setEditable(false);

    return { nameItem, idItem, typeItem, statusItem };
}

const Sound::Bank *SoundManagerDialog::FindBank(int bankId) const
{
    for (const Sound::Bank &bank : soundData.banks)
    {
        if (bank.id == bankId)
            return &bank;
    }
    return nullptr;
}

const Sound::Song *SoundManagerDialog::FindSong(int songId) const
{
    for (const Sound::Song &song : soundData.songs)
    {
        if (song.id == songId)
            return &song;
    }
    return nullptr;
}

void SoundManagerDialog::PopulateTree()
{
    populatingTree = true;
    entryModel->removeRows(0, entryModel->rowCount());

    if (!soundData.error.isEmpty())
    {
        populatingTree = false;
        return;
    }

    const bool showEmpty = ui->checkBoxShowEmpty->isChecked();
    const bool showChanged = ui->checkBoxShowChanged->isChecked();

    auto makeRoot = [this](const QString &title) -> QStandardItem *
    {
        auto *nameItem = new QStandardItem(title);
        nameItem->setEditable(false);
        QList<QStandardItem *> row;
        row << nameItem
            << new QStandardItem(QString())
            << new QStandardItem(QString())
            << new QStandardItem(QString());
        for (int i = 1; i < row.size(); ++i)
            row[i]->setEditable(false);
        entryModel->appendRow(row);
        return nameItem;
    };

    // Songs: a flat, id-ordered list. There is no category metadata in the ROM.
    QStandardItem *songsRoot = makeRoot(tr("Songs"));
    QVector<const Sound::Song *> songs;
    for (const Sound::Song &song : soundData.songs)
    {
        if (song.state == Sound::SongState::Empty && !showEmpty)
            continue;
        if (showChanged && !song.changed)
            continue;
        songs.append(&song);
    }
    std::sort(songs.begin(), songs.end(), [](const Sound::Song *a, const Sound::Song *b)
    {
        return a->id < b->id;
    });
    for (const Sound::Song *song : songs)
    {
        QString stateText = QStringLiteral("Vanilla");
        if (song->state == Sound::SongState::Empty)
            stateText = QStringLiteral("Empty");
        else if (song->state == Sound::SongState::Custom)
            stateText = QStringLiteral("Custom");
        if (song->changed)
            stateText = QStringLiteral("Changed");

        songsRoot->appendRow(CreateEntryRow(song->name, song->id, EntryType::Song,
                                            tr("Song"), stateText));
    }

    // Banks and Programs
    QStandardItem *banksRoot = makeRoot(tr("Banks"));
    for (const Sound::Bank &bank : soundData.banks)
    {
        int usedPrograms = 0;
        for (const Sound::Program &program : bank.programs)
        {
            if (program.used)
                ++usedPrograms;
        }

        QString roleText;
        switch (bank.role)
        {
        case Sound::BankRole::Music: roleText = tr("Music"); break;
        case Sound::BankRole::SubBank: roleText = tr("Sub-bank"); break;
        case Sound::BankRole::Custom: roleText = tr("Custom"); break;
        }

        QList<QStandardItem *> bankRow = CreateEntryRow(
                    bank.name, bank.id, EntryType::Bank,
                    QStringLiteral("%1 / %2").arg(roleText, tr("%1 programs").arg(usedPrograms)),
                    bank.isCustom ? tr("Custom") : tr("Vanilla"));
        QStandardItem *bankItem = bankRow.first();
        banksRoot->appendRow(bankRow);

        for (const Sound::Program &program : bank.programs)
        {
            if (!program.used && !showEmpty)
                continue;
            if (showChanged && !bank.isCustom)
                continue;

            bankItem->appendRow(CreateEntryRow(
                                    program.name, program.index, EntryType::Program,
                                    Sound::VoiceTypeName(program.voice.type),
                                    program.used ? tr("Used") : tr("Empty"),
                                    bank.id));
        }
    }

    // Samples
    QStandardItem *samplesRoot = makeRoot(tr("Samples"));
    for (const Sound::Sample &sample : soundData.samples)
    {
        if (sample.id < 0 || sample.pcm.isEmpty())
            continue;
        if (showChanged && !sample.changed)
            continue;
        const QString status = sample.changed
                ? tr("Changed")
                : (sample.isLooped() ? tr("Looped") : tr("One-shot"));
        samplesRoot->appendRow(CreateEntryRow(
                                   sample.name, sample.id, EntryType::Sample,
                                   tr("Sample"), status));
    }

    // Players
    QStandardItem *playersRoot = makeRoot(tr("Players"));
    for (const Sound::Player &player : soundData.players)
    {
        playersRoot->appendRow(CreateEntryRow(
                                   tr("Player %1").arg(player.id), player.id, EntryType::Player,
                                   tr("Player"), tr("%1 tracks").arg(player.maxTracks)));
    }

    populatingTree = false;
    ui->treeViewEntries->expandToDepth(0);
    ApplyFilter();
}

void SoundManagerDialog::ApplyFilter()
{
    if (!entryModel)
        return;

    const QString text = ui->lineEditSearch->text().trimmed();
    const bool hasText = !text.isEmpty();

    auto cellText = [this](QStandardItem *item, int column) -> QString
    {
        if (!item)
            return QString();
        if (item->parent())
        {
            QStandardItem *cell = item->parent()->child(item->row(), column);
            return cell ? cell->text() : QString();
        }
        QStandardItem *cell = entryModel->item(item->row(), column);
        return cell ? cell->text() : QString();
    };

    std::function<bool(QStandardItem *)> filterItem = [&](QStandardItem *item) -> bool
    {
        if (!item)
            return false;

        bool anyChildVisible = false;
        for (int row = 0; row < item->rowCount(); ++row)
        {
            if (filterItem(item->child(row, 0)))
                anyChildVisible = true;
        }

        bool selfMatches = !hasText;
        if (hasText)
        {
            for (int column = 0; column < entryModel->columnCount(); ++column)
            {
                if (cellText(item, column).contains(text, Qt::CaseInsensitive))
                {
                    selfMatches = true;
                    break;
                }
            }
        }

        const bool visible = item->rowCount() > 0 ? (anyChildVisible || selfMatches) : selfMatches;
        const QModelIndex parentIndex = item->parent() ? item->parent()->index() : QModelIndex();
        ui->treeViewEntries->setRowHidden(item->row(), parentIndex, !visible);
        return visible;
    };

    for (int row = 0; row < entryModel->rowCount(); ++row)
        filterItem(entryModel->item(row, 0));
}

void SoundManagerDialog::UpdateStatusLabel()
{
    int changed = 0;
    for (const Sound::Song &song : soundData.songs)
    {
        if (song.changed)
            ++changed;
    }
    for (const Sound::Sample &sample : soundData.samples)
    {
        if (sample.changed)
            ++changed;
    }
    for (const Sound::Bank &bank : soundData.banks)
    {
        for (const Sound::Program &program : bank.programs)
        {
            if (program.changed)
                ++changed;
        }
    }
    ui->labelStatus->setText(tr("Changed: %1").arg(changed));
}

bool SoundManagerDialog::HasStagedChanges() const
{
    // Staged song and sample imports are written only when the user accepts.
    for (const Sound::Song &song : soundData.songs)
    {
        if (song.changed)
            return true;
    }
    for (const Sound::Sample &sample : soundData.samples)
    {
        if (sample.changed)
            return true;
    }
    for (const Sound::Bank &bank : soundData.banks)
    {
        for (const Sound::Program &program : bank.programs)
        {
            if (program.changed)
                return true;
        }
    }
    return false;
}

void SoundManagerDialog::ReloadModelFromROM()
{
    ParseSoundData();
    PopulateTree();
    ClearDetailPanel();
    UpdateStatusLabel();
}

void SoundManagerDialog::accept()
{
    auto printOutput = [](const QString &text)
    {
        if (singleton && singleton->GetOutputWidgetPtr())
            singleton->GetOutputWidgetPtr()->PrintString(text);
    };

    saveSummary.clear();
    if (!HasStagedChanges())
    {
        saveSummary = tr("Sound Manager: No changes to save.");
        QDialog::accept();
        return;
    }

    int changedSongs = 0;
    int changedSamples = 0;
    int changedPrograms = 0;
    for (const Sound::Song &song : soundData.songs)
    {
        if (song.changed)
            ++changedSongs;
    }
    for (const Sound::Sample &sample : soundData.samples)
    {
        if (sample.changed)
            ++changedSamples;
    }
    for (const Sound::Bank &bank : soundData.banks)
    {
        for (const Sound::Program &program : bank.programs)
        {
            if (program.changed)
                ++changedPrograms;
        }
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);

    const QString error = Sound::SoundSave::saveToROM(
                soundData,
                Sound::SoundExport::defaultFolder(ROMUtils::ROMFileMetadata->FilePath),
                [this](int current, int total)
                {
                    ui->progressBar->setRange(0, qMax(1, total));
                    ui->progressBar->setValue(current);
                    QCoreApplication::processEvents();
                });

    QApplication::restoreOverrideCursor();
    ui->progressBar->setValue(0);

    if (!error.isEmpty())
    {
        printOutput(tr("Sound Manager: Save failed: %1").arg(error));
        QMessageBox::critical(this, tr("Save sound data"), error);
        return;
    }

    saveSummary = tr("Sound Manager: Saved %1 song(s), %2 sample(s) and %3 program(s) to ROM.")
            .arg(changedSongs).arg(changedSamples).arg(changedPrograms);
    QDialog::accept();
}

void SoundManagerDialog::reject()
{
    if (HasStagedChanges())
    {
        if (QMessageBox::question(this, tr("Discard changes"),
                                  tr("Discard all unsaved sound edits?")) != QMessageBox::Yes)
            return;
        ReloadModelFromROM();
    }
    QDialog::reject();
}

void SoundManagerDialog::ClearDetailPanel()
{
    while (QLayoutItem *item = ui->verticalLayout_Details->takeAt(0))
    {
        if (QWidget *widget = item->widget())
            delete widget;
        else if (QLayout *layout = item->layout())
            delete layout;
        delete item;
    }
}

QGroupBox *SoundManagerDialog::CreateGroup(const QString &title)
{
    auto *group = new QGroupBox(title, ui->scrollAreaWidgetContents);
    group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *layout = new QFormLayout(group);
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout->setLabelAlignment(Qt::AlignLeft);
    ui->verticalLayout_Details->addWidget(group);
    return group;
}

void SoundManagerDialog::AddReadOnlyRow(QGroupBox *group, const QString &label, const QString &value)
{
    auto *edit = new QLineEdit(value, group);
    edit->setReadOnly(true);
    edit->setCursorPosition(0);
    if (auto *layout = qobject_cast<QFormLayout *>(group->layout()))
        layout->addRow(label, edit);
}

void SoundManagerDialog::BuildSongDetail(const Sound::Song &song)
{
    QGroupBox *general = CreateGroup(tr("General"));
    AddReadOnlyRow(general, tr("ID"), QString::number(song.id));
    AddReadOnlyRow(general, tr("Name"), song.name);

    QString stateText = tr("Vanilla");
    if (song.state == Sound::SongState::Empty)
        stateText = tr("Empty");
    else if (song.state == Sound::SongState::Custom)
        stateText = tr("Custom");
    AddReadOnlyRow(general, tr("State"), stateText);
    AddReadOnlyRow(general, tr("ROM address"), hexAddress(song.romAddress));

    const Sound::Bank *bank = FindBank(song.bankId);
    const QString bankText = bank
            ? QStringLiteral("%1 (%2)").arg(bank->name).arg(bank->id)
            : (song.bankId >= 0 ? QString::number(song.bankId) : tr("Unknown"));
    AddReadOnlyRow(general, tr("Bank"), bankText);
    AddReadOnlyRow(general, tr("Player (ms/me)"),
                   QStringLiteral("%1 / %2").arg(song.ms).arg(song.me));
    AddReadOnlyRow(general, tr("Priority"), QString::number(song.priority));
    AddReadOnlyRow(general, tr("Echo"), QString::number(song.echo));
    AddReadOnlyRow(general, tr("Tracks"), QString::number(song.trackCount));
    if (!song.description.isEmpty())
        AddReadOnlyRow(general, tr("Description"), song.description);

    if (!song.trackAddresses.isEmpty())
    {
        QGroupBox *tracks = CreateGroup(tr("Track Addresses"));
        auto *table = new QTableWidget(song.trackAddresses.size(), 2, tracks);
        table->setHorizontalHeaderLabels({ tr("Track"), tr("ROM address") });
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setStretchLastSection(true);
        for (int row = 0; row < song.trackAddresses.size(); ++row)
        {
            table->setItem(row, 0, new QTableWidgetItem(QString::number(row)));
            table->setItem(row, 1, new QTableWidgetItem(hexAddress(song.trackAddresses[row])));
        }
        table->resizeColumnsToContents();
        if (auto *layout = qobject_cast<QFormLayout *>(tracks->layout()))
            layout->addRow(table);
    }
}

void SoundManagerDialog::BuildBankDetail(const Sound::Bank &bank)
{
    QGroupBox *general = CreateGroup(tr("Bank"));
    AddReadOnlyRow(general, tr("ID"), QString::number(bank.id));
    AddReadOnlyRow(general, tr("Name"), bank.name);
    QString roleText;
    switch (bank.role)
    {
    case Sound::BankRole::Music: roleText = tr("Music"); break;
    case Sound::BankRole::SubBank: roleText = tr("Sub-bank"); break;
    case Sound::BankRole::Custom: roleText = tr("Custom"); break;
    }
    AddReadOnlyRow(general, tr("Role"), roleText);
    AddReadOnlyRow(general, tr("Base"), hexAddress(bank.base));
    AddReadOnlyRow(general, tr("State"), bank.isCustom ? tr("Custom") : tr("Vanilla"));

    int used = 0;
    for (const Sound::Program &program : bank.programs)
    {
        if (program.used)
            ++used;
    }
    AddReadOnlyRow(general, tr("Used programs"), QString::number(used));

    QGroupBox *programs = CreateGroup(tr("Programs"));
    auto *table = new QTableWidget(bank.programs.size(), 5, programs);
    table->setHorizontalHeaderLabels({ tr("Program"), tr("Used"), tr("Type"), tr("Name"), tr("Sample") });
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    for (int row = 0; row < bank.programs.size(); ++row)
    {
        const Sound::Program &program = bank.programs[row];
        table->setItem(row, 0, new QTableWidgetItem(QString::number(program.index)));
        table->setItem(row, 1, new QTableWidgetItem(yesNo(program.used)));
        table->setItem(row, 2, new QTableWidgetItem(Sound::VoiceTypeName(program.voice.type)));
        table->setItem(row, 3, new QTableWidgetItem(program.name));
        QString sampleText;
        if (program.voice.sampleId >= 0 && program.voice.sampleId < soundData.samples.size())
            sampleText = soundData.samples[program.voice.sampleId].name;
        table->setItem(row, 4, new QTableWidgetItem(sampleText));
    }
    table->resizeColumnsToContents();
    if (auto *layout = qobject_cast<QFormLayout *>(programs->layout()))
        layout->addRow(table);
}

void SoundManagerDialog::BuildProgramDetail(int bankId, int programIndex)
{
    const Sound::Bank *bank = FindBank(bankId);
    if (!bank || programIndex < 0 || programIndex >= bank->programs.size())
        return;
    const Sound::Program &program = bank->programs[programIndex];
    const Sound::Voice &voice = program.voice;

    QGroupBox *general = CreateGroup(tr("Program"));
    AddReadOnlyRow(general, tr("Bank"), QStringLiteral("%1 (%2)").arg(bank->name).arg(bank->id));
    AddReadOnlyRow(general, tr("Program"), QString::number(program.index));
    AddReadOnlyRow(general, tr("Name"), program.name);
    AddReadOnlyRow(general, tr("Used"), yesNo(program.used));
    AddReadOnlyRow(general, tr("ROM address"), hexAddress(voice.romAddress));
    AddReadOnlyRow(general, tr("Type"), Sound::VoiceTypeName(voice.type));
    AddReadOnlyRow(general, tr("Key"), QString::number(voice.key));
    AddReadOnlyRow(general, tr("Pan"), QString::number(voice.pan));
    AddReadOnlyRow(general, tr("ADSR"),
                   QStringLiteral("%1 / %2 / %3 / %4")
                   .arg(voice.attack).arg(voice.decay).arg(voice.sustain).arg(voice.release));
    if (voice.sampleId >= 0 && voice.sampleId < soundData.samples.size())
    {
        const Sound::Sample &sample = soundData.samples[voice.sampleId];
        AddReadOnlyRow(general, tr("Sample"),
                       QStringLiteral("%1 (%2)").arg(sample.name, hexAddress(sample.romAddress)));
    }
    if (voice.waveAddress)
        AddReadOnlyRow(general, tr("PSG wave"), hexAddress(voice.waveAddress));

    if (!voice.keyTable.isEmpty())
    {
        QGroupBox *keySplit = CreateGroup(tr("Key Split Table"));
        auto *table = new QTableWidget(voice.keyTable.size(), 4, keySplit);
        table->setHorizontalHeaderLabels({ tr("Key"), tr("Sub-voice"), tr("Type"), tr("Sample") });
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setStretchLastSection(true);
        for (int key = 0; key < voice.keyTable.size(); ++key)
        {
            const quint8 subIndex = voice.keyTable[key];
            QString subType;
            QString sampleName;
            if (subIndex < voice.subVoices.size())
            {
                const Sound::SubVoice &sub = voice.subVoices[subIndex];
                subType = Sound::VoiceTypeName(sub.type);
                if (sub.sampleId >= 0 && sub.sampleId < soundData.samples.size())
                    sampleName = soundData.samples[sub.sampleId].name;
            }
            table->setItem(key, 0, new QTableWidgetItem(QString::number(key)));
            table->setItem(key, 1, new QTableWidgetItem(QString::number(subIndex)));
            table->setItem(key, 2, new QTableWidgetItem(subType));
            table->setItem(key, 3, new QTableWidgetItem(sampleName));
        }
        table->resizeColumnsToContents();
        if (auto *layout = qobject_cast<QFormLayout *>(keySplit->layout()))
            layout->addRow(table);
    }
    else if (!voice.subVoices.isEmpty())
    {
        QGroupBox *drumKit = CreateGroup(tr("Drum Kit"));
        auto *table = new QTableWidget(voice.subVoices.size(), 5, drumKit);
        table->setHorizontalHeaderLabels({ tr("Key"), tr("Type"), tr("Sample"), tr("Pan"), tr("ADSR") });
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setStretchLastSection(true);
        for (int key = 0; key < voice.subVoices.size(); ++key)
        {
            const Sound::SubVoice &sub = voice.subVoices[key];
            QString sampleName;
            if (sub.sampleId >= 0 && sub.sampleId < soundData.samples.size())
                sampleName = soundData.samples[sub.sampleId].name;
            table->setItem(key, 0, new QTableWidgetItem(QString::number(key)));
            table->setItem(key, 1, new QTableWidgetItem(Sound::VoiceTypeName(sub.type)));
            table->setItem(key, 2, new QTableWidgetItem(sampleName));
            table->setItem(key, 3, new QTableWidgetItem(QString::number(sub.pan)));
            table->setItem(key, 4, new QTableWidgetItem(
                               QStringLiteral("%1 / %2 / %3 / %4")
                               .arg(sub.attack).arg(sub.decay).arg(sub.sustain).arg(sub.release)));
        }
        table->resizeColumnsToContents();
        if (auto *layout = qobject_cast<QFormLayout *>(drumKit->layout()))
            layout->addRow(table);
    }

    QGroupBox *raw = CreateGroup(tr("Raw Voice"));
    QString rawText;
    for (int i = 0; i < 12; ++i)
    {
        if (i)
            rawText += QLatin1Char(' ');
        rawText += QStringLiteral("%1").arg(voice.raw[i], 2, 16, QLatin1Char('0')).toUpper();
    }
    AddReadOnlyRow(raw, tr("12 bytes"), rawText);
}

void SoundManagerDialog::BuildSampleDetail(const Sound::Sample &sample)
{
    QGroupBox *general = CreateGroup(tr("Sample"));
    AddReadOnlyRow(general, tr("ID"), QString::number(sample.id));
    AddReadOnlyRow(general, tr("Name"), sample.name);
    AddReadOnlyRow(general, tr("ROM address"), hexAddress(sample.romAddress));
    AddReadOnlyRow(general, tr("Flag"), QStringLiteral("0x%1").arg(sample.flag, 2, 16, QLatin1Char('0')).toUpper());
    AddReadOnlyRow(general, tr("Looped"), yesNo(sample.isLooped()));
    AddReadOnlyRow(general, tr("Pitch"), QStringLiteral("0x%1").arg(sample.pitch, 8, 16, QLatin1Char('0')).toUpper());
    AddReadOnlyRow(general, tr("Sample rate"), QString::number(sample.sampleRate(), 'f', 2) + tr(" Hz"));
    AddReadOnlyRow(general, tr("Loop start"), QString::number(sample.loopStart));
    AddReadOnlyRow(general, tr("Size"), QString::number(sample.size) + tr(" bytes"));
    AddReadOnlyRow(general, tr("SHA-1"), QString::fromLatin1(sample.sha1));

}

void SoundManagerDialog::BuildPlayerDetail(const Sound::Player &player)
{
    QGroupBox *general = CreateGroup(tr("Player"));
    AddReadOnlyRow(general, tr("ID"), QString::number(player.id));
    AddReadOnlyRow(general, tr("Max tracks"), QString::number(player.maxTracks));
    AddReadOnlyRow(general, tr("Flags"), QString::number(player.flags));

    QGroupBox *songs = CreateGroup(tr("Songs Using This Player"));
    auto *table = new QTableWidget(player.songIds.size(), 2, songs);
    table->setHorizontalHeaderLabels({ tr("Song"), tr("Name") });
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    for (int row = 0; row < player.songIds.size(); ++row)
    {
        const int songId = player.songIds[row];
        const Sound::Song *song = FindSong(songId);
        table->setItem(row, 0, new QTableWidgetItem(QString::number(songId)));
        table->setItem(row, 1, new QTableWidgetItem(song ? song->name : QString()));
    }
    table->resizeColumnsToContents();
    if (auto *layout = qobject_cast<QFormLayout *>(songs->layout()))
        layout->addRow(table);
}

void SoundManagerDialog::onEntryCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);

    if (populatingTree || !current.isValid() || !entryModel)
    {
        ClearDetailPanel();
        return;
    }

    QStandardItem *item = entryModel->itemFromIndex(current.siblingAtColumn(0));
    if (!item)
    {
        ClearDetailPanel();
        return;
    }

    const int typeValue = item->data(RoleEntryType).toInt();
    if (typeValue <= 0)
    {
        ClearDetailPanel();
        return;
    }

    const int id = item->data(RoleEntryId).toInt();
    const int parentId = item->data(RoleParentId).toInt();
    ClearDetailPanel();

    switch (static_cast<EntryType>(typeValue))
    {
    case EntryType::Song:
        if (const Sound::Song *song = FindSong(id))
            BuildSongDetail(*song);
        break;
    case EntryType::Bank:
        if (const Sound::Bank *bank = FindBank(id))
            BuildBankDetail(*bank);
        break;
    case EntryType::Program:
        BuildProgramDetail(parentId, id);
        break;
    case EntryType::Sample:
        if (id >= 0 && id < soundData.samples.size())
            BuildSampleDetail(soundData.samples[id]);
        break;
    case EntryType::Player:
        if (id >= 0 && id < soundData.players.size())
            BuildPlayerDetail(soundData.players[id]);
        break;
    }

    ui->scrollArea->verticalScrollBar()->setValue(0);
}

void SoundManagerDialog::on_lineEditSearch_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    ApplyFilter();
}

void SoundManagerDialog::on_checkBoxShowEmpty_checkStateChanged(const Qt::CheckState &arg1)
{
    // "Show Empty" and "Show Changed" are alternative views; keep them
    // mutually exclusive so the tree never shows an ambiguous mixed filter.
    if (arg1 == Qt::Checked)
    {
        QSignalBlocker blocker(ui->checkBoxShowChanged);
        ui->checkBoxShowChanged->setChecked(false);
    }
    PopulateTree();
}

void SoundManagerDialog::on_checkBoxShowChanged_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked)
    {
        QSignalBlocker blocker(ui->checkBoxShowEmpty);
        ui->checkBoxShowEmpty->setChecked(false);
    }
    PopulateTree();
}

void SoundManagerDialog::on_pushButtonExportAll_clicked()
{
    if (!soundData.error.isEmpty())
    {
        QMessageBox::warning(this, tr("Export sound project"), soundData.error);
        return;
    }

    if (HasStagedChanges())
    {
        QMessageBox::warning(this, tr("Export sound project"),
                             tr("There are staged .s imports. Save or cancel them before exporting, "
                                "otherwise the export would overwrite the edited .s files."));
        return;
    }

    const QString folder = Sound::SoundExport::defaultFolder(
                ROMUtils::ROMFileMetadata->FilePath);
    if (QDir(folder).exists())
    {
        if (QMessageBox::question(this, tr("Export sound project"),
                                  tr("The folder already exists:\n%1\n\nOverwrite its files?")
                                  .arg(folder)) != QMessageBox::Yes)
            return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);

    const QString error = Sound::SoundExport::exportAll(
                soundData,
                ROMUtils::ROMFileMetadata->ROMDataPtr,
                ROMUtils::ROMFileMetadata->Length,
                folder,
                [this](int current, int total)
                {
                    ui->progressBar->setRange(0, qMax(1, total));
                    ui->progressBar->setValue(current);
                    QCoreApplication::processEvents();
                });

    QApplication::restoreOverrideCursor();
    ui->progressBar->setValue(0);

    if (!error.isEmpty())
    {
        QMessageBox::critical(this, tr("Export sound project"), error);
        return;
    }

    UpdateStatusLabel();
    QMessageBox::information(this, tr("Export sound project"),
                             tr("Sound project exported to:\n%1").arg(folder));
}

void SoundManagerDialog::on_pushButtonImportAll_clicked()
{
    if (!soundData.error.isEmpty())
    {
        QMessageBox::warning(this, tr("Import sound project"), soundData.error);
        return;
    }

    const QString folder = Sound::SoundExport::defaultFolder(
                ROMUtils::ROMFileMetadata->FilePath);
    if (!QDir(folder).exists())
    {
        QMessageBox::warning(this, tr("Import sound project"),
                             tr("The sound project folder does not exist:\n%1\n\n"
                                "Use Export All first.").arg(folder));
        return;
    }

    const Sound::ImportResult result = Sound::SoundImporter::importAll(
                folder, soundData,
                ROMUtils::ROMFileMetadata->ROMDataPtr,
                ROMUtils::ROMFileMetadata->Length);

    if (!result.error.isEmpty())
    {
        QMessageBox::critical(this, tr("Import sound project"), result.error);
        return;
    }

    if (result.changedSongIds.isEmpty() && result.changedSampleIds.isEmpty() &&
            result.changedProgramCount == 0)
    {
        QMessageBox::information(this, tr("Import sound project"),
                                 tr("No changed .s, .wav or SF2 data was found."));
        return;
    }

    PopulateTree();
    ClearDetailPanel();
    UpdateStatusLabel();

    QString message = tr("%1 song(s), %2 sample(s) and %3 program(s) staged for import.")
            .arg(result.changedSongIds.size())
            .arg(result.changedSampleIds.size())
            .arg(result.changedProgramCount);
    if (!result.warnings.isEmpty())
    {
        message += tr("\n\nWarnings:\n") + result.warnings.mid(0, 10).join(QLatin1Char('\n'));
        if (result.warnings.size() > 10)
            message += tr("\n... and %1 more.").arg(result.warnings.size() - 10);
    }
    QMessageBox::information(this, tr("Import sound project"), message);
}
