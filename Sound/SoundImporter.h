#ifndef SOUNDIMPORTER_H
#define SOUNDIMPORTER_H

#include "SoundTypes.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace Sound
{
    struct ImportResult
    {
        QString error;
        QStringList warnings;
        QVector<int> changedSongIds;
        QVector<int> changedSampleIds;
        QVector<int> changedBankIds;
        int changedProgramCount = 0;
    };

    // Reads soundmanifest.json from the sound project folder and stages the
    // changed .s files on the model. It never writes the ROM; SoundSave does
    // that when the dialog is accepted.
    class SoundImporter
    {
    public:
        static ImportResult importAll(const QString &folder, ParseResult &data,
                                      const unsigned char *rom, quint32 romSize);
    };
} // namespace Sound

#endif // SOUNDIMPORTER_H
