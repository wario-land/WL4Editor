#include "PatchIO.h"

#include "FileIOUtils.h"
#include "ROMUtils.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonValue>
#include <cmath>

namespace
{
    // ------------------------------ Generic helpers ------------------------------

    // Convert a path to the portable form used in the JSON: '/' separators and no "./".
    QString PortablePath(const QString &path)
    {
        QString portable = QDir::fromNativeSeparators(path);
        while(portable.startsWith("./"))
        {
            portable = portable.mid(2);
        }
        portable = QDir::cleanPath(portable);
        return portable == "." ? QString() : portable;
    }

    bool IsHexString(const QString &text)
    {
        for(const QChar &c : text)
        {
            const ushort value = c.unicode();
            if(!((value >= '0' && value <= '9') ||
                 (value >= 'A' && value <= 'F') ||
                 (value >= 'a' && value <= 'f')))
            {
                return false;
            }
        }
        return true;
    }

    // Parse an address which may be a JSON number or a "0x..." string.
    unsigned int JsonToUInt(const QJsonValue &value, bool *ok)
    {
        if(ok) *ok = false;

        if(value.isString())
        {
            QString text = value.toString().trimmed();
            if(text.startsWith("0x", Qt::CaseInsensitive))
            {
                text = text.mid(2);
            }
            bool parsed = false;
            unsigned int result = text.toUInt(&parsed, 16);
            if(ok) *ok = parsed;
            return parsed ? result : 0;
        }

        if(value.isDouble())
        {
            double number = value.toDouble();
            if(number < 0.0 || number > 4294967295.0 || std::floor(number) != number)
            {
                return 0;
            }
            if(ok) *ok = true;
            return static_cast<unsigned int>(number);
        }

        return 0;
    }

    QString Sha1OfFile(const QString &path, bool *ok)
    {
        if(ok) *ok = false;
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly)) return "";
        QCryptographicHash hash(QCryptographicHash::Sha1);
        if(!hash.addData(&file)) return "";
        if(ok) *ok = true;
        return QString::fromLatin1(hash.result().toHex());
    }

    QString PatchTypeToString(enum PatchType type)
    {
        switch(type)
        {
            case PatchType::Binary: return "Binary";
            case PatchType::Assembly: return "Assembly";
            case PatchType::C: return "C";
            case PatchType::C_dependency: return "C_dependency";
        }
        return "";
    }

    bool PatchTypeFromJson(const QJsonValue &value, enum PatchType &type)
    {
        if(value.isDouble())
        {
            int number = value.toInt(-1);
            if(number < static_cast<int>(PatchType::Binary) ||
               number > static_cast<int>(PatchType::C_dependency))
            {
                return false;
            }
            type = static_cast<enum PatchType>(number);
            return true;
        }

        if(value.isString())
        {
            QString name = value.toString().trimmed();
            if(name.compare("Binary", Qt::CaseInsensitive) == 0)
            {
                type = PatchType::Binary;
                return true;
            }
            if(name.compare("Assembly", Qt::CaseInsensitive) == 0)
            {
                type = PatchType::Assembly;
                return true;
            }
            if(name.compare("C", Qt::CaseInsensitive) == 0)
            {
                type = PatchType::C;
                return true;
            }
            if(name.compare("C_dependency", Qt::CaseInsensitive) == 0 ||
               name.compare("C-dependency", Qt::CaseInsensitive) == 0)
            {
                type = PatchType::C_dependency;
                return true;
            }

            bool parsed = false;
            int number = name.toInt(&parsed);
            if(parsed && number >= static_cast<int>(PatchType::Binary) &&
               number <= static_cast<int>(PatchType::C_dependency))
            {
                type = static_cast<enum PatchType>(number);
                return true;
            }
        }

        return false;
    }

    bool ExtensionMatchesPatchType(const QString &fileName, enum PatchType type)
    {
        const QString suffix = QFileInfo(fileName).suffix().toLower();
        switch(type)
        {
            case PatchType::Binary: return suffix == "bin";
            case PatchType::Assembly: return suffix == "s";
            case PatchType::C:
            case PatchType::C_dependency: return suffix == "c";
        }
        return false;
    }

    QString HexString(quint64 value)
    {
        return QString::number(value, 16).toUpper();
    }
}

