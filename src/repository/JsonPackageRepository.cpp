/**
 * @file   JsonPackageRepository.cpp
 * @brief  JSON persistence implementation for IPackageRepository.
 *
 * @author Huynh Phuc Nguyen
 * @date   2026-06-10
 *
 * @update
 * @author Duong Anh Hao
 * @date   2026-06-16
 * @changelog
 *   - Fixed missing Qt headers (<QStringList>, <QByteArray>, <QJsonParseError>).
 *   - Included concrete state headers to resolve 'undeclared identifier' and
 *     std::make_unique errors.
 *   - Refactored parameters to use std::string to strictly match the
 *     IPackageRepository interface, ensuring the domain layer remains Qt-free.
 *
 * @update
 * @author Lam Hong Hai Hoang Le
 * @date   2026-06-22
 * @changelog
 *   - Fixed crash related to using std::string instead of QString
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-06-24
 * @changelog
 *   - Change packageFromJson() to use load() instead of constructor.
 *
 * @update
 * @author Do Minh Khang
 * @date   2026-07-11
 * @changelog
 *   - Add package name field to metadataToJson() / metadataFromJson().
 *
 * @update
 * @author Do Minh Khang
 * @date   2026-07-16
 * @changelog
 *   - Implement findByCriteria().
 *
 * @update
 * @author Huynh Phuc Nguyen
 * @date   2026-07-19
 * @changelog
 *   - Replace all local categoryToString / categoryFromString / stateIdToString /
 *     stateIdFromString / dateToString / dateFromString definitions with calls to
 *     the shared helpers in RepositoryHelpers.h. No behaviour change; this removes
 *     ~60 lines of code that were duplicated verbatim in SqlitePackageRepository.cpp.
 * 
 * @update
 * @author Nguyen Viet Bach
 * @date   2026-07-25
 * @changelog
 *   - Implement exportToJson / importFromJson / exportToCsv / importFromCsv.
 *     exportToJson writes a QJsonDocument (Indented) to an arbitrary path,
 *     reusing the existing packageToJson() helpers.
 *     importFromJson reads the same format and upserts into m_store (add or
 *     update by id) without touching m_filePath.
 *     exportToCsv writes a 27-column header + data rows via QTextStream;
 *     fields containing commas or double-quotes are RFC 4180-quoted.
 *     importFromCsv parses those same rows and upserts into m_store.
 * @update
 * @author Duong Anh Hao
 * @date   2026-07-28
 * @changelog
 *   - Fixed timezone bug in findByCriteria() where std::chrono::system_clock (UTC) 
 *     caused incorrect date matching for "Imported Today" and "Export Due Today".
 *   - Replaced UTC time fetching with QDate::currentDate() to strictly evaluate 
 *     filtering logic against the local system time.
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-07-31
 * @changelog
 *   - Adding condition for late package at build and bind function.
 *   - Correct the findByCriteria() function for Overdue.
 * 
 * @update
 * @author Duong Anh Hao
 * @date   2026-08-02
 * @changelog
 *   - Update findByCriteria() memory filtering logic to evaluate the new 
 *     importDate and exportDate optional fields.
 */
#include "repository/JsonPackageRepository.h"
#include "repository/RepositoryHelpers.h"

#include "domain/states/PackageStateId.h"
#include "domain/states/InStorageState.h"
#include "domain/states/DispatchedState.h"
#include "domain/states/MissingState.h"
#include "domain/states/OverdueState.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QJsonParseError>
#include <QDate>

#include <stdexcept>
#include <chrono>
#include <memory>
#include <algorithm>
#include <cctype>

namespace wms::repository
{
    // --Construction--

    JsonPackageRepository::JsonPackageRepository(QString filePath)
        : m_filePath{ std::move(filePath) }
    {
        // Attempt to load existing data; silently ignore if file doesn't exist yet.
        try { load(); }
        catch (...) {}
    }

    // --IPackageRepository - Read--

    std::vector<domain::Package> JsonPackageRepository::getAll() const
    {
        std::vector<domain::Package> result;
        result.reserve(m_store.size());
        for (const auto& [id, pkg] : m_store)
            result.push_back(pkg);
        return result;
    }

    std::optional<domain::Package> JsonPackageRepository::getById(const std::string& id) const
    {
        auto it = m_store.find(id);
        if (it == m_store.end())
            return std::nullopt;
        return it->second;
    }

    // --IPackageRepository - Write--

