#include "SoundFontReader.h"

#include <QFile>
#include <QRegularExpression>

#include <cstring>

namespace Sound
{
    namespace
    {
        quint16 readU16(const QByteArray &data, int offset)
        {
            if (offset + 2 > data.size())
                return 0;
            return static_cast<quint16>(static_cast<quint8>(data.at(offset))) |
                   (static_cast<quint16>(static_cast<quint8>(data.at(offset + 1))) << 8);
        }

        quint32 readU32(const QByteArray &data, int offset)
        {
            if (offset + 4 > data.size())
                return 0;
            return static_cast<quint32>(static_cast<quint8>(data.at(offset))) |
                   (static_cast<quint32>(static_cast<quint8>(data.at(offset + 1))) << 8) |
                   (static_cast<quint32>(static_cast<quint8>(data.at(offset + 2))) << 16) |
                   (static_cast<quint32>(static_cast<quint8>(data.at(offset + 3))) << 24);
        }

        bool findChunk(const QByteArray &data, int listStart, int listEnd,
                       const char id[4], int &offset, int &size)
        {
            int pos = listStart;
            while (pos + 8 <= listEnd)
            {
                const QByteArray chunkId = data.mid(pos, 4);
                const quint32 chunkSize = readU32(data, pos + 4);
                const int chunkData = pos + 8;
                if (chunkData + static_cast<int>(chunkSize) > listEnd)
                    return false;
                if (chunkId == QByteArray(id, 4))
                {
                    offset = chunkData;
                    size = static_cast<int>(chunkSize);
                    return true;
                }
                pos = chunkData + static_cast<int>(chunkSize);
                if (chunkSize & 1u)
                    ++pos;
            }
            return false;
        }

        bool findList(const QByteArray &data, const char type[4], int &start, int &end)
        {
            int pos = 12;
            while (pos + 8 <= data.size())
            {
                const QByteArray id = data.mid(pos, 4);
                const quint32 size = readU32(data, pos + 4);
                const int chunkData = pos + 8;
                if (chunkData + static_cast<int>(size) > data.size())
                    return false;
                if (id == QByteArray("LIST", 4) && size >= 4 &&
                        data.mid(chunkData, 4) == QByteArray(type, 4))
                {
                    start = chunkData + 4;
                    end = chunkData + static_cast<int>(size);
                    return true;
                }
                pos = chunkData + static_cast<int>(size);
                if (size & 1u)
                    ++pos;
            }
            return false;
        }

        QString name20(const QByteArray &data, int offset)
        {
            QByteArray name = data.mid(offset, 20);
            const int zero = name.indexOf('\0');
            if (zero >= 0)
                name = name.left(zero);
            return QString::fromLatin1(name).trimmed();
        }
    } // namespace

    Sf2ReadResult SoundFontReader::read(const QString &filePath)
    {
        Sf2ReadResult result;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            result.error = QStringLiteral("Cannot open %1: %2").arg(filePath, file.errorString());
            return result;
        }
        const QByteArray data = file.readAll();
        file.close();

        if (data.size() < 12 || data.mid(0, 4) != QByteArray("RIFF", 4) ||
                data.mid(8, 4) != QByteArray("sfbk", 4))
        {
            result.error = QStringLiteral("%1 is not a SoundFont 2 file.").arg(filePath);
            return result;
        }

        int sdtaStart = 0, sdtaEnd = 0;
        int pdtaStart = 0, pdtaEnd = 0;
        if (!findList(data, "sdta", sdtaStart, sdtaEnd) ||
                !findList(data, "pdta", pdtaStart, pdtaEnd))
        {
            result.error = QStringLiteral("%1 is missing the sdta/pdta lists.").arg(filePath);
            return result;
        }

        int smplOffset = 0, smplSize = 0;
        int shdrOffset = 0, shdrSize = 0;
        if (!findChunk(data, sdtaStart, sdtaEnd, "smpl", smplOffset, smplSize) ||
                !findChunk(data, pdtaStart, pdtaEnd, "shdr", shdrOffset, shdrSize))
        {
            result.error = QStringLiteral("%1 is missing the smpl/shdr chunks.").arg(filePath);
            return result;
        }

        int phdrOffset = 0, phdrSize = 0;
        int pbagOffset = 0, pbagSize = 0;
        int pgenOffset = 0, pgenSize = 0;
        int instOffset = 0, instSize = 0;
        int ibagOffset = 0, ibagSize = 0;
        int igenOffset = 0, igenSize = 0;
        const bool hasPresets =
                findChunk(data, pdtaStart, pdtaEnd, "phdr", phdrOffset, phdrSize) &&
                findChunk(data, pdtaStart, pdtaEnd, "pbag", pbagOffset, pbagSize) &&
                findChunk(data, pdtaStart, pdtaEnd, "pgen", pgenOffset, pgenSize) &&
                findChunk(data, pdtaStart, pdtaEnd, "inst", instOffset, instSize) &&
                findChunk(data, pdtaStart, pdtaEnd, "ibag", ibagOffset, ibagSize) &&
                findChunk(data, pdtaStart, pdtaEnd, "igen", igenOffset, igenSize);