// ------------------------------ Entry JSON ------------------------------

QJsonObject PatchIO::EntryToJson(const struct PatchEntryItem &entry)
{
    QJsonObject object;
    object["fileName"] = PortablePath(entry.FileName);
    object["patchType"] = PatchTypeToString(entry.PatchType);
    object["hookAddress"] = static_cast<int>(entry.HookAddress);
    object["hookString"] = entry.HookString;
    object["patchOffsetInHookString"] = entry.PatchOffsetInHookString == static_cast<unsigned int>(-1)
            ? -1 : static_cast<int>(entry.PatchOffsetInHookString);
    object["description"] = entry.Description;
    return object;
}

bool PatchIO::EntryFromJson(const QJsonObject &object, struct PatchEntryItem &entry, QString &error)
{
    error.clear();
    entry = PatchEntryItem{};
    entry.PatchOffsetInHookString = static_cast<unsigned int>(-1);

    if(!object.value("fileName").isString())
    {
        error = "fileName must be a string";
        return false;
    }
    const QString rawFileName = object.value("fileName").toString();
    entry.FileName = FileIOUtils::NormalizeRelativePath(rawFileName);
    if(!rawFileName.trimmed().isEmpty() && entry.FileName.isEmpty())
    {
        error = QString("fileName is not a valid ROM-relative path: %1").arg(rawFileName);
        return false;
    }

    if(!PatchTypeFromJson(object.value("patchType"), entry.PatchType))
    {
        error = "invalid or missing patchType";
        return false;
    }

    bool addressOk = false;
    entry.HookAddress = JsonToUInt(object.value("hookAddress"), &addressOk);
    if(!addressOk)
    {
        error = "invalid or missing hookAddress";
        return false;
    }

    // Normalize the hook string: no whitespace, no "P", uppercase hex digits.
    const QString rawHookString = object.value("hookString").toString();
    QString hookString;
    for(const QChar &c : rawHookString)
    {
        if(!c.isSpace()) hookString += c;
    }
    hookString = hookString.toUpper();
    if(hookString.contains(QStringLiteral("P")))
    {
        error = "hookString must not contain the P placeholder; use patchOffsetInHookString";
        return false;
    }
    if((hookString.length() % 2) != 0)
    {
        error = "hookString has an odd number of hex digits";
        return false;
    }
    if(!IsHexString(hookString))
    {
        error = "hookString contains non-hex characters";
        return false;
    }
    entry.HookString = hookString;

    const QJsonValue offsetValue = object.value("patchOffsetInHookString");
    if(offsetValue.isUndefined() || offsetValue.isNull())
    {
        entry.PatchOffsetInHookString = static_cast<unsigned int>(-1);
    }
    else if(offsetValue.isDouble())
    {
        const int offset = offsetValue.toInt(-2);
        if(offset < -1)
        {
            error = "invalid patchOffsetInHookString";
            return false;
        }
        entry.PatchOffsetInHookString = offset < 0
                ? static_cast<unsigned int>(-1) : static_cast<unsigned int>(offset);
    }
    else
    {
        error = "invalid patchOffsetInHookString";
        return false;
    }

    if(entry.PatchOffsetInHookString != static_cast<unsigned int>(-1) &&
       entry.PatchOffsetInHookString > static_cast<unsigned int>(entry.HookString.length() / 2))
    {
        error = "patchOffsetInHookString is outside hookString";
        return false;
    }

    entry.Description = object.value("description").toString();
    return true;
}

// ------------------------------ Document JSON ------------------------------

