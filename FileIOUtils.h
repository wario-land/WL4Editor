#ifndef FILEIOUTILS_H
#define FILEIOUTILS_H

#include <QVector>
#include <QPixmap>
#include <functional>

#include "Dialog/TilesetEditDialog.h"

namespace FileIOUtils
{
    bool ExportPalette(QVector<QRgb> palette, QWidget *parent);
    void ImportPalette(QWidget *parent, std::function<void(int, int, QRgb)> SetColorFunction, int selectedPalId);
} // namespace FileIOUtils

#endif // FILEIOUTILS_H
