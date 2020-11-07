#include "FileIOUtils.h"

#include "ROMUtils.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "WL4EditorWindow.h"

extern WL4EditorWindow *singleton;

bool FileIOUtils::ExportPalette(QVector<QRgb> palette, QWidget *parent)
{
    QString romFileDir = QFileInfo(ROMUtils::ROMFilePath).dir().path();
    QString selectedfilter;
    QString qFilePath =
        QFileDialog::getSaveFileName(parent,
                                     QObject::tr("Save palette file"),
                                     romFileDir,
                                     QObject::tr("usenti pal file (*.pal);;YY-CHR pal file (*.pal);;Raw Binary palette (*.bin)"),
                                     &selectedfilter);
    if(qFilePath.isEmpty()) goto ExportPalette_error;
    if(selectedfilter.compare("usenti pal file (*.pal)") == 0)
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
            goto ExportPalette_error;
        }
    }
    else if (selectedfilter.compare("YY-CHR pal file (*.pal)") == 0)
    {
        unsigned char *palettedata = new unsigned char[3 * 16];
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
            delete[] palettedata;
        }
        else
        {
            delete[] palettedata;
            goto ExportPalette_error;
        }
    }
    else if(selectedfilter.compare("Raw Binary palette (*.bin)") == 0)
    {
        unsigned short *palettedata = new unsigned short[16];
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
            out.setByteOrder(QDataStream::LittleEndian); // *** set little endian byte order
            for (int i = 0; i < 16; i++)
            {
                out << quint16(palettedata[i]);
            }
            palfile.close();
            delete[] palettedata;
        }
        else
        {
            delete[] palettedata;
            goto ExportPalette_error;
        }
    }
    return true;
ExportPalette_error:
    QMessageBox::critical(parent, QString(QObject::tr("Error")), QString(QObject::tr("Cannot save file!")));
    return false;
}
void FileIOUtils::ImportPalette(QWidget *parent, std::function<void (int, int, QRgb)> SetColorFunction, int selectedPalId)
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
    if(qFilePath.isEmpty()) return;

    // Check the file extension
    if((!qFilePath.endsWith(".pal", Qt::CaseInsensitive)) && (!qFilePath.endsWith(".bin", Qt::CaseInsensitive)))
    {
        QMessageBox::critical(parent, QString(QObject::tr("Error")), QString(QObject::tr("Wrong file extension! (.bin, .pal) allowed")));
        return;
    }

    // Set palette
    if(selectedfilter.compare("usenti pal file (*.pal)") == 0)
    {
        QFile palfile(qFilePath);
        if(palfile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            // Stream text from the file
            QTextStream in(&palfile);
            QString header = in.readLine();
            if(header.compare("CLRX 8 16"))
            {
                QMessageBox::critical(parent, QString(QObject::tr("Error")), QString(QObject::tr("Wrong file format!")));
                return;
            }
            for(int j = 0; j < 4; ++j)
            {
                QString line = in.readLine();
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
    else if (selectedfilter.compare("YY-CHR pal file (*.pal)") == 0)
    {
        QFile file(qFilePath);
        file.open(QIODevice::ReadOnly);
        int length;
        if (!file.isOpen() || (length = (int) file.size()) < (3 * 16))
        {
            file.close();
            QMessageBox::critical(parent, QString(QObject::tr("Error")), QString(QObject::tr("File size too small! It should be >= 48 bytes.")));
            return;
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

    else if (selectedfilter.compare("Raw Binary palette (*.bin)") == 0)
    {
         QByteArray tmppalettedata;
         QFile palbinfile(qFilePath);
         if(!palbinfile.open(QIODevice::ReadOnly))
         {
             QMessageBox::critical(parent, QString(QObject::tr("Error")), QString(QObject::tr("Cannot open file! \n").append(palbinfile.errorString())));
             return;
         }
         tmppalettedata = palbinfile.readAll();
         if (palbinfile.size() != 32)
         {
             QMessageBox::critical(parent, QString(QObject::tr("Error")),
                                   QString(QObject::tr("Internal error: File size isn't 32 bytes, current size: ")) +
                                   QString::number(palbinfile.size()));
         }
         palbinfile.close();

         QVector<QRgb> tmppalette;
         unsigned short *tmppaldata = new unsigned short[16];
         memset(tmppaldata, 0, 32);
         memcpy(tmppaldata, tmppalettedata.data(), qMin(32, tmppalettedata.size()));
         ROMUtils::LoadPalette(&tmppalette, tmppaldata, true);
         for (int i = 1; i < 16; i++) // Skip the first color
         {
             SetColorFunction(selectedPalId, i, tmppalette[i]);
         }
         delete[] tmppaldata;
    }
}