    void JsonPackageRepository::add(domain::Package package)
    {
        const std::string id = package.id();
        if (m_store.count(id))
            throw std::runtime_error(
                "JsonPackageRepository::add - package id already exists: " + id);
        m_store.emplace(id, std::move(package));
    }

    void JsonPackageRepository::update(domain::Package package)
    {
        const std::string id = package.id();
        if (!m_store.count(id))
            throw std::runtime_error(
                "JsonPackageRepository::update - package not found: " + id);
        m_store.at(id) = std::move(package);
    }

    void JsonPackageRepository::remove(const std::string& id)
    {
        if (!m_store.erase(id))
            throw std::runtime_error(
                "JsonPackageRepository::remove - package not found: " + id);
    }

    // --IPackageRepository - Persistence--

    void JsonPackageRepository::save()
    {
        QJsonArray array;
        for (const auto& [id, pkg] : m_store)
            array.append(packageToJson(pkg));

        QJsonDocument doc{ array };

        QFile file{ m_filePath };
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            throw std::runtime_error(
                "JsonPackageRepository::save - cannot open file: " +
                m_filePath.toStdString());

        file.write(doc.toJson(QJsonDocument::Indented));
    }

    void JsonPackageRepository::load()
    {
        QFile file{ m_filePath };
        if (!file.exists())
            return; // Fresh start - no error

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            throw std::runtime_error(
                "JsonPackageRepository::load - cannot open file: " +
                m_filePath.toStdString());

        const QByteArray raw = file.readAll();
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);

        if (parseError.error != QJsonParseError::NoError)
            throw std::runtime_error(
                "JsonPackageRepository::load - JSON parse error: " +
                parseError.errorString().toStdString());

        if (!doc.isArray())
            throw std::runtime_error(
                "JsonPackageRepository::load - root element must be a JSON array");

