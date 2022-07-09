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


    // definition of project setting global variables
    namespace projectSettings
    {
        QColor cameraboxcolor = QColor(255, 0, 0);
        QColor cameraboxcolor_extended = QColor(0, 255, 0);
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
            newFile.open(QIODevice::WriteOnly);
            newFile.write(QJsonDocument(json).toJson());
            newFile.close();
        }
        ProjectSettingFilePath = projectSettingFilePath;

        // Read file
        QJsonParseError json_error;
        QJsonObject jsonObj;
        QFile loadFile(projectSettingFilePath);
        loadFile.open(QIODevice::ReadOnly);
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

        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
        // the logic to maintain project settings
        // example to load key value pair
        if (list.contains("test-key") && jsonObj["test-key"].isBool())
        {
            // accept and apply the correct key - value pair
            singleton->GetOutputWidgetPtr()->PrintString("project setting file loaded correctly.");
        }
        // modify the key and value to the correct default contents, and write the value we read to the new json, so it is always clean
        json.insert("test-key", true);

        // camera box render color settings
        if (list.contains("camerabox_render_color") && jsonObj["camerabox_render_color"].isString())
        {
            projectSettings::cameraboxcolor = string2color(jsonObj["camerabox_render_color"].toString());
        }
        json.insert("camerabox_render_color", color2string(projectSettings::cameraboxcolor));

        // camera box render color extended settings
        if (list.contains("camerabox_render_color_extended") && jsonObj["camerabox_render_color_extended"].isString())
        {
            projectSettings::cameraboxcolor_extended = string2color(jsonObj["camerabox_render_color_extended"].toString());
        }
        json.insert("camerabox_render_color_extended", color2string(projectSettings::cameraboxcolor_extended));


        // TODO: add more project settings

        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------

        // write the json obj back to the file to modify incorrect settings
        SaveProjectSettings(json);
    }

    void SaveProjectSettings(QJsonObject &jsonobj)
    {
        // test save
        QFile newFile(ProjectSettingFilePath);
        newFile.open(QIODevice::WriteOnly);
        newFile.write(QJsonDocument(jsonobj).toJson(QJsonDocument::Indented));
        newFile.close();
    }

    /// <summary>
    /// int to QColor
    /// </summary>
    QColor string2color(QString data)
    {
        // use RGB888 in both QColor and int
        QStringList colordata = data.split(QChar(','));
        unsigned int r = colordata[0].toUInt(nullptr, 16);
        unsigned int g = colordata[1].toUInt(nullptr, 16);
        unsigned int b = colordata[2].toUInt(nullptr, 16);
        return QColor(r, g, b);
    }

    /// <summary>
    /// QColor to int
    /// </summary>
    QString color2string(QColor &color)
    {
        // use RGB888 in both QColor and int
        return QString::number(color.red(), 16) + "," + QString::number(color.green(), 16) + "," + QString::number(color.blue(), 16);
    }

} // namespace SettingsUtils
