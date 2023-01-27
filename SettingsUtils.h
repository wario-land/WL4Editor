#ifndef SETTINGSUTILS_H
#define SETTINGSUTILS_H

#include <map>
#include <QCoreApplication>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QColor>

namespace SettingsUtils
{
    // Global const
    static const unsigned int RecentFileNum = 5;

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
     * RecentScriptPath_0 = path
     * RecentScriptPath_1 = path
     * RecentScriptPath_2 = path
     * RecentScriptPath_3 = path
     * RecentScriptPath_4 = path
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
        RecentScriptPath_0         = 9,
        RecentScriptPath_1         = 10,
        RecentScriptPath_2         = 11,
        RecentScriptPath_3         = 12,
        RecentScriptPath_4         = 13,
        RecentROM_0_RecentRoom_id  = 14,
        RecentROM_1_RecentRoom_id  = 15,
        RecentROM_2_RecentRoom_id  = 16,
        RecentROM_3_RecentRoom_id  = 17,
        RecentROM_4_RecentRoom_id  = 18,
        RecentROM_0_RecentLevel_id = 19,
        RecentROM_1_RecentLevel_id = 20,
        RecentROM_2_RecentLevel_id = 21,
        RecentROM_3_RecentLevel_id = 22,
        RecentROM_4_RecentLevel_id = 23,
        RecentROM_0_RecentPassage_id = 24,
        RecentROM_1_RecentPassage_id = 25,
        RecentROM_2_RecentPassage_id = 26,
        RecentROM_3_RecentPassage_id = 27,
        RecentROM_4_RecentPassage_id = 28,
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
        "history/RecentScriptPath_0",
        "history/RecentScriptPath_1",
        "history/RecentScriptPath_2",
        "history/RecentScriptPath_3",
        "history/RecentScriptPath_4",
        "history/RecentROM_0_RecentRoom_id",
        "history/RecentROM_1_RecentRoom_id",
        "history/RecentROM_2_RecentRoom_id",
        "history/RecentROM_3_RecentRoom_id",
        "history/RecentROM_4_RecentRoom_id",
        "history/RecentROM_0_RecentLevel_id",
        "history/RecentROM_1_RecentLevel_id",
        "history/RecentROM_2_RecentLevel_id",
        "history/RecentROM_3_RecentLevel_id",
        "history/RecentROM_4_RecentLevel_id",
        "history/RecentROM_0_RecentPassage_id",
        "history/RecentROM_1_RecentPassage_id",
        "history/RecentROM_2_RecentPassage_id",
        "history/RecentROM_3_RecentPassage_id",
        "history/RecentROM_4_RecentPassage_id",
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
        extern QStringList extraEventIDhintChars;
        extern QColor extraTerrainIDhintboxcolor;
        extern QVector<int> extraTerrainIDhintTerrainids;
        extern QStringList extraTerrainIDhintChars;
        extern std::map<int, QString> bgmNameList;
        extern std::map<int, QVector<unsigned short> > cusomOAMdata;
        extern QString customHintRenderJSFilePath;
    }

    // functions
    void LoadProjectSettings();
    void SaveProjectSettings(QJsonObject &jsonobj);

    // helper functions
    template <typename TypeInt> QVector<TypeInt> string2intvec(QString &data);
    QString intvec2string(QVector<int> &intvec);
    QColor string2color(QString data, bool &ok);
    QString color2string(QColor color);
    QStringList string2strlist(QString data, int size_per_str = 1);
    QString strlist2string(QStringList datalist, int size_per_str = 1);

}; // namespace SettingsUtils

#endif // SETTINGSUTILS_H
