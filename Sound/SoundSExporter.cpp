#include "SoundSExporter.h"

#include <QFile>
#include <QMap>
#include <QTextStream>

#include <algorithm>

namespace Sound
{
    namespace
    {
        constexpr quint32 RomBase = 0x08000000;

        quint32 readU32(const unsigned char *rom, quint32 offset, quint32 romSize)
        {
            if (!rom || offset + 4 > romSize)
                return 0;
            return static_cast<quint32>(rom[offset]) |
                   (static_cast<quint32>(rom[offset + 1]) << 8) |
                   (static_cast<quint32>(rom[offset + 2]) << 16) |
                   (static_cast<quint32>(rom[offset + 3]) << 24);
        }

        quint32 pointerToOffset(quint32 pointer)
        {
            if (!pointer)
                return 0xFFFFFFFFu;
            return pointer >= RomBase ? pointer - RomBase : pointer;
        }

        struct PointerInfo
        {
            int length = 1;
            int pointerOffset = -1;
            quint32 target = 0xFFFFFFFFu;
        };

        struct TrackScan
        {
            quint32 start = 0;
            quint32 end = 0;
            QMap<quint32, PointerInfo> pointers; // key: absolute command offset
            bool foundFine = false;
            QString warning;
        };

        TrackScan scanTrack(const unsigned char *rom, quint32 romSize, quint32 start)
        {
            TrackScan scan;
            scan.start = start;
            if (!rom || start >= romSize)
            {
                scan.end = start;
                scan.warning = QStringLiteral("track start is outside the ROM");
                return scan;
            }

            const quint32 limit = std::min<quint32>(romSize, start + 0x20000u);
            quint32 pc = start;
            while (pc < limit)
            {
                const quint8 command = rom[pc];
                int length = 1;
                int pointerOffset = -1;

                if (command == 0xB1) // FINE
                {
                    ++pc;
                    scan.foundFine = true;
                    break;
                }
                else if (command == 0xB2 || command == 0xB3) // GOTO / PATT
                {
                    length = 5;
                    pointerOffset = 1;
                }
                else if (command == 0xB5) // REPT
                {
                    length = 6;
                    pointerOffset = 2;
                }
                else if (command == 0xB9) // MEMACC
                {
                    length = 4;
                }
                else if (command == 0xCD) // XCMD
                {
                    length = 3;
                }
                else if (command >= 0xBA && command <= 0xC5) // PRIO..BEND
                {
                    length = 2;
                }
                else if (command == 0xC8) // TUNE
                {
                    length = 2;
                }
                else if (command == 0xCE || command == 0xCF) // EOT / TIE
                {
                    length = 1;
                    while (length < 3 && pc + length < limit && rom[pc + length] < 0x80)
                        ++length;
                }
                else if (command >= 0xD0) // NOTE with timeout
                {
                    length = 1;
                    while (length < 4 && pc + length < limit && rom[pc + length] < 0x80)
                        ++length;
                }
                else
                {
                    // WAIT, running-status arguments, and unknown commands.
                    length = 1;
                }

                if (pointerOffset >= 0)
                {
                    PointerInfo info;
                    info.length = length;
                    info.pointerOffset = pointerOffset;
                    info.target = pointerToOffset(readU32(rom, pc + pointerOffset, romSize));
                    scan.pointers.insert(pc, info);
                }

                pc += static_cast<quint32>(length);
            }

            scan.end = pc;
            if (!scan.foundFine)
                scan.warning = QStringLiteral("FINE was not found before the scan limit");
            return scan;
        }

        QString trackLabel(int songId, int trackIndex)
        {
            return QStringLiteral("Song%1_Track%2")
                    .arg(songId, 4, 10, QLatin1Char('0'))
                    .arg(trackIndex, 2, 10, QLatin1Char('0'));
        }

        QString pointerLabel(const QString &track, quint32 target, quint32 trackStart)
        {
            if (target == trackStart)
                return track;
            return track + QStringLiteral("_L_%1")
                    .arg(target - trackStart, 4, 16, QLatin1Char('0')).toUpper();
        }
    } // namespace