QJsonObject PatchIO::BuildDocument(const QVector<struct PatchEntryItem> &entries,
                                   const QString &romDirectory,
                                   QStringList *warnings)
{
    QJsonArray entryArray;
    QJsonObject filesObject;
    const QString romDir = QDir::cleanPath(QDir::fromNativeSeparators(romDirectory));

    for(const struct PatchEntryItem &entry : entries)
    {
        const QJsonObject entryObject = EntryToJson(entry);
        entryArray.append(entryObject);

        const QString portableName = entryObject.value("fileName").toString();
        if(portableName.isEmpty()) continue;

        const QString absolutePath = QDir(romDir).absoluteFilePath(portableName);
        const QFileInfo fileInfo(absolutePath);

        QJsonObject fileObject;
        if(!fileInfo.exists() || !fileInfo.isFile())
        {
            fileObject["missing"] = true;
            if(warnings) warnings->append(QString("Patch file is missing: %1").arg(portableName));
        }
        else
        {
            bool hashOk = false;
            const QString sha1 = Sha1OfFile(absolutePath, &hashOk);
            fileObject["size"] = static_cast<double>(fileInfo.size());
            if(hashOk)
            {
                fileObject["sha1"] = sha1;
            }
            else
            {
                fileObject["missing"] = true;
                if(warnings) warnings->append(QString("Patch file cannot be read: %1").arg(portableName));
            }
        }
        filesObject[portableName] = fileObject;
    }

    QJsonObject root;
    root["format"] = QString(FormatName);
    root["version"] = FormatVersion;
    root["entries"] = entryArray;
    root["files"] = filesObject;
    return root;
}

bool PatchIO::ParseDocument(const QJsonDocument &document, ImportDocument &result, QString &error)
{
    result = ImportDocument{};
    error.clear();

    if(!document.isObject())
    {
        error = "The JSON root is not an object";
        return false;
    }
    const QJsonObject root = document.object();

    if(root.contains("format") && root.value("format").toString() != QString(FormatName))
    {
        error = "The JSON file is not a WL4Editor patch list";
        return false;
    }

    if(root.contains("version"))
    {
        const int version = root.value("version").toInt(-1);
        if(version < 0)
        {
            error = "Invalid patch list version";
            return false;
        }
        if(version > FormatVersion)
        {
            error = "The patch list was written by a newer WL4Editor version";
            return false;
        }
    }

    if(!root.value("entries").isArray())
    {
        error = "The JSON file does not contain an entries array";
        return false;
    }
    if(!root.value("files").isObject())
    {
        error = "The JSON file does not contain a files object";
        return false;
    }

    // Parse the file manifest.
    const QJsonObject filesObject = root.value("files").toObject();
    for(auto it = filesObject.constBegin(); it != filesObject.constEnd(); ++it)
    {
        const QString key = FileIOUtils::NormalizeRelativePath(it.key());
        if(key.isEmpty())
        {
            result.Warnings.append(QString("Ignoring invalid file path in manifest: %1").arg(it.key()));
            continue;
        }
        if(!it.value().isObject())
        {
            result.Warnings.append(QString("Ignoring invalid manifest record for file: %1").arg(it.key()));
            continue;
        }

        const QJsonObject recordObject = it.value().toObject();
        FileRecord record;
        record.Missing = recordObject.value("missing").toBool(false);

        if(recordObject.value("size").isDouble())
        {
            const double size = recordObject.value("size").toDouble();
            if(size >= 0.0 && std::floor(size) == size)
            {
                record.HasSize = true;
                record.Size = static_cast<qint64>(size);
            }
            else
            {
                result.Warnings.append(QString("Ignoring invalid size for file: %1").arg(it.key()));
            }
        }

        if(recordObject.value("sha1").isString())
        {
            const QString sha1 = recordObject.value("sha1").toString().trimmed().toLower();
            if(sha1.length() == 40 && IsHexString(sha1))
            {
                record.HasSha1 = true;
                record.Sha1 = sha1;
            }
            else
            {
                result.Warnings.append(QString("Ignoring invalid SHA-1 for file: %1").arg(it.key()));
            }
        }

        if(result.Files.contains(key))
        {
            result.Warnings.append(QString("Duplicate manifest record for file: %1").arg(key));
        }
        result.Files[key] = record;
    }

    // Parse the patch entries.
    const QJsonArray entryArray = root.value("entries").toArray();
    const int entryCount = static_cast<int>(entryArray.size());
    for(int i = 0; i < entryCount; ++i)
    {
        if(!entryArray.at(i).isObject())
        {
            result.EntryErrors.append(QString("entry %1: not a JSON object").arg(i));
            continue;
        }

        struct PatchEntryItem entry;
        QString entryError;
        if(!EntryFromJson(entryArray.at(i).toObject(), entry, entryError))
        {
            result.EntryErrors.append(QString("entry %1: %2").arg(i).arg(entryError));
            continue;
        }
        result.Entries.append(entry);
    }

    return true;
}

