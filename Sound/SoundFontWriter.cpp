#include "SoundFontWriter.h"

#include "SoundEnvelope.h"

#include <QFile>
#include <QHash>
#include <QSet>
#include <QVector>

#include <cmath>

namespace Sound
{
    namespace
    {
        struct SfSample
        {
            QString name;
            QByteArray pcm16;
            bool looped = false;
            int startWord = 0;
            int endWord = 0;
            int loopStartWord = 0;
            int loopEndWord = 0;
            int sampleRate = 1;
            int pitchCorrection = 0;
        };

        struct SfZone
        {
            int sampleIndex = -1;
            int keyLow = 0;
            int keyHigh = 127;
            int pan = 0;
            bool looped = false;
            GbaEnvelope envelope;
        };

        struct SfInstrument
        {
            QString name;
            QVector<SfZone> zones;
        };

        struct SfPreset
        {
            QString name;
            int bank = 0;
            int program = 0;
            int instrument = 0;
        };

        void appendU16(QByteArray &data, quint16 value)
        {
            data.append(static_cast<char>(value & 0xFF));
            data.append(static_cast<char>((value >> 8) & 0xFF));
        }

        void appendU32(QByteArray &data, quint32 value)
        {
            data.append(static_cast<char>(value & 0xFF));
            data.append(static_cast<char>((value >> 8) & 0xFF));
            data.append(static_cast<char>((value >> 16) & 0xFF));
            data.append(static_cast<char>((value >> 24) & 0xFF));
        }

        void appendS16(QByteArray &data, qint16 value)
        {
            appendU16(data, static_cast<quint16>(value));
        }

        QByteArray name20(const QString &name)
        {
            QByteArray result = name.toLatin1().left(20);
            result.resize(20, '\0');
            return result;
        }

        QByteArray makeChunk(const char id[4], const QByteArray &data)
        {
            QByteArray result;
            result.append(id, 4);
            appendU32(result, static_cast<quint32>(data.size()));
            result.append(data);
            if (data.size() & 1)
                result.append('\0');
            return result;
        }

        QByteArray makeList(const char type[4], const QByteArray &content)
        {
            QByteArray result;
            result.append("LIST", 4);
            appendU32(result, static_cast<quint32>(content.size() + 4));
            result.append(type, 4);
            result.append(content);
            return result;
        }

        QString sanitize(const QString &name)
        {
            QString result;
            for (const QChar ch : name)
            {
                if (ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char('-'))
                    result.append(ch);
                else if (ch.isSpace())
                    result.append(QLatin1Char('_'));
            }
            if (result.isEmpty())
                result = QStringLiteral("Unnamed");
            return result.left(20);
        }

        QString uniqueSampleName(const QString &base, int index, QSet<QString> &used)
        {
            QString name = QStringLiteral("S%1 %2")
                    .arg(index, 4, 10, QLatin1Char('0'))
                    .arg(sanitize(base));
            name = name.left(20);
            QString candidate = name;
            int suffix = 1;
            while (used.contains(candidate))
            {
                const QString tail = QStringLiteral("_%1").arg(suffix++);
                candidate = name.left(20 - tail.size()) + tail;
            }
            used.insert(candidate);
            return candidate;
        }

        QString presetName(const QString &base, int bank, int program)
        {
            QString name = QStringLiteral("VG%1P%2 %3")
                    .arg(bank, 3, 10, QLatin1Char('0'))
                    .arg(program, 3, 10, QLatin1Char('0'))
                    .arg(sanitize(base));
            return name.left(20);
        }

        int sf2Pan(quint8 gbaPan)
        {
            // Bit 7 is the "force this pan" flag; otherwise the channel pan is
            // used and 0 (center) is the neutral value in SF2.
            if ((gbaPan & 0x80) == 0)
                return 0;
            const int pan = gbaPan & 0x7F;
            return qBound(-500, (pan - 64) * 500 / 63, 500);
        }
    } // namespace

