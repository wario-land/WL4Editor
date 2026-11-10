#include "SoundParser.h"

#include "ROMUtils.h"
#include "SoundDatabase.h"
#include "WL4Constants.h"

#include <QCryptographicHash>
#include <QHash>
#include <QStringList>

#include <cstring>

namespace Sound
{
    namespace
    {
        constexpr quint32 RomBase = 0x08000000;
        constexpr int ProgramCount = 128;

        const quint8 kUnusedVoice[12] =
        {
            0x01, 0x3C, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00
        };

        bool isKnownVoiceType(quint8 type)
        {
            switch (type)
            {
            case 0x00: case 0x01: case 0x02: case 0x03: case 0x04:
            case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C:
            case 0x40: case 0x80:
                return true;
            default:
                return false;
            }
        }

        bool isSampleType(quint8 type)
        {
            return type == 0x00 || type == 0x08;
        }

        bool isPsgWaveType(quint8 type)
        {
            return type == 0x03 || type == 0x0B;
        }

        class Parser
        {
        public:
            Parser(const unsigned char *rom, quint32 romSize)
                : rom(rom), romSize(romSize)
            {
            }

            ParseResult run()
            {
                SoundDatabase::instance().load();

                if (!rom || romSize < 0x800000)
                {
                    result.error = QStringLiteral("The ROM is too small to contain WL4 sound data.");
                    return result;
                }
                if (std::memcmp(rom + 0xAC, "AWAE", 4) != 0)
                {
                    result.error = QStringLiteral("Unsupported sound layout: expected the US version (game code AWAE).");
                    return result;
                }

                const quint32 songTable = WL4Constants::SongTable;
                if (!inRom(songTable, 8) ||
                        !inRom(pointerToOffset(readU32(songTable)), 8))
                {
                    result.error = QStringLiteral("The sound tables do not match the known US layout.");
                    return result;
                }

                result.layout.songTable = WL4Constants::SongTable;
                result.layout.playerTable = WL4Constants::PlayerTable;
                result.layout.voicePoolStart = WL4Constants::VoicePoolStart;
                result.layout.sampleRegionStart = WL4Constants::SampleRegionStart;
                result.layout.songRegionStart = WL4Constants::SongRegionStart;
                result.layout.songCount = WL4Constants::SongCount;
                result.layout.playerCount = WL4Constants::PlayerCount;
                for (int i = 0; i < WL4Constants::SoundBankCount; ++i)
                    result.layout.bankBase[i] = WL4Constants::SoundBankBase[i];

                parsePlayers();
                parseSongs();
                parseBanks();

                for (Song &song : result.songs)
                {
                    if (song.ms < result.players.size())
                        result.players[song.ms].songIds.append(song.id);
                }
                return result;
            }

        private:
            static constexpr quint32 InvalidOffset = 0xFFFFFFFFu;

            // All addresses in Sound::Layout and Sound::Song/Sample/Voice are
            // ROM file offsets. Pointers stored in the ROM are GBA addresses
            // with the 0x08000000 base, so they are converted at the boundary.
            quint32 pointerToOffset(quint32 pointer) const
            {
                if (!pointer)
                    return InvalidOffset;
                return pointer >= RomBase ? pointer - RomBase : pointer;
            }

            bool inRom(quint32 offset, quint32 length) const
            {
                return offset != InvalidOffset && offset <= romSize &&
                       length <= romSize - offset;
            }

            quint8 readU8(quint32 offset) const
            {
                return inRom(offset, 1) ? rom[offset] : 0;
            }

            quint16 readU16(quint32 offset) const
            {
                if (!inRom(offset, 2))
                    return 0;
                return static_cast<quint16>(rom[offset] | (rom[offset + 1] << 8));
            }

            quint32 readU32(quint32 offset) const
            {
                if (!inRom(offset, 4))
                    return 0;
                return static_cast<quint32>(rom[offset]) |
                       (static_cast<quint32>(rom[offset + 1]) << 8) |
                       (static_cast<quint32>(rom[offset + 2]) << 16) |
                       (static_cast<quint32>(rom[offset + 3]) << 24);
            }

            void warn(const QString &message)
            {
                if (!result.warnings.contains(message))
                    result.warnings.append(message);
            }

