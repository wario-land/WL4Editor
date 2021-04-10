#include "FileIOUtils.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QColorDialog>
#include "ROMUtils.h"
#include "Dialog/SelectColorDialog.h"

#include "WL4EditorWindow.h"

extern WL4EditorWindow *singleton;

/// <summary>
/// Export a palette to file.
/// </summary>
/// <param name="parent">
/// the parent widget pointer used in model dialog.
/// </param>
/// <param name="palette">
/// The palette to export.
/// </param>
/// <returns>
/// True if the save was successful.
/// </returns>
bool FileIOUtils::ExportPalette(QWidget *parent, QVector<QRgb> palette)
{
    QString romFileDir = QFileInfo(ROMUtils::ROMFilePath).dir().path();
    QString selectedfilter;
    QString usentiFilter(QObject::tr("usenti pal file") + " (*.pal)"),
            yychrFilter(QObject::tr("YY-CHR pal file") + " (*.pal)"),
            binaryFilter(QObject::tr("Raw Binary palette") + " (*.bin)");
    QString qFilePath =
        QFileDialog::getSaveFileName(parent,
                                     QObject::tr("Save palette file"),
                                     romFileDir,
                                     usentiFilter + ";;" + yychrFilter + ";;" + binaryFilter,
                                     &selectedfilter);
    if(qFilePath.isEmpty()) return false;

    if(selectedfilter == usentiFilter)
    {
        QFile palfile(qFilePath);
        if(palfile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            // Stream text to the file
            QTextStream out(&palfile);
            out << QString("CLRX 8 16\n");
            for(int j = 0; j < 4; ++j)
            {
                for(int i = 0; i < 4; ++i)
                {
                    // RGB888(QRgb) -> BGR888(usenti pal file)
                    int color = ((palette[i + 4 * j] & 0xFF0000) >> 16) |
                            (palette[i + 4 * j] & 0xFF00) |
                            ((palette[i + 4 * j] & 0xFF) << 16);
                    out << QString("0x") + QString("%1").arg(color, 8, 16, QChar('0')) + QString(" ");
                }
                out << QString("\n");
            }
            palfile.close();
        }
        else
        {
            singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("File IO error: Unable to open palette output file \"%1\" for writing. Filter: %2").arg(palfile.fileName()).arg(usentiFilter));
            goto ExportPalette_error;
        }
    }
    else if (selectedfilter == yychrFilter)
    {
        unsigned char palettedata[3 * 16];
        for(int j = 0; j < 16; ++j)
        {
            palettedata[3 * j] = (palette[j] & 0xFF0000) >> 16; // R
            palettedata[3 * j + 1] = (palette[j] & 0xFF00) >> 8; // G
            palettedata[3 * j + 2] = palette[j] & 0xFF; // B
        }
        QFile palfile(qFilePath);
        palfile.open(QIODevice::WriteOnly);
        if (palfile.isOpen())
        {
            palfile.write(reinterpret_cast<const char*>(palettedata), 3 * 16);
            palfile.close();
        }
        else
        {
            singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("File IO error: Unable to open palette output file \"%1\" for writing. Filter: %2").arg(palfile.fileName()).arg(yychrFilter));
            goto ExportPalette_error;
        }
    }
    else if(selectedfilter == binaryFilter)
    {
        unsigned short palettedata[16];
        for(int j = 0; j < 16; ++j)
        {
            int red = (palette[j] & 0xFF0000) >> 16; // R
            int green = (palette[j] & 0xFF00) >> 8; // G
            int blue = palette[j] & 0xFF; // B

            // Going from 8 bits to 5 bits
            red >>= 3;
            green >>= 3;
            blue >>= 3;

            // Assemble color from left to right with OR operator (blue->green->red)
            short newcolor = 0;
            newcolor |= blue;
            newcolor <<= 5;
            newcolor |= green;
            newcolor <<= 5;
            newcolor |= red;
            palettedata[j] = newcolor;
        }
        QFile palfile(qFilePath);
        palfile.open(QIODevice::WriteOnly);
        if (palfile.isOpen())
        {
            QDataStream out(&palfile);
            out.setByteOrder(QDataStream::LittleEndian); // set little endian byte order
            for (int i = 0; i < 16; i++)
            {
                out << quint16(palettedata[i]);
            }
            palfile.close();
        }
        else
        {
            singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("File IO error: Unable to open palette output file \"%1\" for writing. Filter: %2").arg(palfile.fileName()).arg(binaryFilter));
            goto ExportPalette_error;
        }
    }
    else
    {
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Internal error: Invalid filter selection for palette export"));
        return false;
    }
    return true;

