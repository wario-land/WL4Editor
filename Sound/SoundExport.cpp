#include "SoundExport.h"

#include "SoundFontWriter.h"
#include "SoundMidiExporter.h"
#include "SoundPsgIO.h"
#include "SoundSExporter.h"
#include "SoundWavIO.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace Sound
{
    namespace
    {
        QString hexValue(quint32 value)
        {
            return QStringLiteral("0x") + QString::number(value, 16).toUpper();
        }

        QString sanitizeFileName(const QString &name)
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
                result = QStringLiteral("song");
            return result.left(48);
        }

        QString roleName(BankRole role)
        {
            switch (role)
            {
            case BankRole::Music: return QStringLiteral("music");
            case BankRole::SubBank: return QStringLiteral("sub-bank");
            case BankRole::Custom: return QStringLiteral("custom");
            }
            return QStringLiteral("music");
        }

        QString stateName(SongState state)
        {
            switch (state)
            {
            case SongState::Empty: return QStringLiteral("empty");
            case SongState::Vanilla: return QStringLiteral("vanilla");
            case SongState::Custom: return QStringLiteral("custom");
            }
            return QStringLiteral("empty");
        }

        bool voiceHasSf2Sample(const Voice &voice, const ParseResult &data)
        {
            auto sampleOk = [&data](int sampleId)
            {
                return sampleId >= 0 && sampleId < data.samples.size() &&
                       !data.samples[sampleId].pcm.isEmpty();
            };

            switch (voice.type)
            {
            case VoiceType::Sample:
            case VoiceType::SampleFixed:
                return sampleOk(voice.sampleId);
            case VoiceType::KeySplit:
                for (const SubVoice &sub : voice.subVoices)
                {
                    if (sampleOk(sub.sampleId))
                        return true;
                }
                return false;
            case VoiceType::DrumKit:
                for (const SubVoice &sub : voice.subVoices)
                {
                    if (sampleOk(sub.sampleId))
                        return true;
                }
                return false;
            default:
                return false;
            }
        }
    } // namespace

    QString SoundExport::defaultFolder(const QString &romPath)
    {
        if (romPath.isEmpty())
            return QDir::current().filePath(QStringLiteral("WL4_sound"));
        const QFileInfo info(romPath);
        return info.dir().filePath(info.completeBaseName() + QStringLiteral("_sound"));
    }

    QString SoundExport::exportAll(const ParseResult &data, const unsigned char *rom,
                                   quint32 romSize, const QString &folder,
                                   const std::function<void(int, int)> &progress)
    {
        if (!rom || romSize < 0x800000)
            return QStringLiteral("Invalid ROM data.");

        QDir folderDir(folder);
        if (!folderDir.exists() && !QDir().mkpath(folder))
            return QStringLiteral("Cannot create export folder: %1").arg(folder);
        if (!QDir().mkpath(folder + QStringLiteral("/songs")))
            return QStringLiteral("Cannot create songs folder in %1").arg(folder);
        if (!QDir().mkpath(folder + QStringLiteral("/soundfont")))
            return QStringLiteral("Cannot create soundfont folder in %1").arg(folder);
        if (!QDir().mkpath(folder + QStringLiteral("/samples")))
            return QStringLiteral("Cannot create samples folder in %1").arg(folder);

        int songCount = 0;
        for (const Song &song : data.songs)
        {
            if (song.state != SongState::Empty)
                ++songCount;
        }
        int sampleCount = 0;
        for (const Sample &sample : data.samples)
        {
            if (!sample.pcm.isEmpty())
                ++sampleCount;
        }
        const int total = songCount + sampleCount + 3;
        int current = 0;
        if (progress)
            progress(0, total);

        QStringList warnings;
        QJsonArray songsArray;

        for (const Song &song : data.songs)
        {
            if (song.state == SongState::Empty)
                continue;

            const QString fileName = QStringLiteral("%1_%2.s")
                    .arg(song.id, 4, 10, QLatin1Char('0'))
                    .arg(sanitizeFileName(song.name));
            const QString filePath = folder + QStringLiteral("/songs/") + fileName;
            const QString error = SoundSExporter::exportSong(data, song, rom, romSize,
                                                             filePath, &warnings);
            if (!error.isEmpty())
                return error;

            QFile sourceFile(filePath);
            QByteArray sourceHash;
            if (sourceFile.open(QIODevice::ReadOnly))
            {
                sourceHash = QCryptographicHash::hash(sourceFile.readAll(),
                                                      QCryptographicHash::Sha1).toHex();
                sourceFile.close();
            }

            const QString midiFileName = QStringLiteral("%1_%2.mid")
                    .arg(song.id, 4, 10, QLatin1Char('0'))
                    .arg(sanitizeFileName(song.name));
            const QString midiPath = folder + QStringLiteral("/songs/") + midiFileName;
            const QString midiError = SoundMidiExporter::exportSong(data, song, rom, romSize,
                                                                   midiPath, &warnings);
            if (!midiError.isEmpty())
                return midiError;

            QJsonObject object;
            object.insert(QStringLiteral("id"), song.id);
            object.insert(QStringLiteral("name"), song.name);
            object.insert(QStringLiteral("symbol"), song.symbol);
            object.insert(QStringLiteral("category"), SongCategoryName(song.category));
            object.insert(QStringLiteral("state"), stateName(song.state));
            object.insert(QStringLiteral("romAddress"), hexValue(song.romAddress));
            object.insert(QStringLiteral("trackCount"), song.trackCount);
            object.insert(QStringLiteral("ms"), song.ms);
            object.insert(QStringLiteral("me"), song.me);
            object.insert(QStringLiteral("priority"), song.priority);
            object.insert(QStringLiteral("echo"), song.echo);
            object.insert(QStringLiteral("bankId"), song.bankId);
            object.insert(QStringLiteral("sFile"), QStringLiteral("songs/") + fileName);
            object.insert(QStringLiteral("sHash"), QString::fromLatin1(sourceHash));
            object.insert(QStringLiteral("mFile"), QStringLiteral("songs/") + midiFileName);
            QJsonArray tracks;
            for (quint32 address : song.trackAddresses)
                tracks.append(hexValue(address));
            object.insert(QStringLiteral("trackAddresses"), tracks);
            songsArray.append(object);

            ++current;
            if (progress)
                progress(current, total);
        }

        QHash<int, QString> sampleWavFiles;
        QHash<int, QString> sampleWavHashes;
        for (const Sample &sample : data.samples)
        {
            if (sample.pcm.isEmpty())
                continue;

            const QString fileName = QStringLiteral("samples/%1_%2.wav")
                    .arg(sample.id, 4, 10, QLatin1Char('0'))
                    .arg(sanitizeFileName(sample.name));
            const QString filePath = folder + QLatin1Char('/') + fileName;
            const QString wavError = SoundWavIO::write(sample, filePath);
            if (!wavError.isEmpty())
                return wavError;

            QFile wavFile(filePath);
            if (wavFile.open(QIODevice::ReadOnly))
            {
                sampleWavHashes.insert(sample.id,
                                       QString::fromLatin1(QCryptographicHash::hash(
                                                               wavFile.readAll(),
                                                               QCryptographicHash::Sha1).toHex()));
                wavFile.close();
            }
            sampleWavFiles.insert(sample.id, fileName);

            ++current;
            if (progress)
                progress(current, total);
        }

        const QString psgPath = folder + QStringLiteral("/psg.json");
        const QString psgError = SoundPsgIO::write(data, psgPath);
        if (!psgError.isEmpty())
            return psgError;
        ++current;
        if (progress)
            progress(current, total);

        const QString sf2Path = folder + QStringLiteral("/soundfont/WL4.sf2");
        const QString sf2Error = SoundFontWriter::write(data, sf2Path, &warnings);
        if (!sf2Error.isEmpty())
            return sf2Error;
        ++current;
        if (progress)
            progress(current, total);

        QJsonObject romObject;
        romObject.insert(QStringLiteral("gameCode"),
                         QString::fromLatin1(reinterpret_cast<const char *>(rom + 0xAC), 4));
        romObject.insert(QStringLiteral("sha1"),
                         QString::fromLatin1(QCryptographicHash::hash(
                                                 QByteArray(reinterpret_cast<const char *>(rom),
                                                            static_cast<int>(romSize)),
                                                 QCryptographicHash::Sha1).toHex()));
        romObject.insert(QStringLiteral("size"), static_cast<int>(romSize));

        QJsonObject layout;
        layout.insert(QStringLiteral("songTable"), hexValue(data.layout.songTable));
        layout.insert(QStringLiteral("playerTable"), hexValue(data.layout.playerTable));
        layout.insert(QStringLiteral("voicePoolStart"), hexValue(data.layout.voicePoolStart));
        layout.insert(QStringLiteral("sampleRegionStart"), hexValue(data.layout.sampleRegionStart));
        layout.insert(QStringLiteral("songRegionStart"), hexValue(data.layout.songRegionStart));
        layout.insert(QStringLiteral("songCount"), data.layout.songCount);
        layout.insert(QStringLiteral("playerCount"), data.layout.playerCount);
        romObject.insert(QStringLiteral("layout"), layout);

        QJsonArray banksArray;
        for (const Bank &bank : data.banks)
        {
            QJsonObject bankObject;
            bankObject.insert(QStringLiteral("id"), bank.id);
            bankObject.insert(QStringLiteral("name"), bank.name);
            bankObject.insert(QStringLiteral("role"), roleName(bank.role));
            bankObject.insert(QStringLiteral("base"), hexValue(bank.base));
            bankObject.insert(QStringLiteral("isCustom"), bank.isCustom);

            QJsonArray programsArray;
            for (const Program &program : bank.programs)
            {
                if (!program.used)
                    continue;
                QJsonObject programObject;
                programObject.insert(QStringLiteral("index"), program.index);
                programObject.insert(QStringLiteral("name"), program.name);
                programObject.insert(QStringLiteral("type"),
                                     VoiceTypeName(program.voice.type));
                if (program.voice.sampleId >= 0 &&
                        program.voice.sampleId < data.samples.size())
                {
                    programObject.insert(QStringLiteral("sample"),
                                         data.samples[program.voice.sampleId].name);
                }
                programObject.insert(QStringLiteral("exportedToSf2"),
                                     voiceHasSf2Sample(program.voice, data));
                programsArray.append(programObject);
            }
            bankObject.insert(QStringLiteral("programs"), programsArray);
            banksArray.append(bankObject);
        }

        QJsonArray samplesArray;
        for (const Sample &sample : data.samples)
        {
            if (sample.id < 0 || sample.pcm.isEmpty())
                continue;
            QJsonObject object;
            object.insert(QStringLiteral("id"), sample.id);
            object.insert(QStringLiteral("name"), sample.name);
            object.insert(QStringLiteral("romAddress"), hexValue(sample.romAddress));
            object.insert(QStringLiteral("pitch"), hexValue(sample.pitch));
            object.insert(QStringLiteral("sampleRate"), sample.sampleRate());
            object.insert(QStringLiteral("looped"), sample.isLooped());
            object.insert(QStringLiteral("loopStart"), static_cast<int>(sample.loopStart));
            object.insert(QStringLiteral("size"), static_cast<int>(sample.size));
            object.insert(QStringLiteral("sha1"), QString::fromLatin1(sample.sha1));
            object.insert(QStringLiteral("wavFile"), sampleWavFiles.value(sample.id));
            object.insert(QStringLiteral("wavHash"), sampleWavHashes.value(sample.id));
            samplesArray.append(object);
        }

        QJsonArray warningsArray;
        for (const QString &warning : warnings)
            warningsArray.append(warning);

        QJsonObject root;
        root.insert(QStringLiteral("format"), QStringLiteral("WL4Editor.soundProject"));
        root.insert(QStringLiteral("version"), 1);
        root.insert(QStringLiteral("generated"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        root.insert(QStringLiteral("rom"), romObject);
        root.insert(QStringLiteral("songs"), songsArray);
        root.insert(QStringLiteral("banks"), banksArray);
        root.insert(QStringLiteral("samples"), samplesArray);
        root.insert(QStringLiteral("warnings"), warningsArray);
        QJsonObject soundfont;
        soundfont.insert(QStringLiteral("file"), QStringLiteral("soundfont/WL4.sf2"));
        QFile sf2File(sf2Path);
        if (sf2File.open(QIODevice::ReadOnly))
        {
            soundfont.insert(QStringLiteral("hash"),
                             QString::fromLatin1(QCryptographicHash::hash(
                                                     sf2File.readAll(),
                                                     QCryptographicHash::Sha1).toHex()));
            sf2File.close();
        }
        root.insert(QStringLiteral("soundfont"), soundfont);

        QJsonObject psgObject;
        psgObject.insert(QStringLiteral("file"), QStringLiteral("psg.json"));
        QFile psgFile(psgPath);
        if (psgFile.open(QIODevice::ReadOnly))
        {
            psgObject.insert(QStringLiteral("hash"),
                             QString::fromLatin1(QCryptographicHash::hash(
                                                     psgFile.readAll(),
                                                     QCryptographicHash::Sha1).toHex()));
            psgFile.close();
        }
        root.insert(QStringLiteral("psg"), psgObject);

        QFile manifest(folder + QStringLiteral("/soundmanifest.json"));
        if (!manifest.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return QStringLiteral("Cannot write soundmanifest.json: %1").arg(manifest.errorString());
        manifest.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        manifest.close();

        QFile readme(folder + QStringLiteral("/README.txt"));
        if (readme.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            const QByteArray text =
                    "WL4Editor Sound Project\r\n"
                    "======================\r\n\r\n"
                    "songs/*.s        Authoritative MPlay source. Edit loops/patterns/MEMACC here.\r\n"
                    "songs/*.mid      Lossy MuseScore convenience export; .s is authoritative.\r\n"
                    "soundfont/*.sf2  SoundFont 2 bank for Polyphone. PSG Programs are omitted.\r\n"
                    "samples/*.wav    Editable sample PCM; import replaces existing sample slots.\r\n"
                    "psg.json         Raw PSG Program/sub-voice records (SF2 cannot represent PSG).\r\n"
                    "soundmanifest.json  IDs, names, ROM addresses, hashes and SF2 mapping.\r\n\r\n"
                    "Notes:\r\n"
                    "- All addresses are ROM file offsets (the GBA 0x08000000 base is not shown).\r\n"
                    "- SF2 does not contain GBA PSG Programs, and Phase 1 does not approximate ADSR.\r\n"
                    "- MIDI is a lossy convenience export; .s is the authoritative song format.\r\n";
            readme.write(text);
            readme.close();
        }

        if (progress)
            progress(total, total);
        return QString();
    }
} // namespace Sound
