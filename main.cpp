#include "Dialog/DoorConfigDialog.h"
#include "Dialog/RoomConfigDialog.h"
#include "Dialog/PatchEditDialog.h"
#include "Dialog/CreditsEditDialog.h"
#include "DockWidget/CameraControlDockWidget.h"
#include "Dialog/GraphicManagerDialog.h"
#include "Dialog/SpritesEditorDialog.h"
#include "WL4EditorWindow.h"
#include "SettingsUtils.h"

#include <QApplication>

/// <summary>
/// Perform all static class initializations.
/// </summary>
static void StaticInitialization_BeforeROMLoading()
{
    ROMUtils::StaticInitialization();
    RoomConfigDialog::StaticComboBoxesInitialization();
    DoorConfigDialog::StaticInitialization();
    CameraControlDockWidget::StaticInitialization();
    CreditsEditDialog::StaticInitialization();
    PatchEditDialog::StaticComboBoxesInitialization();
    GraphicManagerDialog::StaticInitialization();
    SpritesEditorDialog::StaticInitialization();
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

    // High DPI support in Qt6 will cause problems in position calculation without using devicePixelRatio
    // use this to deal with the problems
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);

    QApplication application(argc, argv);
    SettingsUtils::InitProgramSetupPath(application);
    application.setWindowIcon(QIcon("./images/icon.ico"));
    WL4EditorWindow window;
    // window.show();

    // Quickly test or debug by automatically loading the ROM without UI used by GSB
    // comment by ssp: a real man will never use this code (doge)
    //-------------------------------------------------------------------
//#ifdef _WIN32 // Windows
//    QString restoreFilePath = "C:\\Users\\Andrew\\Desktop\\WL4 PW 2.gba";
//    QString filePath = "C:\\Users\\Andrew\\Desktop\\WL4 PW.gba";
//#else // Linux
//    QString restoreFilePath = "/home/andrew/Desktop/WL4 2.gba";
//    QString filePath = "/home/andrew/Desktop/WL4.gba";
//#endif
//    QFile restoreFile(restoreFilePath);
//    QFile testFile(filePath);
//    testFile.remove();
//    restoreFile.copy(filePath);
//    if(testFile.exists())
//    {
//        window.LoadROMDataFromFile(filePath);
//    }
//    testFile.close();
    //-------------------------------------------------------------------

    // Parse command-line flags
    bool startMcp = false;
    QString romPath;
    for (int i = 1; i < argc; i++)
    {
        QString arg(argv[i]);
        if (arg == "--mcp")
        {
            startMcp = true;
        }
        else if (arg == "--rom" && i + 1 < argc)
        {
            romPath = QString::fromLocal8Bit(argv[++i]);
            ROMUtils::FormatPathSeperators(romPath);
        }
        else if (!arg.startsWith('-') && romPath.isEmpty())
        {
            // Positional ROM path (backward compatible)
            romPath = arg;
            ROMUtils::FormatPathSeperators(romPath);
        }
    }

    // In MCP mode, don't show the GUI if not needed
    if (!startMcp)
        window.show();

    // Load ROM if specified
    if (!romPath.isEmpty())
    {
        if (QFile::exists(romPath) && romPath.endsWith(".gba", Qt::CaseInsensitive))
        {
            window.LoadROMDataFromFile(romPath);
            // Show window after ROM is loaded unless in headless MCP mode
            if (!startMcp)
                window.show();
            else
                window.show(); // Still show GUI in MCP mode so user can see the editor
        }
        else if (!startMcp)
        {
            QMessageBox::critical(&window, "WL4Editor", QObject::tr("Wrong command line parameter:\n"
                                                                    "it needs to be a valid gba file path."));
        }
    }
    else if (!startMcp)
    {
        window.show();
    }

    // Auto-start MCP server if --mcp flag was passed and ROM loaded successfully
    if (startMcp && window.FirstROMIsLoaded())
    {
        window.StartMCPServer();
    }

    return application.exec();
}
