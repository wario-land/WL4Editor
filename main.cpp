#include "Dialog/DoorConfigDialog.h"
#include "Dialog/RoomConfigDialog.h"
#include "Dialog/PatchEditDialog.h"
#include "Dialog/CreditsEditDialog.h"
#include "DockWidget/CameraControlDockWidget.h"
#include "Dialog/GraphicManagerDialog.h"
#include "Dialog/SpritesEditorDialog.h"
#include "WL4EditorWindow.h"

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
    window.show();

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

    // load the ROM file from argv if exists
    if (argc > 1)
    {
        QString filePath(argv[1]);
        ROMUtils::FormatPathSeperators(filePath);
        if(QFile::exists(filePath) && filePath.endsWith(".gba", Qt::CaseInsensitive))
        {
            window.LoadROMDataFromFile(filePath);
        }
        else
        {
            QMessageBox::critical(&window, "WL4Editor", QObject::tr("Wrong command line parameter:\n"
                                                                    "it needs to be a valid gba file path."));
        }
    }

    return application.exec();
}
