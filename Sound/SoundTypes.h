#ifndef SOUNDTYPES_H
#define SOUNDTYPES_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

namespace Sound
{
    // Canonical terminology:
    // Bank  : a set of up to 128 Programs; the GBA engine calls it a voicegroup.
    // Program: a 0..127 slot in a Bank selected by the MPlay VOICE command.
    // Voice : the 12-byte instrument definition a Program points to.
    // Sample: the 16-byte header plus 8-bit PCM referenced by a Voice.
    // Wave  : the 16-byte PSG3 waveform referenced by a PSG Voice.

    enum class VoiceType : quint8
    {
        Sample = 0x00,
        PsgSquare1 = 0x01,
        PsgSquare2 = 0x02,
        PsgWave = 0x03,
        PsgNoise = 0x04,
        SampleFixed = 0x08,
        PsgSquare1Alias = 0x09,
        PsgSquare2Alias = 0x0A,
        PsgWaveAlias = 0x0B,
        PsgNoiseAlias = 0x0C,
        KeySplit = 0x40,
        DrumKit = 0x80,
        Unknown = 0xFF
    };

    enum class SongCategory
    {
        Bgm,
        Sfx,
        Voice,
        Fanfare,
        Ending,
        Other
    };

    enum class BankRole
    {
        Music,
        SubBank,
        Custom
    };

    enum class SongState
    {
        Empty,
        Vanilla,
        Custom
    };

    struct Sample
    {
        int id = -1;
        QString name;
        quint32 romAddress = 0; // ROM file offset, without the GBA 0x08000000 base
        quint8 flag = 0;
        quint32 pitch = 0;
        quint32 loopStart = 0;
        quint32 size = 0;
        QByteArray pcm;
        QByteArray sha1;

        // Staged WAV import state (Phase 2B).
        bool changed = false;
        bool isNew = false;
        int sf2SampleId = -1;
        QString importedWavFile;
        QByteArray importedWavHash;
        QString importedSf2File;
        QByteArray importedSf2Hash;

        bool isLooped() const { return (flag & 0x40) != 0; }
        double sampleRate() const { return static_cast<double>(pitch) / 1024.0; }
    };

    struct Wave
    {
        int id = -1;
        quint32 romAddress = 0; // ROM file offset
        QByteArray data;
    };

    // A sub-voice referenced by a Key Split or a Drum Kit. It is deliberately a
    // different type from Voice so the data model stays non-recursive.
    struct SubVoice
    {
        VoiceType type = VoiceType::Unknown;
        quint8 raw[12] = {};
        quint8 key = 0;
        quint8 pan = 0;
        quint8 attack = 0;
        quint8 decay = 0;
        quint8 sustain = 0;
        quint8 release = 0;
        int sampleId = -1;
        quint32 waveAddress = 0;
        quint32 romAddress = 0;
    };

    struct Voice
    {
        VoiceType type = VoiceType::Unknown;
        quint8 raw[12] = {};
        quint8 key = 0;
        quint8 pan = 0;
        quint8 attack = 0;
        quint8 decay = 0;
        quint8 sustain = 0;
        quint8 release = 0;
        int sampleId = -1;
        quint32 waveAddress = 0; // ROM file offset
        quint32 romAddress = 0;  // ROM file offset
        quint32 subBase = 0;     // ROM file offset
        quint32 tableAddress = 0;

        // Key Split: keyTable[key] is an index into subVoices.
        QVector<quint8> keyTable;
        // Key Split: unique sub-voices referenced by the table.
        // Drum Kit : exactly 128 entries, indexed by MIDI key.
        QVector<SubVoice> subVoices;

        bool isValid() const { return type != VoiceType::Unknown; }
    };

    struct Program
    {
        int index = 0;
        bool used = false;
        bool changed = false;
        QString name;
        Voice voice;
    };

    struct Bank
    {
        int id = 0;
        QString name;
        BankRole role = BankRole::Music;
        quint32 base = 0;          // vanilla table base
        quint32 dataAddress = 0;   // actual voice data (vanilla or SoundBankChunk)
        quint32 dataSize = 0;
        bool isCustom = false;
        QVector<Program> programs; // 128 entries
    };

    struct Song
    {
        int id = 0;
        SongState state = SongState::Empty;
        quint16 ms = 0;
        quint16 me = 0;
        quint8 priority = 0;
        quint8 echo = 0;
        int bankId = -1;
        int trackCount = 0;
        quint32 romAddress = 0;            // ROM file offset
        QVector<quint32> trackAddresses;   // ROM file offsets
        QString name;
        QString symbol;
        QString description;
        SongCategory category = SongCategory::Other;
        bool changed = false;

        // Staged import state (Phase 2). The .s file is assembled at save time.
        bool importFromS = false;
        QString importedSFile;
        QByteArray importedSHash;
    };

    struct Player
    {
        int id = 0;
        quint16 maxTracks = 0;
        quint16 flags = 0;
        QVector<int> songIds;
    };

    // All addresses are ROM file offsets. Pointers stored inside the ROM are
    // GBA addresses and are converted by SoundParser.
    struct Layout
    {
        quint32 songTable = 0;
        quint32 playerTable = 0;
        quint32 voicePoolStart = 0;
        quint32 sampleRegionStart = 0;
        quint32 songRegionStart = 0;
        int songCount = 0;
        int playerCount = 0;
        quint32 bankBase[11] = {};
    };

    struct ParseResult
    {
        Layout layout;
        QVector<Song> songs;
        QVector<Bank> banks;
        QVector<Sample> samples;
        QVector<Wave> waves;
        QVector<Player> players;
        QStringList warnings;
        QString error;

        bool isValid() const { return error.isEmpty(); }
        int nonEmptySongCount() const;
        int usedProgramCount() const;
    };

    QString VoiceTypeName(VoiceType type);
    QString SongCategoryName(SongCategory category);
} // namespace Sound

#endif // SOUNDTYPES_H
