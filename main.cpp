#include "WL4EditorWindow.h"
#include <QApplication>

#include <fstream>
#include "ROMUtils.h"

#include "LevelComponents/Level.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <lmcons.h>
#endif

#ifdef linux
#include <unistd.h>
#endif

extern LevelComponents::Level *CurrentLevel;
extern int selectedRoom;

void LoadROMFile(std::string filePath)
{
    // Read ROM file into current file array
    std::ifstream ifs(filePath, std::ios::binary | std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();
    int length = pos;
    ROMUtils::CurrentFileSize = length;
    ROMUtils::CurrentFile = new unsigned char[length];
    strcpy(ROMUtils::ROMFilePath, filePath.c_str());
    ifs.seekg(0, std::ios::beg);
    ifs.read((char*) ROMUtils::CurrentFile, length);
    ifs.close();
}

int main(int argc, char *argv[])
{
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
    delete username;
    //-------------------------------------------------------------------

    return a.exec();
}