    QString SoundFontWriter::write(const ParseResult &data, const QString &filePath,
                                   QStringList *warnings)
    {
        QVector<SfSample> samples;
        QHash<int, int> sampleIndexById;
        QSet<QString> usedSampleNames;

        QByteArray smpl;
        samples.reserve(data.samples.size());
        for (const Sample &sample : data.samples)
        {
            if (sample.pcm.isEmpty())
                continue;

            SfSample sf;
            sf.name = uniqueSampleName(sample.name, sample.id, usedSampleNames);
            sf.looped = sample.isLooped();
            sf.pcm16.reserve(sample.pcm.size() * 2);

            for (int i = 0; i < sample.pcm.size(); ++i)
            {
                const qint8 value = static_cast<qint8>(sample.pcm.at(i));
                const qint16 expanded = static_cast<qint16>(value) << 8;
                appendS16(sf.pcm16, expanded);
            }

            const double exactRate = sample.pitch / 1024.0;
            sf.sampleRate = qMax(1, qRound(exactRate));
            const double cents = exactRate > 0.0
                    ? 1200.0 * std::log2(exactRate / static_cast<double>(sf.sampleRate))
                    : 0.0;
            sf.pitchCorrection = qBound(-128, qRound(cents), 127);

            sf.startWord = smpl.size() / 2;
            smpl.append(sf.pcm16);
            sf.endWord = sf.startWord + sample.pcm.size();
            if (sf.looped)
            {
                sf.loopStartWord = sf.startWord + static_cast<int>(sample.loopStart);
                sf.loopEndWord = sf.endWord;
            }
            else
            {
                sf.loopStartWord = 0;
                sf.loopEndWord = 0;
            }
            sampleIndexById.insert(sample.id, samples.size());
            samples.append(sf);
        }

        // SF2 requires 46 zero words after the last sample for interpolation.
        smpl.append(QByteArray(46 * 2, '\0'));

        QVector<SfInstrument> instruments;
        QVector<SfPreset> presets;

        auto addZone = [&](QVector<SfZone> &zones, int sampleId, int keyLow, int keyHigh,
                           quint8 pan, bool looped, const GbaEnvelope &envelope)
        {
            const auto it = sampleIndexById.constFind(sampleId);
            if (it == sampleIndexById.constEnd())
                return;
            SfZone zone;
            zone.sampleIndex = it.value();
            zone.keyLow = qBound(0, keyLow, 127);
            zone.keyHigh = qBound(zone.keyLow, keyHigh, 127);
            zone.pan = sf2Pan(pan);
            zone.looped = looped;
            zone.envelope = envelope;
            zones.append(zone);
        };

        auto buildZones = [&](const Voice &voice) -> QVector<SfZone>
        {
            QVector<SfZone> zones;
            switch (voice.type)
            {
            case VoiceType::Sample:
            case VoiceType::SampleFixed:
                if (voice.sampleId >= 0)
                {
                    GbaEnvelope envelope{ voice.attack, voice.decay, voice.sustain, voice.release };
                    addZone(zones, voice.sampleId, 0, 127, voice.pan,
                            voice.sampleId < data.samples.size() && data.samples[voice.sampleId].isLooped(),
                            envelope);
                }
                break;

            case VoiceType::KeySplit:
                if (voice.keyTable.size() == 128)
                {
                    int key = 0;
                    while (key < 128)
                    {
                        const quint8 subIndex = voice.keyTable[key];
                        int end = key + 1;
                        while (end < 128 && voice.keyTable[end] == subIndex)
                            ++end;
                        if (subIndex < voice.subVoices.size())
                        {
                            const SubVoice &sub = voice.subVoices[subIndex];
                            if (sub.sampleId >= 0)
                            {
                                GbaEnvelope envelope{ sub.attack, sub.decay, sub.sustain, sub.release };
                                addZone(zones, sub.sampleId, key, end - 1, sub.pan,
                                        sub.sampleId < data.samples.size() &&
                                        data.samples[sub.sampleId].isLooped(),
                                        envelope);
                            }
                        }
                        key = end;
                    }
                }
                break;

            case VoiceType::DrumKit:
                for (int key = 0; key < voice.subVoices.size() && key < 128; ++key)
                {
                    const SubVoice &sub = voice.subVoices[key];
                    if (sub.sampleId >= 0)
                    {
                        GbaEnvelope envelope{ sub.attack, sub.decay, sub.sustain, sub.release };
                        addZone(zones, sub.sampleId, key, key, sub.pan,
                                sub.sampleId < data.samples.size() &&
                                data.samples[sub.sampleId].isLooped(),
                                envelope);
                    }
                }
                break;

            default:
                break;
            }
            return zones;
        };

        for (const Bank &bank : data.banks)
        {
            for (const Program &program : bank.programs)
            {
                QVector<SfZone> zones = buildZones(program.voice);
                if (zones.isEmpty())
                    continue;

                SfInstrument instrument;
                instrument.name = presetName(program.name, bank.id, program.index);
                instrument.zones = zones;
                const int instrumentIndex = instruments.size();
                instruments.append(instrument);

                SfPreset preset;
                preset.name = instrument.name;
                preset.bank = bank.id;
                preset.program = program.index;
                preset.instrument = instrumentIndex;
                presets.append(preset);
            }
        }

        if (warnings && !presets.isEmpty())
        {
            const int skipped = data.usedProgramCount() - presets.size();
            if (skipped > 0)
            {
                warnings->append(QStringLiteral("%1 Program(s) without a sample (PSG/unsupported) "
                                                "are not exported to the SF2.").arg(skipped));
            }
        }

        QByteArray infoContent;
        QByteArray ifil;
        appendU16(ifil, 2);
        appendU16(ifil, 1);
        infoContent.append(makeChunk("ifil", ifil));
        QByteArray isng("EMU8000", 7);
        isng.append('\0');
        infoContent.append(makeChunk("isng", isng));
        QByteArray inam("WL4 SoundFont", 13);
        inam.append('\0');
        infoContent.append(makeChunk("INAM", inam));

        QByteArray sdtaContent;
        sdtaContent.append(makeChunk("smpl", smpl));

        QByteArray phdr;
        for (int i = 0; i < presets.size(); ++i)
        {
            const SfPreset &preset = presets[i];
            phdr.append(name20(preset.name));
            appendU16(phdr, static_cast<quint16>(preset.program));
            appendU16(phdr, static_cast<quint16>(preset.bank));
            appendU16(phdr, static_cast<quint16>(i));
            appendU32(phdr, 0);
            appendU32(phdr, 0);
            appendU32(phdr, 0);
        }
        phdr.append(name20(QStringLiteral("EOP")));
        phdr.append(QByteArray(18, '\0'));

        QByteArray pbag;
        for (int i = 0; i < presets.size(); ++i)
        {
            appendU16(pbag, static_cast<quint16>(i));
            appendU16(pbag, 0);
        }
        pbag.append(QByteArray(4, '\0'));

        QByteArray pmod(10, '\0');

        QByteArray pgen;
        for (const SfPreset &preset : presets)
        {
            appendU16(pgen, 0x0029); // instrument
            appendU16(pgen, static_cast<quint16>(preset.instrument));
        }
        pgen.append(QByteArray(4, '\0'));

        QByteArray inst;
        int zoneOffset = 0;
        for (const SfInstrument &instrument : instruments)
        {
            inst.append(name20(instrument.name));
            appendU16(inst, static_cast<quint16>(zoneOffset));
            zoneOffset += instrument.zones.size();
        }
        inst.append(name20(QStringLiteral("EOI")));
        inst.append(QByteArray(2, '\0'));

        QByteArray ibag;
        QByteArray igen;
        int generatorOffset = 0;
        for (const SfInstrument &instrument : instruments)
        {
            for (const SfZone &zone : instrument.zones)
            {
                appendU16(ibag, static_cast<quint16>(generatorOffset));
                appendU16(ibag, 0);

                appendU16(igen, 0x002B); // keyRange
                appendU16(igen, static_cast<quint16>(zone.keyLow | (zone.keyHigh << 8)));
                appendU16(igen, 0x0036); // sampleModes
                appendU16(igen, zone.looped ? 1 : 0);
                appendU16(igen, 0x003A); // overridingRootKey
                appendU16(igen, 60);
                appendU16(igen, 0x0011); // pan
                appendU16(igen, static_cast<quint16>(static_cast<qint16>(zone.pan)));
                const Sf2Envelope envelope = envelopeToSf2(zone.envelope);
                appendU16(igen, 0x0022); // attackVolEnv
                appendU16(igen, static_cast<quint16>(static_cast<qint16>(envelope.attackTimecents)));
                appendU16(igen, 0x0023); // decayVolEnv
                appendU16(igen, static_cast<quint16>(static_cast<qint16>(envelope.decayTimecents)));
                appendU16(igen, 0x0024); // sustainVolEnv
                appendU16(igen, static_cast<quint16>(static_cast<qint16>(envelope.sustainCentibels)));
                appendU16(igen, 0x0025); // releaseVolEnv
                appendU16(igen, static_cast<quint16>(static_cast<qint16>(envelope.releaseTimecents)));
                appendU16(igen, 0x0035); // sampleID (must be last)
                appendU16(igen, static_cast<quint16>(zone.sampleIndex));
                generatorOffset += 9;
            }
        }
        ibag.append(QByteArray(4, '\0'));
        igen.append(QByteArray(4, '\0'));
        QByteArray imod(10, '\0');

        QByteArray shdr;
        for (const SfSample &sample : samples)
        {
            shdr.append(name20(sample.name));
            appendU32(shdr, static_cast<quint32>(sample.startWord));
            appendU32(shdr, static_cast<quint32>(sample.endWord));
            appendU32(shdr, static_cast<quint32>(sample.loopStartWord));
            appendU32(shdr, static_cast<quint32>(sample.loopEndWord));
            appendU32(shdr, static_cast<quint32>(sample.sampleRate));
            shdr.append(static_cast<char>(60)); // originalPitch
            shdr.append(static_cast<char>(sample.pitchCorrection));
            appendU16(shdr, 0); // sampleLink
            appendU16(shdr, 1); // mono
        }
        shdr.append(name20(QStringLiteral("EOS")));
        shdr.append(QByteArray(26, '\0'));

        QByteArray pdtaContent;
        pdtaContent.append(makeChunk("phdr", phdr));
        pdtaContent.append(makeChunk("pbag", pbag));
        pdtaContent.append(makeChunk("pmod", pmod));
        pdtaContent.append(makeChunk("pgen", pgen));
        pdtaContent.append(makeChunk("inst", inst));
        pdtaContent.append(makeChunk("ibag", ibag));
        pdtaContent.append(makeChunk("imod", imod));
        pdtaContent.append(makeChunk("igen", igen));
        pdtaContent.append(makeChunk("shdr", shdr));

        const QByteArray infoList = makeList("INFO", infoContent);
        const QByteArray sdtaList = makeList("sdta", sdtaContent);
        const QByteArray pdtaList = makeList("pdta", pdtaContent);

        QByteArray riffContent;
        riffContent.append("sfbk", 4);
        riffContent.append(infoList);
        riffContent.append(sdtaList);
        riffContent.append(pdtaList);

        QByteArray file;
        file.append("RIFF", 4);
        appendU32(file, static_cast<quint32>(riffContent.size()));
        file.append(riffContent);

        QFile out(filePath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return QStringLiteral("Cannot open %1 for writing: %2").arg(filePath, out.errorString());
        if (out.write(file) != file.size())
            return QStringLiteral("Cannot write %1: %2").arg(filePath, out.errorString());
        out.close();
        return QString();
    }
} // namespace Sound