ExportPalette_error:
    QMessageBox::critical(parent, QString(QObject::tr("Error")), QString(QObject::tr("Cannot save file!")));
    singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Failed to save palette to file."));
    return false;
}

/// <summary>
/// Import a palette from file to some editor instances.
/// </summary>
/// <param name="parent">
/// the parent widget pointer used in model dialog.
/// </param>
/// <param name="SetColorFunction">
/// Use different SetColor() functions here for different class.
/// </param>
/// <param name="selectedPalId">
/// the if of the palette need to replace in the whole palettes table.
/// </param>
/// <returns>
/// True if the save was successful.
/// </returns>
bool FileIOUtils::ImportPalette(QWidget *parent, std::function<void (int, int, QRgb)> SetColorFunction, int selectedPalId)
{
    QString romFileDir = QFileInfo(ROMUtils::ROMFilePath).dir().path();
    QString selectedfilter;
    QString qFilePath = QFileDialog::getOpenFileName(
                parent,
                QObject::tr("Open palette file"),
                romFileDir,
                QObject::tr("usenti pal file (*.pal);;YY-CHR pal file (*.pal);;Raw Binary palette (*.bin)"),
                &selectedfilter
    );
    if(qFilePath.isEmpty()) return false;

    // Check the file extension
    if((!qFilePath.endsWith(".pal", Qt::CaseInsensitive)) && (!qFilePath.endsWith(".bin", Qt::CaseInsensitive)))
    {
        QMessageBox::critical(parent, QString(QObject::tr("Error")), QString(QObject::tr("Wrong file extension! (.bin, .pal) allowed")));
        return false;
    }

    // Set palette
    if(selectedfilter == "usenti pal file (*.pal)")
    {
        QFile palfile(qFilePath);
        if(palfile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            // Stream text from the file
            QTextStream in(&palfile);
            QString header = in.readLine();
            if(header != "CLRX 8 16")
            {
                QMessageBox::critical(parent, QString(QObject::tr("Error")), QString(QObject::tr("Wrong file format!")));
                return false;
            }
            for(int j = 0; j < 4; ++j)
            {
                QString line = in.readLine();
                if (line.isEmpty())
                {
                    QMessageBox::critical(parent,
                                          QString(QObject::tr("Error")),
                                          QString(QObject::tr("No enough color data in the file!")));
                    return false;
                }
                QStringList fields = line.split(" ");
                for(int i = 0; i < 4; ++i)
                {
                    if(i == 0 && j == 0) continue; // Skip the first color
                    // BGR888(usenti pal file) -> RGB888(QRgb)
                    int fileformatcolor = fields[i].toInt(nullptr, 16);
                    int color = ((fileformatcolor & 0xFF0000) >> 16) |
                            (fileformatcolor & 0xFF00) |
                            ((fileformatcolor & 0xFF) << 16);
                    QColor newcolor = QColor::fromRgb(color);
                    newcolor.setAlpha(0xFF);
                    SetColorFunction(selectedPalId, i + 4 * j, newcolor.rgba());
                }
            }
            palfile.close();
        }
    }
    else if (selectedfilter == "YY-CHR pal file (*.pal)")
    {
        QFile file(qFilePath);
        file.open(QIODevice::ReadOnly);
        int length;
        if (!file.isOpen() || (length = (int) file.size()) < (3 * 16))
        {
            file.close();
            QMessageBox::critical(parent, QString(QObject::tr("Error")), QString(QObject::tr("File size too small! It should be >= 48 bytes.")));
            return false;
        }

        // Read data
        unsigned char *paldata = new unsigned char[length];
        file.read((char *) paldata, length);
        file.close();
        for(int j = 1; j < 16; ++j) // Skip the first color
        {
            int color = (paldata[3 * j] << 16) |
                    (paldata[3 * j + 1] << 8) |
                    paldata[3 * j + 2];
            QColor newcolor = QColor::fromRgb(color);
            newcolor.setAlpha(0xFF);
            SetColorFunction(selectedPalId, j, newcolor.rgba());
        }
    }

    else if (selectedfilter == "Raw Binary palette (*.bin)")
    {
         QByteArray tmppalettedata;
         QFile palbinfile(qFilePath);
         if(!palbinfile.open(QIODevice::ReadOnly))
         {
             QMessageBox::critical(parent, QString(QObject::tr("Error")), QString(QObject::tr("Cannot open file! \n").append(palbinfile.errorString())));
             return false;
         }
         tmppalettedata = palbinfile.readAll();
         if (palbinfile.size() != 32)
         {
             QMessageBox::critical(parent, QString(QObject::tr("Error")),
                                   QString(QObject::tr("Internal error: File size isn't 32 bytes, current size: ")) +
                                   QString::number(palbinfile.size()));
             palbinfile.close();
             return false;
         }
         palbinfile.close();

         QVector<QRgb> tmppalette;
         unsigned short tmppaldata[16];
         memset(tmppaldata, 0, 32);
         memcpy(tmppaldata, tmppalettedata.data(), qMin(32, tmppalettedata.size()));
         ROMUtils::LoadPalette(&tmppalette, tmppaldata, true);
         for (int i = 1; i < 16; i++) // Skip the first color
         {
             SetColorFunction(selectedPalId, i, tmppalette[i]);
         }
    }
    return true;
}

