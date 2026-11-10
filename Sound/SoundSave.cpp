#include "SoundSave.h"

#include "ROMUtils.h"
#include "SoundPsgIO.h"
#include "SoundSAssembler.h"
#include "WL4Constants.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QVector>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace Sound
{
    namespace
    {
        constexpr quint32 RomBase = 0x08000000;

        enum class JobKind
        {
            Sample,
            Bank,
            Song
        };

        struct Job
        {
            JobKind kind = JobKind::Song;
            Sample *sample = nullptr;
            Bank *bank = nullptr;
            Song *song = nullptr;
            QString sFile;
            QByteArray data; // sample header+PCM, or a copy of the bank
            quint32 oldBase = 0;
            quint32 oldEnd = 0;
            unsigned int chunkIndex = 0;
        };

        quint32 readU32(const unsigned char *data, quint32 size, quint32 offset)
        {
            if (!data || offset + 4 > size)
                return 0;
            return static_cast<quint32>(data[offset]) |
                   (static_cast<quint32>(data[offset + 1]) << 8) |
                   (static_cast<quint32>(data[offset + 2]) << 16) |
                   (static_cast<quint32>(data[offset + 3]) << 24);
        }

        quint32 pointerToOffset(quint32 pointer)
        {
            if (!pointer)
                return 0xFFFFFFFFu;
            return pointer >= RomBase ? pointer - RomBase : pointer;
        }

        void writeU16(unsigned char *data, quint16 value)
        {
            data[0] = static_cast<unsigned char>(value & 0xFF);
            data[1] = static_cast<unsigned char>((value >> 8) & 0xFF);
        }

        void writeU32(unsigned char *data, quint32 value)
        {
            data[0] = static_cast<unsigned char>(value & 0xFF);
            data[1] = static_cast<unsigned char>((value >> 8) & 0xFF);
            data[2] = static_cast<unsigned char>((value >> 16) & 0xFF);
            data[3] = static_cast<unsigned char>((value >> 24) & 0xFF);
        }

        bool voiceUsesSample(const Voice &voice, const QSet<int> &sampleIds)
        {
            if (voice.sampleId >= 0 && sampleIds.contains(voice.sampleId))
                return true;
            for (const SubVoice &sub : voice.subVoices)
            {
                if (sub.sampleId >= 0 && sampleIds.contains(sub.sampleId))
                    return true;
            }
            return false;
        }

        bool isChunkAt(quint32 dataAddress, ROMUtils::SaveDataChunkType expected)
        {
            if (dataAddress == 0xFFFFFFFFu || dataAddress < 12)
                return false;
            if (dataAddress >= ROMUtils::ROMFileMetadata->Length)
                return false;
            ROMUtils::SaveDataChunkType type;
            return ROMUtils::GetChunkType(dataAddress, type) && type == expected;
        }

        void collectSampleChunkReferences(
                const ParseResult &data, const unsigned char *rom, quint32 romSize,
                QSet<quint32> &referenced)
        {
            auto addPointer = [&](quint32 pointerAddress)
            {
                const quint32 pointer = pointerToOffset(readU32(rom, romSize, pointerAddress));
                if (isChunkAt(pointer, ROMUtils::SaveDataChunkType::SoundSampleChunk))
                    referenced.insert(pointer);
            };

            for (const Bank &bank : data.banks)
            {
                for (const Program &program : bank.programs)
                {
                    const Voice &voice = program.voice;
                    if (voice.type == VoiceType::Sample || voice.type == VoiceType::SampleFixed)
                    {
                        if (voice.romAddress)
                            addPointer(voice.romAddress + 4);
                    }
                    else if (voice.type == VoiceType::KeySplit || voice.type == VoiceType::DrumKit)
                    {
                        for (const SubVoice &sub : voice.subVoices)
                        {
                            if (sub.romAddress)
                                addPointer(sub.romAddress + 4);
                        }
                    }
                }
            }
        }

        bool validateSongBinary(const QByteArray &binary, quint32 dataAddress, QString &error)
        {
            if (binary.size() < 8)
            {
                error = QStringLiteral("Assembled song is too small.");
                return false;
            }
            if (binary.size() > 0xFFFF)
            {
                error = QStringLiteral("Assembled song is larger than the 64 KB chunk writer limit.");
                return false;
            }

            const int trackCount = static_cast<quint8>(binary.at(0));
            if (trackCount < 1 || trackCount > 16 ||
                    binary.size() < 8 + trackCount * 4)
            {
                error = QStringLiteral("Assembled song has an invalid track count (%1).").arg(trackCount);
                return false;
            }

            for (int i = 0; i < trackCount; ++i)
            {
                const quint32 pointer = readU32(
                            reinterpret_cast<const unsigned char *>(binary.constData()),
                            static_cast<quint32>(binary.size()), 8 + i * 4);
                const quint32 offset = pointerToOffset(pointer);
                if (offset < dataAddress ||
                        offset >= dataAddress + static_cast<quint32>(binary.size()))
                {
                    error = QStringLiteral("Assembled song track %1 pointer is outside the chunk.").arg(i);
                    return false;
                }
            }
            return true;
        }

        void updateManifestAfterSave(const QString &folder, const QVector<Job> &jobs)
        {
            if (folder.isEmpty())
                return;

            const QString path = QDir(folder).filePath(QStringLiteral("soundmanifest.json"));
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly))
                return;

            QJsonParseError error{};
            const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
            file.close();
            if (error.error != QJsonParseError::NoError || !document.isObject())
                return;

            QJsonObject root = document.object();
            QJsonObject rom = root.value(QStringLiteral("rom")).toObject();
            const QByteArray hash = QCryptographicHash::hash(
                        QByteArray(reinterpret_cast<const char *>(ROMUtils::ROMFileMetadata->ROMDataPtr),
                                   static_cast<int>(ROMUtils::ROMFileMetadata->Length)),
                        QCryptographicHash::Sha1).toHex();
            rom.insert(QStringLiteral("sha1"), QString::fromLatin1(hash));
            rom.insert(QStringLiteral("size"),
                       static_cast<int>(ROMUtils::ROMFileMetadata->Length));
            root.insert(QStringLiteral("rom"), rom);

            QJsonArray songs = root.value(QStringLiteral("songs")).toArray();
            for (int i = 0; i < songs.size(); ++i)
            {
                QJsonObject object = songs.at(i).toObject();
                const int id = object.value(QStringLiteral("id")).toInt(-1);
                for (const Job &job : jobs)
                {
                    if (job.kind != JobKind::Song || job.song->id != id)
                        continue;
                    QFile source(job.sFile);
                    if (source.open(QIODevice::ReadOnly))
                    {
                        const QByteArray sourceHash = QCryptographicHash::hash(
                                    source.readAll(), QCryptographicHash::Sha1).toHex();
                        source.close();
                        object.insert(QStringLiteral("sHash"),
                                      QString::fromLatin1(sourceHash));
                        songs[i] = object;
                    }
                    break;
                }
            }
            root.insert(QStringLiteral("songs"), songs);

            QJsonArray samples = root.value(QStringLiteral("samples")).toArray();
            for (int i = 0; i < samples.size(); ++i)
            {
                QJsonObject object = samples.at(i).toObject();
                const int id = object.value(QStringLiteral("id")).toInt(-1);
                for (const Job &job : jobs)
                {
                    if (job.kind != JobKind::Sample || job.sample->id != id)
                        continue;
                    if (!job.sample->importedWavFile.isEmpty())
                    {
                        QFile source(job.sample->importedWavFile);
                        if (source.open(QIODevice::ReadOnly))
                        {
                            const QByteArray sourceHash = QCryptographicHash::hash(
                                        source.readAll(), QCryptographicHash::Sha1).toHex();
                            source.close();
                            object.insert(QStringLiteral("wavHash"),
                                          QString::fromLatin1(sourceHash));
                        }
                    }
                    object.insert(QStringLiteral("sha1"),
                                  QString::fromLatin1(job.sample->sha1));
                    object.insert(QStringLiteral("size"),
                                  static_cast<int>(job.sample->size));
                    samples[i] = object;
                    break;
                }
            }
            root.insert(QStringLiteral("samples"), samples);

            QJsonObject soundfont = root.value(QStringLiteral("soundfont")).toObject();
            const QString sf2Relative = soundfont.value(QStringLiteral("file")).toString();
            if (!sf2Relative.isEmpty())
            {
                QFile sf2(QDir(folder).absoluteFilePath(sf2Relative));
                if (sf2.open(QIODevice::ReadOnly))
                {
                    soundfont.insert(QStringLiteral("hash"),
                                     QString::fromLatin1(QCryptographicHash::hash(
                                                             sf2.readAll(),
                                                             QCryptographicHash::Sha1).toHex()));
                    sf2.close();
                    root.insert(QStringLiteral("soundfont"), soundfont);
                }
            }

            root.insert(QStringLiteral("generated"),
                        QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
                file.close();
            }
        }
    } // namespace

    QString SoundSave::saveToROM(ParseResult &data, const QString &projectFolder,
                                 const std::function<void(int, int)> &progress)
    {
        if (!ROMUtils::ROMFileMetadata->ROMDataPtr)
            return QStringLiteral("No ROM is loaded.");

        unsigned char *romData = ROMUtils::ROMFileMetadata->ROMDataPtr;
        const quint32 romLength = ROMUtils::ROMFileMetadata->Length;

        QVector<Job> jobs;
        QSet<int> changedSamples;

        bool hasNewSample = false;
        for (const Sample &sample : data.samples)
        {
            if (sample.isNew)
            {
                hasNewSample = true;
                break;
            }
        }

        for (Sample &sample : data.samples)
        {
            if (sample.id < 0 || sample.pcm.isEmpty())
                continue;
            if (!sample.changed && !hasNewSample)
                continue;
            if (!sample.isNew && sample.romAddress + 16 > romLength)
                return QStringLiteral("Sample %1 header is outside the ROM.").arg(sample.id);

            QByteArray header(16, '\0');
            if (!sample.isNew && sample.romAddress)
            {
                header = QByteArray(reinterpret_cast<const char *>(romData + sample.romAddress), 16);
            }
            header[3] = static_cast<char>(sample.flag);
            QByteArray chunk = header;
            writeU32(reinterpret_cast<unsigned char *>(chunk.data()) + 4, sample.pitch);
            writeU32(reinterpret_cast<unsigned char *>(chunk.data()) + 8, sample.loopStart);
            writeU32(reinterpret_cast<unsigned char *>(chunk.data()) + 12, sample.size);
            chunk.append(sample.pcm);
            QByteArray sampleName = sample.name.toUtf8();
            if (sampleName.size() > 0xFFFF)
                sampleName = sampleName.left(0xFFFF);
            chunk.append(static_cast<char>(sampleName.size() & 0xFF));
            chunk.append(static_cast<char>((sampleName.size() >> 8) & 0xFF));
            chunk.append(sampleName);
            chunk.append("W4S2", 4);
            chunk.append(static_cast<char>(sample.id & 0xFF));
            chunk.append(static_cast<char>((sample.id >> 8) & 0xFF));
            chunk.append(2, '\0');
            if (chunk.size() > 0xFFFF)
                return QStringLiteral("Sample %1 is larger than the 64 KB chunk writer limit.")
                        .arg(sample.id);

            Job job;
            job.kind = JobKind::Sample;
            job.sample = &sample;
            job.data = chunk;
            jobs.append(job);
            changedSamples.insert(sample.id);
            if (hasNewSample)
            {
                // Writing all samples with ID trailers gives every sample a
                // stable ID, so later parser passes cannot renumber them.
                for (const Sample &other : data.samples)
                {
                    if (other.id >= 0 && !other.pcm.isEmpty())
                        changedSamples.insert(other.id);
                }
            }
        }

        QSet<int> affectedBanks;
        for (Bank &bank : data.banks)
        {
            bool affected = false;
            for (const Program &program : bank.programs)
            {
                if (program.changed || voiceUsesSample(program.voice, changedSamples))
                {
                    affected = true;
                    break;
                }
            }
            if (!affected)
                continue;

            bool usedBySong = false;
            for (const Song &song : data.songs)
            {
                if (song.bankId == bank.id && song.state != SongState::Empty)
                {
                    usedBySong = true;
                    break;
                }
            }
            if (!usedBySong)
                continue;

            if (!bank.dataAddress || bank.dataSize < 12 ||
                    bank.dataAddress + bank.dataSize > romLength)
            {
                return QStringLiteral("Bank %1 data range is outside the ROM.").arg(bank.id);
            }
            if (bank.dataSize + 8 > 0xFFFF)
            {
                return QStringLiteral("Bank %1 is larger than the 64 KB chunk writer limit.")
                        .arg(bank.id);
            }

            Job job;
            job.kind = JobKind::Bank;
            job.bank = &bank;
            job.oldBase = bank.dataAddress;
            job.oldEnd = bank.dataAddress + bank.dataSize;
            job.data = QByteArray(reinterpret_cast<const char *>(romData + bank.dataAddress),
                                  static_cast<int>(bank.dataSize));
            job.data.append("W4BK", 4);
            job.data.append(static_cast<char>(bank.id));
            job.data.append(3, '\0');
            jobs.append(job);
            affectedBanks.insert(bank.id);
        }

        for (Song &song : data.songs)
        {
            if (!song.changed || !song.importFromS)
                continue;
            if (song.bankId < 0 || song.bankId >= WL4Constants::SoundBankCount ||
                    WL4Constants::SoundBankBase[song.bankId] == 0)
            {
                return QStringLiteral("Song %1 uses an invalid bank id %2.")
                        .arg(song.id).arg(song.bankId);
            }
            if (!QFileInfo::exists(song.importedSFile))
            {
                return QStringLiteral("Song %1 source file is missing: %2")
                        .arg(song.id).arg(song.importedSFile);
            }

            Job job;
            job.kind = JobKind::Song;
            job.song = &song;
            job.sFile = song.importedSFile;
            jobs.append(job);
        }

        if (jobs.isEmpty())
            return QString();

        bool needsAssembler = false;
        for (const Job &job : jobs)
        {
            if (job.kind == JobKind::Song)
            {
                needsAssembler = true;
                break;
            }
        }
        if (needsAssembler)
        {
            const QString eabiError = SoundSAssembler::ensureEabi();
            if (!eabiError.isEmpty())
                return eabiError;
        }

        // Build the reference sets from the current ROM so unchanged custom
        // chunks survive the save.
        QSet<quint32> referencedSongChunks;
        QSet<quint32> referencedBankChunks;
        QSet<quint32> referencedSampleChunks;
        for (const Song &song : data.songs)
        {
            const quint32 entry = WL4Constants::SongTable + static_cast<quint32>(song.id) * 8;
            const quint32 songPointer = pointerToOffset(readU32(romData, romLength, entry));
            if (isChunkAt(songPointer, ROMUtils::SaveDataChunkType::SoundSongChunk))
                referencedSongChunks.insert(songPointer);

            if (song.romAddress + 8 <= romLength)
            {
                const quint32 bankPointer = pointerToOffset(
                            readU32(romData, romLength, song.romAddress + 4));
                if (isChunkAt(bankPointer, ROMUtils::SaveDataChunkType::SoundBankChunk))
                    referencedBankChunks.insert(bankPointer);
            }
        }
        collectSampleChunkReferences(data, romData, romLength, referencedSampleChunks);

        QSet<quint32> changedOldChunks;
        for (const Job &job : jobs)
        {
            if (job.kind == JobKind::Song)
            {
                const quint32 entry = WL4Constants::SongTable +
                        static_cast<quint32>(job.song->id) * 8;
                const quint32 pointer = pointerToOffset(readU32(romData, romLength, entry));
                if (isChunkAt(pointer, ROMUtils::SaveDataChunkType::SoundSongChunk))
                    changedOldChunks.insert(pointer);
            }
            else if (job.kind == JobKind::Bank)
            {
                for (const Song &song : data.songs)
                {
                    if (song.bankId != job.bank->id || song.romAddress + 8 > romLength)
                        continue;
                    const quint32 pointer = pointerToOffset(
                                readU32(romData, romLength, song.romAddress + 4));
                    if (isChunkAt(pointer, ROMUtils::SaveDataChunkType::SoundBankChunk))
                        changedOldChunks.insert(pointer);
                }
            }
            else if (job.kind == JobKind::Sample)
            {
                if (isChunkAt(job.sample->romAddress,
                              ROMUtils::SaveDataChunkType::SoundSampleChunk))
                    changedOldChunks.insert(job.sample->romAddress);
            }
        }

        QVector<unsigned int> invalidationChunks;
        QSet<quint32> invalidated;
        auto addInvalidation = [&](quint32 dataAddress)
        {
            if (dataAddress != 0xFFFFFFFFu && !invalidated.contains(dataAddress))
            {
                invalidated.insert(dataAddress);
                invalidationChunks.append(dataAddress);
            }
        };
        for (quint32 address : changedOldChunks)
            addInvalidation(address);

        auto collectOrphans = [&](ROMUtils::SaveDataChunkType type,
                                  const QSet<quint32> &referenced)
        {
            const QVector<unsigned int> chunks = ROMUtils::FindAllChunksInROM(
                        romData, romLength, WL4Constants::AvailableSpaceBeginningInROM, type);
            for (quint32 ratsAddress : chunks)
            {
                const quint32 dataAddress = ratsAddress + 12;
                if (!referenced.contains(dataAddress) ||
                        changedOldChunks.contains(dataAddress))
                {
                    addInvalidation(dataAddress);
                }
            }
        };
        collectOrphans(ROMUtils::SaveDataChunkType::SoundSongChunk, referencedSongChunks);
        collectOrphans(ROMUtils::SaveDataChunkType::SoundBankChunk, referencedBankChunks);
        collectOrphans(ROMUtils::SaveDataChunkType::SoundSampleChunk, referencedSampleChunks);

        std::sort(invalidationChunks.begin(), invalidationChunks.end());
        invalidationChunks.erase(
                    std::unique(invalidationChunks.begin(), invalidationChunks.end()),
                    invalidationChunks.end());

        int jobIndex = 0;
        QVector<unsigned int> jobChunkIndices(jobs.size(), 0);
        QString allocatorError;
        const int totalSteps = jobs.size();

        if (progress)
            progress(0, totalSteps);

        std::function<ROMUtils::ChunkAllocationStatus(
                unsigned char *, ROMUtils::FreeSpaceRegion, ROMUtils::SaveData *,
                bool, int *)> allocator =
                [&](unsigned char *, ROMUtils::FreeSpaceRegion freeSpace,
                    ROMUtils::SaveData *saveData, bool resetChunkIndex, int *requiredSize)
        {
            if (resetChunkIndex)
            {
                jobIndex = 0;
                jobChunkIndices.fill(0);
            }
            if (jobIndex >= jobs.size())
                return ROMUtils::ChunkAllocationStatus::NoMoreChunks;

            Job &job = jobs[jobIndex];
            const int alignmentOffset = static_cast<int>(
                        ((freeSpace.addr + 3) & ~3u) - freeSpace.addr);
            const quint32 dataAddress = freeSpace.addr + alignmentOffset + 12;

            QByteArray chunkData;
            if (job.kind == JobKind::Song)
            {
                QString assembleError = SoundSAssembler::assemble(
                            job.sFile, dataAddress, chunkData);
                if (!assembleError.isEmpty())
                {
                    allocatorError = assembleError;
                    return ROMUtils::ChunkAllocationStatus::ProcessingError;
                }
                QString validationError;
                if (!validateSongBinary(chunkData, dataAddress, validationError))
                {
                    allocatorError = QStringLiteral("Song %1: %2")
                            .arg(job.song->id).arg(validationError);
                    return ROMUtils::ChunkAllocationStatus::ProcessingError;
                }
            }
            else
            {
                chunkData = job.data;
            }

            if (chunkData.size() + alignmentOffset + 12 > freeSpace.size)
            {
                *requiredSize = chunkData.size() + 12;
                return ROMUtils::ChunkAllocationStatus::InsufficientSpace;
            }

            unsigned char *data = static_cast<unsigned char *>(malloc(chunkData.size()));
            if (!data)
            {
                allocatorError = QStringLiteral("Out of memory while saving sound data.");
                return ROMUtils::ChunkAllocationStatus::ProcessingError;
            }
            std::memcpy(data, chunkData.constData(), chunkData.size());

            ROMUtils::SaveData chunk = {};
            chunk.ptr_addr = 0;
            chunk.size = static_cast<unsigned int>(chunkData.size());
            chunk.data = data;
            chunk.index = ROMUtils::SaveDataIndex++;
            chunk.alignment = true;
            chunk.dest_index = 0;
            chunk.old_chunk_addr = 0;
            chunk.ChunkType = job.kind == JobKind::Sample
                    ? ROMUtils::SaveDataChunkType::SoundSampleChunk
                    : (job.kind == JobKind::Bank
                       ? ROMUtils::SaveDataChunkType::SoundBankChunk
                       : ROMUtils::SaveDataChunkType::SoundSongChunk);

            jobChunkIndices[jobIndex] = chunk.index;
            *saveData = chunk;
            ++jobIndex;

            if (progress)
                progress(jobIndex, totalSteps);
            return ROMUtils::ChunkAllocationStatus::Success;
        };

        QString postprocessError;
        const bool saved = ROMUtils::SaveFile(
                    ROMUtils::ROMFileMetadata->FilePath,
                    invalidationChunks,
                    allocator,
                    [&jobs, &jobChunkIndices, &data, &postprocessError](unsigned char *tempFile,
                                                     std::map<int, int> indexToChunkPtr) -> QString
        {
            QHash<int, quint32> newSampleData;
            QHash<int, quint32> newBankData;
            QVector<quint32> newSongData(jobs.size(), 0);

            for (int i = 0; i < jobs.size(); ++i)
            {
                const auto it = indexToChunkPtr.find(static_cast<int>(jobChunkIndices[i]));
                if (it == indexToChunkPtr.end())
                    return postprocessError = QStringLiteral("Internal error: missing chunk address.");
                const quint32 dataAddress = static_cast<quint32>(it->second) + 12;
                if (jobs[i].kind == JobKind::Sample)
                    newSampleData.insert(jobs[i].sample->id, dataAddress);
                else if (jobs[i].kind == JobKind::Bank)
                    newBankData.insert(jobs[i].bank->id, dataAddress);
                else
                    newSongData[i] = dataAddress;
            }

            auto samplePointerFor = [&](int sampleId) -> quint32
            {
                const auto newSample = newSampleData.constFind(sampleId);
                if (newSample != newSampleData.constEnd())
                    return newSample.value() | RomBase;
                for (const Sample &candidate : data.samples)
                {
                    if (candidate.id == sampleId && candidate.romAddress)
                        return candidate.romAddress | RomBase;
                }
                return 0;
            };

            for (const Job &job : jobs)
            {
                if (job.kind != JobKind::Bank)
                    continue;
                const quint32 oldBase = job.oldBase;
                const quint32 oldEnd = job.oldEnd;
                const quint32 bankSize = oldEnd - oldBase;
                const quint32 newBase = newBankData.value(job.bank->id);

                auto rebase = [&](quint32 offset, quint32 oldPointer)
                {
                    if (oldPointer >= oldBase && oldPointer < oldEnd && offset + 4 <= bankSize)
                        writeU32(tempFile + newBase + offset,
                                 (newBase + (oldPointer - oldBase)) | RomBase);
                };
                auto patchSample = [&](quint32 offset, int sampleId) -> bool
                {
                    if (sampleId < 0 || offset + 4 > bankSize)
                        return true;
                    const quint32 pointer = samplePointerFor(sampleId);
                    if (!pointer)
                        return false;
                    writeU32(tempFile + newBase + offset, pointer);
                    return true;
                };

                for (const Program &program : job.bank->programs)
                {
                    const quint32 entry = static_cast<quint32>(program.index) * 12;
                    if (entry + 12 > bankSize)
                        continue;
                    const Voice &voice = program.voice;
                    if (voice.type == VoiceType::Sample || voice.type == VoiceType::SampleFixed)
                    {
                        if (!patchSample(entry + 4, voice.sampleId))
                            return postprocessError = QStringLiteral("Bank %1 program %2 references a missing sample.")
                                    .arg(job.bank->id).arg(program.index);
                    }
                    else if (voice.type == VoiceType::KeySplit)
                    {
                        rebase(entry + 4, voice.subBase);
                        rebase(entry + 8, voice.tableAddress);
                        for (const SubVoice &sub : voice.subVoices)
                        {
                            if (!sub.romAddress || sub.romAddress < oldBase)
                                continue;
                            const quint32 subOffset = sub.romAddress - oldBase;
                            if (subOffset + 12 > bankSize)
                                continue;
                            if (!patchSample(subOffset + 4, sub.sampleId))
                                return postprocessError = QStringLiteral("Bank %1 program %2 references a missing sample.")
                                        .arg(job.bank->id).arg(program.index);
                        }
                    }
                    else if (voice.type == VoiceType::DrumKit)
                    {
                        rebase(entry + 4, voice.subBase);
                        for (const SubVoice &sub : voice.subVoices)
                        {
                            if (!sub.romAddress || sub.romAddress < oldBase)
                                continue;
                            const quint32 subOffset = sub.romAddress - oldBase;
                            if (subOffset + 12 > bankSize)
                                continue;
                            if (!patchSample(subOffset + 4, sub.sampleId))
                                return postprocessError = QStringLiteral("Bank %1 program %2 references a missing sample.")
                                        .arg(job.bank->id).arg(program.index);
                        }
                    }
                    else if (voice.type == VoiceType::PsgWave ||
                             voice.type == VoiceType::PsgWaveAlias)
                    {
                        rebase(entry + 4, voice.waveAddress);
                    }
                }

                for (const Program &program : job.bank->programs)
                {
                    if (!program.changed)
                        continue;
                    const quint32 entry = static_cast<quint32>(program.index) * 12;
                    if (entry + 12 > bankSize)
                        continue; // short sub-banks do not physically hold all 128 slots
                    const Voice &voice = program.voice;
                    unsigned char *voiceData = tempFile + newBase + entry;
                    voiceData[0] = voice.raw[0];
                    voiceData[1] = voice.raw[1];
                    voiceData[3] = voice.raw[3];

                    if (voice.type == VoiceType::Sample || voice.type == VoiceType::SampleFixed)
                    {
                        for (int b = 8; b < 12; ++b)
                            voiceData[b] = voice.raw[b];
                    }
                    else if (SoundPsgIO::isPsgType(voice.type))
                    {
                        for (int b = 8; b < 12; ++b)
                            voiceData[b] = voice.raw[b];
                        // raw[4..7] is the wave pointer for PSG wave voices;
                        // it was already rebased by the pointer pass above.
                    }
                    else if (voice.type == VoiceType::KeySplit ||
                             voice.type == VoiceType::DrumKit)
                    {
                        for (const SubVoice &sub : voice.subVoices)
                        {
                            if (!sub.romAddress || sub.romAddress < oldBase)
                                continue;
                            const quint32 subOffset = sub.romAddress - oldBase;
                            if (subOffset + 12 > bankSize)
                                continue;
                            unsigned char *subData = tempFile + newBase + subOffset;
                            subData[0] = sub.raw[0];
                            subData[1] = sub.raw[1];
                            subData[3] = sub.raw[3];
                            for (int b = 8; b < 12; ++b)
                                subData[b] = sub.raw[b];
                        }
                        if (voice.type == VoiceType::KeySplit &&
                                voice.tableAddress >= oldBase &&
                                voice.tableAddress + 128 <= oldEnd &&
                                voice.keyTable.size() == 128)
                        {
                            std::memcpy(tempFile + newBase + (voice.tableAddress - oldBase),
                                        voice.keyTable.constData(), 128);
                        }
                    }
                }
            }

            for (int i = 0; i < jobs.size(); ++i)
            {
                if (jobs[i].kind != JobKind::Song)
                    continue;
                Song *song = jobs[i].song;
                quint32 bankPointer = 0;
                const auto newBank = newBankData.constFind(song->bankId);
                if (newBank != newBankData.constEnd())
                    bankPointer = newBank.value() | RomBase;
                for (const Bank &bank : data.banks)
                {
                    if (bank.id == song->bankId)
                    {
                        if (!bankPointer)
                            bankPointer = bank.dataAddress | RomBase;
                        break;
                    }
                }
                if (!bankPointer)
                    return postprocessError = QStringLiteral("Song %1 uses an invalid bank.").arg(song->id);

                const quint32 songData = newSongData[i];
                writeU32(tempFile + songData + 4, bankPointer);

                const quint32 entry = WL4Constants::SongTable +
                        static_cast<quint32>(song->id) * 8;
                writeU32(tempFile + entry, songData | RomBase);
                writeU16(tempFile + entry + 4, song->ms);
                writeU16(tempFile + entry + 6, song->me);
            }

            for (const Song &song : data.songs)
            {
                if (song.changed && song.importFromS)
                    continue;
                const auto newBank = newBankData.constFind(song.bankId);
                if (newBank == newBankData.constEnd())
                    continue;
                if (song.romAddress + 8 > ROMUtils::ROMFileMetadata->Length)
                    continue;
                writeU32(tempFile + song.romAddress + 4, newBank.value() | RomBase);
            }

            return QString();
        });

        if (!saved)
        {
            if (!postprocessError.isEmpty())
                return postprocessError;
            return allocatorError.isEmpty()
                    ? QStringLiteral("Failed to save the sound data to the ROM.")
                    : allocatorError;
        }

        updateManifestAfterSave(projectFolder, jobs);
        if (progress)
            progress(totalSteps, totalSteps);

        for (Bank &bank : data.banks)
        {
            for (Program &program : bank.programs)
                program.changed = false;
        }

        for (Job &job : jobs)
        {
            if (job.kind == JobKind::Song)
            {
                job.song->changed = false;
                job.song->importFromS = false;
                job.song->importedSFile.clear();
                job.song->importedSHash.clear();
            }
            else if (job.kind == JobKind::Sample)
            {
                job.sample->changed = false;
                job.sample->importedWavFile.clear();
                job.sample->importedWavHash.clear();
                job.sample->importedSf2File.clear();
                job.sample->importedSf2Hash.clear();
            }
        }
        return QString();
    }
} // namespace Sound
