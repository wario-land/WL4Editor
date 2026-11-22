#include "Dialog/DoorConfigDialog.h"
#include "Dialog/RoomConfigDialog.h"
#include "Dialog/PatchEditDialog.h"
#include "Dialog/CreditsEditDialog.h"
#include "DockWidget/CameraControlDockWidget.h"
#include "Dialog/GraphicManagerDialog.h"
#include "Dialog/SpritesEditorDialog.h"
#include "WL4EditorWindow.h"
#include "SettingsUtils.h"
#include "ROMUtils.h"
#include "Sound/SoundMPlayCompiler.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>

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

    if (argc >= 4 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--sound-compile"))
    {
        const QString sourcePath = QString::fromLocal8Bit(argv[2]);
        const QString outputPath = QString::fromLocal8Bit(argv[3]);
        quint32 dataAddress = 0;
        if (argc >= 5)
        {
            bool ok = false;
            dataAddress = QString::fromLocal8Bit(argv[4]).toUInt(&ok, 0);
            if (!ok)
            {
                QTextStream(stderr) << "Invalid data address: " << argv[4] << '\n';
                return 2;
            }
        }

        QByteArray binary;
        QString error;
        QString normalized;
        error = Sound::SoundMPlayCompiler::compileFile(sourcePath, dataAddress, binary, &normalized);
        if (error.isEmpty())
        {
            QFile output(outputPath);
            if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate))
                error = QStringLiteral("Cannot write %1: %2").arg(outputPath, output.errorString());
            else
            {
                output.write(binary);
                output.close();
            }
        }
        if (!error.isEmpty())
        {
            QTextStream(stderr) << error << '\n';
            return 1;
        }
        QTextStream(stdout) << "Compiled " << sourcePath << " -> " << outputPath
                            << " (" << binary.size() << " bytes)\n";
        return 0;
    }

    application.setWindowIcon(QIcon("./images/icon.ico"));
    WL4EditorWindow window;
    window.show();


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
