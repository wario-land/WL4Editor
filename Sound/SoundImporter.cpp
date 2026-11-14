#include "SoundImporter.h"

#include "FileIOUtils.h"
#include "SoundEnvelope.h"
#include "SoundFontReader.h"
#include "SoundPsgIO.h"
#include "SoundWavIO.h"
#include "WL4Constants.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>

namespace Sound
{
    namespace
    {
        Song *findSong(ParseResult &data, int id)
        {
            for (Song &song : data.songs)
            {
                if (song.id == id)
                    return &song;
            }
            return nullptr;
        }

        Sample *findSample(ParseResult &data, int id)
        {
            for (Sample &sample : data.samples)
            {
                if (sample.id == id)
                    return &sample;
            }
            return nullptr;
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

        int sf2PanForVoice(quint8 gbaPan)
        {
            if ((gbaPan & 0x80) == 0)
                return 0;
            const int pan = gbaPan & 0x7F;
            return qBound(-500, (pan - 64) * 500 / 63, 500);
        }

        int metadataInt(const QString &text, const QString &key, int fallback)
        {
            const QRegularExpression expression(
                        QStringLiteral("@\\s*%1\\s*:\\s*(-?\\d+)").arg(QRegularExpression::escape(key)));
            const QRegularExpressionMatch match = expression.match(text);
            return match.hasMatch() ? match.captured(1).toInt() : fallback;
        }

        QString metadataString(const QString &text, const QString &key)
        {
            const QRegularExpression expression(
                        QStringLiteral("@\\s*%1\\s*:\\s*(.*)").arg(QRegularExpression::escape(key)));
            const QRegularExpressionMatch match = expression.match(text);
            return match.hasMatch() ? match.captured(1).trimmed() : QString();
        }
    } // namespace

    ImportResult SoundImporter::importAll(const QString &folder, ParseResult &data,
                                          const unsigned char *rom, quint32 romSize)
    {
        ImportResult result;

        const QString manifestPath = QDir(folder).filePath(QStringLiteral("soundmanifest.json"));
        QFile manifestFile(manifestPath);
        if (!manifestFile.open(QIODevice::ReadOnly))
        {
            result.error = QStringLiteral("Cannot open %1: %2").arg(manifestPath, manifestFile.errorString());
            return result;
        }

        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(manifestFile.readAll(), &parseError);
        manifestFile.close();
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            result.error = QStringLiteral("Invalid soundmanifest.json: %1")
                    .arg(parseError.errorString());
            return result;
        }

        const QJsonObject root = document.object();
        if (root.value(QStringLiteral("format")).toString() != QStringLiteral("WL4Editor.soundProject"))
        {
            result.error = QStringLiteral("soundmanifest.json is not a WL4Editor sound project.");
            return result;
        }

        if (!rom || romSize < 0x800000)
        {
            result.error = QStringLiteral("Invalid ROM data.");
            return result;
        }

        const QByteArray romHash = QCryptographicHash::hash(
                    QByteArray(reinterpret_cast<const char *>(rom), static_cast<int>(romSize)),
                    QCryptographicHash::Sha1).toHex();
        const QString manifestRomHash = root.value(QStringLiteral("rom")).toObject()
                .value(QStringLiteral("sha1")).toString();
        if (!manifestRomHash.isEmpty() && manifestRomHash.compare(QString::fromLatin1(romHash),
                                                                  Qt::CaseInsensitive) != 0)
        {
            // Other managers can modify the ROM after the sound project was
            // exported. The sound table layout is still assumed to be the
            // supported layout, so warn instead of blocking the import.
            result.warnings.append(QStringLiteral(
                        "The ROM SHA-1 differs from the sound project export "
                        "(manifest %1, current %2). The fixed sound table layout is assumed unchanged.")
                    .arg(manifestRomHash, QString::fromLatin1(romHash)));
        }

        const QJsonArray songs = root.value(QStringLiteral("songs")).toArray();
        if (songs.isEmpty())
        {
            result.error = QStringLiteral("soundmanifest.json contains no songs.");
            return result;
        }