        m_store.clear();
        const QJsonArray array = doc.array();
        for (const QJsonValue& val : array)
        {
            domain::Package pkg = packageFromJson(val.toObject());
            const std::string id = pkg.id();
            m_store.emplace(id, std::move(pkg));
        }
    }

    // --Bulk I/O--

    void JsonPackageRepository::exportToJson(const std::string& filePath) const
    {
        QJsonArray array;
        for (const auto& [id, pkg] : m_store)
            array.append(packageToJson(pkg));

        QJsonDocument doc{ array };

        QFile file{ QString::fromStdString(filePath) };
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            throw std::runtime_error(
                "JsonPackageRepository::exportToJson - cannot open file: " + filePath);

        file.write(doc.toJson(QJsonDocument::Indented));
    }

    void JsonPackageRepository::importFromJson(const std::string& filePath)
    {
        QFile file{ QString::fromStdString(filePath) };
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            throw std::runtime_error(
                "JsonPackageRepository::importFromJson - cannot open file: " + filePath);

        const QByteArray raw = file.readAll();
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);

        if (parseError.error != QJsonParseError::NoError)
            throw std::runtime_error(
                "JsonPackageRepository::importFromJson - JSON parse error: " +
                parseError.errorString().toStdString());

        if (!doc.isArray())
            throw std::runtime_error(
                "JsonPackageRepository::importFromJson - root element must be a JSON array");

        const QJsonArray array = doc.array();
        for (const QJsonValue& val : array)
        {
            domain::Package pkg = packageFromJson(val.toObject());
            const std::string id = pkg.id();
            if (m_store.count(id))
                m_store.at(id) = pkg;       // update existing
            else
                m_store.emplace(id, pkg);   // insert new
        }
    }

    namespace
    {
        // CSV helpers (local to this translation unit)

        /// Wraps a field in double-quotes if it contains a comma, double-quote,
        /// or newline (RFC 4180 §2.6-2.7). Internal quotes are escaped as "".
        QString csvQuote(const QString& field)
        {
            if (!field.contains(',') && !field.contains('"') && !field.contains('\n'))
                return field;
            return '"' + QString(field).replace('"', """""")
                                       + '"';
        }

        /// CSV header (must stay in sync with the write/read code below).
        constexpr const char* CSV_HEADER =
            "id,state,name,category,weight,dimLength,dimWidth,dimHeight,cost,description,"
            "importDate,expectedExportDate,importVehicle,exportVehicle,containerId,"
            "srcStreet,srcCity,srcCountry,srcPostal,"
            "dstStreet,dstCity,dstCountry,dstPostal,"
            "zone,aisle,shelf,slot";

        /// Parse one CSV line into tokens, respecting RFC 4180 double-quote escaping.
        QStringList parseCsvLine(const QString& line)
        {
            QStringList tokens;
            QString current;
            bool inQuotes = false;

            for (int i = 0; i < line.size(); ++i)
            {
                const QChar ch = line[i];
                if (inQuotes)
                {
                    if (ch == '"')
                    {
                        // Peek: two consecutive double-quotes = escaped quote
                        if (i + 1 < line.size() && line[i + 1] == '"')
                        {
                            current += '"';
                            ++i;
                        }
                        else
                        {
                            inQuotes = false;
                        }
                    }
                    else
                    {
                        current += ch;
                    }
                }
                else
                {
                    if (ch == '"')
                    {
                        inQuotes = true;
                    }
                    else if (ch == ',')
                    {
                        tokens.append(current);
                        current.clear();
                    }
                    else
                    {
                        current += ch;
                    }
                }
            }
            tokens.append(current); // last field
            return tokens;
        }
    } // anonymous namespace

    void JsonPackageRepository::exportToCsv(const std::string& filePath) const
    {
        QFile file{ QString::fromStdString(filePath) };
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            throw std::runtime_error(
                "JsonPackageRepository::exportToCsv - cannot open file: " + filePath);

        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);

        out << CSV_HEADER << "\n";

        for (const auto& [id, pkg] : m_store)
        {
            const auto& meta  = pkg.metadata();
            const auto& log   = pkg.logistics();
            const auto& loc   = pkg.location();
            const auto& src   = pkg.source();
            const auto& dst   = pkg.destination();

            out << csvQuote(QString::fromStdString(pkg.id()))                        << ','
                << csvQuote(helpers::stateIdToString(pkg.currentStateId()))          << ','
                << csvQuote(QString::fromStdString(meta.name))                       << ','
                << csvQuote(helpers::categoryToString(meta.category))                << ','
                << meta.weight                                                       << ','
                << meta.dimensions.length                                            << ','
                << meta.dimensions.width                                             << ','
                << meta.dimensions.height                                            << ','
                << meta.cost                                                         << ','
                << csvQuote(QString::fromStdString(meta.description))               << ','
                << csvQuote(helpers::dateToString(log.importDate))                   << ','
                << csvQuote(helpers::dateToString(log.expectedExportDate))           << ','
                << csvQuote(QString::fromStdString(log.importVehicle))               << ','
                << csvQuote(QString::fromStdString(log.exportVehicle))               << ','
                << csvQuote(QString::fromStdString(log.containerId))                 << ','
                << csvQuote(QString::fromStdString(src.street))                      << ','
                << csvQuote(QString::fromStdString(src.city))                        << ','
                << csvQuote(QString::fromStdString(src.country))                     << ','
                << csvQuote(QString::fromStdString(src.postalCode))                  << ','
                << csvQuote(QString::fromStdString(dst.street))                      << ','
                << csvQuote(QString::fromStdString(dst.city))                        << ','
                << csvQuote(QString::fromStdString(dst.country))                     << ','
                << csvQuote(QString::fromStdString(dst.postalCode))                  << ','
                << csvQuote(QString::fromStdString(loc.zone))                        << ','
                << csvQuote(QString::fromStdString(loc.aisle))                       << ','
                << loc.shelf                                                         << ','
                << loc.slot                                                          << '\n';
        }
    }

    void JsonPackageRepository::importFromCsv(const std::string& filePath)
    {
        QFile file{ QString::fromStdString(filePath) };
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            throw std::runtime_error(
                "JsonPackageRepository::importFromCsv - cannot open file: " + filePath);

        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);

        // Skip header
        if (!in.atEnd())
            in.readLine();

        int lineNumber = 1;
        while (!in.atEnd())
        {
            ++lineNumber;
            const QString line = in.readLine().trimmed();
            if (line.isEmpty())
                continue;

            const QStringList f = parseCsvLine(line);
            if (f.size() < 27)
                throw std::runtime_error(
                    "JsonPackageRepository::importFromCsv - expected 27 columns at line " +
                    std::to_string(lineNumber));

            // Column mapping (0-indexed):
            // 0 id, 1 state, 2 name, 3 category, 4 weight,
            // 5 dimLength, 6 dimWidth, 7 dimHeight, 8 cost, 9 description,
            // 10 importDate, 11 expectedExportDate,
            // 12 importVehicle, 13 exportVehicle, 14 containerId,
            // 15-18 src(street,city,country,postal),
            // 19-22 dst(street,city,country,postal),
            // 23 zone, 24 aisle, 25 shelf, 26 slot
            domain::PackageMetadata meta{
                f[2].toStdString(),
                helpers::categoryFromString(f[3]),
                f[4].toDouble(),
                domain::Dimension{ f[5].toDouble(), f[6].toDouble(), f[7].toDouble() },
                f[8].toDouble(),
                f[9].toStdString()
            };
            domain::LogisticsInfo log{
                helpers::dateFromString(f[10]),
                helpers::dateFromString(f[11]),
                f[12].toStdString(),
                f[13].toStdString(),
                f[14].toStdString()
            };
            domain::Address src{ f[15].toStdString(), f[16].toStdString(),
                                  f[17].toStdString(), f[18].toStdString() };
            domain::Address dst{ f[19].toStdString(), f[20].toStdString(),
                                  f[21].toStdString(), f[22].toStdString() };
            domain::StorageLocation loc{
                f[23].toStdString(), f[24].toStdString(),
                f[25].toInt(), f[26].toInt()
            };

            domain::Package pkg = domain::Package::load(
                f[0].toStdString(),
                std::move(meta),
                std::move(src),
                std::move(dst),
                std::move(log),
                std::move(loc),
                helpers::stateIdFromString(f[1])
            );

            const std::string id = pkg.id();
            if (m_store.count(id))
                m_store.at(id) = pkg;       // update existing
            else
                m_store.emplace(id, pkg);   // insert new
        }
    }


    QJsonObject JsonPackageRepository::packageToJson(const domain::Package& pkg)
    {
        QJsonObject obj;
        obj["id"]          = QString::fromStdString(pkg.id());
        obj["state"]       = helpers::stateIdToString(pkg.currentStateId());
        obj["metadata"]    = metadataToJson(pkg.metadata());
        obj["source"]      = addressToJson(pkg.source());
        obj["destination"] = addressToJson(pkg.destination());
        obj["logistics"]   = logisticsToJson(pkg.logistics());
        obj["location"]    = locationToJson(pkg.location());
        return obj;
    }

    domain::Package JsonPackageRepository::packageFromJson(const QJsonObject& obj)
    {
        auto metadata    = metadataFromJson(obj["metadata"].toObject());
        auto source      = addressFromJson(obj["source"].toObject());
        auto destination = addressFromJson(obj["destination"].toObject());
        auto logistics   = logisticsFromJson(obj["logistics"].toObject());
        auto location    = locationFromJson(obj["location"].toObject());

        const std::string id = obj["id"].toString().toStdString();
        const domain::PackageStateId stateId =
            helpers::stateIdFromString(obj["state"].toString());

        return domain::Package::load(
            id,
            std::move(metadata),
            std::move(source),
            std::move(destination),
            std::move(logistics),
            std::move(location),
            stateId
        );
    }

    // --Serialisation helpers - Address--

    QJsonObject JsonPackageRepository::addressToJson(const domain::Address& a)
    {
        QJsonObject obj;
        obj["street"]     = QString::fromStdString(a.street);
        obj["city"]       = QString::fromStdString(a.city);
        obj["country"]    = QString::fromStdString(a.country);
        obj["postalCode"] = QString::fromStdString(a.postalCode);
        return obj;
    }

    domain::Address JsonPackageRepository::addressFromJson(const QJsonObject& o)
    {
        return domain::Address{
            o["street"].toString().toStdString(),
            o["city"].toString().toStdString(),
            o["country"].toString().toStdString(),
            o["postalCode"].toString().toStdString()
        };
    }

    // --Serialisation helpers - LogisticsInfo--

    QJsonObject JsonPackageRepository::logisticsToJson(const domain::LogisticsInfo& l)
    {
        QJsonObject obj;
        obj["importDate"]         = helpers::dateToString(l.importDate);
        obj["expectedExportDate"] = helpers::dateToString(l.expectedExportDate);
        obj["importVehicle"]      = QString::fromStdString(l.importVehicle);
        obj["exportVehicle"]      = QString::fromStdString(l.exportVehicle);
        obj["containerId"]        = QString::fromStdString(l.containerId);
        return obj;
    }

    domain::LogisticsInfo JsonPackageRepository::logisticsFromJson(const QJsonObject& o)
    {
        return domain::LogisticsInfo{
            helpers::dateFromString(o["importDate"].toString()),
            helpers::dateFromString(o["expectedExportDate"].toString()),
            o["importVehicle"].toString().toStdString(),
            o["exportVehicle"].toString().toStdString(),
            o["containerId"].toString().toStdString()
        };
    }

    // --Serialisation helpers - StorageLocation--

    QJsonObject JsonPackageRepository::locationToJson(const domain::StorageLocation& l)
    {
        QJsonObject obj;
        obj["zone"]  = QString::fromStdString(l.zone);
        obj["aisle"] = QString::fromStdString(l.aisle);
        obj["shelf"] = l.shelf;
        obj["slot"]  = l.slot;
        return obj;
    }

    domain::StorageLocation JsonPackageRepository::locationFromJson(const QJsonObject& o)
    {
        return domain::StorageLocation{
            o["zone"].toString().toStdString(),
            o["aisle"].toString().toStdString(),
            o["shelf"].toInt(),
            o["slot"].toInt()
        };
    }

    // --Serialisation helpers - PackageMetadata--

    QJsonObject JsonPackageRepository::metadataToJson(const domain::PackageMetadata& m)
    {
        QJsonObject dim;
        dim["length"] = m.dimensions.length;
        dim["width"]  = m.dimensions.width;
        dim["height"] = m.dimensions.height;

        QJsonObject obj;
        obj["name"]        = QString::fromStdString(m.name);
        obj["category"]    = helpers::categoryToString(m.category);
        obj["weight"]      = m.weight;
        obj["cost"]        = m.cost;
        obj["description"] = QString::fromStdString(m.description);
        obj["dimensions"]  = dim;
        return obj;
    }

    domain::PackageMetadata JsonPackageRepository::metadataFromJson(const QJsonObject& o)
    {
        const QJsonObject dim = o["dimensions"].toObject();
        return domain::PackageMetadata{
            o["name"].toString().toStdString(),
            helpers::categoryFromString(o["category"].toString()),
            o["weight"].toDouble(),
            domain::Dimension{
                dim["length"].toDouble(),
                dim["width"].toDouble(),
                dim["height"].toDouble()
            },
            o["cost"].toDouble(),
            o["description"].toString().toStdString()
        };
    }

    // --Query--

    std::vector<domain::Package> JsonPackageRepository::findByCriteria(
        const domain::PackageQueryCriteria& criteria) const
    {
        // Filtering logic is evaluated in-memory here rather than delegating
        // to service::PackageFilter, because repository/ must not depend on
        // service/ per the layer dependency rules.
        std::vector<domain::Package> result;
        result.reserve(m_store.size());

        const auto toLower = [](std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        };

        std::optional<std::string> lowerName;
        if (criteria.name.has_value())
            lowerName = toLower(*criteria.name);

        std::optional<std::string> lowerKeyword;
        if (criteria.descriptionKeyword.has_value())
            lowerKeyword = toLower(*criteria.descriptionKeyword);

        const QDate qToday = QDate::currentDate(); // Get Local Windows time
        const auto today = std::chrono::year_month_day{
            std::chrono::year{qToday.year()},
            std::chrono::month{static_cast<unsigned>(qToday.month())},
            std::chrono::day{static_cast<unsigned>(qToday.day())}
        };

        for (const auto& [id, pkg] : m_store)
        {
            if (lowerName.has_value() &&
                toLower(pkg.metadata().name).find(*lowerName) == std::string::npos)
                continue;
            if (criteria.state.has_value() && pkg.currentStateId() != *criteria.state)
                continue;
            if (criteria.category.has_value() && pkg.metadata().category != *criteria.category)
                continue;
            if (criteria.minWeight.has_value() && pkg.metadata().weight < *criteria.minWeight)
                continue;
            if (criteria.maxWeight.has_value() && pkg.metadata().weight > *criteria.maxWeight)
                continue;
            if (criteria.zone.has_value() && pkg.location().zone != *criteria.zone)
                continue;
            if (criteria.containerId.has_value() &&
                pkg.logistics().containerId != *criteria.containerId)
                continue;
            if (lowerKeyword.has_value() &&
                toLower(pkg.metadata().description).find(*lowerKeyword) == std::string::npos)
                continue;
            if (criteria.overdueOnly && today < pkg.logistics().expectedExportDate)
                continue;
            if (criteria.lateOnly && today < pkg.logistics().importDate)
                continue;
            if (criteria.importedToday && pkg.logistics().importDate != today)
                continue;
            if (criteria.exportDueToday && pkg.logistics().expectedExportDate != today)
                continue;

            if (criteria.importDate.has_value()) {
                if (pkg.logistics().importDate != criteria.importDate.value()) continue;
            }
            if (criteria.exportDate.has_value()) {
                if (pkg.logistics().expectedExportDate != criteria.exportDate.value()) continue;
            }

            result.push_back(pkg);
        }

        return result;
    }

}