            bool customBankInfo(quint32 dataAddress, int &bankId, quint32 &dataSize) const
            {
                if (!inRom(dataAddress, 1) || dataAddress < 12)
                    return false;
                if (rom[dataAddress - 4] !=
                        static_cast<unsigned char>(ROMUtils::SaveDataChunkType::SoundBankChunk))
                {
                    return false;
                }
                const quint32 size = static_cast<quint32>(readU16(dataAddress - 8)) |
                        (static_cast<quint32>(readU8(dataAddress - 3)) << 16);
                if (size < 8 || dataAddress + size > romSize)
                    return false;
                const quint32 trailer = dataAddress + size - 8;
                if (std::memcmp(rom + trailer, "W4BK", 4) != 0)
                    return false;
                bankId = rom[trailer + 4];
                if (bankId < 0 || bankId >= WL4Constants::SoundBankCount)
                    return false;
                dataSize = size - 8;
                return true;
            }

            quint32 vanillaBankEnd(quint32 base) const
            {
                quint32 end = WL4Constants::SampleRegionStart;
                for (int i = 0; i < WL4Constants::SoundBankCount; ++i)
                {
                    const quint32 candidate = WL4Constants::SoundBankBase[i];
                    if (candidate > base && candidate < end)
                        end = candidate;
                }
                return end;
            }

            int findBankId(quint32 base) const
            {
                if (!base)
                    return -1;
                for (int i = 0; i < WL4Constants::SoundBankCount; ++i)
                {
                    if (WL4Constants::SoundBankBase[i] && WL4Constants::SoundBankBase[i] == base)
                        return i;
                }
                return -1;
            }

            QString fallbackBankName(int id) const
            {
                switch (id)
                {
                case 0: return QStringLiteral("KeySplitBank");
                case 1: return QStringLiteral("SoundEffects");
                case 2: return QStringLiteral("SunnySideUp");
                case 3: return QStringLiteral("GeneralMusic");
                case 5: return QStringLiteral("RhythmSet");
                case 6: return QStringLiteral("SfxSet");
                case 7: return QStringLiteral("VoiceSet");
                case 8: return QStringLiteral("SfxCdSet");
                case 9: return QStringLiteral("WarioVoiceSet");
                case 10: return QStringLiteral("ReverseWind");
                default: return QStringLiteral("Bank%1").arg(id);
                }
            }

            SongCategory categoryForSong(int id, const QString &symbol) const
            {
                const SongCategory dbCategory = SoundDatabase::instance().songCategory(id);
                if (dbCategory != SongCategory::Other)
                    return dbCategory;

                const QString upper = symbol.toUpper();
                if (upper.startsWith(QStringLiteral("S_V")) || upper.startsWith(QStringLiteral("V_")))
                    return SongCategory::Voice;
                if (upper.startsWith(QStringLiteral("S_")))
                    return SongCategory::Sfx;
                if (upper.startsWith(QStringLiteral("M_ENDING")))
                    return SongCategory::Ending;
                if (upper.startsWith(QStringLiteral("M_BGM")) || upper.startsWith(QStringLiteral("M_")))
                    return SongCategory::Bgm;
                if (upper.startsWith(QStringLiteral("F_")))
                    return SongCategory::Fanfare;
                return SongCategory::Other;
            }

            bool isUnused(const quint8 *raw) const
            {
                bool allZero = true;
                for (int i = 0; i < 12; ++i)
                {
                    if (raw[i] != 0)
                    {
                        allZero = false;
                        break;
                    }
                }
                return allZero || std::memcmp(raw, kUnusedVoice, 12) == 0;
            }

            void parsePlayers()
            {
                result.players.reserve(WL4Constants::PlayerCount);
                for (int i = 0; i < WL4Constants::PlayerCount; ++i)
                {
                    const quint32 base = WL4Constants::PlayerTable + static_cast<quint32>(i) * 12;
                    Player player;
                    player.id = i;
                    // The first two fields are RAM pointers; only the track count
                    // and flags are meaningful for the editor.
                    player.maxTracks = readU16(base + 8);
                    player.flags = readU16(base + 10);
                    result.players.append(player);
                }
            }