// ------------------------------ Import checks ------------------------------

QVector<PatchIO::FileCheckIssue> PatchIO::CheckEntryFiles(
        const QVector<struct PatchEntryItem> &entries,
        const QHash<QString, FileRecord> &files,
        const QString &romDirectory)
{
    QVector<FileCheckIssue> issues;
    const QDir romDir(QDir::cleanPath(QDir::fromNativeSeparators(romDirectory)));
    const int entryCount = static_cast<int>(entries.size());

    for(int i = 0; i < entryCount; ++i)
    {
        const struct PatchEntryItem &entry = entries.at(i);
        if(entry.FileName.isEmpty()) continue;

        FileCheckIssue issue;
        issue.EntryIndex = i;
        issue.FileName = entry.FileName;

        const QString relativePath = FileIOUtils::NormalizeRelativePath(entry.FileName);
        if(relativePath.isEmpty())
        {
            issue.MissingFile = true;
            issue.Reason = "fileName is not a valid ROM-relative path";
            issues.append(issue);
            continue;
        }

        const QString absolutePath = romDir.absoluteFilePath(relativePath);
        if(!FileIOUtils::IsPathInsideDirectory(absolutePath, romDir.absolutePath()))
        {
            issue.MissingFile = true;
            issue.Reason = "file path escapes the ROM directory";
            issues.append(issue);
            continue;
        }

        const QFileInfo fileInfo(absolutePath);
        if(!fileInfo.exists() || !fileInfo.isFile())
        {
            issue.MissingFile = true;
            issue.Reason = fileInfo.exists() ? "not a regular file" : "missing file";
            issues.append(issue);
            continue;
        }
        if(!fileInfo.isReadable())
        {
            issue.MissingFile = true;
            issue.Reason = "file is not readable";
            issues.append(issue);
            continue;
        }

        const auto manifestIter = files.constFind(relativePath);
        if(manifestIter == files.constEnd() ||
           (!manifestIter->HasSize && !manifestIter->HasSha1 && !manifestIter->Missing))
        {
            issue.Reason = "no size/SHA-1 record in manifest";
            issues.append(issue);
            continue;
        }
        if(manifestIter->Missing)
        {
            issue.Reason = "recorded as missing at export; no size/SHA-1 to verify";
            issues.append(issue);
            continue;
        }

        bool mismatch = false;
        QStringList reasonParts;
        if(manifestIter->HasSize && fileInfo.size() != manifestIter->Size)
        {
            mismatch = true;
            reasonParts.append(QString("size mismatch (expected %1, actual %2)")
                               .arg(manifestIter->Size).arg(fileInfo.size()));
        }
        if(manifestIter->HasSha1)
        {
            bool hashOk = false;
            const QString actualSha1 = Sha1OfFile(absolutePath, &hashOk);
            if(!hashOk)
            {
                issue.MissingFile = true;
                issue.Reason = "cannot read file to compute SHA-1";
                issues.append(issue);
                continue;
            }
            if(actualSha1.compare(manifestIter->Sha1, Qt::CaseInsensitive) != 0)
            {
                mismatch = true;
                reasonParts.append(QString("SHA-1 mismatch (expected %1, actual %2)")
                                   .arg(manifestIter->Sha1, actualSha1));
            }
        }

        if(mismatch)
        {
            issue.Reason = reasonParts.join("; ");
            issue.RecordsSizeOrHashMismatch = true;
            issues.append(issue);
        }
    }

    return issues;
}

