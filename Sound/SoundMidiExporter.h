#ifndef SOUNDMIDIEXPORTER_H
#define SOUNDMIDIEXPORTER_H

#include "SoundTypes.h"

#include <QString>
#include <QStringList>

namespace Sound
{
    // Lossy MIDI export for MuseScore. The .s file is the authoritative source;
    // this writer emits the linear note/control stream of each track and stops
    // at GOTO loops. PATT/REPT/MEMACC are reported as unsupported so the user
    // knows when the MIDI is incomplete.
    class SoundMidiExporter
    {
    public:
        static QString exportSong(const ParseResult &data, const Song &song,
                                  const unsigned char *rom, quint32 romSize,
                                  const QString &filePath,
                                  QStringList *warnings = nullptr);
    };
} // namespace Sound

#endif // SOUNDMIDIEXPORTER_H