            void parseSongs()
            {
                result.songs.reserve(WL4Constants::SongCount);
                for (int id = 0; id < WL4Constants::SongCount; ++id)
                {
                    const quint32 entry = WL4Constants::SongTable + static_cast<quint32>(id) * 8;
                    Song song;
                    song.id = id;
                    song.ms = readU16(entry + 4);
                    song.me = readU16(entry + 6);

                    const QString fallbackName = QStringLiteral("Song%1").arg(id, 4, 10, QLatin1Char('0'));
                    song.name = SoundDatabase::instance().songName(id, fallbackName);
                    song.symbol = SoundDatabase::instance().songSymbol(id, fallbackName);
                    song.description = SoundDatabase::instance().songDescription(id);
                    song.category = categoryForSong(id, song.symbol);

                    const quint32 header = pointerToOffset(readU32(entry));
                    song.romAddress = header;
                    if (!inRom(header, 8))
                    {
                        result.songs.append(song);
                        continue;
                    }

                    song.trackCount = readU8(header);
                    song.priority = readU8(header + 2);
                    song.echo = readU8(header + 3);
                    const quint32 bankBase = pointerToOffset(readU32(header + 4));
                    song.bankId = findBankId(bankBase);
                    if (song.bankId < 0)
                    {
                        int customBankId = -1;
                        quint32 customBankSize = 0;
                        if (customBankInfo(bankBase, customBankId, customBankSize))
                        {
                            song.bankId = customBankId;
                            const auto existing = customBankAddresses.constFind(customBankId);
                            if (existing == customBankAddresses.constEnd())
                            {
                                customBankAddresses.insert(customBankId, bankBase);
                                customBankSizes.insert(customBankId, customBankSize);
                            }
                            else if (existing.value() != bankBase)
                            {
                                warn(QStringLiteral("Bank %1 has multiple custom bank chunks; using 0x%2.")
                                     .arg(customBankId)
                                     .arg(existing.value(), 8, 16, QLatin1Char('0')));
                            }
                        }
                    }

                    if (song.trackCount == 0 || song.trackCount > 16)
                    {
                        // Empty song slots use a placeholder header; keep the slot
                        // visible but do not try to parse tracks.
                        song.state = SongState::Empty;
                        song.trackCount = 0;
                        result.songs.append(song);
                        continue;
                    }

                    const bool isCustomChunk =
                            header >= 12 &&
                            std::memcmp(rom + header - 12, "STAR", 4) == 0 &&
                            rom[header - 4] == static_cast<unsigned char>(
                                ROMUtils::SaveDataChunkType::SoundSongChunk);
                    song.state = isCustomChunk ? SongState::Custom : SongState::Vanilla;
                    song.trackAddresses.reserve(song.trackCount);
                    for (int track = 0; track < song.trackCount; ++track)
                    {
                        const quint32 trackAddress = pointerToOffset(
                                    readU32(header + 8 + static_cast<quint32>(track) * 4));
                        if (!inRom(trackAddress, 1))
                        {
                            warn(QStringLiteral("Song %1: invalid track pointer 0x%2.")
                                 .arg(id).arg(trackAddress, 8, 16, QLatin1Char('0')));
                            continue;
                        }
                        song.trackAddresses.append(trackAddress);
                    }
                    if (song.trackAddresses.isEmpty())
                        song.state = SongState::Empty;

                    result.songs.append(song);
                }
            }

            int sampleIdForAddress(quint32 address)
            {
                if (!address)
                    return -1;
                const auto existing = sampleIndex.constFind(address);
                if (existing != sampleIndex.constEnd())
                    return existing.value();

                if (!inRom(address, 16))
                {
                    warn(QStringLiteral("Sample header 0x%1 is outside the ROM.")
                         .arg(address, 8, 16, QLatin1Char('0')));
                    return -1;
                }

                const quint8 flag = readU8(address + 3);
                const quint32 pitch = readU32(address + 4);
                const quint32 loopStart = readU32(address + 8);
                const quint32 size = readU32(address + 12);

                if (size == 0 || !inRom(address + 16, size))
                {
                    warn(QStringLiteral("Sample 0x%1 has an invalid size %2.")
                         .arg(address, 8, 16, QLatin1Char('0')).arg(size));
                    return -1;
                }
                if ((flag & 0x40) && loopStart > size)
                {
                    warn(QStringLiteral("Sample 0x%1 has an invalid loop start %2.")
                         .arg(address, 8, 16, QLatin1Char('0')).arg(loopStart));
                }

                int sampleId = result.samples.size();
                QString storedName;
                if (address >= 12 &&
                        rom[address - 4] == static_cast<unsigned char>(
                            ROMUtils::SaveDataChunkType::SoundSampleChunk))
                {
                    const quint32 chunkLen = static_cast<quint32>(readU16(address - 8)) |
                            (static_cast<quint32>(readU8(address - 3)) << 16);
                    if (chunkLen >= 8 && address + chunkLen <= romSize)
                    {
                        const quint32 trailer = address + chunkLen - 8;
                        if (std::memcmp(rom + trailer, "W4SM", 4) == 0)
                        {
                            const int storedId = static_cast<int>(readU16(trailer + 4));
                            if (storedId >= 0)
                                sampleId = storedId;
                        }
                        else if (std::memcmp(rom + trailer, "W4S2", 4) == 0)
                        {
                            const int storedId = static_cast<int>(readU16(trailer + 4));
                            if (storedId >= 0)
                                sampleId = storedId;
                            const quint32 nameOffset = address + 16 + size;
                            if (nameOffset + 2 <= trailer)
                            {
                                const quint32 nameLength = readU16(nameOffset);
                                if (nameOffset + 2 + nameLength <= trailer)
                                {
                                    storedName = QString::fromUtf8(
                                                reinterpret_cast<const char *>(
                                                    rom + nameOffset + 2),
                                                static_cast<int>(nameLength));
                                }
                            }
                        }
                    }
                }

                Sample sample;
                sample.id = sampleId;
                sample.romAddress = address;
                sample.flag = flag;
                sample.pitch = pitch;
                sample.loopStart = loopStart;
                sample.size = size;
                sample.pcm = QByteArray(reinterpret_cast<const char *>(rom + address + 16),
                                        static_cast<int>(size));
                sample.sha1 = QCryptographicHash::hash(sample.pcm, QCryptographicHash::Sha1).toHex();
                const QString fallbackName = QStringLiteral("Sample%1")
                        .arg(sample.id, 4, 10, QLatin1Char('0'));
                sample.name = storedName.isEmpty()
                        ? SoundDatabase::instance().sampleName(address, fallbackName)
                        : storedName;

                if (sample.id >= result.samples.size())
                    result.samples.resize(sample.id + 1);
                result.samples[sample.id] = sample;
                sampleIndex.insert(address, sample.id);
                return sample.id;
            }

