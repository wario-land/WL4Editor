#ifndef FILEIOUTILS_H
#define FILEIOUTILS_H

#include <QVector>
#include <QColor>
#include <QImage>
#include <functional>

#include "Dialog/TilesetEditDialog.h"

namespace FileIOUtils
{
    bool ExportPalette(QWidget *parent, QVector<QRgb> palette);
    bool ImportPalette(QWidget *parent, std::function<void(int, int, QRgb)> SetColorFunction, int selectedPalId);
    bool ImportTile8x8GfxData(QWidget *parent, QVector<QRgb> ref_palette, std::function<void (QByteArray&, QWidget *)> TilesReplaceCallback);
    QImage RenderBGColor(QImage image, QWidget *parent);
} // namespace FileIOUtils

#endif // FILEIOUTILS_H
