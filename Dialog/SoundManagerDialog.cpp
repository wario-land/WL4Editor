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
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
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
#include <QSpinBox>
#include <QCheckBox>
#include <QOverload>
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
    ui->pushButtonExportSelected->setEnabled(false);
    ui->pushButtonImportSelected->setEnabled(false);
    ui->pushButtonExportSelected->setToolTip(tr("Export the selected Song or Sample."));
    ui->pushButtonImportSelected->setToolTip(tr("Import the selected Song or Sample from the project folder."));

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
    UpdateSelectedActions();
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

void SoundManagerDialog::UpdateSelectedActions()
{
    const bool supported = selectedEntryType == static_cast<int>(EntryType::Song) ||
            selectedEntryType == static_cast<int>(EntryType::Sample);
    ui->pushButtonExportSelected->setEnabled(supported);
    ui->pushButtonImportSelected->setEnabled(supported);
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

QSpinBox *SoundManagerDialog::AddSpinRow(QGroupBox *group, const QString &label,
                                        int value, int minimum, int maximum)
{
    auto *spin = new QSpinBox(group);
    spin->setRange(minimum, maximum);
    spin->setValue(value);
    spin->setKeyboardTracking(false);
    if (auto *layout = qobject_cast<QFormLayout *>(group->layout()))
        layout->addRow(label, spin);
    return spin;
}

void SoundManagerDialog::MarkSelectedChanged()
{
    const QModelIndex current = ui->treeViewEntries->currentIndex();
    if (!current.isValid() || !entryModel)
        return;
    if (QStandardItem *status = entryModel->itemFromIndex(current.siblingAtColumn(3)))
        status->setText(tr("Changed"));
}

Sound::Bank *SoundManagerDialog::FindBankMutable(int bankId)
{
    for (Sound::Bank &bank : soundData.banks)
    {
        if (bank.id == bankId)
            return &bank;
    }
    return nullptr;
}

Sound::Sample *SoundManagerDialog::FindSampleMutable(int sampleId)
{
    for (Sound::Sample &sample : soundData.samples)
    {
        if (sample.id == sampleId)
            return &sample;
    }
    return nullptr;
}

Sound::Program *SoundManagerDialog::FindProgramMutable(int bankId, int programIndex)
{
    Sound::Bank *bank = FindBankMutable(bankId);
    if (!bank || programIndex < 0 || programIndex >= bank->programs.size())
        return nullptr;
    return &bank->programs[programIndex];
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

    QGroupBox *editable = CreateGroup(tr("Editable Parameters"));
    QSpinBox *key = AddSpinRow(editable, tr("Key"), voice.key, 0, 127);
    QSpinBox *pan = AddSpinRow(editable, tr("Pan"), voice.pan, 0, 127);
    QSpinBox *attack = AddSpinRow(editable, tr("Attack"), voice.attack, 0, 255);
    QSpinBox *decay = AddSpinRow(editable, tr("Decay"), voice.decay, 0, 255);
    QSpinBox *sustain = AddSpinRow(editable, tr("Sustain"), voice.sustain, 0, 255);
    QSpinBox *release = AddSpinRow(editable, tr("Release"), voice.release, 0, 255);
    auto applyProgram = [this, bankId, programIndex, key, pan, attack, decay, sustain, release]
    {
        Sound::Program *editableProgram = FindProgramMutable(bankId, programIndex);
        if (!editableProgram)
            return;
        Sound::Voice &editableVoice = editableProgram->voice;
        editableVoice.key = static_cast<quint8>(key->value());
        editableVoice.pan = static_cast<quint8>(pan->value());
        editableVoice.attack = static_cast<quint8>(attack->value());
        editableVoice.decay = static_cast<quint8>(decay->value());
        editableVoice.sustain = static_cast<quint8>(sustain->value());
        editableVoice.release = static_cast<quint8>(release->value());
        editableVoice.raw[1] = editableVoice.key;
        editableVoice.raw[3] = editableVoice.pan;
        editableVoice.raw[8] = editableVoice.attack;
        editableVoice.raw[9] = editableVoice.decay;
        editableVoice.raw[10] = editableVoice.sustain;
        editableVoice.raw[11] = editableVoice.release;
        editableProgram->changed = true;
        MarkSelectedChanged();
        UpdateStatusLabel();
    };
    connect(key, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [applyProgram](int) { applyProgram(); });
    connect(pan, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [applyProgram](int) { applyProgram(); });
    connect(attack, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [applyProgram](int) { applyProgram(); });
    connect(decay, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [applyProgram](int) { applyProgram(); });
    connect(sustain, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [applyProgram](int) { applyProgram(); });
    connect(release, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [applyProgram](int) { applyProgram(); });

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

    QGroupBox *editable = CreateGroup(tr("Editable Parameters"));
    auto *loop = new QCheckBox(tr("Loop sample"), editable);
    loop->setChecked(sample.isLooped());
    if (auto *layout = qobject_cast<QFormLayout *>(editable->layout()))
        layout->addRow(tr("Loop"), loop);
    QSpinBox *pitch = AddSpinRow(editable, tr("Pitch (raw 1/1024 Hz)"),
                                 static_cast<int>(sample.pitch), 0, 0x7FFFFFFF);
    QSpinBox *loopStart = AddSpinRow(editable, tr("Loop start (bytes)"),
                                     static_cast<int>(sample.loopStart), 0, 0x7FFFFFFF);
    auto applySample = [this, sampleId = sample.id, loop, pitch, loopStart]
    {
        Sound::Sample *editableSample = FindSampleMutable(sampleId);
        if (!editableSample)
            return;
        if (loop->isChecked() && editableSample->size > 0 &&
                static_cast<quint32>(loopStart->value()) >= editableSample->size)
        {
            QMessageBox::warning(this, tr("Invalid loop"),
                                 tr("Loop start must be smaller than the sample size."));
            loopStart->setValue(static_cast<int>(editableSample->loopStart));
            return;
        }
        editableSample->flag = loop->isChecked()
                ? static_cast<quint8>(editableSample->flag | 0x40)
                : static_cast<quint8>(editableSample->flag & ~0x40);
        editableSample->pitch = static_cast<quint32>(pitch->value());
        editableSample->loopStart = static_cast<quint32>(loopStart->value());
        editableSample->changed = true;
        MarkSelectedChanged();
        UpdateStatusLabel();
    };
    connect(loop, &QCheckBox::toggled, this, [applySample](bool) { applySample(); });
    connect(pitch, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [applySample](int) { applySample(); });
    connect(loopStart, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [applySample](int) { applySample(); });
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

    selectedEntryType = 0;
    selectedEntryId = -1;

    if (populatingTree || !current.isValid() || !entryModel)
    {
        ClearDetailPanel();
        UpdateSelectedActions();
        return;
    }

    QStandardItem *item = entryModel->itemFromIndex(current.siblingAtColumn(0));
    if (!item)
    {
        ClearDetailPanel();
        UpdateSelectedActions();
        return;
    }

    const int typeValue = item->data(RoleEntryType).toInt();
    if (typeValue <= 0)
    {
        ClearDetailPanel();
        UpdateSelectedActions();
        return;
    }

    const int id = item->data(RoleEntryId).toInt();
    const int parentId = item->data(RoleParentId).toInt();
    selectedEntryType = typeValue;
    selectedEntryId = id;
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

    UpdateSelectedActions();
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
                             tr("There are staged imports. Save or cancel them before exporting."));
        return;
    }

    const QString folder = Sound::SoundExport::defaultFolder(
                ROMUtils::ROMFileMetadata->FilePath);
    if (QDir(folder).exists())
    {
        if (QMessageBox::question(this, tr("Export sound project"),
                                  tr("The folder already exists:\n%1\n\n"
                                     "Regenerate auxiliary files? Existing .s files are preserved; "
                                     "fresh disassembly is written as .exported.s.")
                                  .arg(folder)) != QMessageBox::Yes)
            return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);

    QStringList exportWarnings;
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
                },
                &exportWarnings);

    QApplication::restoreOverrideCursor();
    ui->progressBar->setValue(0);

    if (!error.isEmpty())
    {
        QMessageBox::critical(this, tr("Export sound project"), error);
        return;
    }

    UpdateStatusLabel();
    QString message = tr("Sound project exported to:\n%1").arg(folder);
    if (!exportWarnings.isEmpty())
    {
        message += tr("\n\nWarnings:\n") + exportWarnings.mid(0, 10).join(QLatin1Char('\n'));
        if (exportWarnings.size() > 10)
            message += tr("\n... and %1 more.").arg(exportWarnings.size() - 10);
    }
    QMessageBox::information(this, tr("Export sound project"), message);
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

