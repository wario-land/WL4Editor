#include "SettingsUtils.h"

namespace SettingsUtils
{
    // Global variables
    QString ProgramSettingFilePath;

    /// <summary>
    /// Initialize ini file path and check the ini file.
    /// </summary>
    void InitProgramSetupPath(QCoreApplication &application)
    {
        ProgramSettingFilePath = QCoreApplication::applicationDirPath();
        ProgramSettingFilePath += QString("/WL4Editor.ini");

        // Check INI file
        QFileInfo fileInfo(ProgramSettingFilePath);
        if (!fileInfo.isFile())
        {
            QSettings WL4EditorIni(ProgramSettingFilePath, QSettings::IniFormat);
            for (const auto &i : KeyStringSet)
                WL4EditorIni.setValue(i, "");
        }
    }

    /// <summary>
    /// Set a Key in the ini file.
    /// </summary>
    /// <param name="key">
    /// The key whose value to set.
    /// </param>
    /// <param name="value">
    /// The new value of the Key.
    /// </param>
    void SetKey(IniKeys key, const QString &value)
    {
        QSettings WL4EditorIni(ProgramSettingFilePath, QSettings::IniFormat);
        WL4EditorIni.setValue(KeyStringSet[key], value);
    }

    /// <summary>
    /// Get a Key value from the ini file.
    /// </summary>
    /// <param name="key">
    /// The key whose value to get.
    /// </param>
    /// <returns>
    /// The value of the key.
    /// </returns>
    QString GetKey(IniKeys key)
    {
        QSettings WL4EditorIni(ProgramSettingFilePath, QSettings::IniFormat);
        return WL4EditorIni.value(KeyStringSet[key]).toString();
    }
} // namespace SettingsUtils
