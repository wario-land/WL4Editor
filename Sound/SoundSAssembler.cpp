#include "SoundSAssembler.h"

#include "PatchUtils.h"
#include "SettingsUtils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>

namespace Sound
{
    namespace
    {
        QString toolName(const QString &base)
        {
#ifdef Q_OS_WIN
            return base + QStringLiteral(".exe");
#else
            return base;
#endif
        }

        QString runProcess(const QString &executable, const QStringList &arguments)
        {
            QProcess process;
            process.start(executable, arguments);
            if (!process.waitForStarted())
                return QStringLiteral("Cannot start %1").arg(executable);
            process.waitForFinished(-1);
            if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
            {
                const QString stderrText = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
                return QStringLiteral("%1 failed: %2")
                        .arg(QFileInfo(executable).fileName(), stderrText);
            }
            return QString();
        }
    } // namespace

    QString SoundSAssembler::ensureEabi()
    {
        if (PatchUtils::EABI_INSTALLATION.isEmpty())
        {
            const QString configured = SettingsUtils::GetKey(
                        SettingsUtils::IniKeys::eabi_binfile_path);
            if (!configured.isEmpty() && QDir(configured).exists())
                PatchUtils::EABI_INSTALLATION = configured;
        }

        QString missing;
        if (PatchUtils::EABI_INSTALLATION.isEmpty() || !PatchUtils::VerifyEABI(&missing))
        {
            return QStringLiteral("The arm-none-eabi toolchain is not configured. "
                                  "Set the EABI bin directory in the Patch Manager first.");
        }
        return QString();
    }

    QString SoundSAssembler::assemble(const QString &sFile, quint32 dataAddress, QByteArray &binary)
    {
        if (!QFileInfo::exists(sFile))
            return QStringLiteral("Song source file does not exist: %1").arg(sFile);

        const QString eabiError = ensureEabi();
        if (!eabiError.isEmpty())
            return eabiError;

        QTemporaryDir temp;
        if (!temp.isValid())
            return QStringLiteral("Cannot create a temporary directory for the assembler.");

        const QString objectFile = temp.filePath(QStringLiteral("song.o"));
        const QString linkerScript = temp.filePath(QStringLiteral("song.ld"));
        const QString elfFile = temp.filePath(QStringLiteral("song.elf"));
        const QString binaryFile = temp.filePath(QStringLiteral("song.bin"));
        const QString cleanSource = temp.filePath(QStringLiteral("song.s"));

        // Editors on Windows often add a UTF-8 BOM; arm-none-eabi-as treats it
        // as an instruction. Strip it before assembling.
        QFile source(sFile);
        if (!source.open(QIODevice::ReadOnly))
            return QStringLiteral("Cannot read %1: %2").arg(sFile, source.errorString());
        QByteArray sourceData = source.readAll();
        source.close();
        if (sourceData.startsWith("\xEF\xBB\xBF"))
            sourceData.remove(0, 3);

        QFile clean(cleanSource);
        if (!clean.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return QStringLiteral("Cannot write %1: %2").arg(cleanSource, clean.errorString());
        clean.write(sourceData);
        clean.close();

        const QString as = PatchUtils::EABI_INSTALLATION + QLatin1Char('/') +
                toolName(QStringLiteral("arm-none-eabi-as"));
        const QString ld = PatchUtils::EABI_INSTALLATION + QLatin1Char('/') +
                toolName(QStringLiteral("arm-none-eabi-ld"));
        const QString objcopy = PatchUtils::EABI_INSTALLATION + QLatin1Char('/') +
                toolName(QStringLiteral("arm-none-eabi-objcopy"));

        QString error = runProcess(as, { QStringLiteral("-mthumb"), cleanSource,
                                        QStringLiteral("-o"), objectFile });
        if (!error.isEmpty())
            return error;

        QString script;
        {
            // Assembly labels and .word track pointers must be GBA pointers,
            // not file offsets. The chunk data is copied back at dataAddress,
            // but the values inside the song header still need the 0x08000000
            // address space.
            constexpr quint32 RomBase = 0x08000000;
            QTextStream out(&script);
            out << "SECTIONS\n{\n";
            out << "    .text 0x" << QString::number(dataAddress + RomBase, 16)
                << " : { *(.text) }\n";
            out << "    .rodata : { *(.rodata) }\n";
            out << "    .data : { *(.data) }\n";
            out << "}\n";
        }

        QFile ldFile(linkerScript);
        if (!ldFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return QStringLiteral("Cannot write linker script: %1").arg(ldFile.errorString());
        ldFile.write(script.toUtf8());
        ldFile.close();

        error = runProcess(ld, { QStringLiteral("-T"), linkerScript,
                                 QStringLiteral("-o"), elfFile, objectFile });
        if (!error.isEmpty())
            return error;

        error = runProcess(objcopy, { QStringLiteral("-O"), QStringLiteral("binary"),
                                      QStringLiteral("-j"), QStringLiteral(".text"),
                                      elfFile, binaryFile });
        if (!error.isEmpty())
            return error;

        QFile binFile(binaryFile);
        if (!binFile.open(QIODevice::ReadOnly))
            return QStringLiteral("Cannot read assembled song: %1").arg(binFile.errorString());
        binary = binFile.readAll();
        binFile.close();

        if (binary.isEmpty())
            return QStringLiteral("The assembler produced an empty song binary.");
        return QString();
    }
} // namespace Sound
