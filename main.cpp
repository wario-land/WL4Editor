#include "WL4EditorWindow.h"
#include <QApplication>

#include <fstream>
#include "ROMUtils.h"

//--------[testing headers]--------
#include "LevelComponents/Level.h"
#include <iostream>
#include <windows.h>
#include <lmcons.h>
#include <cstring>
//---------------------------------

LevelComponents::Level *CurrentLevel;

void LoadROMFile(std::string filePath)
{
    // Read ROM file into current file array
    std::ifstream ifs(filePath, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();
    int length = pos;
    ROMUtils::CurrentFile = new unsigned char[length];
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
    TCHAR usernameTC[UNLEN + 1];
    DWORD size = UNLEN + 1;
    GetUserName((TCHAR*)usernameTC, &size);
    char username[UNLEN + 1];
    for(int i = 0; i < UNLEN; ++i)
    {
        if(!usernameTC[i])
        {
            username[i] = '\0';
            break;
        }
        username[i] = (char) (usernameTC[i] & 0xFF);
    }
    if(!strncmp(username, "Andrew", UNLEN)) // Goldensunboy
    {
        // Andrew's tests
        std::string filePath = "C:\\Users\\Andrew\\Desktop\\WL4.gba";
        LoadROMFile(filePath);

        // Load level (0, 0)
        CurrentLevel = new LevelComponents::Level(LevelComponents::EntryPassage, LevelComponents::FirstLevel);

        // Render the screen
        w.RenderScreen(CurrentLevel->GetRooms()[0]);

        std::cout << CurrentLevel->GetDoors().size() << std::endl;
        std::cout << "\"" << CurrentLevel->GetLevelName() << "\"" << std::endl;
    }
    else if(!strncmp(username, "Administrator", UNLEN)) // SSP
    {
        // TODO
    }
    //-------------------------------------------------------------------

    return a.exec();
}
