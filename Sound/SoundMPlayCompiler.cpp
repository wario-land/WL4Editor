#include "SoundMPlayCompiler.h"

#include "SoundSAssembler.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryDir>

namespace Sound
{
    namespace
    {
        QString compatDefinitions()
        {
            QFile file(QStringLiteral(":/Sound/MPlayCompat.s"));
            if (!file.open(QIODevice::ReadOnly))
                return QString();
            return QString::fromUtf8(file.readAll());
        }

        int declaredTrackCount(const QString &source)
        {
            const QRegularExpression expression(
                        QStringLiteral("(?m)^\\s*\\.byte\\s+(\\d+)\\s*@\\s*NumTrks\\b"));
            const QRegularExpressionMatch match = expression.match(source);
            if (!match.hasMatch())
                return -1;
            bool ok = false;
            const int count = match.captured(1).toInt(&ok);
            return ok ? count : -1;
        }
    } // namespace

    QString SoundMPlayCompiler::compileFile(const QString &sFile, quint32 dataAddress,
                                            QByteArray &binary, QString *normalizedSource)
    {
        if (!QFileInfo::exists(sFile))
            return QStringLiteral("MPlay source file does not exist: %1").arg(sFile);

        QFile input(sFile);
        if (!input.open(QIODevice::ReadOnly))
            return QStringLiteral("Cannot read %1: %2").arg(sFile, input.errorString());
        QByteArray bytes = input.readAll();
        input.close();
        if (bytes.startsWith("\xEF\xBB\xBF"))
            bytes.remove(0, 3);

        QString source = QString::fromUtf8(bytes);
        const QString compat = compatDefinitions();
        if (compat.isEmpty())
            return QStringLiteral("Cannot load the built-in MPlay compatibility constants.");

        const QRegularExpression includeExpression(
                    QStringLiteral("(?m)^\\s*\\.include\\s+\"(?:MPlayDef|MPlayCompat)\\.s\"\\s*$"));
        source.replace(includeExpression, compat);

        const int trackCount = declaredTrackCount(source);
        if (trackCount >= 0 && (trackCount < 1 || trackCount > 16))
            return QStringLiteral("MPlay source declares %1 tracks; WL4 supports 1..16.")
                    .arg(trackCount);

        if (normalizedSource)
            *normalizedSource = source;

        QTemporaryDir temp;
        if (!temp.isValid())
            return QStringLiteral("Cannot create a temporary directory for the MPlay compiler.");
        const QString normalizedPath = temp.filePath(QStringLiteral("song.s"));
        QFile normalized(normalizedPath);
        if (!normalized.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return QStringLiteral("Cannot write normalized MPlay source: %1")
                    .arg(normalized.errorString());
        normalized.write(source.toUtf8());
        normalized.close();

        QString error = SoundSAssembler::assemble(normalizedPath, dataAddress, binary);
        if (!error.isEmpty())
            return error;
        if (binary.isEmpty())
            return QStringLiteral("The MPlay compiler produced an empty binary.");
        if (binary.size() > 0x10000)
            return QStringLiteral("The compiled song is larger than the 64 KB chunk limit.");
        return QString();
    }
} // namespace Sound
