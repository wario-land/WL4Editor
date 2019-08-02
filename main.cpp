#include "Dialog/DoorConfigDialog.h"
#include "Dialog/RoomConfigDialog.h"
#include "DockWidget/CameraControlDockWidget.h"
#include "LevelComponents/Level.h"
#include "ROMUtils.h"
#include "SettingsUtils.h"
#include "WL4Application.h"
#include "WL4EditorWindow.h"
#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <cstring>
#include <fstream>
#include <iostream>

#ifdef _WIN32
#include <lmcons.h>
#include <windows.h>
#if _MSC_VER && !__INTEL_COMPILER
#pragma comment(lib, "Advapi32.lib")
#endif // _MSC_VER
#else  // _WIN32 (else linux)
#include <unistd.h>
#endif

#include "Compress.h"

extern int selectedRoom;

/// <summary>
/// Load a ROM file into the data array in ROMUtils.cpp.
/// </summary>
/// <param name="filePath">
/// The path to the file that will be read.
/// </param>
bool LoadROMFile(QString filePath)
{
    // Read ROM file into current file array
    QFile file(filePath);
    file.open(QIODevice::ReadOnly);

    // To check OPEN file
    int length;
    if (!file.isOpen() || (length = (int) file.size()) <= 0xB0)
    {
        file.close();
        return false;
    }

    // Read data
    unsigned char *ROMAddr = new unsigned char[length];
    file.read((char *) ROMAddr, length);
    file.close();

    // To check ROM correct
    if (strncmp((const char *) (ROMAddr + 0xA0), "WARIOLANDE", 10))
    { // if loaded a wrong ROM
        delete[] ROMAddr;
        return false;
    }
    else
    {
        ROMUtils::CurrentFileSize = length;
        ROMUtils::ROMFilePath = filePath;
        ROMUtils::CurrentFile = (unsigned char *) ROMAddr;
        return true;
    }
}

/// <summary>
/// Perform all static class initializations.
/// </summary>
static void StaticInitialization_BeforeROMLoading()
{
    RoomConfigDialog::StaticComboBoxesInitialization();
    DoorConfigDialog::StaticInitialization();
    CameraControlDockWidget::StaticInitialization();
    SettingsUtils::InitProgramSetupPath();
}

/// <summary>
/// Perform static initializations, and then create the main window for the application.
/// </summary>
/// <param name="argc">
/// Number of command line arguments.
/// </param>
/// <param name="argv">
/// Array of command line arguments.
/// </param>
int main(int argc, char *argv[])
{
    StaticInitialization_BeforeROMLoading();

    QApplication application(argc, argv);
    WL4EditorWindow window;
    window.show();

    // Quickly test or debug by automatically loading the ROM without UI
    //-------------------------------------------------------------------
    char *username;
#ifdef _WIN32
    TCHAR usernameTC[UNLEN + 1];
    DWORD size = UNLEN + 1;
    GetUserName((TCHAR *) usernameTC, &size);
    username = new char[UNLEN + 1];
    for (int i = 0; i < UNLEN; ++i)
    {
        if (!usernameTC[i])
        {
            username[i] = '\0';
            break;
        }
        username[i] = (char) (usernameTC[i] & 0xDF); // Makes username uppercase
    }
#else
    username = new char[33]; // Maximum length is 32 (plus 1 for null termination)
    getlogin_r(username, 33);
    for (int i = 0; i < 32; ++i)
    {
        username[i] &= '\xDF'; // Make username uppercase
    }
#endif
    if (!strncmp(username, "ANDREW", strlen(username))) // Goldensunboy
    {
        // Andrew's tests
        extern QString dialogInitialPath;
        dialogInitialPath = QString("C:\\Users\\Andrew\\Desktop\\WL4.gba");
        window.OpenROM();
    }
    else if (!strncmp(username, "ADMINISTRATOR", strlen(username))) // SSP
    {
        /*
        std::string filePath = "E:\\Wario Harker\\0169 - Wario Land 4.gba";
        LoadROMFile(filePath);

        // Load level (0, 0)
        CurrentLevel = new LevelComponents::Level(LevelComponents::EmeraldPassage, LevelComponents::FourthLevel);

        // Render the screen
        w.RenderScreen(CurrentLevel->GetRooms()[0]);

        std::cout << CurrentLevel->GetDoors().size() << std::endl;
        std::cout << "\"" << CurrentLevel->GetLevelName() << "\"" << std::endl;
        */
    }
    delete[] username;
    //-------------------------------------------------------------------

    return application.exec();
}
