#ifndef SOUNDSASSEMBLER_H
#define SOUNDSASSEMBLER_H

#include <QByteArray>
#include <QString>

namespace Sound
{
    // Thin wrapper around the arm-none-eabi toolchain used by the Patch
    // Manager. The .s file is linked at the exact ROM data address where the
    // SoundSongChunk will be written, so all internal MPlay pointers are final.
    class SoundSAssembler
    {
    public:
        static QString ensureEabi();
        static QString assemble(const QString &sFile, quint32 dataAddress, QByteArray &binary);
    };
} // namespace Sound

#endif // SOUNDSASSEMBLER_H
