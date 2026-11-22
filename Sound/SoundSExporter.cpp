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

        QString hexByte(int value)
        {
            return QStringLiteral("0x%1").arg(value, 2, 16, QLatin1Char('0')).toUpper();
        }

        QString commandComment(const unsigned char *rom, quint32 romSize,
                               quint32 pc, int length)
        {
            Q_UNUSED(length);
            if (!rom || pc >= romSize)
                return QString();
            const unsigned char command = rom[pc];

            if (command >= 0x80 && command <= 0xB0)
            {
                static const char *names[] = {
                    "W00", "W01", "W02", "W03", "W04", "W05", "W06", "W07",
                    "W08", "W09", "W10", "W11", "W12", "W13", "W14", "W15",
                    "W16", "W17", "W18", "W19", "W20", "W21", "W22", "W23",
                    "W24", "W28", "W30", "W32", "W36", "W40", "W42", "W44",
                    "W48", "W52", "W54", "W56", "W60", "W64", "W66", "W68",
                    "W72", "W76", "W78", "W80", "W84", "W88", "W90", "W92",
                    "W96"
                };
                return QString::fromLatin1(names[command - 0x80]);
            }
            if (command >= 0xD0)
            {
                static const char *names[] = {
                    "N01", "N02", "N03", "N04", "N05", "N06", "N07", "N08",
                    "N09", "N10", "N11", "N12", "N13", "N14", "N15", "N16",
                    "N17", "N18", "N19", "N20", "N21", "N22", "N23", "N24",
                    "N28", "N30", "N32", "N36", "N40", "N42", "N44", "N48",
                    "N52", "N54", "N56", "N60", "N64", "N66", "N68", "N72",
                    "N76", "N78", "N80", "N84", "N88", "N90", "N92", "N96"
                };
                const int index = command - 0xD0;
                if (index >= 0 && index < static_cast<int>(sizeof(names) / sizeof(names[0])))
                    return QString::fromLatin1(names[index]);
                return QStringLiteral("NOTE");
            }

            switch (command)
            {
            case 0xB1: return QStringLiteral("FINE");
            case 0xB2: return QStringLiteral("GOTO");
            case 0xB3: return QStringLiteral("PATT");
            case 0xB4: return QStringLiteral("PEND");
            case 0xB5: return QStringLiteral("REPT");
            case 0xB9: return QStringLiteral("MEMACC");
            case 0xBA: return QStringLiteral("PRIO");
            case 0xBB: return QStringLiteral("TEMPO");
            case 0xBC: return QStringLiteral("KEYSH");
            case 0xBD: return QStringLiteral("VOICE");
            case 0xBE: return QStringLiteral("VOL");
            case 0xBF: return QStringLiteral("PAN");
            case 0xC0: return QStringLiteral("BEND");
            case 0xC1: return QStringLiteral("BENDR");
            case 0xC2: return QStringLiteral("LFOS");
            case 0xC3: return QStringLiteral("LFODL");
            case 0xC4: return QStringLiteral("MOD");
            case 0xC5: return QStringLiteral("MODT");
            case 0xC8: return QStringLiteral("TUNE");
            case 0xCD: return QStringLiteral("XCMD");
            case 0xCE: return QStringLiteral("EOT");
            case 0xCF: return QStringLiteral("TIE");
            default: return QString();
            }
        }

        int commandLength(const unsigned char *rom, quint32 romSize, quint32 pc)
        {
            if (!rom || pc >= romSize)
                return 1;
            const unsigned char command = rom[pc];
            if (command == 0xB2 || command == 0xB3)
                return 5;
            if (command == 0xB5)
                return 6;
            if (command == 0xB9)
                return 4;
            if (command == 0xCD)
                return 3;
            if ((command >= 0xBA && command <= 0xC5) || command == 0xC8)
                return 2;
            if (command == 0xCE || command == 0xCF)
            {
                int length = 1;
                while (length < 3 && pc + length < romSize && rom[pc + length] < 0x80)
                    ++length;
                return length;
            }
            if (command >= 0xD0)
            {
                int length = 1;
                while (length < 4 && pc + length < romSize && rom[pc + length] < 0x80)
                    ++length;
                return length;
            }
            return 1;
        }

        QString rawByteComment(const unsigned char *rom, quint32 romSize,
                               quint32 pc, int length)
        {
            QString raw;
            for (int i = 0; i < length && pc + i < romSize; ++i)
            {
                if (!raw.isEmpty())
                    raw += QStringLiteral(", ");
                raw += hexByte(rom[pc + i]);
            }
            return raw;
        }

        QString memName(int value)
        {
            static const char *names[] = {
                "mem_set", "mem_add", "mem_sub", "mem_mem_set", "mem_mem_add",
                "mem_mem_sub", "mem_beq", "mem_bne", "mem_bhi", "mem_bhs",
                "mem_bls", "mem_blo", "mem_mem_beq", "mem_mem_bne", "mem_mem_bhi",
                "mem_mem_bhs", "mem_mem_bls", "mem_mem_blo"
            };
            if (value >= 0 && value < static_cast<int>(sizeof(names) / sizeof(names[0])))
                return QString::fromLatin1(names[value]);
            return QString::number(value);
        }

        QString modName(int value)
        {
            switch (value)
            {
            case 0: return QStringLiteral("mod_vib");
            case 1: return QStringLiteral("mod_tre");
            case 2: return QStringLiteral("mod_pan");
            default: return QString::number(value);
            }
        }

        QString instructionLine(const unsigned char *rom, quint32 romSize,
                                quint32 pc, int length)
        {
            const unsigned char command = rom[pc];
            const QString mnemonic = commandComment(rom, romSize, pc, length);
            if (mnemonic.isEmpty())
                return QString();

            QStringList operands;
            operands.append(mnemonic);
            for (int i = 1; i < length && pc + i < romSize; ++i)
                operands.append(QString::number(rom[pc + i]));

            if (mnemonic.startsWith(QLatin1Char('W')))
            {
                operands = { mnemonic };
            }
            else if (mnemonic.startsWith(QLatin1Char('N')))
            {
                // N01, key, velocity. Keep the key/velocity numeric so the
                // line does not depend on the note-name aliases.
                operands = { mnemonic };
                for (int i = 1; i < length && pc + i < romSize; ++i)
                    operands.append(hexByte(rom[pc + i]));
            }
            else if (command == 0xB9 && length >= 4)
            {
                operands = { mnemonic, memName(rom[pc + 1]),
                             hexByte(rom[pc + 2]), hexByte(rom[pc + 3]) };
            }
            else if (command == 0xCD && length >= 3)
            {
                QString sub;
                if (rom[pc + 1] == 0x08)
                    sub = QStringLiteral("xIECV");
                else if (rom[pc + 1] == 0x09)
                    sub = QStringLiteral("xIECL");
                else
                    sub = hexByte(rom[pc + 1]);
                operands = { mnemonic, sub, hexByte(rom[pc + 2]) };
            }
            else if (command == 0xC5 && length >= 2)
            {
                operands = { mnemonic, modName(rom[pc + 1]) };
            }
            else if ((command == 0xBF || command == 0xC0 || command == 0xC8) &&
                     length >= 2 && rom[pc + 1] == 0x40)
            {
                operands = { mnemonic, QStringLiteral("c_v") };
            }
            else if (length == 1)
            {
                operands = { mnemonic };
            }

            QString line = QStringLiteral(".byte ") + operands.join(QStringLiteral(", "));
            line += QStringLiteral("   @ raw: ") + rawByteComment(rom, romSize, pc, length);
            return line;
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
        out << ".include \"MPlayCompat.s\"\n\n";
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

                    if (command == 0xB5)
                    {
                        out << ".byte REPT, " << static_cast<int>(rom[pc + 1])
                            << "   @ raw: " << hexByte(0xB5) << ", " << hexByte(rom[pc + 1])
                            << "\n";
                    }
                    else
                    {
                        out << ".byte " << commandName
                            << "   @ raw: " << hexByte(command) << "\n";
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

                // Emit one readable command when the byte stream is known.
                // Unknown commands and raw argument runs keep the byte-level
                // fallback so no information is lost.
                const int instructionLength = commandLength(rom, romSize, pc);
                if (instructionLength > 0 && pc + instructionLength <= scan.end &&
                        !commandComment(rom, romSize, pc, instructionLength).isEmpty())
                {
                    out << instructionLine(rom, romSize, pc, instructionLength) << "\n";
                    pc += static_cast<quint32>(instructionLength);
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
                    << "   @ raw fallback (unknown command or omitted/running-status form); offset 0x"
                    << QStringLiteral("%1").arg(chunkStart, 5, 16, QLatin1Char('0')).toUpper()
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