        // Import is a full diff against the manifest: discard any previously
        // staged state, then stage every .s file whose hash changed.
        for (Song &song : data.songs)
        {
            song.changed = false;
            song.importFromS = false;
            song.importedSFile.clear();
            song.importedSHash.clear();
        }
        for (Sample &sample : data.samples)
        {
            sample.changed = false;
            sample.importedWavFile.clear();
            sample.importedWavHash.clear();
            sample.importedSf2File.clear();
            sample.importedSf2Hash.clear();
        }
        for (Bank &bank : data.banks)
        {
            for (Program &program : bank.programs)
                program.changed = false;
        }

        for (const QJsonValue &value : songs)
        {
            const QJsonObject object = value.toObject();
            const int id = object.value(QStringLiteral("id")).toInt(-1);
            Song *song = findSong(data, id);
            if (!song)
            {
                result.warnings.append(QStringLiteral("Song %1 from the manifest is not in the ROM.")
                                       .arg(id));
                continue;
            }

            const QString relativePath = object.value(QStringLiteral("sFile")).toString();
            if (relativePath.isEmpty())
            {
                result.warnings.append(QStringLiteral("Song %1 has no .s file in the manifest.").arg(id));
                continue;
            }

            const QString absolutePath = QDir(folder).absoluteFilePath(relativePath);
            if (!FileIOUtils::IsPathInsideDirectory(absolutePath, folder))
            {
                result.warnings.append(QStringLiteral("Song %1 .s path escapes the project folder.").arg(id));
                continue;
            }

            QFile source(absolutePath);
            if (!source.open(QIODevice::ReadOnly))
            {
                result.warnings.append(QStringLiteral("Song %1 .s file is missing: %2")
                                       .arg(id).arg(relativePath));
                continue;
            }
            const QByteArray content = source.readAll();
            source.close();

            const QByteArray sourceHash = QCryptographicHash::hash(content, QCryptographicHash::Sha1).toHex();
            const QString manifestSourceHash = object.value(QStringLiteral("sHash")).toString();
            if (!manifestSourceHash.isEmpty() &&
                    manifestSourceHash.compare(QString::fromLatin1(sourceHash), Qt::CaseInsensitive) == 0)
            {
                continue; // not edited since export
            }

            const QString text = QString::fromUtf8(content);
            const int bankFromSource = metadataInt(text, QStringLiteral("wl4-bank"), -1);
            const int bankFromManifest = object.value(QStringLiteral("bankId")).toInt(-1);
            const int bankId = bankFromSource >= 0 ? bankFromSource : bankFromManifest;
            if (bankId < 0 || bankId >= WL4Constants::SoundBankCount ||
                    WL4Constants::SoundBankBase[bankId] == 0)
            {
                result.warnings.append(QStringLiteral("Song %1 uses an invalid bank id %2.")
                                       .arg(id).arg(bankId));
                continue;
            }

            const int player = metadataInt(text, QStringLiteral("wl4-player"),
                                           object.value(QStringLiteral("ms")).toInt(song->ms));
            const int ms = metadataInt(text, QStringLiteral("wl4-ms"), player);
            const int me = metadataInt(text, QStringLiteral("wl4-me"), ms);
            const int priority = metadataInt(text, QStringLiteral("wl4-priority"),
                                             object.value(QStringLiteral("priority")).toInt(song->priority));
            const int echo = metadataInt(text, QStringLiteral("wl4-echo"),
                                         object.value(QStringLiteral("echo")).toInt(song->echo));
            const QString name = metadataString(text, QStringLiteral("wl4-name"));

            song->bankId = bankId;
            song->ms = static_cast<quint16>(qBound(0, ms, 7));
            song->me = static_cast<quint16>(qBound(0, me, 7));
            song->priority = static_cast<quint8>(qBound(0, priority, 255));
            song->echo = static_cast<quint8>(qBound(0, echo, 255));
            if (!name.isEmpty())
                song->name = name;
            song->importFromS = true;
            song->importedSFile = absolutePath;
            song->importedSHash = sourceHash;
            song->changed = true;

            result.changedSongIds.append(song->id);
        }

