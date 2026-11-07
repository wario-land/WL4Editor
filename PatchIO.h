#ifndef PATCHIO_H
#define PATCHIO_H

#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <QVector>

#include "PatchUtils.h"

// Serialization and import validation helpers for the Patch Manager.
//
// The JSON document is a portable manifest of the patch entries and of the code files they
// reference. It intentionally does not store the target-derived fields PatchAddress and
// SubstitutedBytes: those are recalculated by SavePatchesToROM when the patch list is saved.
//
// The file paths are always ROM-relative. The target ROM directory is resolved by the caller
// from the ROM that is currently open, so an exported list can be imported into a ROM which
// lives in a different directory as long as the code files are placed at the same relative
// paths under that ROM directory.
namespace PatchIO
{
    inline constexpr int FormatVersion = 1;
    inline constexpr const char *FormatName = "WL4Editor.patchList";

    // The manifest record of one code file, keyed by its ROM-relative path.
    struct FileRecord
    {
        bool Missing = false;
        bool HasSize = false;
        qint64 Size = 0;
        bool HasSha1 = false;
        QString Sha1; // lowercase hex
    };

    // The parsed JSON document.
    struct ImportDocument
    {
        QVector<struct PatchEntryItem> Entries;
        QHash<QString, FileRecord> Files; // key: normalized ROM-relative path
        QStringList EntryErrors;          // "entry N: reason"
        QStringList Warnings;
    };

    // One problem found while checking the code files of imported entries.
    struct FileCheckIssue
    {
        int EntryIndex = -1;
        QString FileName;
        QString Reason;
        // True when the file is missing, unreadable, not a regular file, or its path is not
        // a valid ROM-relative path. The caller must abort the whole import in that case.
        bool MissingFile = false;
        // True only for an actual size/SHA-1 mismatch with an existing file. Only these
        // entries are eligible for the hookstring re-parse check.
        bool RecordsSizeOrHashMismatch = false;
    };

    // Problems which cancel the whole import.
    struct ImportValidation
    {
        QStringList Errors;
        QStringList HookConflicts;
    };

    // Convert one entry to the documented JSON schema. PatchAddress and SubstitutedBytes are
    // not exported. The fileName only has its separators normalized; no validation is done.
    QJsonObject EntryToJson(const struct PatchEntryItem &entry);

    // Parse one entry of the documented JSON schema. The entry is value-initialized first.
    bool EntryFromJson(const QJsonObject &object, struct PatchEntryItem &entry, QString &error);

    // Build the complete export document for the given entries. Missing/unreadable files are
    // recorded as {"missing": true} and reported through warnings.
    QJsonObject BuildDocument(const QVector<struct PatchEntryItem> &entries,
                              const QString &romDirectory,
                              QStringList *warnings = nullptr);

    // Parse a complete import document. Document-level errors return false; per-entry errors
    // are collected in ImportDocument::EntryErrors.
    bool ParseDocument(const QJsonDocument &document, ImportDocument &result, QString &error);

    // Check the code files of the imported entries against the manifest. Missing/unusable
    // files set MissingFile, size/SHA-1 mismatches set RecordsSizeOrHashMismatch, and the
    // remaining manifest problems are reported as soft issues.
    QVector<FileCheckIssue> CheckEntryFiles(const QVector<struct PatchEntryItem> &entries,
                                            const QHash<QString, FileRecord> &files,
                                            const QString &romDirectory);

    // Check all imported entries for structural errors and hook range conflicts against the
    // current patch table (the entries which SavePatchesToROM will write). All problems are
    // collected instead of stopping at the first one.
    ImportValidation ValidateForImport(const QVector<struct PatchEntryItem> &existingEntries,
                                       const QVector<struct PatchEntryItem> &importedEntries,
                                       const QString &romDirectory);
}

#endif // PATCHIO_H
