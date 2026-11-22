#ifndef SOUNDSEXPORTER_H
#define SOUNDSEXPORTER_H

#include "SoundTypes.h"

#include <QString>
#include <QStringList>

namespace Sound
{
    // MPlay disassembler. It walks the command stream to find
    // GOTO/PATT/REPT pointers and emits a self-contained .s file that the
    // external arm-none-eabi toolchain can assemble. Known commands are
    // emitted as readable .byte rows with mnemonic comments; unknown commands
    // and running-status argument runs remain raw .byte data so no byte is
    // lost. All addresses are ROM file offsets.
    class SoundSExporter
    {
    public:
        static QString exportSong(const ParseResult &data, const Song &song,
                                  const unsigned char *rom, quint32 romSize,
                                  const QString &filePath,
                                  QStringList *warnings = nullptr);
    };
} // namespace Sound

#endif // SOUNDSEXPORTER_H
