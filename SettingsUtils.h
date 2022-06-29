#ifndef SETTINGSUTILS_H
#define SETTINGSUTILS_H

#include <QCoreApplication>
#include <QString>
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
     *
     * [settings]
     * EditorThemeId    = string (convert to int after being read)
     * RollingSaveLimit = string (convert to int as the number of temp files the editor will keep)
     *                      (set 0 to disable this feature, set -1 to save infinite temp files)
     */
    enum IniKeys
    {
        eabi_binfile_path          = 0,
        RecentROMPath_0            = 1,
        RecentROMPath_1            = 2,
        RecentROMPath_2            = 3,
        RecentROMPath_3            = 4,
        RecentROMPath_4            = 5,
        EditorThemeId              = 6,
        RollingSaveLimit           = 7,
    };

    // Static Key QString set
    // clang-format off
    static QVector<QString> KeyStringSet =
    {
        "patch/eabi_binfile_path",
        "history/RecentROMPath_0",
        "history/RecentROMPath_1",
        "history/RecentROMPath_2",
        "history/RecentROMPath_3",
        "history/RecentROMPath_4",
        "settings/EditorThemeId",
        "settings/RollingSaveLimit",
    };
    // clang-format on

    // functions
    void InitProgramSetupPath(QCoreApplication &application);
    void SetKey(enum IniKeys key, QString value);
    QString GetKey(enum IniKeys key);
    // ---------------------------------------------------------------
    // Project Settings
    extern QString ProjectSettingFilePath;

    // functions
    void ParseProjectSettings();

}; // namespace SettingsUtils

#endif // SETTINGSUTILS_H