        QSet<int> changedSamples;
        QSet<int> wavChangedSamples;
        const QJsonArray samples = root.value(QStringLiteral("samples")).toArray();
        for (const QJsonValue &value : samples)
        {
            const QJsonObject object = value.toObject();
            const int id = object.value(QStringLiteral("id")).toInt(-1);
            Sample *sample = findSample(data, id);
            if (!sample)
                continue;

            const QString relativePath = object.value(QStringLiteral("wavFile")).toString();
            if (relativePath.isEmpty())
                continue;

            const QString absolutePath = QDir(folder).absoluteFilePath(relativePath);
            if (!FileIOUtils::IsPathInsideDirectory(absolutePath, folder))
            {
                result.warnings.append(QStringLiteral("Sample %1 WAV path escapes the project folder.").arg(id));
                continue;
            }

            QFile source(absolutePath);
            if (!source.open(QIODevice::ReadOnly))
            {
                result.warnings.append(QStringLiteral("Sample %1 WAV file is missing: %2")
                                       .arg(id).arg(relativePath));
                continue;
            }
            const QByteArray content = source.readAll();
            source.close();

            const QByteArray sourceHash = QCryptographicHash::hash(content, QCryptographicHash::Sha1).toHex();
            const QString manifestSourceHash = object.value(QStringLiteral("wavHash")).toString();
            if (!manifestSourceHash.isEmpty() &&
                    manifestSourceHash.compare(QString::fromLatin1(sourceHash), Qt::CaseInsensitive) == 0)
            {
                continue;
            }

            QByteArray pcm8;
            quint32 sampleRate = 0;
            const QString wavError = SoundWavIO::read(absolutePath, pcm8, sampleRate);
            if (!wavError.isEmpty())
            {
                result.warnings.append(QStringLiteral("Sample %1: %2").arg(id).arg(wavError));
                continue;
            }
            if (pcm8.isEmpty())
            {
                result.warnings.append(QStringLiteral("Sample %1 WAV contains no frames.").arg(id));
                continue;
            }
            if (sampleRate > 0 &&
                    qAbs(static_cast<double>(sampleRate) - sample->sampleRate()) > 1.0)
            {
                result.warnings.append(QStringLiteral(
                            "Sample %1 WAV rate %2 Hz differs from the original %3 Hz; "
                            "the original pitch is kept.")
                        .arg(id).arg(sampleRate).arg(sample->sampleRate(), 0, 'f', 2));
            }

            sample->pcm = pcm8;
            sample->size = static_cast<quint32>(pcm8.size());
            sample->sha1 = QCryptographicHash::hash(pcm8, QCryptographicHash::Sha1).toHex();
            if (sample->isLooped() && sample->loopStart >= sample->size)
            {
                sample->flag &= static_cast<quint8>(~0x40);
                sample->loopStart = 0;
                result.warnings.append(QStringLiteral(
                            "Sample %1 is shorter than its loop start; the loop was disabled.").arg(id));
            }
            sample->changed = true;
            sample->importedWavFile = absolutePath;
            sample->importedWavHash = sourceHash;
            changedSamples.insert(id);
            wavChangedSamples.insert(id);
            result.changedSampleIds.append(id);
        }

