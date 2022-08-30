#ifndef FILEIOUTILS_H
#define FILEIOUTILS_H

#include <QVector>
#include <QColor>
#include <QImage>
#include <functional>

#include "Dialog/TilesetEditDialog.h"

namespace FileIOUtils
{
    QString LoadROMFile(QString filePath);

    // graphics and palettes stuff
    bool ExportPalette(QWidget *parent, QVector<QRgb> palette);
    bool ImportPalette(QWidget *parent, std::function<void(int, int, QRgb)> SetColorFunction, int selectedPalId);
    bool ImportTile8x8GfxData(QWidget *parent,
                              QVector<QRgb> ref_palette,
                              QString dialogTitle,
                              std::function<void (QByteArray&, QWidget *)> TilesReplaceCallback);
    QImage RenderBGColor(QImage image, QWidget *parent);

    // patch parser stuff
    QString PatchParamFromTextFile(QString filePath, QString identifier, QRegExp validator);
    unsigned int FindEntryFunctionAddress(QString txtfilePath, QString entryFunctionSymbol = "");

    // helper functions
    int quasi_memcmp(unsigned char *_Buf1, unsigned char *_Buf2, size_t _Size);
    unsigned char *find_less_feature_buff(unsigned char *_Buf1, unsigned char *_Buf2, size_t _Size);
    QString relativeFilePathToAbsoluteFilePath(QString relativeFilePath);
} // namespace FileIOUtils

#endif // FILEIOUTILS_H
