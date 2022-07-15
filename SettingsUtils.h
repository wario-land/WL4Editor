#ifndef SETTINGSUTILS_H
#define SETTINGSUTILS_H

#include <QCoreApplication>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QColor>

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
     * OpenRomInitPath  = path
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
        OpenRomInitPath            = 8,
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
        "settings/OpenRomInitPath",
    };
    // clang-format on

    // functions
    void InitProgramSetupPath(QCoreApplication &application);
    void SetKey(enum IniKeys key, QString value);
    QString GetKey(enum IniKeys key);
    // ---------------------------------------------------------------
    // Project Settings
    extern QString ProjectSettingFilePath;

    // expose the global variables
    namespace projectSettings
    {
        extern QColor cameraboxcolor;
        extern QColor cameraboxcolor_extended;
        extern QColor doorboxcolor;
        extern QColor doorboxcolor_filling;
        extern QColor doorboxcolorselected;
        extern QColor doorboxcolorselected_filling;
        extern QColor entityboxcolor;
        extern QColor entityboxcolorselected;
        extern QColor extraEventIDhintboxcolor;
        extern QVector<int> extraEventIDhinteventids;
        extern QColor extraTerrainIDhintboxcolor;
        extern QVector<int> extraTerrainIDhintTerrainids;
    }

    // functions
    void LoadProjectSettings();
    void SaveProjectSettings(QJsonObject &jsonobj);

    // helper functions
    QVector<int> string2intvec(QString &data);
    QString intvec2string(QVector<int> &intvec);
    QColor string2color(QString data, bool &ok);
    QString color2string(QColor &color);

}; // namespace SettingsUtils

#endif // SETTINGSUTILS_H