        // SF2 sample import is text-free: sample names in the SF2 keep the
        // Sxxxx mapping written by SoundFontWriter.
        const QJsonObject soundfont = root.value(QStringLiteral("soundfont")).toObject();
        const QString sf2Relative = soundfont.value(QStringLiteral("file")).toString();
        if (!sf2Relative.isEmpty())
        {
            const QString sf2Path = QDir(folder).absoluteFilePath(sf2Relative);
            if (FileIOUtils::IsPathInsideDirectory(sf2Path, folder))
            {
                QFile sf2File(sf2Path);
                if (!sf2File.open(QIODevice::ReadOnly))
                {
                    result.warnings.append(QStringLiteral("SF2 file is missing: %1").arg(sf2Relative));
                }
                else
                {
                    const QByteArray sf2Content = sf2File.readAll();
                    sf2File.close();
                    const QByteArray sf2Hash = QCryptographicHash::hash(
                                sf2Content, QCryptographicHash::Sha1).toHex();
                    const QString manifestSf2Hash = soundfont.value(QStringLiteral("hash")).toString();
                    if (manifestSf2Hash.isEmpty() ||
                            manifestSf2Hash.compare(QString::fromLatin1(sf2Hash),
                                                    Qt::CaseInsensitive) != 0)
                    {
                        const Sf2ReadResult sf2 = SoundFontReader::read(sf2Path);
                        for (const QString &warning : sf2.warnings)
                            result.warnings.append(warning);
                        if (!sf2.error.isEmpty())
                        {
                            result.warnings.append(QStringLiteral("SF2 import failed: %1").arg(sf2.error));
                        }
                        else
                        {
                            QHash<int, int> sf2SampleIdMap;
                            int nextSampleId = 0;
                            for (const Sample &sample : data.samples)
                            {
                                if (sample.id >= nextSampleId)
                                    nextSampleId = sample.id + 1;
                            }

                            for (const Sf2Sample &sf2Sample : sf2.samples)
                            {
                                Sample *sample = findSample(data, sf2Sample.sampleId);
                                if (!sample)
                                {
                                    for (Sample &candidate : data.samples)
                                    {
                                        if (candidate.sf2SampleId == sf2Sample.sampleId)
                                        {
                                            sample = &candidate;
                                            break;
                                        }
                                    }
                                }
                                if (!sample)
                                {
                                    if (sf2Sample.pcm.isEmpty())
                                        continue;
                                    Sample fresh;
                                    fresh.id = nextSampleId++;
                                    fresh.name = sf2Sample.name;
                                    fresh.romAddress = 0;
                                    fresh.isNew = true;
                                    fresh.loopStart = sf2Sample.looped
                                            ? static_cast<quint32>(sf2Sample.loopStart) : 0;
                                    fresh.size = static_cast<quint32>(sf2Sample.pcm.size());
                                    fresh.flag = sf2Sample.looped ? 0x40 : 0;
                                    fresh.pitch = static_cast<quint32>(
                                                qMax(1, sf2Sample.sampleRate)) * 1024u;
                                    fresh.pcm = sf2Sample.pcm;
                                    fresh.sha1 = QCryptographicHash::hash(
                                                fresh.pcm, QCryptographicHash::Sha1).toHex();
                                    fresh.changed = true;
                                    fresh.sf2SampleId = sf2Sample.sampleId;
                                    fresh.importedSf2File = sf2Path;
                                    fresh.importedSf2Hash = sf2Hash;
                                    if (fresh.id >= data.samples.size())
                                        data.samples.resize(fresh.id + 1);
                                    data.samples[fresh.id] = fresh;
                                    sample = &data.samples[fresh.id];
                                    changedSamples.insert(sample->id);
                                    result.changedSampleIds.append(sample->id);
                                }
                                sf2SampleIdMap.insert(sf2Sample.sampleId, sample->id);
                                sample->sf2SampleId = sf2Sample.sampleId;
                                if (sample->isNew)
                                {
                                    changedSamples.insert(sample->id);
                                    if (!result.changedSampleIds.contains(sample->id))
                                        result.changedSampleIds.append(sample->id);
                                }

                                if (wavChangedSamples.contains(sample->id))
                                {
                                    result.warnings.append(QStringLiteral(
                                                "Sample %1 was changed by both WAV and SF2; WAV takes precedence.")
                                            .arg(sample->id));
                                    continue;
                                }
                                if (sf2Sample.pcm.isEmpty())
                                    continue;

                                const bool pcmChanged = sample->pcm != sf2Sample.pcm;
                                const bool loopChanged = sf2Sample.looped != sample->isLooped() ||
                                        (sf2Sample.looped &&
                                         sample->loopStart != static_cast<quint32>(sf2Sample.loopStart));
                                if (!pcmChanged && !loopChanged)
                                    continue;

                                sample->pcm = sf2Sample.pcm;
                                sample->size = static_cast<quint32>(sample->pcm.size());
                                sample->sha1 = QCryptographicHash::hash(
                                            sample->pcm, QCryptographicHash::Sha1).toHex();
                                // Keep the GBA pitch. SF2 sample rates are the
                                // same value rounded to Hz by SoundFontWriter.
                                if (sf2Sample.looped &&
                                        sf2Sample.loopStart < sample->pcm.size())
                                {
                                    sample->flag |= 0x40;
                                    sample->loopStart = static_cast<quint32>(sf2Sample.loopStart);
                                }
                                else
                                {
                                    sample->flag &= static_cast<quint8>(~0x40);
                                    sample->loopStart = 0;
                                }
                                sample->changed = true;
                                sample->importedSf2File = sf2Path;
                                sample->importedSf2Hash = sf2Hash;
                                changedSamples.insert(sample->id);
                                result.changedSampleIds.append(sample->id);
                            }

                            // Preset/instrument-level changes for simple sample
                            // Programs. Key-split and drum-kit zone remapping is
                            // handled separately in a later phase.
                            int sf2MissingPrograms = 0;
                            int sf2SkippedLayout = 0;
                            int sf2IncompleteEnvelope = 0;
                            auto applySubVoice = [&](SubVoice &sub, const Sf2Zone &zone,
                                                     int sampleId, int &incompleteEnvelope) -> bool
                            {
                                bool changed = false;
                                if (sampleId >= 0 && sub.sampleId != sampleId)
                                {
                                    sub.sampleId = sampleId;
                                    changed = true;
                                }
                                if (zone.hasPan)
                                {
                                    const int expected = sf2PanForVoice(sub.pan);
                                    if (qAbs(zone.pan - expected) > 1)
                                    {
                                        const int pan = qBound(0, qRound(64.0 +
                                                                         (zone.pan < 0
                                                                          ? zone.pan * 64.0 / 500.0
                                                                          : zone.pan * 63.0 / 500.0)),
                                                               127);
                                        const quint8 rawPan = static_cast<quint8>(pan | 0x80);
                                        sub.pan = rawPan;
                                        sub.raw[3] = rawPan;
                                        changed = true;
                                    }
                                }

                                const bool hasEnvelope = zone.hasAttack && zone.hasDecay &&
                                        zone.hasSustain && zone.hasRelease;
                                const bool partialEnvelope = !hasEnvelope &&
                                        (zone.hasAttack || zone.hasDecay ||
                                         zone.hasSustain || zone.hasRelease);
                                if (partialEnvelope)
                                {
                                    ++incompleteEnvelope;
                                }
                                else if (hasEnvelope)
                                {
                                    const Sf2Envelope sf2Envelope{
                                        zone.attack, zone.decay, zone.sustain, zone.release };
                                    const GbaEnvelope current{
                                        sub.attack, sub.decay, sub.sustain, sub.release };
                                    if (!envelopeMatchesSf2(current, sf2Envelope))
                                    {
                                        const GbaEnvelope envelope =
                                                envelopeFromSf2(sf2Envelope);
                                        sub.attack = envelope.attack;
                                        sub.decay = envelope.decay;
                                        sub.sustain = envelope.sustain;
                                        sub.release = envelope.release;
                                        sub.raw[8] = envelope.attack;
                                        sub.raw[9] = envelope.decay;
                                        sub.raw[10] = envelope.sustain;
                                        sub.raw[11] = envelope.release;
                                        changed = true;
                                    }
                                }
                                return changed;
                            };

                            for (const Sf2Program &sf2Program : sf2.programs)
                            {
                                Bank *targetBank = nullptr;
                                Program *targetProgram = nullptr;
                                for (Bank &bank : data.banks)
                                {
                                    if (bank.id != sf2Program.bank)
                                        continue;
                                    targetBank = &bank;
                                    for (Program &program : bank.programs)
                                    {
                                        if (program.index == sf2Program.program)
                                        {
                                            targetProgram = &program;
                                            break;
                                        }
                                    }
                                    break;
                                }
                                bool createdProgram = false;
                                if (targetBank && (!targetProgram || !targetProgram->used) &&
                                        sf2Program.program >= 0 &&
                                        sf2Program.program < targetBank->programs.size() &&
                                        static_cast<quint32>(sf2Program.program) * 12 + 12 <=
                                            targetBank->dataSize &&
                                        !targetBank->programs[sf2Program.program].used &&
                                        !sf2Program.zones.isEmpty())
                                {
                                    const int firstSample = sf2SampleIdMap.value(
                                                sf2Program.zones.first().sampleId,
                                                sf2Program.zones.first().sampleId);
                                    if (firstSample >= 0 && findSample(data, firstSample))
                                    {
                                        Program fresh;
                                        fresh.index = sf2Program.program;
                                        fresh.used = true;
                                        fresh.changed = true;
                                        fresh.name = sf2Program.name;
                                        fresh.voice.type = VoiceType::Sample;
                                        fresh.voice.romAddress = targetBank->dataAddress +
                                                static_cast<quint32>(fresh.index) * 12;
                                        fresh.voice.sampleId = firstSample;
                                        fresh.voice.key = 60;
                                        fresh.voice.raw[0] = 0;
                                        fresh.voice.raw[1] = 60;
                                        targetBank->programs[fresh.index] = fresh;
                                        targetProgram = &targetBank->programs[fresh.index];
                                        createdProgram = true;
                                    }
                                }
                                if (!targetBank || !targetProgram)
                                {
                                    ++sf2MissingPrograms;
                                    continue;
                                }
                                if (targetProgram->voice.type == VoiceType::KeySplit ||
                                        targetProgram->voice.type == VoiceType::DrumKit)
                                {
                                    bool programChanged = false;
                                    int zoneIndex = 0;
                                    for (const Sf2Zone &zone : sf2Program.zones)
                                    {
                                        const int modelSampleId = sf2SampleIdMap.value(
                                                    zone.sampleId, zone.sampleId);
                                        if (modelSampleId < 0 ||
                                                !findSample(data, modelSampleId))
                                        {
                                            ++sf2SkippedLayout;
                                            ++zoneIndex;
                                            continue;
                                        }
                                        const int low = zone.hasKeyRange ? zone.keyLow : 0;
                                        const int high = zone.hasKeyRange ? zone.keyHigh : 127;
                                        if (targetProgram->voice.type == VoiceType::KeySplit)
                                        {
                                            if (targetProgram->voice.keyTable.size() != 128 ||
                                                    zoneIndex >= targetProgram->voice.subVoices.size())
                                            {
                                                ++sf2SkippedLayout;
                                                ++zoneIndex;
                                                continue;
                                            }
                                            const int subIndex = zoneIndex;
                                            if (applySubVoice(
                                                        targetProgram->voice.subVoices[subIndex],
                                                        zone, modelSampleId, sf2IncompleteEnvelope))
                                            {
                                                programChanged = true;
                                            }
                                            for (int key = low; key <= high; ++key)
                                            {
                                                if (targetProgram->voice.keyTable[key] != subIndex)
                                                {
                                                    targetProgram->voice.keyTable[key] = subIndex;
                                                    programChanged = true;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            for (int key = low; key <= high; ++key)
                                            {
                                                if (key >= targetProgram->voice.subVoices.size())
                                                    break;
                                                if (applySubVoice(
                                                            targetProgram->voice.subVoices[key],
                                                            zone, modelSampleId, sf2IncompleteEnvelope))
                                                {
                                                    programChanged = true;
                                                }
                                            }
                                        }
                                        ++zoneIndex;
                                    }
                                    if (programChanged)
                                    {
                                        targetProgram->changed = true;
                                        ++result.changedProgramCount;
                                    }
                                    continue;
                                }
                                if (targetProgram->voice.type != VoiceType::Sample &&
                                        targetProgram->voice.type != VoiceType::SampleFixed)
                                {
                                    continue;
                                }
                                if (sf2Program.zones.size() != 1 ||
                                        (sf2Program.zones.first().hasKeyRange &&
                                         (sf2Program.zones.first().keyLow != 0 ||
                                          sf2Program.zones.first().keyHigh != 127)))
                                {
                                    ++sf2SkippedLayout;
                                    continue;
                                }

                                const Sf2Zone &zone = sf2Program.zones.first();
                                bool programChanged = createdProgram;

                                const bool hasEnvelope = zone.hasAttack && zone.hasDecay &&
                                        zone.hasSustain && zone.hasRelease;
                                const bool partialEnvelope = !hasEnvelope &&
                                        (zone.hasAttack || zone.hasDecay ||
                                         zone.hasSustain || zone.hasRelease);
                                if (partialEnvelope)
                                {
                                    ++sf2IncompleteEnvelope;
                                }
                                else if (hasEnvelope)
                                {
                                    const Sf2Envelope sf2Envelope{
                                        zone.attack, zone.decay, zone.sustain, zone.release };
                                    const GbaEnvelope currentEnvelope{
                                        targetProgram->voice.attack,
                                        targetProgram->voice.decay,
                                        targetProgram->voice.sustain,
                                        targetProgram->voice.release };
                                    if (!envelopeMatchesSf2(currentEnvelope, sf2Envelope))
                                    {
                                        const GbaEnvelope envelope =
                                                envelopeFromSf2(sf2Envelope);
                                        targetProgram->voice.attack = envelope.attack;
                                        targetProgram->voice.decay = envelope.decay;
                                        targetProgram->voice.sustain = envelope.sustain;
                                        targetProgram->voice.release = envelope.release;
                                        targetProgram->voice.raw[8] = envelope.attack;
                                        targetProgram->voice.raw[9] = envelope.decay;
                                        targetProgram->voice.raw[10] = envelope.sustain;
                                        targetProgram->voice.raw[11] = envelope.release;
                                        programChanged = true;
                                    }
                                }
                                const int modelSampleId = sf2SampleIdMap.value(
                                            zone.sampleId, zone.sampleId);
                                if (modelSampleId >= 0 &&
                                        modelSampleId != targetProgram->voice.sampleId)
                                {
                                    if (!findSample(data, modelSampleId))
                                    {
                                        result.warnings.append(QStringLiteral(
                                                    "SF2 preset %1 references missing sample %2.")
                                                .arg(sf2Program.name).arg(modelSampleId));
                                        continue;
                                    }
                                    targetProgram->voice.sampleId = modelSampleId;
                                    programChanged = true;
                                }
                                // Only apply a pan change if SF2 actually differs
                                // from what the writer produced for the current
                                // GBA pan. This avoids round-trip noise.
                                if (zone.hasPan)
                                {
                                    const int expectedPan = sf2PanForVoice(targetProgram->voice.pan);
                                    if (qAbs(zone.pan - expectedPan) > 1)
                                    {
                                        const int pan = qBound(0, qRound(64.0 +
                                                                         (zone.pan < 0
                                                                          ? zone.pan * 64.0 / 500.0
                                                                          : zone.pan * 63.0 / 500.0)),
                                                               127);
                                        const quint8 rawPan = static_cast<quint8>(pan | 0x80);
                                        targetProgram->voice.pan = rawPan;
                                        targetProgram->voice.raw[3] = rawPan;
                                        programChanged = true;
                                    }
                                }
                                if (programChanged)
                                {
                                    targetProgram->changed = true;
                                    ++result.changedProgramCount;
                                }
                            }
                            if (sf2MissingPrograms)
                            {
                                result.warnings.append(QStringLiteral(
                                            "%1 SF2 preset(s) do not map to an existing bank/program.")
                                        .arg(sf2MissingPrograms));
                            }
                            if (sf2SkippedLayout)
                            {
                                result.warnings.append(QStringLiteral(
                                            "%1 SF2 preset(s) with a non-simple key layout were skipped.")
                                        .arg(sf2SkippedLayout));
                            }
                            if (sf2IncompleteEnvelope)
                            {
                                result.warnings.append(QStringLiteral(
                                            "%1 SF2 preset(s) have only a partial envelope; "
                                            "ADSR was not applied.")
                                        .arg(sf2IncompleteEnvelope));
                            }
                        }
                    }
                }
            }
        }

        for (const Bank &bank : data.banks)
        {
            for (const Program &program : bank.programs)
            {
                if (voiceUsesSample(program.voice, changedSamples))
                {
                    result.changedBankIds.append(bank.id);
                    break;
                }
            }
        }

        const QString psgPath = QDir(folder).filePath(QStringLiteral("psg.json"));
        QFile psgFile(psgPath);
        if (psgFile.exists() && psgFile.open(QIODevice::ReadOnly))
        {
            const QByteArray psgBytes = psgFile.readAll();
            psgFile.close();
            const QJsonObject psgManifest =
                    document.object().value(QStringLiteral("psg")).toObject();
            const QString expectedHash = psgManifest.value(QStringLiteral("hash")).toString();
            const QString actualHash = QString::fromLatin1(
                        QCryptographicHash::hash(psgBytes, QCryptographicHash::Sha1).toHex());
            if (expectedHash.isEmpty() || expectedHash != actualHash)
            {
                QVector<int> changedProgramKeys;
                const QString psgError = SoundPsgIO::read(
                            psgPath, data, &changedProgramKeys, &result.warnings);
                if (!psgError.isEmpty())
                    result.warnings.append(psgError);
                for (int key : changedProgramKeys)
                {
                    const int bankId = key >> 16;
                    if (!result.changedBankIds.contains(bankId))
                        result.changedBankIds.append(bankId);
                }
                result.changedProgramCount += changedProgramKeys.size();
            }
        }

        return result;
    }
} // namespace Sound
