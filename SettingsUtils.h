#ifndef SETTINGSUTILS_H
#define SETTINGSUTILS_H

#include <QCoreApplication>
#include <QString>
#include <QFileInfo>
#include <QSettings>
#include <QVector>

namespace SettingsUtils
{
    // Global variables
    extern QString ProgramSettingFilePath;

    /* Sections, keys and values included in the ini file
     * [patch]
     * eabi_binfile_path = path
     *
     * [history]
     * RecentROMPath_0 = path
     * RecentROMPath_1 = path
     * RecentROMPath_2 = path
     * RecentROMPath_3 = path
     * RecentROMPath_4 = path
     */
    enum IniKeys
    {
        eabi_binfile_path = 0,
        RecentROMPath_0   = 1,
        RecentROMPath_1   = 2,
        RecentROMPath_2   = 3,
        RecentROMPath_3   = 4,
        RecentROMPath_4   = 5,
    };

    // Static Key QString set
    static QVector<QString> KeyStringSet =
    {
        "patch/eabi_binfile_path",
        "history/RecentROMPath_0",
        "history/RecentROMPath_1",
        "history/RecentROMPath_2",
        "history/RecentROMPath_3",
        "history/RecentROMPath_4",
    };

    void InitProgramSetupPath();
    void SetKey(enum IniKeys key, QString value);
    QString GetKey(enum IniKeys key);
};

#endif // SETTINGSUTILS_H
