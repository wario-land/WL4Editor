#ifndef ASSORTEDGRAPHICIO_H
#define ASSORTEDGRAPHICIO_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include "AssortedGraphicUtils.h"

namespace LevelComponents
{
    class Level;
    class Room;
    class Tileset;
}

// Serialization helpers for the Graphic Manager entries which are used by the Room/Level scripts
// and by the Graphic Manager import/export buttons.
//
// The JSON schema is a script-facing format. Scripts are free to add their own keys around the
// entry objects, but the entry object keys produced here are the documented default schema (see
// Scripts/README.md). The same schema is used by the Graphic Manager import/export buttons so a
// file exported by one side can be imported by the other side.
namespace AssortedGraphicIO
{
    // Convert an entry into the documented JSON schema.
    //
    // includeData == true:
    //   The source of every data part is decided from the actual data: a vanilla ROM address whose
    //   data still matches the entry is exported as "address", everything else is exported as
    //   "data". This is the format used by the export scripts and by the Graphic Manager export.
    //
    // includeData == false:
    //   Every part is forced to "address". This is the format used for the resolved entries which
    //   are returned by ImportEntries and passed to the Apply APIs.
    QJsonObject EntryToJson(const AssortedGraphicUtils::AssortedGraphicEntryItem &entry, bool includeData);

    // Parse one entry object of the documented JSON schema.
    //
    // loadAddressDataFromRom:
    //   When true, the data parts which use an address are read from the currently opened ROM so
    //   the entry can be fingerprinted and previewed. The Room/Level import scripts and the
    //   Graphic Manager import both need this.
    bool EntryFromJson(const QJsonObject &object, AssortedGraphicUtils::AssortedGraphicEntryItem &entry,
                       QString &error, bool loadAddressDataFromRom);

    // Build the fingerprint which is used to deduplicate entries. Vanilla addresses are part of the
    // fingerprint as addresses; custom data is hashed.
    QString EntryFingerprint(const AssortedGraphicUtils::AssortedGraphicEntryItem &entry);

    // Make the TileDataName/MappingDataName pair of the entry unique among the given entries.
    void EnsureUniqueEntryNames(AssortedGraphicUtils::AssortedGraphicEntryItem &entry,
                                const QVector<AssortedGraphicUtils::AssortedGraphicEntryItem> &existingEntries);

    // Build the graphics section of the current Room. roomId < 0 means the current Room.
    QJsonObject BuildRoomGraphics(int roomId);

    // Build the graphics section of the current Level.
    QJsonObject BuildLevelGraphics();

    // Build the list of all entries of the ROM.
    QJsonObject BuildAllEntries();

    // Import the given entry objects. The result object has the keys:
    //   success, error, createdCount, reusedCount, entries
    // The entries array is in the same order as the input array and contains the resolved entries
    // (all data parts are "address").
    QJsonObject ImportEntries(const QJsonArray &entries);
}

#endif // ASSORTEDGRAPHICIO_H
