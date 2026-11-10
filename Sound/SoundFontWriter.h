#ifndef SOUNDFONTWRITER_H
#define SOUNDFONTWRITER_H

#include "SoundTypes.h"

#include <QString>
#include <QStringList>

namespace Sound
{
    // Minimal SoundFont 2 writer for the WL4 sound banks.
    //
    // Phase 1 writes:
    //   - all unique samples (8-bit GBA PCM expanded to 16-bit),
    //   - one SF2 instrument per sampled / key-split / drum-kit Program,
    //   - one preset per Program (bank = Bank id, program = Program index),
    //   - key ranges, loop mode, root key, tuning and pan.
    //
    // PSG Programs have no sample and are therefore not written to the SF2;
    // the manifest records them as not exported. GBA ADSR is preserved in the
    // manifest and is not approximated in this phase.
    class SoundFontWriter
    {
    public:
        static QString write(const ParseResult &data, const QString &filePath,
                             QStringList *warnings = nullptr);
    };
} // namespace Sound

#endif // SOUNDFONTWRITER_H