        if (hasPresets)
        {
            const int presetCount = phdrSize / 38;
            const int pbagCount = pbagSize / 4;
            const int pgenCount = pgenSize / 4;
            const int instCount = instSize / 22;
            const int ibagCount = ibagSize / 4;
            const int igenCount = igenSize / 4;

            for (int presetIndex = 0; presetIndex < presetCount; ++presetIndex)
            {
                const int presetOffset = phdrOffset + presetIndex * 38;
                const QString presetName = name20(data, presetOffset);
                if (presetName == QStringLiteral("EOP"))
                    break;

                Sf2Program program;
                program.name = presetName;
                program.program = readU16(data, presetOffset + 20);
                program.bank = readU16(data, presetOffset + 22);
                const int firstBag = readU16(data, presetOffset + 24);
                const int nextBag = (presetIndex + 1 < presetCount)
                        ? readU16(data, phdrOffset + (presetIndex + 1) * 38 + 24)
                        : pbagCount - 1;

                for (int bag = firstBag; bag < nextBag && bag < pbagCount - 1; ++bag)
                {
                    const int firstGen = readU16(data, pbagOffset + bag * 4);
                    const int nextGen = (bag + 1 < pbagCount - 1)
                            ? readU16(data, pbagOffset + (bag + 1) * 4)
                            : pgenCount - 1;
                    int instrumentIndex = -1;
                    for (int gen = firstGen; gen < nextGen && gen < pgenCount - 1; ++gen)
                    {
                        const int genOffset = pgenOffset + gen * 4;
                        const quint16 oper = readU16(data, genOffset);
                        if (oper == 0x0029)
                            instrumentIndex = readU16(data, genOffset + 2);
                    }
                    if (instrumentIndex < 0 || instrumentIndex >= instCount - 1)
                        continue;

                    const int instBag = readU16(data, instOffset + instrumentIndex * 22 + 20);
                    const int nextInstBag = (instrumentIndex + 1 < instCount - 1)
                            ? readU16(data, instOffset + (instrumentIndex + 1) * 22 + 20)
                            : ibagCount - 1;
                    for (int ibag = instBag; ibag < nextInstBag && ibag < ibagCount - 1; ++ibag)
                    {
                        const int firstIgen = readU16(data, ibagOffset + ibag * 4);
                        const int nextIgen = (ibag + 1 < ibagCount - 1)
                                ? readU16(data, ibagOffset + (ibag + 1) * 4)
                                : igenCount - 1;
                        Sf2Zone zone;
                        for (int gen = firstIgen; gen < nextIgen && gen < igenCount - 1; ++gen)
                        {
                            const int genOffset = igenOffset + gen * 4;
                            const quint16 oper = readU16(data, genOffset);
                            const quint16 amount = readU16(data, genOffset + 2);
                            switch (oper)
                            {
                            case 0x002B: // keyRange
                                zone.keyLow = qBound(0, static_cast<int>(amount & 0xFF), 127);
                                zone.keyHigh = qBound(zone.keyLow,
                                                      static_cast<int>((amount >> 8) & 0xFF),
                                                      127);
                                zone.hasKeyRange = true;
                                break;
                            case 0x0035: // sampleID
                                zone.sampleId = amount;
                                break;
                            case 0x0011: // pan
                                zone.pan = static_cast<qint16>(amount);
                                zone.hasPan = true;
                                break;
                            case 0x0022:
                                zone.attack = static_cast<qint16>(amount);
                                zone.hasAttack = true;
                                break;
                            case 0x0023:
                                zone.decay = static_cast<qint16>(amount);
                                zone.hasDecay = true;
                                break;
                            case 0x0024:
                                zone.sustain = amount;
                                zone.hasSustain = true;
                                break;
                            case 0x0025:
                                zone.release = static_cast<qint16>(amount);
                                zone.hasRelease = true;
                                break;
                            default:
                                break;
                            }
                        }
                        if (zone.sampleId >= 0)
                            program.zones.append(zone);
                    }
                }
                result.programs.append(program);
            }
        }

        const int sampleHeaders = shdrSize / 46;
        for (int i = 0; i < sampleHeaders; ++i)
        {
            const int offset = shdrOffset + i * 46;
            const QString name = name20(data, offset);
            if (name == QStringLiteral("EOS"))
                break;

            const quint32 start = readU32(data, offset + 20);
            const quint32 end = readU32(data, offset + 24);
            const quint32 startLoop = readU32(data, offset + 28);
            const quint32 endLoop = readU32(data, offset + 32);
            const quint32 sampleRate = readU32(data, offset + 36);

            if (end <= start || end > static_cast<quint32>(smplSize / 2))
            {
                result.warnings.append(QStringLiteral("SF2 sample %1 has an invalid range.").arg(name));
                continue;
            }

            Sf2Sample sample;
            sample.name = name;
            sample.sampleRate = static_cast<int>(sampleRate);
            sample.pcm.reserve(static_cast<int>(end - start));
            for (quint32 frame = start; frame < end; ++frame)
            {
                const qint16 value = static_cast<qint16>(
                            readU16(data, smplOffset + static_cast<int>(frame) * 2));
                sample.pcm.append(static_cast<char>(value >> 8));
            }

            if (startLoop < endLoop && endLoop <= end)
            {
                sample.looped = true;
                sample.loopStart = static_cast<int>(startLoop - start);
                sample.loopEnd = static_cast<int>(endLoop - start);
            }

            const QRegularExpression idExpression(QStringLiteral("^S(\\d{4})"));
            const QRegularExpressionMatch match = idExpression.match(name);
            if (match.hasMatch())
                sample.sampleId = match.captured(1).toInt();
            else
                sample.sampleId = i;

            result.samples.append(sample);
        }

        if (result.samples.isEmpty())
            result.error = QStringLiteral("%1 contains no usable samples.").arg(filePath);
        return result;
    }
} // namespace Sound