PatchIO::ImportValidation PatchIO::ValidateForImport(
        const QVector<struct PatchEntryItem> &existingEntries,
        const QVector<struct PatchEntryItem> &importedEntries,
        const QString &romDirectory)
{
    ImportValidation result;
    QVector<struct PatchEntryItem> accepted = existingEntries;
    const int existingCount = static_cast<int>(existingEntries.size());
    const int importedCount = static_cast<int>(importedEntries.size());

    auto appendError = [&result](int index, const struct PatchEntryItem &entry, const QString &message)
    {
        result.Errors.append(QString("entry %1 (%2): %3")
                             .arg(index)
                             .arg(entry.FileName.isEmpty() ? QString("(no file)") : entry.FileName)
                             .arg(message));
    };

    for(int i = 0; i < importedCount; ++i)
    {
        const struct PatchEntryItem &entry = importedEntries.at(i);

        if(entry.PatchType < PatchType::Binary || entry.PatchType > PatchType::C_dependency)
        {
            appendError(i, entry, "invalid patch type");
            continue;
        }

        const QString relativePath = FileIOUtils::NormalizeRelativePath(entry.FileName);
        const bool hasFile = !entry.FileName.isEmpty();
        if(hasFile && relativePath.isEmpty())
        {
            appendError(i, entry, "fileName is not a valid ROM-relative path");
        }
        if(entry.FileName.contains(QStringLiteral(";")))
        {
            appendError(i, entry, "fileName contains ';'");
        }
        if(entry.Description.contains(QStringLiteral(";")))
        {
            appendError(i, entry, "description contains ';'");
        }
        if(hasFile && !relativePath.isEmpty() && !ExtensionMatchesPatchType(relativePath, entry.PatchType))
        {
            appendError(i, entry, "file extension does not match the patch type");
        }

        const int hookLength = entry.GetHookLength();
        const quint64 hookStart = entry.HookAddress;
        const quint64 hookEnd = hookStart + static_cast<quint64>(hookLength);
        if(hookEnd > WL4Constants::AvailableSpaceBeginningInROM)
        {
            appendError(i, entry, QString("hook range (0x%1-0x%2) overlaps the save chunk area")
                        .arg(HexString(hookStart), HexString(hookEnd - 1)));
        }
        if(entry.HookAddress < 4 && entry.PatchType != PatchType::C_dependency)
        {
            appendError(i, entry, "a patch on the first 4 bytes of the ROM is not allowed");
        }

        if(entry.PatchType == PatchType::C_dependency)
        {
            if(entry.HookAddress)
            {
                appendError(i, entry, "C_dependency entries cannot set HookAddress");
            }
            if(!entry.HookString.isEmpty())
            {
                appendError(i, entry, "C_dependency entries cannot set HookString");
            }
            if(relativePath.isEmpty())
            {
                appendError(i, entry, "C_dependency entries must reference a file");
            }
        }

        if(entry.PatchOffsetInHookString != static_cast<unsigned int>(-1) &&
           entry.PatchOffsetInHookString > static_cast<unsigned int>(entry.HookString.length() / 2))
        {
            appendError(i, entry, "patchOffsetInHookString is outside hookString");
        }

        if(hasFile)
        {
            if(entry.PatchOffsetInHookString == static_cast<unsigned int>(-1) &&
               entry.PatchType != PatchType::C_dependency)
            {
                appendError(i, entry, "a file is specified but the hook does not contain the P identifier");
            }
            if(!relativePath.isEmpty())
            {
                const QString absolutePath = QDir(romDirectory).absoluteFilePath(relativePath);
                if(!FileIOUtils::IsPathInsideDirectory(absolutePath, romDirectory))
                {
                    appendError(i, entry, "file path is outside the ROM directory");
                }
            }
        }
        else
        {
            if(entry.PatchOffsetInHookString != static_cast<unsigned int>(-1))
            {
                appendError(i, entry, "a patch address offset is specified but no file is given");
            }
            if(entry.HookString.isEmpty())
            {
                appendError(i, entry, "entry has neither a file nor hook data");
            }
        }

        if(!relativePath.isEmpty())
        {
            bool duplicateFile = false;
            for(const struct PatchEntryItem &other : accepted)
            {
                const QString otherPath = FileIOUtils::NormalizeRelativePath(other.FileName);
                if(!otherPath.isEmpty() && otherPath == relativePath)
                {
                    duplicateFile = true;
                    break;
                }
            }
            if(duplicateFile)
            {
                appendError(i, entry, QString("another entry already uses the file: %1").arg(relativePath));
            }
        }

        accepted.append(entry);
    }

    // Hook range conflict check. The mount point of a hook is a range of bytes, not a single
    // address: [HookAddress, HookAddress + GetHookLength()).
    struct HookRange
    {
        QString Label;
        QString FileName;
        quint64 Start;
        quint64 End;
    };

    QVector<HookRange> existingRanges;
    for(int i = 0; i < existingCount; ++i)
    {
        const struct PatchEntryItem &entry = existingEntries.at(i);
        if(entry.PatchType == PatchType::C_dependency) continue;
        const quint64 start = entry.HookAddress;
        const quint64 end = start + static_cast<quint64>(entry.GetHookLength());
        if(end > start)
        {
            existingRanges.append(HookRange{ QString("current entry %1").arg(i), entry.FileName, start, end });
        }
    }

    QVector<HookRange> importedRanges;
    importedRanges.reserve(importedCount);
    for(int i = 0; i < importedCount; ++i)
    {
        const struct PatchEntryItem &entry = importedEntries.at(i);
        if(entry.PatchType == PatchType::C_dependency) continue;
        const quint64 start = entry.HookAddress;
        const quint64 end = start + static_cast<quint64>(entry.GetHookLength());
        if(end > start)
        {
            importedRanges.append(HookRange{ QString("imported entry %1").arg(i), entry.FileName, start, end });
        }
    }

    auto rangesOverlap = [](const HookRange &a, const HookRange &b)
    {
        return a.Start < b.End && b.Start < a.End;
    };

    for(const HookRange &importedRange : importedRanges)
    {
        for(const HookRange &existingRange : existingRanges)
        {
            if(!rangesOverlap(importedRange, existingRange)) continue;
            QString message = QString("%1 (%2) hook 0x%3-0x%4 overlaps %5 (%6) hook 0x%7-0x%8");
            message = message.arg(importedRange.Label);
            message = message.arg(importedRange.FileName.isEmpty() ? QString("(no file)") : importedRange.FileName);
            message = message.arg(HexString(importedRange.Start));
            message = message.arg(HexString(importedRange.End - 1));
            message = message.arg(existingRange.Label);
            message = message.arg(existingRange.FileName.isEmpty() ? QString("(no file)") : existingRange.FileName);
            message = message.arg(HexString(existingRange.Start));
            message = message.arg(HexString(existingRange.End - 1));
            result.HookConflicts.append(message);
        }
    }

    const int importedRangeCount = static_cast<int>(importedRanges.size());
    for(int i = 0; i < importedRangeCount; ++i)
    {
        for(int j = 0; j < i; ++j)
        {
            if(!rangesOverlap(importedRanges.at(i), importedRanges.at(j))) continue;
            QString message = QString("%1 (%2) hook 0x%3-0x%4 overlaps %5 (%6) hook 0x%7-0x%8");
            message = message.arg(importedRanges.at(i).Label);
            message = message.arg(importedRanges.at(i).FileName.isEmpty() ? QString("(no file)") : importedRanges.at(i).FileName);
            message = message.arg(HexString(importedRanges.at(i).Start));
            message = message.arg(HexString(importedRanges.at(i).End - 1));
            message = message.arg(importedRanges.at(j).Label);
            message = message.arg(importedRanges.at(j).FileName.isEmpty() ? QString("(no file)") : importedRanges.at(j).FileName);
            message = message.arg(HexString(importedRanges.at(j).Start));
            message = message.arg(HexString(importedRanges.at(j).End - 1));
            result.HookConflicts.append(message);
        }
    }

    return result;
}
