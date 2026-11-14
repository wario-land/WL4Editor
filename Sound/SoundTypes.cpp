#include "SoundTypes.h"

namespace Sound
{
    QString VoiceTypeName(VoiceType type)
    {
        switch (type)
        {
        case VoiceType::Sample: return QStringLiteral("Sample");
        case VoiceType::SampleFixed: return QStringLiteral("Fixed Sample");
        case VoiceType::PsgSquare1:
        case VoiceType::PsgSquare1Alias: return QStringLiteral("PSG Square 1");
        case VoiceType::PsgSquare2:
        case VoiceType::PsgSquare2Alias: return QStringLiteral("PSG Square 2");
        case VoiceType::PsgWave:
        case VoiceType::PsgWaveAlias: return QStringLiteral("PSG Wave");
        case VoiceType::PsgNoise:
        case VoiceType::PsgNoiseAlias: return QStringLiteral("PSG Noise");
        case VoiceType::KeySplit: return QStringLiteral("Key Split");
        case VoiceType::DrumKit: return QStringLiteral("Drum Kit");
        default: return QStringLiteral("Unknown");
        }
    }

    int ParseResult::nonEmptySongCount() const
    {
        int count = 0;
        for (const Song &song : songs)
        {
            if (song.state != SongState::Empty)
                ++count;
        }
        return count;
    }

    int ParseResult::usedProgramCount() const
    {
        int count = 0;
        for (const Bank &bank : banks)
        {
            for (const Program &program : bank.programs)
            {
                if (program.used)
                    ++count;
            }
        }
        return count;
    }
} // namespace Sound
