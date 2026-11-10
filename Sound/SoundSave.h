#ifndef SOUNDSAVE_H
#define SOUNDSAVE_H

#include "SoundTypes.h"

#include <QString>

#include <functional>

namespace Sound
{
    // Saves the staged .s imports as SoundSongChunks and patches the vanilla
    // Song Table pointers. It calls ROMUtils::SaveFile directly (not
    // SaveLevel), matching the Patch and Graphic managers.
    //
    // Phase 2 writes Song and Sample chunks. Sample chunks replace existing
    // sample slots; bank voice sample pointers are patched in place so the
    // vanilla Bank structure (and the parser's bank-id detection) stays valid.
    // A dedicated Bank chunk is reserved for the later SF2/bank rebuild phase.
    class SoundSave
    {
    public:
        static QString saveToROM(ParseResult &data,
                                 const QString &projectFolder = QString(),
                                 const std::function<void(int, int)> &progress = {});
    };
} // namespace Sound

#endif // SOUNDSAVE_H
