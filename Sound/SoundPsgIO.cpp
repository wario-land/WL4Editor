#include "SoundPsgIO.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>

#include <cstring>

namespace Sound
{
    namespace
    {
        QString rawToHex(const quint8 raw[12])
        {
            return QString::fromLatin1(
                        QByteArray(reinterpret_cast<const char *>(raw), 12).toHex());
        }

        bool rawFromHex(const QString &text, quint8 raw[12])
        {
            const QByteArray bytes = QByteArray::fromHex(text.toLatin1());
            if (bytes.size() != 12)
                return false;
            std::memcpy(raw, bytes.constData(), 12);
            return true;
        }

        void applyRaw(void *targetRaw, VoiceType &type, quint8 &key, quint8 &pan,
                      quint8 &attack, quint8 &decay, quint8 &sustain, quint8 &release,
                      const quint8 raw[12])
        {
            std::memcpy(targetRaw, raw, 12);
            type = static_cast<VoiceType>(raw[0]);
            key = raw[1];
            pan = raw[3];
            attack = raw[8];
            decay = raw[9];
            sustain = raw[10];
            release = raw[11];
        }
    } // namespace

    bool SoundPsgIO::isPsgType(VoiceType type)
    {
        switch (type)
        {
        case VoiceType::PsgSquare1:
        case VoiceType::PsgSquare2:
        case VoiceType::PsgWave:
        case VoiceType::PsgNoise:
        case VoiceType::PsgSquare1Alias:
        case VoiceType::PsgSquare2Alias:
        case VoiceType::PsgWaveAlias:
        case VoiceType::PsgNoiseAlias:
            return true;
        default:
            return false;
        }
    }

    QString SoundPsgIO::write(const ParseResult &data, const QString &path)
    {
        QJsonArray programs;
        for (const Bank &bank : data.banks)
        {
            for (const Program &program : bank.programs)
            {
                if (isPsgType(program.voice.type))
                {
                    QJsonObject object;
                    object.insert(QStringLiteral("kind"), QStringLiteral("program"));
                    object.insert(QStringLiteral("bank"), bank.id);
                    object.insert(QStringLiteral("program"), program.index);
                    object.insert(QStringLiteral("name"), program.name);
                    object.insert(QStringLiteral("raw"), rawToHex(program.voice.raw));
                    if (program.voice.waveAddress)
                    {
                        object.insert(QStringLiteral("wave"),
                                      QStringLiteral("0x%1").arg(
                                          program.voice.waveAddress, 8, 16, QLatin1Char('0')));
                    }
                    programs.append(object);
                }

                for (int subIndex = 0; subIndex < program.voice.subVoices.size(); ++subIndex)
                {
                    const SubVoice &sub = program.voice.subVoices[subIndex];
                    if (!isPsgType(sub.type))
                        continue;
                    QJsonObject object;
                    object.insert(QStringLiteral("kind"), QStringLiteral("sub"));
                    object.insert(QStringLiteral("bank"), bank.id);
                    object.insert(QStringLiteral("program"), program.index);
                    object.insert(QStringLiteral("sub"), subIndex);
                    object.insert(QStringLiteral("raw"), rawToHex(sub.raw));
                    programs.append(object);
                }
            }
        }

        QJsonObject root;
        root.insert(QStringLiteral("format"), QStringLiteral("WL4Editor.psg"));
        root.insert(QStringLiteral("version"), 1);
        root.insert(QStringLiteral("programs"), programs);

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            return QStringLiteral("Cannot write %1: %2").arg(path, file.errorString());
        }
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
        return QString();
    }

    QString SoundPsgIO::read(const QString &path, ParseResult &data,
                             QVector<int> *changedProgramKeys, QStringList *warnings)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return QString();

        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        file.close();
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            if (warnings)
            {
                warnings->append(QStringLiteral("psg.json is not valid JSON: %1")
                                 .arg(parseError.errorString()));
            }
            return QString();
        }
        if (document.object().value(QStringLiteral("format")).toString() !=
                QStringLiteral("WL4Editor.psg"))
        {
            if (warnings)
                warnings->append(QStringLiteral("psg.json has an unknown format."));
            return QString();
        }

        QSet<int> changed;
        const QJsonArray programs = document.object().value(QStringLiteral("programs")).toArray();
        for (const QJsonValue &value : programs)
        {
            if (!value.isObject())
                continue;
            const QJsonObject object = value.toObject();
            const int bankId = object.value(QStringLiteral("bank")).toInt(-1);
            const int programIndex = object.value(QStringLiteral("program")).toInt(-1);
            Bank *bank = nullptr;
            Program *program = nullptr;
            for (Bank &candidate : data.banks)
            {
                if (candidate.id != bankId)
                    continue;
                bank = &candidate;
                for (Program &candidateProgram : candidate.programs)
                {
                    if (candidateProgram.index == programIndex)
                    {
                        program = &candidateProgram;
                        break;
                    }
                }
                break;
            }
            if (!bank || !program)
            {
                if (warnings)
                {
                    warnings->append(QStringLiteral(
                                "psg.json references missing bank %1 program %2.")
                                     .arg(bankId).arg(programIndex));
                }
                continue;
            }

            quint8 raw[12];
            if (!rawFromHex(object.value(QStringLiteral("raw")).toString(), raw) ||
                    !isPsgType(static_cast<VoiceType>(raw[0])))
            {
                if (warnings)
                {
                    warnings->append(QStringLiteral(
                                "psg.json bank %1 program %2 has an invalid raw voice.")
                                     .arg(bankId).arg(programIndex));
                }
                continue;
            }

            const QString kind = object.value(QStringLiteral("kind")).toString();
            bool changedTarget = false;
            if (kind == QStringLiteral("sub"))
            {
                const int subIndex = object.value(QStringLiteral("sub")).toInt(-1);
                if (subIndex < 0 || subIndex >= program->voice.subVoices.size())
                {
                    if (warnings)
                    {
                        warnings->append(QStringLiteral(
                                    "psg.json bank %1 program %2 has invalid sub-voice %3.")
                                         .arg(bankId).arg(programIndex).arg(subIndex));
                    }
                    continue;
                }
                SubVoice &sub = program->voice.subVoices[subIndex];
                if (std::memcmp(sub.raw, raw, 12) != 0)
                {
                    applyRaw(sub.raw, sub.type, sub.key, sub.pan, sub.attack,
                             sub.decay, sub.sustain, sub.release, raw);
                    changedTarget = true;
                }
            }
            else
            {
                if (std::memcmp(program->voice.raw, raw, 12) != 0)
                {
                    applyRaw(program->voice.raw, program->voice.type, program->voice.key,
                             program->voice.pan, program->voice.attack,
                             program->voice.decay, program->voice.sustain,
                             program->voice.release, raw);
                    changedTarget = true;
                }
            }

            if (changedTarget)
            {
                program->changed = true;
                changed.insert((bankId << 16) | (programIndex & 0xFFFF));
            }
        }

        if (changedProgramKeys)
        {
            for (int key : changed)
                changedProgramKeys->append(key);
        }
        return QString();
    }
} // namespace Sound
