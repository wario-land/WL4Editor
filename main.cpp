#include "Dialog/DoorConfigDialog.h"
#include "Dialog/RoomConfigDialog.h"
#include "Dialog/PatchEditDialog.h"
#include "Dialog/CreditsEditDialog.h"
#include "DockWidget/CameraControlDockWidget.h"
#include "LevelComponents/Level.h"
#include "ROMUtils.h"
#include "SettingsUtils.h"
#include "WL4Application.h"
#include "WL4EditorWindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QMessageBox>
#include <cstring>
#include <fstream>
#include <iostream>
#include "Compress.h"

/// <summary>
/// Load a ROM file into the data array in ROMUtils.cpp.
/// </summary>
/// <param name="filePath">
/// The path to the file that will be read.
/// </param>
QString LoadROMFile(QString filePath)
{
    // Read ROM file into current file array
    QFile file(filePath);
    file.open(QIODevice::ReadOnly);

    // To check OPEN file
    int length;
    if (!file.isOpen())
    {
        file.close();
        return QObject::tr("Cannot open file!") + filePath;
    }
    if ((length = (int) file.size()) < 0x800000)
    {
        file.close();
        return QObject::tr("The file size is smaller than 8 MB!");
    }

    // Read data
    unsigned char *ROMAddr = new unsigned char[length];
    file.read((char *) ROMAddr, length);
    file.close();

    // To check ROM correct
    if (strncmp((const char *) (ROMAddr + 0xA0), "WARIOLAND", 9))
    { // if loaded a wrong ROM
        delete[] ROMAddr;
        return QObject::tr("The rom header indicates that it is not a WL4 rom!");
    }
    if (strncmp((const char *) ROMAddr, "\x2E\x00\x00", 3))
    { // if the first 4 bytes are different
        delete[] ROMAddr;
        return QObject::tr("The rom you load has a Nintendo intro which will cause problems in the editor! "
                           "Please load a rom without intro instead.");
    }

    ROMUtils::CurrentFileSize = length;
    ROMUtils::ROMFilePath = filePath;
    ROMUtils::CurrentFile = (unsigned char *) ROMAddr;
    return "";
}

/// <summary>
/// Perform all static class initializations.
/// </summary>
static void StaticInitialization_BeforeROMLoading()
{
    RoomConfigDialog::StaticComboBoxesInitialization();
    DoorConfigDialog::StaticInitialization();
    CameraControlDockWidget::StaticInitialization();
    CreditsEditDialog::StaticInitialization();
    PatchEditDialog::StaticComboBoxesInitialization();
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

    // High DPI support, perhaps won't work in all the OS but better than nothing
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication application(argc, argv);
    SettingsUtils::InitProgramSetupPath(application);
    application.setWindowIcon(QIcon("./images/icon.ico"));
    WL4EditorWindow window;
    window.show();

    return application.exec();
}
