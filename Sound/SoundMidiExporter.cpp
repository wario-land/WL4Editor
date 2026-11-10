#include "SoundMidiExporter.h"

#include <QFile>
#include <QHash>
#include <QVector>

#include <algorithm>

namespace Sound
{
    namespace
    {
        // W00..W24 then W28..W96, exactly the MPlay wait table.
        const int WaitTicks[49] =
        {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
            18, 19, 20, 21, 22, 23, 24,
            28, 30, 36, 40, 42, 44, 48, 52, 54, 56, 60, 64, 66, 68, 72, 76,
            78, 80, 84, 88, 90, 92, 96
        };

        struct Event
        {
            int tick = 0;
            int order = 0;
            QByteArray data;
        };

        void writeVarLen(QByteArray &data, quint32 value)
        {
            quint32 buffer = value & 0x7F;
            while ((value >>= 7))
            {
                buffer <<= 8;
                buffer |= (value & 0x7F) | 0x80;
            }
            while (true)
            {
                data.append(static_cast<char>(buffer & 0xFF));
                if (buffer & 0x80)
                    buffer >>= 8;
                else
                    break;
            }
        }

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

        QByteArray noteOn(int channel, int key, int velocity)
        {
            QByteArray data;
            data.append(static_cast<char>(0x90 | (channel & 0x0F)));
            data.append(static_cast<char>(key & 0x7F));
            data.append(static_cast<char>(qBound(1, velocity, 127)));
            return data;
        }

        QByteArray noteOff(int channel, int key)
        {
            QByteArray data;
            data.append(static_cast<char>(0x80 | (channel & 0x0F)));
            data.append(static_cast<char>(key & 0x7F));
            data.append('\0');
            return data;
        }

        QByteArray controlChange(int channel, int controller, int value)
        {
            QByteArray data;
            data.append(static_cast<char>(0xB0 | (channel & 0x0F)));
            data.append(static_cast<char>(controller & 0x7F));
            data.append(static_cast<char>(qBound(0, value, 127)));
            return data;
        }

        QByteArray programChange(int channel, int program)
        {
            QByteArray data;
            data.append(static_cast<char>(0xC0 | (channel & 0x0F)));
            data.append(static_cast<char>(program & 0x7F));
            return data;
        }

        QByteArray tempoMeta(int bpm)
        {
            const quint32 usPerBeat = static_cast<quint32>(60000000.0 / qMax(1, bpm));
            QByteArray data;
            data.append(static_cast<char>(0xFF));
            data.append(static_cast<char>(0x51));
            data.append(static_cast<char>(0x03));
            data.append(static_cast<char>((usPerBeat >> 16) & 0xFF));
            data.append(static_cast<char>((usPerBeat >> 8) & 0xFF));
            data.append(static_cast<char>(usPerBeat & 0xFF));
            return data;
        }
    } // namespace

