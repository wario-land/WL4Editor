#ifndef FILEIOUTILS_H
#define FILEIOUTILS_H

#include <map>
#include <QVector>
#include <QColor>
#include <QImage>
#include <QString>
#include <functional>

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
    QString GetParamFromSourceFile(QString filePath, QString identifier, QRegularExpression validator);
    bool GetGlobalSymbolsFromSourceFile(QString txtfilepath, std::map<unsigned int, QString>& gConsts, std::map<unsigned int, QString>& gFunctions);
    unsigned int FindEntryFunctionAddress(QString txtfilePath, QString entryFunctionSymbol = "");

    // helper functions
    int quasi_memcmp(unsigned char *_Buf1, unsigned char *_Buf2, size_t _Size);
    unsigned char *find_less_feature_buff(unsigned char *_Buf1, unsigned char *_Buf2, size_t _Size);
    QString RelativeFilePathToAbsoluteFilePath(QString relativeFilePath);

    // Normalize a ROM-relative path: use '/' separators, remove "./", and reject absolute
    // paths or paths escaping the root. Returns an empty string for invalid input.
    QString NormalizeRelativePath(const QString &path);

    // Return true when filePath is inside directory. The comparison uses canonical paths
    // and a directory boundary, so "projA2" is not considered to be inside "projA".
    bool IsPathInsideDirectory(const QString &filePath, const QString &directory);

    // Convert an absolute path inside directory to a relative path. Returns an empty string
    // and sets *ok to false when the path is outside the directory or cannot be represented.
    QString PathRelativeToDirectory(const QString &filePath, const QString &directory, bool *ok = nullptr);
} // namespace FileIOUtils

#endif // FILEIOUTILS_H
