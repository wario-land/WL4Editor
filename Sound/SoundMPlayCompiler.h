#ifndef SOUNDMPLAYCOMPILER_H
#define SOUNDMPLAYCOMPILER_H

#include <QByteArray>
#include <QString>

namespace Sound
{
    // Compiles a standalone MPlay .s source into a song binary.
    //
    // Unlike the Sound Manager import path, this compiler does not require
    // @ wl4-* metadata. It is intended for GBA developers who already have a
    // complete song header and track bytecode. The source may use the
    // standard MPlay symbolic constants; `.include "MPlayDef.s"` is replaced
    // with WL4Editor's clean-room constant file before assembly.
    //
    // The ARM EABI assembler/linker remains the encoder backend. No automatic
    // compression, wait merging, or PATT/REPT generation is performed.
    class SoundMPlayCompiler
    {
    public:
        static QString compileFile(const QString &sFile, quint32 dataAddress,
                                   QByteArray &binary, QString *normalizedSource = nullptr);
    };
} // namespace Sound

#endif // SOUNDMPLAYCOMPILER_H
