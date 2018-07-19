#include "WL4EditorWindow.h"
#include <QApplication>
#include <QMessageBox>
#include <fstream>
#include "ROMUtils.h"
#include "LevelComponents/Level.h"
#include "Dialog/RoomConfigDialog.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <lmcons.h>
#endif

#ifdef linux
#include <unistd.h>
#endif

extern int selectedRoom;

bool LoadROMFile(std::string filePath)
{
    // Read ROM file into current file array
    std::ifstream ifs(filePath, std::ios::binary | std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();
    // To check ROM size
    int length = pos;
    if(length<=0xB0)
    {
		ifs.close();
        return false;
    }
    unsigned char * ROMAddr = new unsigned char[length];
    ifs.seekg(0, std::ios::beg);
    ifs.read((char*) ROMAddr, length);
    ifs.close();
    // To check ROM correct
    if(strncmp((const char*)(ROMAddr + 0xA0), "WARIOLANDE", 10)!=0)
    {//if loaded a wrong ROM
        delete[] ROMAddr;
        return false;
    }
    else
    {
        ROMUtils::CurrentFileSize = length;
        strcpy(ROMUtils::ROMFilePath, filePath.c_str());
        ROMUtils::CurrentFile=(unsigned char*)ROMAddr;
        return true;
    }

}

static void AllStaticInitialization()
{
    RoomConfigDialog::StaticInitialization();
}

/// <summary>
/// Perform static initializations, and then create the main window for the application.
/// </summary>
/// <param name="argc">Number of command line arguments.</param>
/// <param name="argv">Array of command line arguments.</param>
int main(int argc, char *argv[])
{
    AllStaticInitialization();

    QApplication a(argc, argv);
    WL4EditorWindow w;
    w.show();

    // Quickly test or debug by automatically loading the ROM without UI
    //-------------------------------------------------------------------
    char *username;
#ifdef _WIN32
    TCHAR usernameTC[UNLEN + 1];
    DWORD size = UNLEN + 1;
    GetUserName((TCHAR*)usernameTC, &size);
    username = new char[UNLEN + 1];
    for(int i = 0; i < UNLEN; ++i)
    {
        if(!usernameTC[i])
        {
            username[i] = '\0';
            break;
        }
        username[i] = (char) (usernameTC[i] & 0xDF); // Makes username uppercase
    }
#endif
#ifdef linux
    username = new char[33]; // Maximum length is 32 (plus 1 for null termination)
    getlogin_r(username, 33);
    for(int i = 0; i < 32; ++i)
    {
        username[i] &= '\xDF'; // Make username uppercase
    }
#endif
    if(!strncmp(username, "ANDREW", strlen(username))) // Goldensunboy
    {
	/*
        // Andrew's tests
        std::string filePath = "C:\\Users\\Andrew\\Desktop\\WL4.gba";
        LoadROMFile(filePath);

        // Load a level
        CurrentLevel = new LevelComponents::Level(LevelComponents::EmeraldPassage, LevelComponents::SecondLevel);

        // Render the screen
        w.LoadRoom();

        std::cout << CurrentLevel->GetDoors().size() << std::endl;
        std::cout << "\"" << CurrentLevel->GetLevelName() << "\"" << std::endl;
	*/
    }
    else if(!strncmp(username, "ADMINISTRATOR", strlen(username))) // SSP
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

    return a.exec();
}
