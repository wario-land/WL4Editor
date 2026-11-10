#include "SoundWavIO.h"

#include <QFile>

namespace Sound
{
    namespace
    {
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

        qint32 readSigned(const QByteArray &data, int offset, int bits)
        {
            if (bits == 8)
                return static_cast<qint8>(data.at(offset));
            if (bits == 16)
                return static_cast<qint16>(readU16(data, offset));
            if (bits == 24)
            {
                qint32 value = static_cast<qint32>(readU16(data, offset)) |
                        (static_cast<qint32>(static_cast<qint8>(data.at(offset + 2))) << 16);
                return value;
            }
            if (bits == 32)
                return static_cast<qint32>(readU32(data, offset));
            return 0;
        }
    } // namespace

    QString SoundWavIO::write(const Sample &sample, const QString &filePath)
    {
        if (sample.pcm.isEmpty())
            return QStringLiteral("Sample %1 has no PCM data.").arg(sample.id);

        const quint32 sampleRate = static_cast<quint32>(
                    qMax(1, qRound(sample.sampleRate())));
        QByteArray pcm16;
        pcm16.reserve(sample.pcm.size() * 2);
        for (int i = 0; i < sample.pcm.size(); ++i)
        {
            const qint16 value = static_cast<qint16>(
                        static_cast<qint8>(sample.pcm.at(i))) << 8;
            appendU16(pcm16, static_cast<quint16>(value));
        }

        QByteArray file;
        file.append("RIFF", 4);
        appendU32(file, static_cast<quint32>(36 + pcm16.size()));
        file.append("WAVE", 4);
        file.append("fmt ", 4);
        appendU32(file, 16);
        appendU16(file, 1); // PCM
        appendU16(file, 1); // mono
        appendU32(file, sampleRate);
        appendU32(file, sampleRate * 2);
        appendU16(file, 2);
        appendU16(file, 16);
        file.append("data", 4);
        appendU32(file, static_cast<quint32>(pcm16.size()));
        file.append(pcm16);

        QFile out(filePath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return QStringLiteral("Cannot write %1: %2").arg(filePath, out.errorString());
        if (out.write(file) != file.size())
            return QStringLiteral("Cannot write %1: %2").arg(filePath, out.errorString());
        out.close();
        return QString();
    }

    QString SoundWavIO::read(const QString &filePath, QByteArray &pcm8, quint32 &sampleRate)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
            return QStringLiteral("Cannot open %1: %2").arg(filePath, file.errorString());
        const QByteArray data = file.readAll();
        file.close();

        if (data.size() < 12 || data.mid(0, 4) != QByteArray("RIFF", 4) ||
                data.mid(8, 4) != QByteArray("WAVE", 4))
        {
            return QStringLiteral("%1 is not a RIFF/WAVE file.").arg(filePath);
        }

        int format = 0;
        int channels = 0;
        int bits = 0;
        quint32 rate = 0;
        int dataOffset = -1;
        int dataSize = 0;

        int offset = 12;
        while (offset + 8 <= data.size())
        {
            const QByteArray id = data.mid(offset, 4);
            const quint32 size = readU32(data, offset + 4);
            const int chunkData = offset + 8;
            if (chunkData + static_cast<int>(size) > data.size())
                break;

            if (id == QByteArray("fmt ", 4) && size >= 16)
            {
                format = readU16(data, chunkData);
                channels = readU16(data, chunkData + 2);
                rate = readU32(data, chunkData + 4);
                bits = readU16(data, chunkData + 14);
            }
            else if (id == QByteArray("data", 4))
            {
                dataOffset = chunkData;
                dataSize = static_cast<int>(size);
            }

            offset = chunkData + static_cast<int>(size);
            if (size & 1u)
                ++offset;
        }

        if (format != 1)
            return QStringLiteral("%1 is not uncompressed PCM WAV.").arg(filePath);
        if (channels < 1 || channels > 2)
            return QStringLiteral("%1 has an unsupported channel count.").arg(filePath);
        if (bits != 8 && bits != 16 && bits != 24 && bits != 32)
            return QStringLiteral("%1 has an unsupported bit depth.").arg(filePath);
        if (dataOffset < 0 || dataSize <= 0)
            return QStringLiteral("%1 has no audio data.").arg(filePath);

        const int bytesPerSample = bits / 8;
        const int frameBytes = bytesPerSample * channels;
        const int frames = dataSize / frameBytes;
        if (frames <= 0)
            return QStringLiteral("%1 has no audio frames.").arg(filePath);

        pcm8.clear();
        pcm8.reserve(frames);
        for (int frame = 0; frame < frames; ++frame)
        {
            qint32 mixed = 0;
            for (int channel = 0; channel < channels; ++channel)
            {
                const int sampleOffset = dataOffset + frame * frameBytes +
                        channel * bytesPerSample;
                if (bits == 8)
                {
                    // WAV 8-bit PCM is unsigned, GBA PCM is signed.
                    mixed += static_cast<int>(static_cast<quint8>(data.at(sampleOffset))) - 128;
                }
                else
                {
                    const qint32 value = readSigned(data, sampleOffset, bits);
                    mixed += value >> (bits - 8);
                }
            }
            mixed /= channels;
            pcm8.append(static_cast<char>(qBound(-128, mixed, 127)));
        }

        sampleRate = rate;
        return QString();
    }
} // namespace Sound
