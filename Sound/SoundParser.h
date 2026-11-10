#ifndef SOUNDPARSER_H
#define SOUNDPARSER_H

#include "SoundTypes.h"

namespace Sound
{
    // Read-only parser for the vanilla US (AWAE) Wario Land 4 sound data.
    //
    // The layout is fixed and lives in WL4Constants.h; this parser never scans
    // for tables or guesses addresses. Unsupported ROMs return an error instead
    // of producing a partially valid model.
    class SoundParser
    {
    public:
        static Layout retailUSLayout();
        static ParseResult parse(const unsigned char *rom, quint32 romSize);
    };
} // namespace Sound

#endif // SOUNDPARSER_H
