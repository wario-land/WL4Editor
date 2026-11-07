#include "AssortedGraphicIO.h"

#include "LevelComponents/Level.h"
#include "LevelComponents/Room.h"
#include "LevelComponents/Tileset.h"
#include "ROMUtils.h"
#include "WL4EditorWindow.h"

#include <QCryptographicHash>
#include <QHash>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMap>
#include <QStringList>
#include <algorithm>
#include <cstring>

extern WL4EditorWindow *singleton;

namespace
{
    // ------------------------------ Generic helpers ------------------------------

    unsigned int JsonAddress(const QJsonValue &value)
    {
        if (value.isString())
        {
            QString text = value.toString().trimmed();
            if (text.startsWith("0x", Qt::CaseInsensitive))
            {
                text = text.mid(2);
            }
            bool ok = false;
            unsigned int address = text.toUInt(&ok, 16);
            return ok ? address : 0;
        }
        return value.toVariant().toUInt();
    }

    QString HashBytes(const QByteArray &data)
    {
        return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex());
    }

    QString SlotKey(const QVector<unsigned int> &slotIDs)
    {
        QStringList parts;
        for (unsigned int slot : slotIDs)
        {
            parts.append(QString::number(slot));
        }
        return parts.join(",");
    }

    unsigned int RamOffsetForTileDataSize(unsigned int sizeInByte)
    {
        const unsigned int tileCount = sizeInByte / 32;
        return (tileCount > 0x3FF) ? 0 : (0x3FF - tileCount);
    }

    QByteArray PaletteDataBytes(const AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
    {
        QByteArray raw;
        for (unsigned int slot : entry.PaletteSlotIDs)
        {
            if (slot >= 16)
            {
                continue;
            }
            for (int colorId = 0; colorId < 16; ++colorId)
            {
                QRgb color = (entry.palettes[slot].size() > colorId)
                        ? entry.palettes[slot][colorId] : QColor(0, 0, 0, 0xFF).rgba();
                unsigned short value = ROMUtils::QRgbToData(color);
                raw.append(static_cast<char>((value >> 8) & 0xFF));
                raw.append(static_cast<char>(value & 0xFF));
            }
        }
        return raw;
    }

    QByteArray MappingDataBytes(const QVector<unsigned short> &words)
    {
        QByteArray raw;
        for (unsigned short word : words)
        {
            raw.append(static_cast<char>((word >> 8) & 0xFF));
            raw.append(static_cast<char>(word & 0xFF));
        }
        return raw;
    }

    bool TileDataMatchesRom(const AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
    {
        if (!AssortedGraphicUtils::IsVanillaAddress(entry.TileDataAddress))
        {
            return false;
        }
        if (!entry.TileDataSizeInByte || (entry.tileData.size() < static_cast<int>(entry.TileDataSizeInByte)))
        {
            return false;
        }
        if (entry.TileDataAddress + entry.TileDataSizeInByte > ROMUtils::ROMFileMetadata->Length)
        {
            return false;
        }
        return !memcmp(ROMUtils::ROMFileMetadata->ROMDataPtr + entry.TileDataAddress,
                       entry.tileData.constData(), entry.TileDataSizeInByte);
    }

    bool PaletteMatchesRom(const AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
    {
        if (!AssortedGraphicUtils::IsVanillaAddress(entry.PaletteAddress))
        {
            return false;
        }
        for (unsigned int slot : entry.PaletteSlotIDs)
        {
            if (slot >= 16)
            {
                return false;
            }
            unsigned int subPalettePtr = entry.PaletteAddress + slot * 32;
            if (subPalettePtr + 32 > ROMUtils::ROMFileMetadata->Length)
            {
                return false;
            }
            if (entry.palettes[slot].size() < 16)
            {
                return false;
            }
            QVector<QRgb> romPalette;
            ROMUtils::LoadPalette(&romPalette,
                                  reinterpret_cast<unsigned short *>(ROMUtils::ROMFileMetadata->ROMDataPtr
                                                                     + subPalettePtr),
                                  true);
            if (romPalette.size() < 16)
            {
                return false;
            }
            for (int colorId = 0; colorId < 16; ++colorId)
            {
                if (romPalette[colorId] != entry.palettes[slot][colorId])
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool MappingDataMatchesRom(const AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
    {
        if (!AssortedGraphicUtils::IsVanillaAddress(entry.MappingDataAddress))
        {
            return false;
        }
        if (!entry.optionalGraphicWidth || !entry.optionalGraphicHeight)
        {
            return false;
        }
        if (entry.mappingData.size()
                != static_cast<int>(entry.optionalGraphicWidth * entry.optionalGraphicHeight))
        {
            return false;
        }
        if (entry.MappingDataAddress + 1 >= ROMUtils::ROMFileMetadata->Length)
        {
            return false;
        }
        LevelComponents::Layer layer(entry.MappingDataAddress, LevelComponents::LayerTile8x8);
        if (!layer.GetLayerData())
        {
            return false;
        }
        if ((layer.GetLayerWidth() != static_cast<int>(entry.optionalGraphicWidth))
                || (layer.GetLayerHeight() != static_cast<int>(entry.optionalGraphicHeight)))
        {
            return false;
        }
        return !memcmp(layer.GetLayerData(), entry.mappingData.constData(),
                       entry.optionalGraphicWidth * entry.optionalGraphicHeight * 2);
    }

    void ResetPalettes(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
    {
        for (int slot = 0; slot < 16; ++slot)
        {
            entry.palettes[slot].clear();
            for (int colorId = 0; colorId < 16; ++colorId)
            {
                entry.palettes[slot].push_back(QColor(0, 0, 0, 0xFF).rgba());
            }
        }
    }

    bool LoadTileDataFromRom(AssortedGraphicUtils::AssortedGraphicEntryItem &entry, QString &error)
    {
        if (!entry.TileDataAddress || !entry.TileDataSizeInByte)
        {
            entry.tileData.clear();
            return true;
        }
        if (entry.TileDataAddress + entry.TileDataSizeInByte > ROMUtils::ROMFileMetadata->Length)
        {
            error = "Tile8x8 data address is out of the ROM range";
            return false;
        }
        entry.tileData.resize(static_cast<int>(entry.TileDataSizeInByte));
        memcpy(entry.tileData.data(), ROMUtils::ROMFileMetadata->ROMDataPtr + entry.TileDataAddress,
               entry.TileDataSizeInByte);
        return true;
    }

    bool LoadPaletteDataFromRom(AssortedGraphicUtils::AssortedGraphicEntryItem &entry, QString &error)
    {
        ResetPalettes(entry);
        if (!entry.PaletteAddress)
        {
            return true;
        }
        for (int slotIndex = 0; slotIndex < entry.PaletteSlotIDs.size(); ++slotIndex)
        {
            unsigned int slot = entry.PaletteSlotIDs[slotIndex];
            if (slot >= 16)
            {
                error = "Palette slot ID is out of range";
                return false;
            }
            unsigned int subPalettePtr = AssortedGraphicUtils::IsVanillaAddress(entry.PaletteAddress)
                    ? entry.PaletteAddress + slot * 32
                    : entry.PaletteAddress + static_cast<unsigned int>(slotIndex) * 32;
            if (subPalettePtr + 32 > ROMUtils::ROMFileMetadata->Length)
            {
                error = "Palette data address is out of the ROM range";
                return false;
            }
            entry.palettes[slot].clear();
            ROMUtils::LoadPalette(&(entry.palettes[slot]),
                                  reinterpret_cast<unsigned short *>(ROMUtils::ROMFileMetadata->ROMDataPtr
                                                                     + subPalettePtr),
                                  true);
        }
        return true;
    }

    bool LoadMappingDataFromRom(AssortedGraphicUtils::AssortedGraphicEntryItem &entry, QString &error)
    {
        if (!entry.MappingDataAddress)
        {
            entry.mappingData.clear();
            entry.optionalGraphicWidth = 0;
            entry.optionalGraphicHeight = 0;
            return true;
        }
        if (entry.MappingDataAddress + 1 >= ROMUtils::ROMFileMetadata->Length)
        {
            error = "Mapping data address is out of the ROM range";
            return false;
        }
        LevelComponents::Layer layer(entry.MappingDataAddress, LevelComponents::LayerTile8x8);
        if (!layer.GetLayerData())
        {
            error = "Failed to decompress the mapping data";
            return false;
        }
        if (entry.optionalGraphicWidth && entry.optionalGraphicHeight
                && ((layer.GetLayerWidth() != static_cast<int>(entry.optionalGraphicWidth))
                    || (layer.GetLayerHeight() != static_cast<int>(entry.optionalGraphicHeight))))
        {
            error = "Mapping data dimensions do not match the JSON description";
            return false;
        }
        entry.optionalGraphicWidth = layer.GetLayerWidth();
        entry.optionalGraphicHeight = layer.GetLayerHeight();
        entry.mappingData.clear();
        for (int i = 0; i < entry.optionalGraphicWidth * entry.optionalGraphicHeight; ++i)
        {
            entry.mappingData.push_back(layer.GetLayerData()[i]);
        }
        return true;
    }

    bool ParseSlotIDs(const QJsonValue &value, QVector<unsigned int> &slotIDs, QString &error)
    {
        slotIDs.clear();
        if (value.isArray())
        {
            const QJsonArray slotArray = value.toArray();
            for (int slotIndex = 0; slotIndex < slotArray.size(); ++slotIndex)
            {
                int slot = slotArray.at(slotIndex).toInt(-1);
                if ((slot < 0) || (slot > 15))
                {
                    error = "Palette slot ID is out of range";
                    return false;
                }
                if (!slotIDs.contains(static_cast<unsigned int>(slot)))
                {
                    slotIDs.push_back(static_cast<unsigned int>(slot));
                }
            }
        }
        else if (value.isString())
        {
            const QStringList parts = value.toString().split(",", Qt::SkipEmptyParts);
            for (const QString &part : parts)
            {
                bool ok = false;
                int slot = part.trimmed().toInt(&ok, 16);
                if (!ok || (slot < 0) || (slot > 15))
                {
                    error = "Palette slot ID is out of range";
                    return false;
                }
                if (!slotIDs.contains(static_cast<unsigned int>(slot)))
                {
                    slotIDs.push_back(static_cast<unsigned int>(slot));
                }
            }
        }
        // The order of the slot IDs is significant for a custom palette chunk: the palette data
        // is stored in this order. Do not sort it.
        return true;
    }

    bool ParsePaletteData(const QString &dataHex, const QVector<unsigned int> &slotIDs,
                          AssortedGraphicUtils::AssortedGraphicEntryItem &entry, QString &error)
    {
        ResetPalettes(entry);
        if (dataHex.isEmpty())
        {
            return true;
        }
        const QByteArray raw = QByteArray::fromHex(dataHex.toLatin1());
        if (raw.size() != slotIDs.size() * 32)
        {
            error = "Palette data size does not match the palette slot count";
            return false;
        }
        for (int slotIndex = 0; slotIndex < slotIDs.size(); ++slotIndex)
        {
            unsigned int slot = slotIDs[slotIndex];
            if (slot >= 16)
            {
                error = "Palette slot ID is out of range";
                return false;
            }
            entry.palettes[slot].clear();
            for (int colorId = 0; colorId < 16; ++colorId)
            {
                int offset = (slotIndex * 16 + colorId) * 2;
                unsigned short value = static_cast<unsigned short>(
                            (static_cast<unsigned char>(raw[offset]) << 8)
                            | static_cast<unsigned char>(raw[offset + 1]));
                int r = (value & 0x1F) << 3;
                int g = ((value >> 5) & 0x1F) << 3;
                int b = ((value >> 10) & 0x1F) << 3;
                entry.palettes[slot].push_back(QColor(r, g, b).rgb());
            }
        }
        return true;
    }

    bool ParseMappingData(const QString &dataHex, unsigned int width, unsigned int height,
                          AssortedGraphicUtils::AssortedGraphicEntryItem &entry, QString &error)
    {
        entry.mappingData.clear();
        if ((width != 32) && (width != 64))
        {
            error = "Mapping data width must be 0x20 or 0x40";
            return false;
        }
        if ((height != 32) && (height != 64))
        {
            error = "Mapping data height must be 0x20 or 0x40";
            return false;
        }
        if (dataHex.size() != static_cast<int>(width * height * 4))
        {
            error = "Mapping data size does not match the mapping data dimensions";
            return false;
        }
        for (unsigned int i = 0; i < width * height; ++i)
        {
            bool ok = false;
            unsigned short value = static_cast<unsigned short>(dataHex.mid(static_cast<int>(i) * 4, 4).toUInt(&ok, 16));
            if (!ok)
            {
                error = "Mapping data contains an invalid hex value";
                return false;
            }
            entry.mappingData.push_back(value);
        }
        return true;
    }

    void RecomputeCompressedSize(AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
    {
        if (!entry.optionalGraphicWidth || !entry.optionalGraphicHeight || entry.mappingData.isEmpty())
        {
            entry.MappingDataSizeAfterCompressionInByte = 0;
            return;
        }
        unsigned int compressedSize = 0;
        unsigned char *compressed = LevelComponents::Layer::CompressLayerData(
                    entry.mappingData, LevelComponents::LayerTile8x8,
                    entry.optionalGraphicWidth, entry.optionalGraphicHeight, &compressedSize);
        delete[] compressed;
        entry.MappingDataSizeAfterCompressionInByte = compressedSize;
    }

    const AssortedGraphicUtils::AssortedGraphicEntryItem *FindMatchingEntry(
            const AssortedGraphicUtils::AssortedGraphicEntryItem &built,
            const QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> &entries)
    {
        const AssortedGraphicUtils::AssortedGraphicEntryItem *best = nullptr;
        int bestScore = -1;
        for (const AssortedGraphicUtils::AssortedGraphicEntryItem &entry : entries)
        {
            if (entry.MappingDataAddress != built.MappingDataAddress)
            {
                continue;
            }
            bool tileMatch = (entry.TileDataAddress == built.TileDataAddress);
            if (!tileMatch && (entry.TileDataSizeInByte == built.TileDataSizeInByte)
                    && (entry.tileData == built.tileData))
            {
                tileMatch = true;
            }
            if (!tileMatch)
            {
                continue;
            }

            int score = 0;
            if (entry.TileDataAddress == built.TileDataAddress)
            {
                score += 4;
            }
            if (entry.PaletteAddress == built.PaletteAddress)
            {
                score += 2;
            }
            if (entry.PaletteSlotIDs == built.PaletteSlotIDs)
            {
                score += 1;
            }
            if (AssortedGraphicUtils::IsVanillaReferencingEntry(entry))
            {
                score += 1;
            }
            if (score > bestScore)
            {
                best = &entry;
                bestScore = score;
            }
        }
        return best;
    }

    bool BuildLayerEntry(LevelComponents::Room *room, LevelComponents::Tileset *tileset, int layerId,
                         const QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> &allEntries,
                         int tilesetId, AssortedGraphicUtils::AssortedGraphicEntryItem &result)
    {
        if (!room || !tileset)
        {
            return false;
        }
        LevelComponents::Layer *layer = room->GetLayer(layerId);
        if (!layer || (layer->GetMappingType() != LevelComponents::LayerTile8x8) || !layer->IsEnabled())
        {
            return false;
        }
        if ((layer->GetLayerWidth() <= 0) || (layer->GetLayerHeight() <= 0) || !layer->GetLayerData())
        {
            return false;
        }

        const int width = layer->GetLayerWidth();
        const int height = layer->GetLayerHeight();
        const unsigned int tileDataAddress = static_cast<unsigned int>(tileset->GetbgGFXptr());
        const unsigned int tileDataSize = static_cast<unsigned int>(tileset->GetbgGFXlen());

        QByteArray tilesetTileData;
        QByteArray tileData(static_cast<int>(tileDataSize), 0);
        if (AssortedGraphicUtils::GetTilesetBGTile8x8RawData(tileset, tilesetTileData))
        {
            const unsigned int startId = (tileDataSize / 32) ? (0x3FF - tileDataSize / 32) : 0;
            const unsigned int count = tileDataSize / 32;
            for (unsigned int i = 0; i < count; ++i)
            {
                const unsigned int tileId = startId + i;
                if (tileId >= 0x400)
                {
                    break;
                }
                memcpy(tileData.data() + i * 32, tilesetTileData.constData() + tileId * 32, 32);
            }
        }

        result.TileDataAddress = tileDataAddress;
        result.TileDataSizeInByte = tileDataSize;
        result.TileDataRAMOffsetNum = RamOffsetForTileDataSize(tileDataSize);
        result.TileDataType = AssortedGraphicUtils::Tile8x8_4bpp_no_comp_Tileset_text_bg;
        result.tileData = tileData;

        result.PaletteAddress = static_cast<unsigned int>(tileset->GetPaletteAddr());
        QVector<unsigned int> usedPaletteSlots;
        for (int i = 0; i < width * height; ++i)
        {
            unsigned int slot = (layer->GetLayerData()[i] >> 12) & 0xF;
            if (!usedPaletteSlots.contains(slot))
            {
                usedPaletteSlots.push_back(slot);
            }
        }
        std::sort(usedPaletteSlots.begin(), usedPaletteSlots.end());
        result.PaletteSlotIDs = usedPaletteSlots;
        ResetPalettes(result);
        for (unsigned int slot : usedPaletteSlots)
        {
            if (slot < 16)
            {
                result.palettes[slot] = tileset->GetPalettes()[slot];
            }
        }

        result.MappingDataAddress = static_cast<unsigned int>(layer->GetDataPtr());
        result.MappingDataCompressType = AssortedGraphicUtils::RLE_mappingtype_0x20;
        result.optionalGraphicWidth = width;
        result.optionalGraphicHeight = height;
        result.mappingData.clear();
        for (int i = 0; i < width * height; ++i)
        {
            result.mappingData.push_back(layer->GetLayerData()[i]);
        }
        RecomputeCompressedSize(result);

        const AssortedGraphicUtils::AssortedGraphicEntryItem *match =
                FindMatchingEntry(result, allEntries);
        if (match)
        {
            result.TileDataName = match->TileDataName;
            result.MappingDataName = match->MappingDataName;
        }
        else
        {
            const QString tilesetIdHex = QString::number(tilesetId, 16).toUpper().rightJustified(2, QLatin1Char('0'));
            result.TileDataName = AssortedGraphicUtils::IsVanillaAddress(tileDataAddress)
                    ? QString("vanilla Tileset 0x%1 bg tiles").arg(tilesetIdHex)
                    : QString("custom bg tiles (Tileset 0x%1)").arg(tilesetIdHex);
            result.MappingDataName = (layerId == 3) ? QString("Layer 3") : QString("Layer 0");
        }
        return true;
    }

    QJsonArray EntryListToJson(const QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> &entries,
                               bool includeData)
    {
        QJsonArray result;
        for (const AssortedGraphicUtils::AssortedGraphicEntryItem &entry : entries)
        {
            result.append(AssortedGraphicIO::EntryToJson(entry, includeData));
        }
        return result;
    }
}

// ------------------------------ Entry JSON ------------------------------

QJsonObject AssortedGraphicIO::EntryToJson(const AssortedGraphicUtils::AssortedGraphicEntryItem &entry,
                                           bool includeData)
{
    QJsonObject object;
    object["tileDataName"] = entry.TileDataName;
    object["mappingDataName"] = entry.MappingDataName;
    object["tileDataType"] = static_cast<int>(entry.TileDataType);
    object["mappingDataCompressType"] = static_cast<int>(entry.MappingDataCompressType);
    object["isVanilla"] = AssortedGraphicUtils::IsVanillaReferencingEntry(entry);

    const bool tileDataUsesAddress = !includeData || TileDataMatchesRom(entry);
    const bool paletteUsesAddress = !includeData || PaletteMatchesRom(entry);
    const bool mappingDataUsesAddress = !includeData || MappingDataMatchesRom(entry);

    QJsonObject tileData;
    tileData["source"] = tileDataUsesAddress ? QString("address") : QString("data");
    tileData["isVanilla"] = tileDataUsesAddress
            && AssortedGraphicUtils::IsVanillaAddress(entry.TileDataAddress);
    tileData["address"] = static_cast<int>(entry.TileDataAddress);
    tileData["sizeInByte"] = static_cast<int>(entry.TileDataSizeInByte);
    tileData["ramOffsetNum"] = static_cast<int>(entry.TileDataRAMOffsetNum);
    tileData["dataHex"] = tileDataUsesAddress ? QString()
                                              : QString::fromLatin1(entry.tileData.toHex());
    object["tileData"] = tileData;

    QJsonObject palette;
    palette["source"] = paletteUsesAddress ? QString("address") : QString("data");
    palette["isVanilla"] = paletteUsesAddress
            && AssortedGraphicUtils::IsVanillaAddress(entry.PaletteAddress);
    palette["address"] = static_cast<int>(entry.PaletteAddress);
    QJsonArray slotIDs;
    for (unsigned int slot : entry.PaletteSlotIDs)
    {
        slotIDs.append(static_cast<int>(slot));
    }
    palette["slotIDs"] = slotIDs;
    palette["dataHex"] = paletteUsesAddress ? QString()
                                            : QString::fromLatin1(PaletteDataBytes(entry).toHex());
    object["palette"] = palette;

    QJsonObject mappingData;
    mappingData["source"] = mappingDataUsesAddress ? QString("address") : QString("data");
    mappingData["isVanilla"] = mappingDataUsesAddress
            && AssortedGraphicUtils::IsVanillaAddress(entry.MappingDataAddress);
    mappingData["address"] = static_cast<int>(entry.MappingDataAddress);
    mappingData["width"] = static_cast<int>(entry.optionalGraphicWidth);
    mappingData["height"] = static_cast<int>(entry.optionalGraphicHeight);
    mappingData["sizeAfterCompressionInByte"] = static_cast<int>(entry.MappingDataSizeAfterCompressionInByte);
    mappingData["dataHex"] = mappingDataUsesAddress ? QString()
                                                    : QString::fromLatin1(MappingDataBytes(entry.mappingData).toHex());
    object["mappingData"] = mappingData;

    return object;
}

bool AssortedGraphicIO::EntryFromJson(const QJsonObject &object,
                                      AssortedGraphicUtils::AssortedGraphicEntryItem &entry,
                                      QString &error, bool loadAddressDataFromRom)
{
    error.clear();
    entry = AssortedGraphicUtils::AssortedGraphicEntryItem();
    // The default constructor of the struct does not initialize every scalar member, so make the
    // state explicit before the JSON values are applied.
    entry.TileDataAddress = 0;
    entry.TileDataSizeInByte = 0;
    entry.TileDataRAMOffsetNum = 0;
    entry.MappingDataAddress = 0;
    entry.MappingDataSizeAfterCompressionInByte = 0;
    entry.PaletteAddress = 0;
    entry.optionalGraphicWidth = 0;
    entry.optionalGraphicHeight = 0;
    entry.TileDataName = object.value("tileDataName").toString();
    entry.MappingDataName = object.value("mappingDataName").toString();
    // The assorted graphic list chunk uses ';' as the field separator.
    entry.TileDataName.remove(QLatin1Char(';'));
    entry.MappingDataName.remove(QLatin1Char(';'));
    entry.TileDataType = static_cast<enum AssortedGraphicUtils::AssortedGraphicTileDataType>(
                object.value("tileDataType").toInt(
                    static_cast<int>(AssortedGraphicUtils::Tile8x8_4bpp_no_comp_Tileset_text_bg)));
    entry.MappingDataCompressType = static_cast<enum AssortedGraphicUtils::AssortedGraphicMappingDataCompressionType>(
                object.value("mappingDataCompressType").toInt(
                    static_cast<int>(AssortedGraphicUtils::RLE_mappingtype_0x20)));

    if (entry.TileDataType != AssortedGraphicUtils::Tile8x8_4bpp_no_comp_Tileset_text_bg)
    {
        error = "Unsupported Tile8x8 data type";
        return false;
    }
    if ((entry.MappingDataCompressType != AssortedGraphicUtils::No_mapping_data_comp)
            && (entry.MappingDataCompressType != AssortedGraphicUtils::RLE_mappingtype_0x20))
    {
        error = "Unsupported mapping data compression type";
        return false;
    }

    // Tile8x8 data
    const QJsonObject tileData = object.value("tileData").toObject();
    const QString tileDataSource = tileData.value("source").toString(
                AssortedGraphicUtils::IsVanillaAddress(JsonAddress(tileData.value("address")))
                ? QString("address") : QString("data"));
    entry.TileDataSizeInByte = static_cast<unsigned int>(tileData.value("sizeInByte").toInt());
    entry.TileDataRAMOffsetNum = JsonAddress(tileData.value("ramOffsetNum"));
    if (entry.TileDataSizeInByte % 32)
    {
        error = "Tile8x8 data size must be a multiple of 32";
        return false;
    }
    if (entry.TileDataSizeInByte
            && ((entry.TileDataRAMOffsetNum + entry.TileDataSizeInByte / 32) > 0x3FF))
    {
        error = "Tile8x8 RAM offset is out of range";
        return false;
    }
    if (tileDataSource == "address")
    {
        entry.TileDataAddress = JsonAddress(tileData.value("address"));
        if (entry.TileDataSizeInByte)
        {
            if (!entry.TileDataAddress
                    || (entry.TileDataAddress + entry.TileDataSizeInByte > ROMUtils::ROMFileMetadata->Length))
            {
                error = "Tile8x8 data address is out of the ROM range";
                return false;
            }
            if (loadAddressDataFromRom && !LoadTileDataFromRom(entry, error))
            {
                return false;
            }
        }
        else
        {
            entry.TileDataAddress = 0;
            entry.tileData.clear();
        }
    }
    else
    {
        entry.TileDataAddress = 0;
        const QString dataHex = tileData.value("dataHex").toString();
        if (dataHex.size() != static_cast<int>(entry.TileDataSizeInByte) * 2)
        {
            error = "Tile8x8 data size does not match the hex data";
            return false;
        }
        entry.tileData = QByteArray::fromHex(dataHex.toLatin1());
    }

    // Palettes
    const QJsonObject palette = object.value("palette").toObject();
    const QString paletteSource = palette.value("source").toString(
                AssortedGraphicUtils::IsVanillaAddress(JsonAddress(palette.value("address")))
                ? QString("address") : QString("data"));
    if (!ParseSlotIDs(palette.value("slotIDs"), entry.PaletteSlotIDs, error))
    {
        return false;
    }
    if (entry.PaletteSlotIDs.isEmpty())
    {
        entry.PaletteSlotIDs.push_back(0);
    }
    if (paletteSource == "address")
    {
        entry.PaletteAddress = JsonAddress(palette.value("address"));
        if (entry.PaletteAddress)
        {
            unsigned int requiredSize = AssortedGraphicUtils::IsVanillaAddress(entry.PaletteAddress)
                    ? 16 * 32 : static_cast<unsigned int>(entry.PaletteSlotIDs.size()) * 32;
            if (entry.PaletteAddress + requiredSize > ROMUtils::ROMFileMetadata->Length)
            {
                error = "Palette data address is out of the ROM range";
                return false;
            }
            if (loadAddressDataFromRom && !LoadPaletteDataFromRom(entry, error))
            {
                return false;
            }
        }
        else
        {
            ResetPalettes(entry);
        }
    }
    else
    {
        entry.PaletteAddress = 0;
        if (!ParsePaletteData(palette.value("dataHex").toString(), entry.PaletteSlotIDs, entry, error))
        {
            return false;
        }
    }

    // Mapping data
    const QJsonObject mappingData = object.value("mappingData").toObject();
    const QString mappingDataSource = mappingData.value("source").toString(
                AssortedGraphicUtils::IsVanillaAddress(JsonAddress(mappingData.value("address")))
                ? QString("address") : QString("data"));
    entry.optionalGraphicWidth = static_cast<unsigned int>(mappingData.value("width").toInt());
    entry.optionalGraphicHeight = static_cast<unsigned int>(mappingData.value("height").toInt());
    entry.MappingDataSizeAfterCompressionInByte =
            static_cast<unsigned int>(mappingData.value("sizeAfterCompressionInByte").toInt());
    if (mappingDataSource == "address")
    {
        entry.MappingDataAddress = JsonAddress(mappingData.value("address"));
        if (entry.MappingDataAddress)
        {
            if (loadAddressDataFromRom && !LoadMappingDataFromRom(entry, error))
            {
                return false;
            }
        }
        else
        {
            entry.mappingData.clear();
            entry.optionalGraphicWidth = 0;
            entry.optionalGraphicHeight = 0;
        }
    }
    else
    {
        entry.MappingDataAddress = 0;
        if (entry.optionalGraphicWidth && entry.optionalGraphicHeight)
        {
            if (!ParseMappingData(mappingData.value("dataHex").toString(),
                                  entry.optionalGraphicWidth, entry.optionalGraphicHeight, entry, error))
            {
                return false;
            }
            RecomputeCompressedSize(entry);
        }
        else
        {
            entry.mappingData.clear();
            entry.MappingDataCompressType = AssortedGraphicUtils::No_mapping_data_comp;
            entry.MappingDataSizeAfterCompressionInByte = 0;
        }
    }

    // An entry without mapping data must not keep the RLE type, otherwise loading it from the
    // list chunk would try to decompress the address 0.
    if (!entry.MappingDataAddress && entry.mappingData.isEmpty())
    {
        entry.MappingDataCompressType = AssortedGraphicUtils::No_mapping_data_comp;
        entry.MappingDataSizeAfterCompressionInByte = 0;
    }

    // Reject a completely empty object. A named entry without data is still accepted, because the
    // Graphic Manager can contain entries which are being edited.
    if (entry.TileDataName.isEmpty() && entry.MappingDataName.isEmpty()
            && !entry.TileDataAddress && !entry.PaletteAddress && !entry.MappingDataAddress
            && entry.tileData.isEmpty() && entry.mappingData.isEmpty())
    {
        error = "the entry is empty";
        return false;
    }

    return true;
}

QString AssortedGraphicIO::EntryFingerprint(const AssortedGraphicUtils::AssortedGraphicEntryItem &entry)
{
    QString tileKey;
    if (AssortedGraphicUtils::IsVanillaAddress(entry.TileDataAddress))
    {
        tileKey = QString("A:%1:%2:%3").arg(entry.TileDataAddress)
                .arg(entry.TileDataSizeInByte).arg(entry.TileDataRAMOffsetNum);
    }
    else
    {
        const QByteArray data = entry.tileData.left(static_cast<int>(entry.TileDataSizeInByte));
        tileKey = QString("D:%1:%2:%3").arg(HashBytes(data))
                .arg(entry.TileDataSizeInByte).arg(entry.TileDataRAMOffsetNum);
    }

    QString paletteKey;
    if (AssortedGraphicUtils::IsVanillaAddress(entry.PaletteAddress))
    {
        paletteKey = QString("A:%1:%2").arg(entry.PaletteAddress).arg(SlotKey(entry.PaletteSlotIDs));
    }
    else
    {
        paletteKey = QString("D:%1:%2").arg(HashBytes(PaletteDataBytes(entry)))
                .arg(SlotKey(entry.PaletteSlotIDs));
    }

    QString mappingKey;
    if (AssortedGraphicUtils::IsVanillaAddress(entry.MappingDataAddress))
    {
        mappingKey = QString("A:%1:%2:%3").arg(entry.MappingDataAddress)
                .arg(entry.optionalGraphicWidth).arg(entry.optionalGraphicHeight);
    }
    else
    {
        mappingKey = QString("D:%1:%2:%3").arg(HashBytes(MappingDataBytes(entry.mappingData)))
                .arg(entry.optionalGraphicWidth).arg(entry.optionalGraphicHeight);
    }

    return tileKey + "|" + paletteKey + "|" + mappingKey;
}

void AssortedGraphicIO::EnsureUniqueEntryNames(
        AssortedGraphicUtils::AssortedGraphicEntryItem &entry,
        const QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> &existingEntries)
{
    if (entry.TileDataName.isEmpty())
    {
        entry.TileDataName = "imported bg tiles";
    }
    if (entry.MappingDataName.isEmpty())
    {
        entry.MappingDataName = "imported mapping data";
    }

    const QString baseName = entry.MappingDataName;
    int suffix = 0;
    while (true)
    {
        bool conflict = false;
        for (const AssortedGraphicUtils::AssortedGraphicEntryItem &existing : existingEntries)
        {
            if ((existing.TileDataName == entry.TileDataName)
                    && (existing.MappingDataName == entry.MappingDataName))
            {
                conflict = true;
                break;
            }
        }
        if (!conflict)
        {
            return;
        }
        ++suffix;
        entry.MappingDataName = baseName + " (imported"
                + (suffix > 1 ? QString(" ") + QString::number(suffix) : QString()) + ")";
    }
}

// ------------------------------ Room / Level graphics ------------------------------

QJsonObject AssortedGraphicIO::BuildRoomGraphics(int roomId)
{
    QJsonObject result;
    result["version"] = 1;

    LevelComponents::Level *level = singleton ? singleton->GetCurrentLevel() : nullptr;
    if (!level)
    {
        result["error"] = "No level is loaded";
        return result;
    }
    if (roomId < 0)
    {
        roomId = singleton->GetCurrentRoomId();
    }
    if ((roomId < 0) || (roomId >= static_cast<int>(level->GetRooms().size())))
    {
        result["error"] = "Room ID is out of range";
        return result;
    }

    LevelComponents::Room *room = level->GetRooms()[roomId];
    LevelComponents::Tileset *tileset = room->GetTileset();
    const QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> allEntries =
            AssortedGraphicUtils::GetAssortedGraphicsFromROM();

    AssortedGraphicUtils::AssortedGraphicEntryItem bgEntry;
    AssortedGraphicUtils::AssortedGraphicEntryItem layer0Entry;
    const bool hasBg = BuildLayerEntry(room, tileset, 3, allEntries, room->GetTilesetID(), bgEntry);
    const bool hasLayer0 = BuildLayerEntry(room, tileset, 0, allEntries, room->GetTilesetID(), layer0Entry);

    QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> entries;
    QHash<QString, int> entryIndices;
    const auto addEntry = [&entries, &entryIndices](
            const AssortedGraphicUtils::AssortedGraphicEntryItem &entry) -> int
    {
        const QString fingerprint = AssortedGraphicIO::EntryFingerprint(entry);
        if (entryIndices.contains(fingerprint))
        {
            return entryIndices.value(fingerprint);
        }
        const int index = static_cast<int>(entries.size());
        entries.push_back(entry);
        entryIndices.insert(fingerprint, index);
        return index;
    };

    const int bgEntryIndex = hasBg ? addEntry(bgEntry) : -1;
    const int layer0EntryIndex = hasLayer0 ? addEntry(layer0Entry) : -1;

    result["roomId"] = roomId;
    result["tilesetId"] = room->GetTilesetID();
    result["entries"] = EntryListToJson(entries, true);

    QJsonObject roomObject;
    roomObject["bgEnabled"] = hasBg;
    roomObject["bgEntryIndex"] = bgEntryIndex;
    roomObject["layer0Enabled"] = hasLayer0;
    roomObject["layer0EntryIndex"] = layer0EntryIndex;
    result["room"] = roomObject;

    QJsonObject tilesetObject;
    tilesetObject["tilesetId"] = room->GetTilesetID();
    tilesetObject["bgEntryIndex"] = hasBg ? bgEntryIndex : (hasLayer0 ? layer0EntryIndex : -1);
    result["tileset"] = tilesetObject;

    return result;
}

QJsonObject AssortedGraphicIO::BuildLevelGraphics()
{
    QJsonObject result;
    result["version"] = 1;

    LevelComponents::Level *level = singleton ? singleton->GetCurrentLevel() : nullptr;
    if (!level)
    {
        result["error"] = "No level is loaded";
        return result;
    }

    const QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> allEntries =
            AssortedGraphicUtils::GetAssortedGraphicsFromROM();
    QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> entries;
    QHash<QString, int> entryIndices;
    const auto addEntry = [&entries, &entryIndices](
            const AssortedGraphicUtils::AssortedGraphicEntryItem &entry) -> int
    {
        const QString fingerprint = AssortedGraphicIO::EntryFingerprint(entry);
        if (entryIndices.contains(fingerprint))
        {
            return entryIndices.value(fingerprint);
        }
        const int index = static_cast<int>(entries.size());
        entries.push_back(entry);
        entryIndices.insert(fingerprint, index);
        return index;
    };

    QJsonArray roomsArray;
    QMap<int, int> tilesetEntryIndex;
    QMap<int, unsigned int> tilesetTileDataAddress;
    const std::vector<LevelComponents::Room *> rooms = level->GetRooms();
    for (int roomId = 0; roomId < static_cast<int>(rooms.size()); ++roomId)
    {
        LevelComponents::Room *room = rooms[roomId];
        LevelComponents::Tileset *tileset = room->GetTileset();
        AssortedGraphicUtils::AssortedGraphicEntryItem bgEntry;
        AssortedGraphicUtils::AssortedGraphicEntryItem layer0Entry;
        const bool hasBg = BuildLayerEntry(room, tileset, 3, allEntries, room->GetTilesetID(), bgEntry);
        const bool hasLayer0 = BuildLayerEntry(room, tileset, 0, allEntries, room->GetTilesetID(), layer0Entry);

        const int bgEntryIndex = hasBg ? addEntry(bgEntry) : -1;
        const int layer0EntryIndex = hasLayer0 ? addEntry(layer0Entry) : -1;

        QJsonObject roomObject;
        roomObject["roomId"] = roomId;
        roomObject["tilesetId"] = room->GetTilesetID();
        roomObject["bgEnabled"] = hasBg;
        roomObject["bgEntryIndex"] = bgEntryIndex;
        roomObject["layer0Enabled"] = hasLayer0;
        roomObject["layer0EntryIndex"] = layer0EntryIndex;
        roomsArray.append(roomObject);

        const int tileDataEntryIndex = hasBg ? bgEntryIndex : (hasLayer0 ? layer0EntryIndex : -1);
        if (tileDataEntryIndex >= 0)
        {
            const int tilesetId = room->GetTilesetID();
            const unsigned int tileDataAddress = entries[tileDataEntryIndex].TileDataAddress;
            if (!tilesetEntryIndex.contains(tilesetId))
            {
                tilesetEntryIndex.insert(tilesetId, tileDataEntryIndex);
                tilesetTileDataAddress.insert(tilesetId, tileDataAddress);
            }
            else if (tilesetTileDataAddress.value(tilesetId) != tileDataAddress)
            {
                singleton->GetOutputWidgetPtr()->PrintString(
                            QString("Graphic export warning: Tileset 0x%1 is used with different "
                                    "background Tile8x8 data sets by different rooms. The first one "
                                    "is exported.")
                            .arg(QString::number(tilesetId, 16).toUpper()));
            }
        }
    }

    QJsonArray tilesetsArray;
    for (auto it = tilesetEntryIndex.constBegin(); it != tilesetEntryIndex.constEnd(); ++it)
    {
        QJsonObject tilesetObject;
        tilesetObject["tilesetId"] = it.key();
        tilesetObject["bgEntryIndex"] = it.value();
        tilesetsArray.append(tilesetObject);
    }

    result["entries"] = EntryListToJson(entries, true);
    result["rooms"] = roomsArray;
    result["tilesets"] = tilesetsArray;
    return result;
}

QJsonObject AssortedGraphicIO::BuildAllEntries()
{
    QJsonObject result;
    result["version"] = 1;
    result["entries"] = EntryListToJson(AssortedGraphicUtils::GetAssortedGraphicsFromROM(), true);
    return result;
}

// ------------------------------ Import ------------------------------

QJsonObject AssortedGraphicIO::ImportEntries(const QJsonArray &entries)
{
    QJsonObject result;
    result["success"] = false;
    result["error"] = QString();
    result["createdCount"] = 0;
    result["reusedCount"] = 0;
    result["entries"] = QJsonArray();

    QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> parsedEntries;
    parsedEntries.reserve(entries.size());
    for (int i = 0; i < entries.size(); ++i)
    {
        if (!entries.at(i).isObject())
        {
            result["error"] = QString("Entry %1 is not a JSON object").arg(i);
            return result;
        }
        AssortedGraphicUtils::AssortedGraphicEntryItem entry;
        QString error;
        if (!EntryFromJson(entries.at(i).toObject(), entry, error, true))
        {
            result["error"] = QString("Entry %1: %2").arg(i).arg(error);
            return result;
        }
        parsedEntries.push_back(entry);
    }

    const QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> existingEntries =
            AssortedGraphicUtils::GetAssortedGraphicsFromROM();
    QHash<QString, int> combinedIndices;
    for (int i = 0; i < existingEntries.size(); ++i)
    {
        const QString fingerprint = EntryFingerprint(existingEntries[i]);
        if (!combinedIndices.contains(fingerprint))
        {
            combinedIndices.insert(fingerprint, i);
        }
    }

    QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> newEntries;
    QVector<int> resolvedIndices(parsedEntries.size(), -1);
    int reusedCount = 0;
    for (int i = 0; i < parsedEntries.size(); ++i)
    {
        const QString fingerprint = EntryFingerprint(parsedEntries[i]);
        if (combinedIndices.contains(fingerprint))
        {
            resolvedIndices[i] = combinedIndices.value(fingerprint);
            ++reusedCount;
            continue;
        }

        QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> knownEntries = existingEntries;
        knownEntries += newEntries;
        EnsureUniqueEntryNames(parsedEntries[i], knownEntries);

        const int combinedIndex = static_cast<int>(existingEntries.size() + newEntries.size());
        resolvedIndices[i] = combinedIndex;
        combinedIndices.insert(fingerprint, combinedIndex);
        newEntries.push_back(parsedEntries[i]);
    }

    if (!newEntries.isEmpty())
    {
        const QString error = AssortedGraphicUtils::AppendAssortedGraphicsToROM(newEntries);
        if (!error.isEmpty())
        {
            result["error"] = error;
            return result;
        }
    }

    QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> combinedEntries = existingEntries;
    combinedEntries += newEntries;
    QJsonArray resolvedArray;
    for (int index : resolvedIndices)
    {
        if ((index < 0) || (index >= combinedEntries.size()))
        {
            result["error"] = "Internal error: resolved entry index is out of range";
            return result;
        }
        resolvedArray.append(EntryToJson(combinedEntries[index], false));
    }

    result["success"] = true;
    result["createdCount"] = static_cast<int>(newEntries.size());
    result["reusedCount"] = reusedCount;
    result["entries"] = resolvedArray;
    return result;
}