            int waveIdForAddress(quint32 address)
            {
                if (!address)
                    return -1;
                const auto existing = waveIndex.constFind(address);
                if (existing != waveIndex.constEnd())
                    return existing.value();
                if (!inRom(address, 16))
                    return -1;

                Wave wave;
                wave.id = result.waves.size();
                wave.romAddress = address;
                wave.data = QByteArray(reinterpret_cast<const char *>(rom + address), 16);
                result.waves.append(wave);
                waveIndex.insert(address, wave.id);
                return wave.id;
            }

            bool parseSubVoice(quint32 address, SubVoice &voice)
            {
                if (!inRom(address, 12))
                    return false;

                quint8 raw[12];
                std::memcpy(raw, rom + address, 12);
                std::memcpy(voice.raw, raw, 12);
                voice.type = static_cast<VoiceType>(raw[0]);
                voice.romAddress = address;
                voice.key = raw[1];
                voice.pan = raw[3];
                voice.attack = raw[8];
                voice.decay = raw[9];
                voice.sustain = raw[10];
                voice.release = raw[11];

                if (!isKnownVoiceType(raw[0]))
                {
                    voice.type = VoiceType::Unknown;
                    return true;
                }
                if (isSampleType(raw[0]))
                {
                    voice.sampleId = sampleIdForAddress(pointerToOffset(readU32(address + 4)));
                }
                else if (isPsgWaveType(raw[0]))
                {
                    voice.waveAddress = pointerToOffset(readU32(address + 4));
                    waveIdForAddress(voice.waveAddress);
                }
                return true;
            }

            bool parseVoice(quint32 address, Voice &voice)
            {
                if (!inRom(address, 12))
                    return false;

                quint8 raw[12];
                std::memcpy(raw, rom + address, 12);
                std::memcpy(voice.raw, raw, 12);
                voice.romAddress = address;
                voice.type = static_cast<VoiceType>(raw[0]);
                voice.key = raw[1];
                voice.pan = raw[3];
                voice.attack = raw[8];
                voice.decay = raw[9];
                voice.sustain = raw[10];
                voice.release = raw[11];

                if (!isKnownVoiceType(raw[0]))
                {
                    voice.type = VoiceType::Unknown;
                    return true;
                }

                if (isSampleType(raw[0]))
                {
                    voice.sampleId = sampleIdForAddress(pointerToOffset(readU32(address + 4)));
                }
                else if (isPsgWaveType(raw[0]))
                {
                    voice.waveAddress = pointerToOffset(readU32(address + 4));
                    waveIdForAddress(voice.waveAddress);
                }
                else if (raw[0] == 0x40)
                {
                    const quint32 subBase = pointerToOffset(readU32(address + 4));
                    const quint32 tableAddress = pointerToOffset(readU32(address + 8));
                    voice.subBase = subBase;
                    voice.tableAddress = tableAddress;
                    if (inRom(tableAddress, 128))
                    {
                        QHash<quint32, quint8> indexByAddress;
                        voice.keyTable.reserve(128);
                        for (int key = 0; key < 128; ++key)
                        {
                            const quint8 index = readU8(tableAddress + static_cast<quint32>(key));
                            const quint32 subAddress = subBase + static_cast<quint32>(index) * 12;
                            auto found = indexByAddress.constFind(subAddress);
                            quint8 localIndex = 0;
                            if (found != indexByAddress.constEnd())
                            {
                                localIndex = found.value();
                            }
                            else
                            {
                                SubVoice subVoice;
                                parseSubVoice(subAddress, subVoice);
                                localIndex = static_cast<quint8>(voice.subVoices.size() & 0xFF);
                                voice.subVoices.append(subVoice);
                                indexByAddress.insert(subAddress, localIndex);
                            }
                            voice.keyTable.append(localIndex);
                        }
                    }
                    else
                    {
                        warn(QStringLiteral("Key split 0x%1 has an invalid table.")
                             .arg(address, 8, 16, QLatin1Char('0')));
                    }
                }
                else if (raw[0] == 0x80)
                {
                    const quint32 subBase = pointerToOffset(readU32(address + 4));
                    voice.subBase = subBase;
                    voice.subVoices.reserve(128);
                    for (int key = 0; key < 128; ++key)
                    {
                        SubVoice subVoice;
                        parseSubVoice(subBase + static_cast<quint32>(key) * 12, subVoice);
                        voice.subVoices.append(subVoice);
                    }
                }
                return true;
            }

