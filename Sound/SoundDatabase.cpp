#include "SoundDatabase.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace Sound
{
    namespace
    {
        SongCategory categoryFromString(const QString &text)
        {
            const QString value = text.trimmed().toLower();
            if (value == QStringLiteral("bgm") || value == QStringLiteral("music"))
                return SongCategory::Bgm;
            if (value == QStringLiteral("sfx") || value == QStringLiteral("soundeffect"))
                return SongCategory::Sfx;
            if (value == QStringLiteral("voice"))
                return SongCategory::Voice;
            if (value == QStringLiteral("fanfare"))
                return SongCategory::Fanfare;
            if (value == QStringLiteral("ending"))
                return SongCategory::Ending;
            return SongCategory::Other;
        }
    } // namespace

    SoundDatabase &SoundDatabase::instance()
    {
        static SoundDatabase database;
        return database;
    }

    bool SoundDatabase::load(QString *error)
    {
        if (loaded)
            return true;

        QByteArray data;
        QFile resource(QStringLiteral(":/SoundData/WL4SoundDatabase.json"));
        if (resource.exists() && resource.open(QIODevice::ReadOnly))
        {
            data = resource.readAll();
        }
        else
        {
            const QString externalPath = QCoreApplication::applicationDirPath() +
                    QStringLiteral("/SoundData/WL4SoundDatabase.json");
            QFile external(externalPath);
            if (external.exists() && external.open(QIODevice::ReadOnly))
                data = external.readAll();
        }

        if (data.isEmpty())
        {
            if (error)
                *error = QStringLiteral("Sound database not found (resource or SoundData/WL4SoundDatabase.json).");
            return false;
        }

        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            if (error)
                *error = QStringLiteral("Sound database JSON error: %1").arg(parseError.errorString());
            return false;
        }

        const QJsonObject root = document.object();

        const QJsonObject songObject = root.value(QStringLiteral("songs")).toObject();
        for (auto it = songObject.begin(); it != songObject.end(); ++it)
        {
            bool ok = false;
            const int id = it.key().toInt(&ok);
            if (!ok)
                continue;
            const QJsonObject entry = it.value().toObject();
            SongInfo info;
            info.name = entry.value(QStringLiteral("name")).toString();
            info.description = entry.value(QStringLiteral("description")).toString();
            info.category = categoryFromString(entry.value(QStringLiteral("category")).toString());
            if (info.name.isEmpty())
                info.name = QStringLiteral("Song%1").arg(id, 4, 10, QLatin1Char('0'));
            songs.insert(id, info);
        }

        const QJsonObject bankObject = root.value(QStringLiteral("banks")).toObject();
        for (auto it = bankObject.begin(); it != bankObject.end(); ++it)
        {
            bool ok = false;
            const int bankId = it.key().toInt(&ok);
            if (!ok)
                continue;
            const QJsonObject entry = it.value().toObject();
            const QString name = entry.value(QStringLiteral("name")).toString();
            if (!name.isEmpty())
                bankNames.insert(bankId, name);

            const QJsonObject programs = entry.value(QStringLiteral("programs")).toObject();
            QHash<int, QString> names;
            for (auto programIt = programs.begin(); programIt != programs.end(); ++programIt)
            {
                bool programOk = false;
                const int program = programIt.key().toInt(&programOk);
                if (!programOk)
                    continue;
                const QString programName = programIt.value().toString();
                if (!programName.isEmpty())
                    names.insert(program, programName);
            }
            if (!names.isEmpty())
                programNames.insert(bankId, names);
        }

        const QJsonObject sampleObject = root.value(QStringLiteral("samples")).toObject();
        for (auto it = sampleObject.begin(); it != sampleObject.end(); ++it)
        {
            bool ok = false;
            const quint32 address = it.key().toUInt(&ok, 0);
            if (!ok)
                continue;
            const QString name = it.value().toString();
            if (!name.isEmpty())
                sampleNames.insert(address, name);
        }

        loaded = true;
        return true;
    }

    QString SoundDatabase::songName(int id, const QString &fallback) const
    {
        const auto it = songs.constFind(id);
        if (it != songs.constEnd() && !it->name.isEmpty())
            return it->name;
        return fallback;
    }

    QString SoundDatabase::songDescription(int id) const
    {
        const auto it = songs.constFind(id);
        return it != songs.constEnd() ? it->description : QString();
    }

    SongCategory SoundDatabase::songCategory(int id) const
    {
        const auto it = songs.constFind(id);
        return it != songs.constEnd() ? it->category : SongCategory::Other;
    }

    QString SoundDatabase::bankName(int id, const QString &fallback) const
    {
        const auto it = bankNames.constFind(id);
        return it != bankNames.constEnd() ? it.value() : fallback;
    }

    QString SoundDatabase::programName(int bankId, int program, const QString &fallback) const
    {
        const auto bankIt = programNames.constFind(bankId);
        if (bankIt != programNames.constEnd())
        {
            const auto programIt = bankIt->constFind(program);
            if (programIt != bankIt->constEnd() && !programIt->isEmpty())
                return programIt.value();
        }
        return fallback;
    }

    QString SoundDatabase::sampleName(quint32 romAddress, const QString &fallback) const
    {
        const auto it = sampleNames.constFind(romAddress);
        return it != sampleNames.constEnd() ? it.value() : fallback;
    }
} // namespace Sound
