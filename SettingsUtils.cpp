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
        QColor cameraboxcolor = QColor(0xFF, 0, 0);
        QColor cameraboxcolor_extended = QColor(0, 0xFF, 0);
        QColor doorboxcolor = QColor(0, 0, 0xFF);
        QColor doorboxcolor_filling = QColor(0, 0, 0xFF, 0x5F);
        QColor doorboxcolorselected = QColor(0, 0xFF, 0xFF);  // cyan
        QColor doorboxcolorselected_filling = QColor(0, 0xFF, 0xFF, 0x5F);
        QColor entityboxcolor = QColor(0xFF, 0xFF, 0);
        QColor entityboxcolorselected = QColor(0xFF, 0x7F, 0);
        QColor extraEventIDhintboxcolor = QColor(255, 153, 18, 0xFF);
        QVector<int> extraEventIDhinteventids = {0x0C, 0x0E, 0x20, 0x22, 0x2E, 0x5C};
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
        QString key = "test-key";
        if (list.contains(key) && jsonObj[key].isBool())
        {
            // accept and apply the correct key - value pair
            singleton->GetOutputWidgetPtr()->PrintString("project setting file loaded correctly.");
        }
        // modify the key and value to the correct default contents, and write the value we read to the new json, so it is always clean
        json.insert(key, true);

        // camera box render color settings
        key = "camerabox_render_color";
        bool okay = false;
        if (list.contains(key) && jsonObj[key].isString())
        {
            if (QColor tmpcolor = string2color(jsonObj[key].toString(), okay) ; okay)
            {
                projectSettings::cameraboxcolor = tmpcolor;
            }

        }
        json.insert(key, color2string(projectSettings::cameraboxcolor));
        key = "camerabox_render_color_extended";
        if (list.contains(key) && jsonObj[key].isString())
        {
            if (QColor tmpcolor = string2color(jsonObj[key].toString(), okay) ; okay)
            {
                projectSettings::cameraboxcolor_extended = tmpcolor;
            }
        }
        json.insert(key, color2string(projectSettings::cameraboxcolor_extended));

        // door box render color settings
        key = "doorbox_render_color";
        if (list.contains(key) && jsonObj[key].isString())
        {
            if (QColor tmpcolor = string2color(jsonObj[key].toString(), okay) ; okay)
            {
                projectSettings::doorboxcolor = tmpcolor;
            }
        }
        json.insert(key, color2string(projectSettings::doorboxcolor));
        key = "doorbox_filling_render_color";
        if (list.contains(key) && jsonObj[key].isString())
        {
            if (QColor tmpcolor = string2color(jsonObj[key].toString(), okay) ; okay)
            {
                projectSettings::doorboxcolor_filling = tmpcolor;
            }
        }
        json.insert(key, color2string(projectSettings::doorboxcolor_filling));
        key = "doorbox_selected_render_color";
        if (list.contains(key) && jsonObj[key].isString())
        {
            if (QColor tmpcolor = string2color(jsonObj[key].toString(), okay) ; okay)
            {
                projectSettings::doorboxcolorselected = tmpcolor;
            }
        }
        json.insert(key, color2string(projectSettings::doorboxcolorselected));
        key = "doorbox_selected_filling_render_color";
        if (list.contains(key) && jsonObj[key].isString())
        {
            if (QColor tmpcolor = string2color(jsonObj[key].toString(), okay) ; okay)
            {
                projectSettings::doorboxcolorselected_filling = tmpcolor;
            }
        }
        json.insert(key, color2string(projectSettings::doorboxcolorselected_filling));

        // entity box render color settings
        key = "entitybox_render_color";
        if (list.contains(key) && jsonObj[key].isString())
        {
            if (QColor tmpcolor = string2color(jsonObj[key].toString(), okay) ; okay)
            {
                projectSettings::entityboxcolor = tmpcolor;
            }
        }
        json.insert(key, color2string(projectSettings::entityboxcolor));
        key = "entitybox_selected_render_color";
        if (list.contains(key) && jsonObj[key].isString())
        {
            if (QColor tmpcolor = string2color(jsonObj[key].toString(), okay) ; okay)
            {
                projectSettings::entityboxcolorselected = tmpcolor;
            }
        }
        json.insert(key, color2string(projectSettings::entityboxcolorselected));

        // extra layer hint box render settings
        key = "extraEventIDhintbox_render_color";
        if (list.contains(key) && jsonObj[key].isString())
        {
            if (QColor tmpcolor = string2color(jsonObj[key].toString(), okay) ; okay)
            {
                projectSettings::extraEventIDhintboxcolor = tmpcolor;
            }
        }
        json.insert(key, color2string(projectSettings::extraEventIDhintboxcolor));
        key = "extraEventIDhintbox_event_indexes";
        if (list.contains(key) && jsonObj[key].isString())
        {
            QString tmpstr = jsonObj[key].toString();
            if (auto datavec = string2intvec(tmpstr) ; datavec.size())
            {
                projectSettings::extraEventIDhinteventids = datavec;
            }
        }
        json.insert(key, intvec2string(projectSettings::extraEventIDhinteventids));

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
    QColor string2color(QString data, bool &ok)
    {
        // use RGB888 in both QColor and int
        QVector<int> intvec = string2intvec(data);
        if (intvec.size() < 3)
        {
            // return non-transparent black if the format is incorrect
            ok = false;
            return QColor(0, 0, 0, 0xFF);
        }
        unsigned int r = intvec[0];
        unsigned int g = intvec[1];
        unsigned int b = intvec[2];
        unsigned int a = 0xFF;
        if (intvec.size() > 3)
        {
            a = intvec[3];
        }
        ok = true;
        return QColor(r, g, b, a);
    }

    /// <summary>
    /// QColor to int
    /// </summary>
    QString color2string(QColor &color)
    {
        // use RGB888 in both QColor and int
        return QString::number(color.red(), 16) + "," + QString::number(color.green(), 16) +
                "," + QString::number(color.blue(), 16) + "," + QString::number(color.alpha(), 16);
    }

    /// <summary>
    /// QString (should always use hex when representing values) to a QVector of int
    /// </summary>
    QVector<int> string2intvec(QString &data)
    {
        QStringList datalist = data.split(QChar(','));
        QVector<int> result;
        for (auto &substr : datalist)
        {
            result.append(substr.toInt(nullptr, 16));
        }
        return result;
    }

    /// <summary>
    /// A QVector of int to a QString (should always use hex when representing values)
    /// </summary>
    QString intvec2string(QVector<int> &intvec)
    {
        QString result;
        for (auto &substr : intvec)
        {
            result += QString::number(substr, 16) + ",";
        }
        result.chop(1); // chop the last ","
        return result;
    }

} // namespace SettingsUtils
