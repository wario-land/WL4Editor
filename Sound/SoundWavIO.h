#ifndef SOUNDWAVIO_H
#define SOUNDWAVIO_H

#include "SoundTypes.h"

#include <QByteArray>
#include <QString>

namespace Sound
{
    // Small PCM WAV helper. Export writes 16-bit mono WAVs from the GBA 8-bit
    // signed PCM; import accepts PCM 8/16/24/32-bit mono or stereo and converts
    // to mono 8-bit signed GBA PCM.
    class SoundWavIO
    {
    public:
        static QString write(const Sample &sample, const QString &filePath);
        static QString read(const QString &filePath, QByteArray &pcm8, quint32 &sampleRate);
    };
} // namespace Sound

#endif // SOUNDWAVIO_H