    QString SoundSExporter::exportSong(const ParseResult &data, const Song &song,
                                       const unsigned char *rom, quint32 romSize,
                                       const QString &filePath, QStringList *warnings)
    {
        Q_UNUSED(data);

        if (song.trackAddresses.isEmpty())
            return QStringLiteral("Song %1 has no tracks.").arg(song.id);

        QVector<TrackScan> scans;
        scans.reserve(song.trackAddresses.size());
        for (quint32 start : song.trackAddresses)
        {
            TrackScan scan = scanTrack(rom, romSize, start);
            if (!scan.warning.isEmpty())
            {
                if (warnings)
                    warnings->append(QStringLiteral("Song %1: %2").arg(song.id).arg(scan.warning));
            }
            scans.append(scan);
        }

        // Global label map: track starts and pointer targets that fall inside
        // one of this song's tracks.
        QMap<quint32, QString> labels;
        for (int i = 0; i < scans.size(); ++i)
        {
            const TrackScan &scan = scans[i];
            const QString track = trackLabel(song.id, i);
            labels.insert(scan.start, track);
            for (auto it = scan.pointers.constBegin(); it != scan.pointers.constEnd(); ++it)
            {
                const quint32 target = it.value().target;
                for (int j = 0; j < scans.size(); ++j)
                {
                    if (target >= scans[j].start && target < scans[j].end)
                    {
                        labels.insert(target, pointerLabel(trackLabel(song.id, j), target, scans[j].start));
                        break;
                    }
                }
            }
        }

        QString output;
        QTextStream out(&output);

        out << "@ WL4Editor MPlay source\n";
        out << "@ wl4-song: " << song.id << "\n";
        out << "@ wl4-name: " << song.name << "\n";
        out << "@ wl4-bank: " << song.bankId << "\n";
        out << "@ wl4-player: " << static_cast<int>(song.ms) << "\n";
        out << "@ wl4-ms: " << static_cast<int>(song.ms) << "\n";
        out << "@ wl4-me: " << static_cast<int>(song.me) << "\n";
        out << "@ wl4-priority: " << static_cast<int>(song.priority) << "\n";
        out << "@ wl4-echo: " << static_cast<int>(song.echo) << "\n";
        out << "@ The bank pointer below is a placeholder. WL4Editor fills it\n";
        out << "@ from the model when the .s file is imported.\n\n";

        out << ".text\n";
        out << ".align 2\n\n";
        out << ".global Song" << QStringLiteral("%1").arg(song.id, 4, 10, QLatin1Char('0')) << "\n";
        out << "Song" << QStringLiteral("%1").arg(song.id, 4, 10, QLatin1Char('0')) << ":\n";

        const quint8 unknown = (song.romAddress != 0xFFFFFFFFu && song.romAddress + 1 < romSize)
                ? rom[song.romAddress + 1] : 0;
        out << ".byte " << song.trackCount << ", " << static_cast<int>(unknown)
            << ", " << static_cast<int>(song.priority) << ", " << static_cast<int>(song.echo) << "\n";
        out << ".byte 0, 0, 0, 0   @ Bank pointer placeholder\n";

        for (int i = 0; i < scans.size(); ++i)
            out << ".word " << trackLabel(song.id, i) << "\n";
        out << "\n";

        for (int i = 0; i < scans.size(); ++i)
        {
            const TrackScan &scan = scans[i];
            const QString track = trackLabel(song.id, i);
            out << track << ":\n";

            quint32 pc = scan.start;
            while (pc < scan.end)
            {
                if (pc != scan.start && labels.contains(pc))
                    out << labels.value(pc) << ":\n";

                const auto pointerIt = scan.pointers.constFind(pc);
                if (pointerIt != scan.pointers.constEnd())
                {
                    const PointerInfo &info = pointerIt.value();
                    const quint8 command = rom[pc];
                    QString commandName = QStringLiteral("POINTER");
                    if (command == 0xB2)
                        commandName = QStringLiteral("GOTO");
                    else if (command == 0xB3)
                        commandName = QStringLiteral("PATT");
                    else if (command == 0xB5)
                        commandName = QStringLiteral("REPT");

                    out << ".byte 0x" << QStringLiteral("%1").arg(command, 2, 16, QLatin1Char('0')).toUpper()
                        << "   @ " << commandName << "\n";
                    if (command == 0xB5)
                    {
                        out << ".byte 0x"
                            << QStringLiteral("%1").arg(rom[pc + 1], 2, 16, QLatin1Char('0')).toUpper()
                            << "   @ REPT count\n";
                    }

                    if (labels.contains(info.target))
                    {
                        out << ".word " << labels.value(info.target) << "\n";
                    }
                    else
                    {
                        out << ".word 0x"
                            << QStringLiteral("%1").arg(info.target, 8, 16, QLatin1Char('0')).toUpper()
                            << "   @ external pointer (not relocated)\n";
                        if (warnings)
                        {
                            warnings->append(QStringLiteral("Song %1 track %2: external %3 target 0x%4.")
                                             .arg(song.id).arg(i).arg(commandName)
                                             .arg(info.target, 8, 16, QLatin1Char('0')));
                        }
                    }
                    pc += static_cast<quint32>(info.length);
                    continue;
                }

                // Emit a run of ordinary bytes, stopping at a label or a
                // pointer command.
                const quint32 chunkStart = pc;
                QStringList bytes;
                while (pc < scan.end && bytes.size() < 16)
                {
                    // The label at chunkStart was already emitted by the outer
                    // loop. Only a later label or pointer ends this byte run.
                    if (pc > chunkStart && labels.contains(pc))
                        break;
                    if (pc > chunkStart && scan.pointers.contains(pc))
                        break;
                    bytes.append(QStringLiteral("0x%1")
                                 .arg(rom[pc], 2, 16, QLatin1Char('0')).toUpper());
                    ++pc;
                }
                out << ".byte " << bytes.join(QStringLiteral(", "))
                    << "   @ 0x" << QStringLiteral("%1").arg(chunkStart, 5, 16, QLatin1Char('0')).toUpper()
                    << "\n";
            }
            out << "\n";
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return QStringLiteral("Cannot open %1 for writing: %2").arg(filePath, file.errorString());
        const QByteArray bytes = output.toUtf8();
        if (file.write(bytes) != bytes.size())
            return QStringLiteral("Cannot write %1: %2").arg(filePath, file.errorString());
        file.close();
        return QString();
    }
} // namespace Sound
