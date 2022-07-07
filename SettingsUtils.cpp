#include "SettingsUtils.h"
#include "ROMUtils.h"
#include "ScriptInterface.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QJsonParseError>
#include <QJsonDocument>

#include "WL4EditorWindow.h"
extern WL4EditorWindow *singleton;

namespace SettingsUtils
{
    // Global variables
    QString ProgramSettingFilePath;
    QString ProjectSettingFilePath;

    /// <summary>
    /// Initialize editor's ini file path and check the ini file.
    /// </summary>
    void InitProgramSetupPath(QCoreApplication &application)
    {
        ProgramSettingFilePath = application.applicationDirPath();
        ProgramSettingFilePath += QString("/WL4Editor.ini");

        // Check INI file
        QFileInfo fileInfo(ProgramSettingFilePath);
        if (!fileInfo.isFile())
        {
            QSettings WL4EditorIni(ProgramSettingFilePath, QSettings::IniFormat);
            for (int i = 0; i < KeyStringSet.size(); i++)
                WL4EditorIni.setValue(KeyStringSet[i], "");
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
    void SetKey(IniKeys key, QString value)
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

    /// <summary>
    /// Load and parse the current project setting file
    /// create the file if it does not exist.
    /// </summary>
    void LoadProjectSettings()
    {
        // generate json for new project
        QJsonObject json;
        QFileInfo curROMFileInfo(ROMUtils::ROMFileMetadata->FilePath);
        QString projectSettingFilePath = QFileInfo(curROMFileInfo).dir().path() +
                "/" + curROMFileInfo.completeBaseName() + ".json";

        QFileInfo fileInfo(projectSettingFilePath);
        if (!fileInfo.isFile())
        {
            // create file
            QFile newFile(projectSettingFilePath);
            newFile.open(QIODevice::ReadWrite);
            newFile.write(QJsonDocument(json).toJson());
            newFile.close();
        }
        ProjectSettingFilePath = projectSettingFilePath;

        // Read file
        QJsonParseError json_error;
        QJsonObject jsonObj;
        QFile loadFile(projectSettingFilePath);
        loadFile.open(QIODevice::ReadWrite);
        QByteArray saveData = loadFile.readAll();
        loadFile.close();
        QJsonDocument settingJsonDoc(QJsonDocument::fromJson(saveData, &json_error));
        if (json_error.error == QJsonParseError::NoError)
        {
            if (settingJsonDoc.isObject())
            {
                jsonObj = settingJsonDoc.object();
            }
        }

        // Load settings one by one, and send them into the runtime global variables of WL4Editor
        QStringList list = jsonObj.keys();

        // example to load key value pair
        if (!list.contains("test-key"))
        {
            if (jsonObj["test-key"].isBool())
            {
                // accept and apply the correct key - value pair
                singleton->GetOutputWidgetPtr()->PrintString("test-key: true");
            }
            else
            {
                goto test_key_insert;
            }
        }
        else
        {
test_key_insert:
            // modify the key and value to the correct default contents
            json.insert("test-key", true);
        }

        // TODO: add more project settings

        // write the json obj back to the file to modify incorrect settings
        SaveProjectSettings(json);
    }

    void SaveProjectSettings(QJsonObject &jsonobj)
    {
        // test save
        QFile newFile(ProjectSettingFilePath);
        newFile.open(QIODevice::WriteOnly);
        newFile.write(QJsonDocument(jsonobj).toJson());
        newFile.close();
    }

} // namespace SettingsUtils
