#ifndef SOUNDEXPORT_H
#define SOUNDEXPORT_H

#include "SoundTypes.h"

#include <QString>

#include <functional>

namespace Sound
{
    // Writes the complete sound project to
    // <rom directory>/<rom base name>_sound/:
    //   soundmanifest.json
    //   songs/<id>_<name>.s
    //   soundfont/WL4.sf2
    //   README.txt
    //
    // The .s files are the authoritative song format. MIDI export is added in
    // the next phase. The function returns an error string, or an empty string
    // on success.
    class SoundExport
    {
    public:
        static QString defaultFolder(const QString &romPath);
        static QString exportAll(const ParseResult &data, const unsigned char *rom,
                                 quint32 romSize, const QString &folder,
                                 const std::function<void(int, int)> &progress = {});
    };
} // namespace Sound

#endif // SOUNDEXPORT_H
