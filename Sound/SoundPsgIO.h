#ifndef SOUNDPSGIO_H
#define SOUNDPSGIO_H

#include "SoundTypes.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace Sound
{
    // PSG voices are not representable in SF2. This sidecar keeps their raw
    // 12-byte voice records editable without pretending they are samples.
    class SoundPsgIO
    {
    public:
        static bool isPsgType(VoiceType type);

        static QString write(const ParseResult &data, const QString &path);
        static QString read(const QString &path, ParseResult &data,
                            QVector<int> *changedProgramKeys,
                            QStringList *warnings);
    };
} // namespace Sound

#endif // SOUNDPSGIO_H
