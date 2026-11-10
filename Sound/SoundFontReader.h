#ifndef SOUNDFONTREADER_H
#define SOUNDFONTREADER_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

namespace Sound
{
    struct Sf2Sample
    {
        int sampleId = -1;
        QString name;
        QByteArray pcm; // GBA 8-bit signed PCM
        int sampleRate = 1;
        int loopStart = 0;
        int loopEnd = 0;
        bool looped = false;
    };

    struct Sf2Zone
    {
        int sampleId = -1;
        int keyLow = 0;
        int keyHigh = 127;
        bool hasKeyRange = false;
        int pan = 0;
        bool hasPan = false;
        int attack = -1;
        int decay = -1;
        int sustain = -1;
        int release = -1;
        bool hasAttack = false;
        bool hasDecay = false;
        bool hasSustain = false;
        bool hasRelease = false;
    };

    struct Sf2Program
    {
        QString name;
        int bank = 0;
        int program = 0;
        QVector<Sf2Zone> zones;
    };

    struct Sf2ReadResult
    {
        QVector<Sf2Sample> samples;
        QVector<Sf2Program> programs;
        QString error;
        QStringList warnings;
    };

    // Minimal SF2 reader for sample data import. It reads the sdta/smpl PCM and
    // the pdta/shdr sample headers; preset/instrument zone import is a later
    // phase.
    class SoundFontReader
    {
    public:
        static Sf2ReadResult read(const QString &filePath);
    };
} // namespace Sound

#endif // SOUNDFONTREADER_H