namespace
{
    bool readJsonObject(const QString &path, QJsonObject *object, QString *error)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
        {
            if (error)
                *error = QStringLiteral("Cannot open %1.").arg(path);
            return false;
        }
        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            if (error)
                *error = QStringLiteral("Invalid JSON in %1: %2")
                        .arg(path, parseError.errorString());
            return false;
        }
        *object = document.object();
        return true;
    }

    bool writeJsonObject(const QString &path, const QJsonObject &object, QString *error)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            if (error)
                *error = QStringLiteral("Cannot write %1.").arg(path);
            return false;
        }
        file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
        return true;
    }

    int jsonEntryIndex(const QJsonArray &array, int id)
    {
        for (int i = 0; i < array.size(); ++i)
        {
            if (array.at(i).toObject().value(QStringLiteral("id")).toInt(-1) == id)
                return i;
        }
        return -1;
    }

    bool copyRelativeFile(const QString &sourceFolder, const QString &targetFolder,
                          const QString &relativePath, QString *error)
    {
        const QString source = QDir(sourceFolder).absoluteFilePath(relativePath);
        const QString target = QDir(targetFolder).absoluteFilePath(relativePath);
        if (!QFileInfo::exists(source))
        {
            if (error)
                *error = QStringLiteral("Missing file: %1").arg(source);
            return false;
        }
        if (!QDir().mkpath(QFileInfo(target).dir().path()))
        {
            if (error)
                *error = QStringLiteral("Cannot create folder for %1.").arg(target);
            return false;
        }
        QFile::remove(target);
        if (!QFile::copy(source, target))
        {
            if (error)
                *error = QStringLiteral("Cannot copy %1 to %2.").arg(source, target);
            return false;
        }
        return true;
    }
}