    QString SoundMidiExporter::exportSong(const ParseResult &data, const Song &song,
                                          const unsigned char *rom, quint32 romSize,
                                          const QString &filePath, QStringList *warnings)
    {
        Q_UNUSED(data);

        if (song.trackAddresses.isEmpty())
            return QStringLiteral("Song %1 has no tracks.").arg(song.id);

        QVector<Event> events;
        int order = 0;
        bool hasTempo = false;
        int unsupportedPatt = 0;
        int unsupportedRept = 0;
        int unsupportedMemacc = 0;
        int ignoredGoto = 0;
        int runningStatusCount = 0;

        for (int trackIndex = 0; trackIndex < song.trackAddresses.size(); ++trackIndex)
        {
            const quint32 start = song.trackAddresses[trackIndex];
            if (start >= romSize)
                continue;
            const int channel = trackIndex % 16;
            const quint32 limit = std::min<quint32>(romSize, start + 0x20000u);
            quint32 pc = start;
            int tick = 0;
            int lastKey = 60;
            int lastVelocity = 100;
            QHash<int, bool> activeNotes;

            while (pc < limit)
            {
                const quint8 command = rom[pc];

                if (command == 0xB1) // FINE
                    break;

                if (command == 0xB2) // GOTO
                {
                    // MIDI has no equivalent of a bytecode jump. Ignore the
                    // command and keep scanning linearly, so a GOTO does not
                    // truncate the rest of the track.
                    ++ignoredGoto;
                    pc += 5;
                    continue;
                }
                if (command == 0xB3) // PATT
                {
                    ++unsupportedPatt;
                    pc += 5;
                    continue;
                }
                if (command == 0xB4) // PEND
                {
                    ++pc;
                    continue;
                }
                if (command == 0xB5) // REPT
                {
                    ++unsupportedRept;
                    pc += 6;
                    continue;
                }
                if (command == 0xB9) // MEMACC
                {
                    ++unsupportedMemacc;
                    pc += 4;
                    continue;
                }
                if (command == 0xCD) // XCMD
                {
                    pc += 3;
                    continue;
                }

                if (command >= 0x80 && command <= 0xB0) // WAIT
                {
                    tick += WaitTicks[command - 0x80];
                    ++pc;
                    continue;
                }

                if ((command >= 0xBA && command <= 0xC5) || command == 0xC8)
                {
                    if (pc + 1 >= limit)
                        break;
                    const quint8 operand = rom[pc + 1];
                    if (command == 0xBB) // TEMPO = BPM/2
                    {
                        const int bpm = qMax(1, static_cast<int>(operand) * 2);
                        Event event;
                        event.tick = tick;
                        event.order = order++;
                        event.data = tempoMeta(bpm);
                        events.append(event);
                        hasTempo = true;
                    }
                    else if (command == 0xBD) // VOICE -> Program Change
                    {
                        Event event;
                        event.tick = tick;
                        event.order = order++;
                        event.data = programChange(channel, operand & 0x7F);
                        events.append(event);
                    }
                    else if (command == 0xBE) // VOL
                    {
                        Event event;
                        event.tick = tick;
                        event.order = order++;
                        event.data = controlChange(channel, 7, qMin<int>(operand, 127));
                        events.append(event);
                    }
                    else if (command == 0xBF) // PAN
                    {
                        Event event;
                        event.tick = tick;
                        event.order = order++;
                        event.data = controlChange(channel, 10, qMin<int>(operand, 127));
                        events.append(event);
                    }
                    else if (command == 0xC0) // BEND
                    {
                        const int bend = (static_cast<int>(operand) - 64) * 512;
                        QByteArray bendData;
                        bendData.append(static_cast<char>(0xE0 | (channel & 0x0F)));
                        bendData.append(static_cast<char>(bend & 0x7F));
                        bendData.append(static_cast<char>((bend >> 7) & 0x7F));
                        Event event;
                        event.tick = tick;
                        event.order = order++;
                        event.data = bendData;
                        events.append(event);
                    }
                    else if (command == 0xC4) // MOD
                    {
                        Event event;
                        event.tick = tick;
                        event.order = order++;
                        event.data = controlChange(channel, 1, qMin<int>(operand, 127));
                        events.append(event);
                    }
                    pc += 2;
                    continue;
                }

                if (command == 0xCE) // EOT / note off
                {
                    int key = lastKey;
                    int args = 0;
                    if (pc + 1 < limit && rom[pc + 1] < 0x80)
                    {
                        key = rom[pc + 1];
                        ++args;
                        if (pc + 2 < limit && rom[pc + 2] < 0x80)
                            ++args;
                    }
                    Event event;
                    event.tick = tick;
                    event.order = order++;
                    event.data = noteOff(channel, key);
                    events.append(event);
                    activeNotes.remove(key);
                    pc += 1 + args;
                    continue;
                }

                if (command == 0xCF) // TIE / note on
                {
                    int key = lastKey;
                    int velocity = lastVelocity;
                    int args = 0;
                    if (pc + 1 < limit && rom[pc + 1] < 0x80)
                    {
                        key = lastKey = rom[pc + 1];
                        ++args;
                        if (pc + 2 < limit && rom[pc + 2] < 0x80)
                        {
                            velocity = lastVelocity = rom[pc + 2];
                            ++args;
                        }
                    }
                    Event event;
                    event.tick = tick;
                    event.order = order++;
                    event.data = noteOn(channel, key, velocity);
                    events.append(event);
                    activeNotes.insert(key, true);
                    pc += 1 + args;
                    continue;
                }

                if (command >= 0xD0) // NOTE with timeout
                {
                    int key = lastKey;
                    int velocity = lastVelocity;
                    int args = 0;
                    if (pc + 1 < limit && rom[pc + 1] < 0x80)
                    {
                        key = lastKey = rom[pc + 1];
                        ++args;
                        if (pc + 2 < limit && rom[pc + 2] < 0x80)
                        {
                            velocity = lastVelocity = rom[pc + 2];
                            ++args;
                        }
                    }
                    // N01 = 0xD0 maps to WaitTicks[1] = 1 tick, N96 = 0xFF
                    // maps to WaitTicks[48] = 96 ticks.
                    const int length = WaitTicks[(command - 0xD0) + 1];
                    Event on;
                    on.tick = tick;
                    on.order = order++;
                    on.data = noteOn(channel, key, velocity);
                    events.append(on);

                    Event off;
                    off.tick = tick + length;
                    off.order = order++;
                    off.data = noteOff(channel, key);
                    events.append(off);
                    pc += 1 + args;
                    continue;
                }

                if (command < 0x80)
                {
                    ++runningStatusCount;
                    ++pc;
                    continue;
                }

                // Unknown command; advance one byte.
                ++pc;
            }

            // Close notes that were left open by TIE without EOT.
            for (auto it = activeNotes.constBegin(); it != activeNotes.constEnd(); ++it)
            {
                Event event;
                event.tick = tick;
                event.order = order++;
                event.data = noteOff(channel, it.key());
                events.append(event);
            }
        }

        if (warnings)
        {
            if (unsupportedPatt)
                warnings->append(QStringLiteral("Song %1: %2 PATT command(s) are not exported to MIDI.")
                                 .arg(song.id).arg(unsupportedPatt));
            if (unsupportedRept)
                warnings->append(QStringLiteral("Song %1: %2 REPT command(s) are not exported to MIDI.")
                                 .arg(song.id).arg(unsupportedRept));
            if (unsupportedMemacc)
                warnings->append(QStringLiteral("Song %1: %2 MEMACC command(s) are not exported to MIDI.")
                                 .arg(song.id).arg(unsupportedMemacc));
            if (ignoredGoto)
                warnings->append(QStringLiteral("Song %1: %2 GOTO command(s) were ignored; "
                                                "MIDI follows the remaining byte stream.")
                                 .arg(song.id).arg(ignoredGoto));
            if (runningStatusCount)
                warnings->append(QStringLiteral("Song %1: %2 running-status byte(s) were skipped in MIDI.")
                                 .arg(song.id).arg(runningStatusCount));
        }

        if (!hasTempo)
        {
            Event event;
            event.tick = 0;
            event.order = -1;
            event.data = tempoMeta(120);
            events.append(event);
        }

        std::stable_sort(events.begin(), events.end(), [](const Event &a, const Event &b)
        {
            if (a.tick != b.tick)
                return a.tick < b.tick;
            return a.order < b.order;
        });

        QByteArray trackData;
        const QByteArray name = song.name.toLatin1();
        trackData.append('\0');
        trackData.append(static_cast<char>(0xFF));
        trackData.append(static_cast<char>(0x03));
        writeVarLen(trackData, static_cast<quint32>(qMin(name.size(), 127)));
        trackData.append(name.left(127));

        int lastTick = 0;
        for (const Event &event : events)
        {
            writeVarLen(trackData, static_cast<quint32>(qMax(0, event.tick - lastTick)));
            lastTick = event.tick;
            trackData.append(event.data);
        }
        trackData.append('\0');
        trackData.append(static_cast<char>(0xFF));
        trackData.append(static_cast<char>(0x2F));
        trackData.append('\0');

        QByteArray file;
        file.append("MThd", 4);
        appendU32(file, 6);
        appendU16(file, 0); // format 0
        appendU16(file, 1);
        appendU16(file, 24); // PPQ 24
        file.append("MTrk", 4);
        appendU32(file, static_cast<quint32>(trackData.size()));
        file.append(trackData);

        QFile out(filePath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return QStringLiteral("Cannot open %1 for writing: %2").arg(filePath, out.errorString());
        if (out.write(file) != file.size())
            return QStringLiteral("Cannot write %1: %2").arg(filePath, out.errorString());
        out.close();
        return QString();
    }
} // namespace Sound