/// <summary>
/// Import tiles8x8 data from file to some editor instances.
/// </summary>
/// <param name="parent">
/// the parent widget pointer used in model dialog.
/// </param>
/// <param name="ref_palette">
/// reference palette used for re-processing the tiles' data.
/// </param>
/// <param name="TilesReplaceCallback">
/// Post-processing to do checking and to update tiles' array after getting the data from files and processing it.
/// </param>
/// <returns>
/// True if the save was successful.
/// </returns>
bool FileIOUtils::ImportTile8x8GfxData(QWidget *parent, QVector<QRgb> ref_palette, std::function<void (QByteArray&, QWidget *)> TilesReplaceCallback)
{
    if (ref_palette.size() != 16)
    {
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Internal Error: ref_palette should have a size of 16."));
        return false;
    }

    // Load gfx bin file
    QString fileName = QFileDialog::getOpenFileName(parent,
                                                    QObject::tr("Load Tileset graphic bin file"), QString(""),
                                                    QObject::tr("bin file") + " (*.bin)");

    // load data into QBytearray
    QByteArray tmptile8x8data, tmptile8x8data_final;
    QFile gfxbinfile(fileName);
    if(!gfxbinfile.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(parent, QObject::tr("Error"), QObject::tr("Cannot open file: %1").arg(gfxbinfile.errorString()));
        return false;
    }
    if(!gfxbinfile.size())
    {
        QMessageBox::critical(parent, QObject::tr("Error"), QObject::tr("File size is 0: %1").arg(gfxbinfile.fileName()));
        return false;
    }
    tmptile8x8data = gfxbinfile.readAll();
    tmptile8x8data_final = tmptile8x8data; // Init
    gfxbinfile.close();

    // Check size
    if(tmptile8x8data.size() & 31)
    {
        QMessageBox::critical(parent, QObject::tr("Error"), QObject::tr("Illegal file size. (should be a multiple of 32 Bytes) File: %1").arg(gfxbinfile.fileName()));
        return false;
    }

    // Load palette data from bin file
    fileName = QFileDialog::getOpenFileName(parent,
                                            QObject::tr("Load palette bin file"), QString(""),
                                            QObject::tr("bin file (*.bin)"));
    QByteArray tmppalettedata;
    QFile palbinfile(fileName);
    if(!palbinfile.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(parent, QObject::tr("Error"), QObject::tr("Cannot open file!"));
        return false;
    }
    tmppalettedata = palbinfile.readAll();
    palbinfile.close();

    QVector<QRgb> tmppalette;
    unsigned short tmppaldata[16];
    memset(tmppaldata, 0, 32);
    memcpy(tmppaldata, tmppalettedata.data(), qMin(32, tmppalettedata.size()));
    ROMUtils::LoadPalette(&tmppalette, tmppaldata, true);

    // Get transparent color id in the palette
    int transparentcolorId = 0;
    SelectColorDialog scdialog;
    scdialog.SetPalette(tmppalette);
    scdialog.SetColor(0);
    if(scdialog.exec() == QDialog::Accepted)
    {
        transparentcolorId = scdialog.GetSelectedColorId();
    }
    else
    {
        return false;
    }

    // transparent-substitute color replacement and load palette
    tmppalette[transparentcolorId] = 0;

    // nybble exchange not needed
    // reset bytearray according to the palette bin file
    for(int i = 0; i < 16; ++i)
    {
        char colorIndex = 0;
        if(i != transparentcolorId)
        {
            // Get the index of the color in the current palette
            QRgb findColor = tmppalette[i];
            auto paletteFound = std::find_if(ref_palette.begin(), ref_palette.end(),
                [findColor](const QRgb& c) {return c == findColor;});
            colorIndex = std::distance(ref_palette.begin(), paletteFound);

            // Edge cases if color not found
            if(paletteFound == ref_palette.end())
            {
                colorIndex = 0;
                if(findColor != 0xFF000000 && findColor != 0xFFFFFFFF && findColor) // black, white or transparent
                {
                    QMessageBox::critical(parent, QObject::tr("Error"),
                        QObject::tr("Color %1 does not exist in the current palette!").arg(QString::number(findColor & 0xFFFFFF, 16).toUpper()));
                    return false;
                }
            }
        }

        // Replace the color[i] in tiledata with the correct id
        for(int j = 0; j < tmptile8x8data.size(); ++j)
        {
            char tmpchr = tmptile8x8data[j];
            char lower4bits, upper4bits;
            upper4bits = (tmpchr >> 4) & 0xF;
            lower4bits = tmpchr & 0xF;
            if (lower4bits == i)
            {
                lower4bits = colorIndex;
            }
            else
            {
                lower4bits = tmptile8x8data_final[j] & 0xF;
            }
            if (upper4bits == i)
            {
                upper4bits = colorIndex;
            }
            else
            {
                upper4bits = (tmptile8x8data_final[j] >> 4) & 0xF;
            }
            tmptile8x8data_final[j] = (upper4bits << 4) | lower4bits;
        }
    }

    TilesReplaceCallback(tmptile8x8data_final, parent);
    return true;
}