void SoundManagerDialog::on_pushButtonExportSelected_clicked()
{
    if (selectedEntryType != static_cast<int>(EntryType::Song) &&
            selectedEntryType != static_cast<int>(EntryType::Sample))
        return;

    if (!soundData.error.isEmpty())
    {
        QMessageBox::warning(this, tr("Export selected"), soundData.error);
        return;
    }

    const QString folder = Sound::SoundExport::defaultFolder(
                ROMUtils::ROMFileMetadata->FilePath);
    const QString projectManifestPath = folder + QStringLiteral("/soundmanifest.json");
    if (!QFileInfo::exists(projectManifestPath))
    {
        // Export Selected needs the project manifest to merge one entry into.
        // Running Export All first is safe: it includes the selected item and
        // establishes all hashes and auxiliary files.
        QMessageBox::information(this, tr("Export selected"),
                                 tr("No complete sound project exists yet.\n"
                                    "Export All will run first; the selected item is included."));
        on_pushButtonExportAll_clicked();
        if (!QFileInfo::exists(projectManifestPath))
            return;
        QMessageBox::information(this, tr("Export selected"),
                                 tr("A complete sound project was exported first.\n"
                                    "The selected item is already included."));
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QTemporaryDir temp;
    if (!temp.isValid())
    {
        QApplication::restoreOverrideCursor();
        QMessageBox::critical(this, tr("Export selected"), tr("Cannot create a temporary folder."));
        return;
    }

    QStringList exportWarnings;
    const QString exportError = Sound::SoundExport::exportAll(
                soundData, ROMUtils::ROMFileMetadata->ROMDataPtr,
                ROMUtils::ROMFileMetadata->Length, temp.path(), {}, &exportWarnings);
    QApplication::restoreOverrideCursor();
    if (!exportError.isEmpty())
    {
        QMessageBox::critical(this, tr("Export selected"), exportError);
        return;
    }

    QJsonObject projectRoot;
    QJsonObject tempRoot;
    QString error;
    if (!readJsonObject(projectManifestPath, &projectRoot, &error) ||
            !readJsonObject(temp.path() + QStringLiteral("/soundmanifest.json"), &tempRoot, &error))
    {
        QMessageBox::critical(this, tr("Export selected"), error);
        return;
    }

    if (selectedEntryType == static_cast<int>(EntryType::Song))
    {
        QJsonArray projectSongs = projectRoot.value(QStringLiteral("songs")).toArray();
        const QJsonArray tempSongs = tempRoot.value(QStringLiteral("songs")).toArray();
        const int projectIndex = jsonEntryIndex(projectSongs, selectedEntryId);
        const int tempIndex = jsonEntryIndex(tempSongs, selectedEntryId);
        if (projectIndex < 0 || tempIndex < 0)
        {
            QMessageBox::warning(this, tr("Export selected"),
                                 tr("The selected song is not in the project manifest."));
            return;
        }
        const QJsonObject tempEntry = tempSongs.at(tempIndex).toObject();
        if (!copyRelativeFile(temp.path(), folder, tempEntry.value(QStringLiteral("sFile")).toString(), &error) ||
                !copyRelativeFile(temp.path(), folder, tempEntry.value(QStringLiteral("mFile")).toString(), &error))
        {
            QMessageBox::critical(this, tr("Export selected"), error);
            return;
        }
        projectSongs.replace(projectIndex, tempEntry);
        projectRoot.insert(QStringLiteral("songs"), projectSongs);
    }
    else
    {
        QJsonArray projectSamples = projectRoot.value(QStringLiteral("samples")).toArray();
        const QJsonArray tempSamples = tempRoot.value(QStringLiteral("samples")).toArray();
        const int projectIndex = jsonEntryIndex(projectSamples, selectedEntryId);
        const int tempIndex = jsonEntryIndex(tempSamples, selectedEntryId);
        if (projectIndex < 0 || tempIndex < 0)
        {
            QMessageBox::warning(this, tr("Export selected"),
                                 tr("The selected sample is not in the project manifest."));
            return;
        }
        const QJsonObject tempEntry = tempSamples.at(tempIndex).toObject();
        if (!copyRelativeFile(temp.path(), folder, tempEntry.value(QStringLiteral("wavFile")).toString(), &error))
        {
            QMessageBox::critical(this, tr("Export selected"), error);
            return;
        }
        projectSamples.replace(projectIndex, tempEntry);
        projectRoot.insert(QStringLiteral("samples"), projectSamples);
    }

    if (!writeJsonObject(projectManifestPath, projectRoot, &error))
    {
        QMessageBox::critical(this, tr("Export selected"), error);
        return;
    }
    QString message = tr("The selected item was exported to:\n%1").arg(folder);
    if (!exportWarnings.isEmpty())
        message += tr("\n\nWarnings:\n") + exportWarnings.mid(0, 10).join(QLatin1Char('\n'));
    QMessageBox::information(this, tr("Export selected"), message);
}


void SoundManagerDialog::on_pushButtonImportSelected_clicked()
{
    if (selectedEntryType != static_cast<int>(EntryType::Song) &&
            selectedEntryType != static_cast<int>(EntryType::Sample))
        return;

    if (!soundData.error.isEmpty())
    {
        QMessageBox::warning(this, tr("Import selected"), soundData.error);
        return;
    }

    const QString folder = Sound::SoundExport::defaultFolder(
                ROMUtils::ROMFileMetadata->FilePath);
    const QString projectManifestPath = folder + QStringLiteral("/soundmanifest.json");
    QJsonObject projectRoot;
    QString error;
    if (!readJsonObject(projectManifestPath, &projectRoot, &error))
    {
        QMessageBox::warning(this, tr("Import selected"),
                             tr("The sound project folder has no valid manifest.\n%1").arg(error));
        return;
    }

    QTemporaryDir temp;
    if (!temp.isValid())
    {
        QMessageBox::critical(this, tr("Import selected"), tr("Cannot create a temporary folder."));
        return;
    }

    QJsonObject tempRoot;
    tempRoot.insert(QStringLiteral("format"), projectRoot.value(QStringLiteral("format")));
    tempRoot.insert(QStringLiteral("version"), projectRoot.value(QStringLiteral("version")));
    tempRoot.insert(QStringLiteral("rom"), projectRoot.value(QStringLiteral("rom")));

    const QString songsKey = QStringLiteral("songs");
    const QString samplesKey = QStringLiteral("samples");
    QJsonArray tempSongs;
    QJsonArray tempSamples;

    if (selectedEntryType == static_cast<int>(EntryType::Song))
    {
        const QJsonArray songs = projectRoot.value(songsKey).toArray();
        const int index = jsonEntryIndex(songs, selectedEntryId);
        if (index < 0)
        {
            QMessageBox::warning(this, tr("Import selected"),
                                 tr("The selected song is not in the project manifest."));
            return;
        }
        const QJsonObject entry = songs.at(index).toObject();
        tempSongs.append(entry);
        if (!copyRelativeFile(folder, temp.path(), entry.value(QStringLiteral("sFile")).toString(), &error))
        {
            QMessageBox::critical(this, tr("Import selected"), error);
            return;
        }
    }
    else
    {
        const QJsonArray samples = projectRoot.value(samplesKey).toArray();
        const int index = jsonEntryIndex(samples, selectedEntryId);
        if (index < 0)
        {
            QMessageBox::warning(this, tr("Import selected"),
                                 tr("The selected sample is not in the project manifest."));
            return;
        }
        const QJsonObject entry = samples.at(index).toObject();
        tempSamples.append(entry);
        if (!copyRelativeFile(folder, temp.path(), entry.value(QStringLiteral("wavFile")).toString(), &error))
        {
            QMessageBox::critical(this, tr("Import selected"), error);
            return;
        }

        // SoundImporter expects at least one song entry. Keep one unchanged
        // song in the temporary manifest so the sample-only import is valid.
        const QJsonArray songs = projectRoot.value(songsKey).toArray();
        if (!songs.isEmpty())
        {
            const QJsonObject songEntry = songs.at(0).toObject();
            tempSongs.append(songEntry);
            copyRelativeFile(folder, temp.path(), songEntry.value(QStringLiteral("sFile")).toString(), nullptr);
        }
    }

    tempRoot.insert(songsKey, tempSongs);
    tempRoot.insert(samplesKey, tempSamples);
    if (!writeJsonObject(temp.path() + QStringLiteral("/soundmanifest.json"), tempRoot, &error))
    {
        QMessageBox::critical(this, tr("Import selected"), error);
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const Sound::ImportResult result = Sound::SoundImporter::importAll(
                temp.path(), soundData, ROMUtils::ROMFileMetadata->ROMDataPtr,
                ROMUtils::ROMFileMetadata->Length);
    QApplication::restoreOverrideCursor();

    if (!result.error.isEmpty())
    {
        QMessageBox::critical(this, tr("Import selected"), result.error);
        return;
    }
    if (result.changedSongIds.isEmpty() && result.changedSampleIds.isEmpty() &&
            result.changedProgramCount == 0)
    {
        QMessageBox::information(this, tr("Import selected"),
                                 tr("The selected item has no changes to import."));
        return;
    }

    PopulateTree();
    ClearDetailPanel();
    UpdateStatusLabel();
    QString message = tr("Selected item staged for import.\n"
                         "Changed songs: %1, samples: %2, programs: %3.")
            .arg(result.changedSongIds.size())
            .arg(result.changedSampleIds.size())
            .arg(result.changedProgramCount);
    if (!result.warnings.isEmpty())
        message += tr("\n\nWarnings:\n") + result.warnings.mid(0, 10).join(QLatin1Char('\n'));
    QMessageBox::information(this, tr("Import selected"), message);
}