            void parseBanks()
            {
                static const int bankIds[] = { 0, 1, 2, 3, 5, 6, 7, 8, 9, 10 };
                result.banks.reserve(static_cast<int>(sizeof(bankIds) / sizeof(bankIds[0])));

                for (int bankId : bankIds)
                {
                    Bank bank;
                    bank.id = bankId;
                    bank.base = WL4Constants::SoundBankBase[bankId];
                    bank.dataAddress = customBankAddresses.value(bankId, bank.base);
                    bank.dataSize = customBankSizes.value(
                                bankId, vanillaBankEnd(bank.base) - bank.base);
                    bank.isCustom = customBankAddresses.contains(bankId);
                    bank.name = SoundDatabase::instance().bankName(bankId, fallbackBankName(bankId));
                    bank.role = (bankId == 1 || bankId == 2 || bankId == 3 || bankId == 10)
                            ? BankRole::Music : BankRole::SubBank;
                    bank.programs.reserve(ProgramCount);

                    for (int programIndex = 0; programIndex < ProgramCount; ++programIndex)
                    {
                        Program program;
                        program.index = programIndex;
                        const quint32 address = bank.dataAddress +
                                static_cast<quint32>(programIndex) * 12;
                        parseVoice(address, program.voice);
                        program.used = program.voice.isValid() && !isUnused(program.voice.raw);

                        QString fallback;
                        if (program.voice.sampleId >= 0 &&
                                program.voice.sampleId < result.samples.size())
                        {
                            fallback = result.samples[program.voice.sampleId].name;
                        }
                        else if (program.voice.type != VoiceType::Unknown)
                        {
                            fallback = QStringLiteral("%1 %2")
                                    .arg(VoiceTypeName(program.voice.type))
                                    .arg(programIndex, 3, 10, QLatin1Char('0'));
                        }
                        else
                        {
                            fallback = QStringLiteral("Program%1")
                                    .arg(programIndex, 3, 10, QLatin1Char('0'));
                        }
                        program.name = SoundDatabase::instance().programName(bankId, programIndex, fallback);
                        bank.programs.append(program);
                    }
                    result.banks.append(bank);
                }
            }

            const unsigned char *rom = nullptr;
            quint32 romSize = 0;
            ParseResult result;
            QHash<quint32, int> sampleIndex;
            QHash<quint32, int> waveIndex;
            QHash<int, quint32> customBankAddresses;
            QHash<int, quint32> customBankSizes;
        };
    } // namespace

    Layout SoundParser::retailUSLayout()
    {
        Layout layout;
        layout.songTable = WL4Constants::SongTable;
        layout.playerTable = WL4Constants::PlayerTable;
        layout.voicePoolStart = WL4Constants::VoicePoolStart;
        layout.sampleRegionStart = WL4Constants::SampleRegionStart;
        layout.songRegionStart = WL4Constants::SongRegionStart;
        layout.songCount = WL4Constants::SongCount;
        layout.playerCount = WL4Constants::PlayerCount;
        for (int i = 0; i < WL4Constants::SoundBankCount; ++i)
            layout.bankBase[i] = WL4Constants::SoundBankBase[i];
        return layout;
    }

    ParseResult SoundParser::parse(const unsigned char *rom, quint32 romSize)
    {
        Parser parser(rom, romSize);
        return parser.run();
    }
} // namespace Sound