/// <summary>
/// Set Image background color for exporting image to graphic file.
/// </summary>
/// <param name="image">
/// The QImage to replace background color.
/// </param>
QImage FileIOUtils::RenderBGColor(QImage image, QWidget *parent)
{
    QColor color = QColorDialog::getColor(Qt::black, parent, QString(QObject::tr("Choose a background color")));
    color.setAlpha(0xFF);
    if(color.isValid())
    {
        for (int j = 0; j < image.height(); ++j)
        {
            for (int k = 0; k < image.width(); ++k)
            {
                if(image.pixelColor(k, j).alpha() == 0) // current pixel is transparent
                {
                    image.setPixel(k, j, color.rgb());
                }
            }
        }
    }
    return image;
}

/// <summary>
/// Load a ROM file into the data array in ROMUtils.cpp.
/// </summary>
/// <param name="filePath">
/// The path to the file that will be read.
/// </param>
QString FileIOUtils::LoadROMFile(QString filePath, bool loadAsTempFile)
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

    if (loadAsTempFile)
    {
        ROMUtils::tmpCurrentFileSize = length;
        ROMUtils::tmpROMFilePath = filePath;
        ROMUtils::tmpCurrentFile = (unsigned char *) ROMAddr;
    }
    else
    {
        ROMUtils::CurrentFileSize = length;
        ROMUtils::ROMFilePath = filePath;
        ROMUtils::CurrentFile = (unsigned char *) ROMAddr;
    }
    return "";
}
