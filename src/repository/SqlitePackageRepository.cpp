/**
 * @file   SqlitePackageRepository.cpp
 * @brief  Implementation of SqlitePackageRepository.
 *
 * @author Do Minh Khang
 * @date   2026-07-18
 *
 * @update
 * @author Huynh Phuc Nguyen
 * @date   2026-07-19
 * @changelog
 *   - Replace all local categoryToString / categoryFromString / stateIdToString /
 *     stateIdFromString / dateToString / dateFromString / todayAsString
 *     definitions with calls to the shared helpers in RepositoryHelpers.h.
 *     No behaviour change; this removes ~80 lines of code that were duplicated
 *     verbatim from JsonPackageRepository.cpp.
 *
 * @update
 * @author Nguyen Viet Bach
 * @date   2026-07-25
 * @changelog
 *   - Implement exportToJson / importFromJson / exportToCsv / importFromCsv.
 *     All four methods retrieve the full dataset via getAll(), then delegate
 *     to the same JSON/CSV serialisation logic used by JsonPackageRepository
 *     (QJsonDocument / QTextStream with RFC 4180 quoting). Imports resolve
 *     conflicts by calling update() for existing ids and add() for new ones.
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-07-31
 * @changelog
 *   - Adding condition for late package at build and bind function.
 *   - Correct the query at build function for Overdue.
 * 
 * @update
 * @author Duong Anh Hao
 * @date   2026-08-02
 * @changelog
 *   - Update buildWhereClause() to append conditions for import_date and 
 *     expected_export_date when custom dates are provided in criteria.
 *   - Update bindWhereClause() to format wms::domain::Date to "YYYY-MM-DD" 
 *     and bind values to the SQL query safely.
 *
 * @update
 * @author Do Minh Khang
 * @date   2026-08-08
 * @changelog
 *   - importFromJson() / importFromCsv(): packages whose importDate is in the
 *     future are always restored as OnRoute, regardless of the state stored
 *     in the file. This prevents data files from smuggling in InStorage or
 *     Dispatched states for packages that have not physically arrived yet.
 */

#include "repository/SqlitePackageRepository.h"
#include "repository/RepositoryHelpers.h"

#include "domain/entities/PackageMetadata.h"
#include "domain/entities/Address.h"
#include "domain/entities/LogisticsInfo.h"
#include "domain/entities/StorageLocation.h"
#include "domain/entities/Dimension.h"
#include "domain/states/OnRouteState.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>
#include <QTextStream>
#include <QStringConverter>
#include <QMetaType>
#include <QSqlError>
#include <QStringList>
#include <QVariant>

#include <chrono>
#include <stdexcept>

namespace wms::repository
{
    // --Construction--

    SqlitePackageRepository::SqlitePackageRepository(const DatabaseConnection& connection)
        : m_connection{ connection }
    {
    }

    // --IPackageRepository - Read--

    std::vector<domain::Package> SqlitePackageRepository::getAll() const
    {
        return findByCriteria(domain::PackageQueryCriteria{});
    }

    std::optional<domain::Package> SqlitePackageRepository::getById(const std::string& id) const
    {
        QSqlQuery query{ m_connection.handle() };
        query.prepare("SELECT * FROM packages WHERE id = :id");
        query.bindValue(":id", QString::fromStdString(id));

        if (!query.exec())
            throw std::runtime_error(
                "SqlitePackageRepository::getById - query failed: " +
                query.lastError().text().toStdString());

        if (!query.next())
            return std::nullopt;

        return packageFromRecord(query);
    }

    // --IPackageRepository - Write--

    void SqlitePackageRepository::add(domain::Package package)
    {
        QSqlQuery query{ m_connection.handle() };
        query.prepare(
            "INSERT INTO packages ("
            "  id, state, package_name, category, weight, dim_length, dim_width, dim_height,"
            "  cost, description,"
            "  src_street, src_city, src_country, src_postal,"
            "  dst_street, dst_city, dst_country, dst_postal,"
            "  import_date, expected_export_date, import_vehicle, export_vehicle,"
            "  container_id, zone, aisle, shelf, slot"
            ") VALUES ("
            "  :id, :state, :packageName, :category, :weight, :dimLength, :dimWidth, :dimHeight,"
            "  :cost, :description,"
            "  :srcStreet, :srcCity, :srcCountry, :srcPostal,"
            "  :dstStreet, :dstCity, :dstCountry, :dstPostal,"
            "  :importDate, :expectedExportDate, :importVehicle, :exportVehicle,"
            "  :containerId, :zone, :aisle, :shelf, :slot"
            ")");

        bindPackageFields(query, package);

        if (!query.exec())
            throw std::runtime_error(
                "SqlitePackageRepository::add - insert failed (id may already "
                "exist): " + query.lastError().text().toStdString());
    }

    void SqlitePackageRepository::update(domain::Package package)
    {
        QSqlQuery query{ m_connection.handle() };
        query.prepare(
            "UPDATE packages SET"
            "  state = :state, package_name = :packageName, category = :category,"
            "  weight = :weight,"
            "  dim_length = :dimLength, dim_width = :dimWidth, dim_height = :dimHeight,"
            "  cost = :cost, description = :description,"
            "  src_street = :srcStreet, src_city = :srcCity,"
            "  src_country = :srcCountry, src_postal = :srcPostal,"
            "  dst_street = :dstStreet, dst_city = :dstCity,"
            "  dst_country = :dstCountry, dst_postal = :dstPostal,"
            "  import_date = :importDate, expected_export_date = :expectedExportDate,"
            "  import_vehicle = :importVehicle, export_vehicle = :exportVehicle,"
            "  container_id = :containerId, zone = :zone, aisle = :aisle,"
            "  shelf = :shelf, slot = :slot"
            " WHERE id = :id");

        bindPackageFields(query, package);

        if (!query.exec())
            throw std::runtime_error(
                "SqlitePackageRepository::update - update failed: " +
                query.lastError().text().toStdString());

        if (query.numRowsAffected() == 0)
            throw std::runtime_error(
                "SqlitePackageRepository::update - package not found: " + package.id());
    }

    void SqlitePackageRepository::remove(const std::string& id)
    {
        QSqlQuery query{ m_connection.handle() };
        query.prepare("DELETE FROM packages WHERE id = :id");
        query.bindValue(":id", QString::fromStdString(id));

        if (!query.exec())
            throw std::runtime_error(
                "SqlitePackageRepository::remove - delete failed: " +
                query.lastError().text().toStdString());

        if (query.numRowsAffected() == 0)
            throw std::runtime_error(
                "SqlitePackageRepository::remove - package not found: " + id);
    }

    // --IPackageRepository - Persistence--

    void SqlitePackageRepository::save()
    {
        // No-op: add()/update()/remove() already commit directly to SQLite.
        // Kept as an explicit override so WarehouseManager's save() call sites
        // work unmodified when swapped from JsonPackageRepository.
    }

    void SqlitePackageRepository::load()
    {
        // No-op for the same reason as save() - there is no separate
        // in-memory cache to refresh from the database.
    }

    // --Bulk I/O--

    namespace
    {
        // Serialisation helpers shared with the JSON bulk-I/O path.
        // These are duplicated (not pulled from JsonPackageRepository) to keep
        // the two translation units independent; the logic is identical.

        QJsonObject pkgToJsonObj(const domain::Package& pkg)
        {
            QJsonObject dim;
            dim["length"] = pkg.metadata().dimensions.length;
            dim["width"]  = pkg.metadata().dimensions.width;
            dim["height"] = pkg.metadata().dimensions.height;

            QJsonObject meta;
            meta["name"]        = QString::fromStdString(pkg.metadata().name);
            meta["category"]    = helpers::categoryToString(pkg.metadata().category);
            meta["weight"]      = pkg.metadata().weight;
            meta["cost"]        = pkg.metadata().cost;
            meta["description"] = QString::fromStdString(pkg.metadata().description);
            meta["dimensions"]  = dim;

            QJsonObject src;
            src["street"]     = QString::fromStdString(pkg.source().street);
            src["city"]       = QString::fromStdString(pkg.source().city);
            src["country"]    = QString::fromStdString(pkg.source().country);
            src["postalCode"] = QString::fromStdString(pkg.source().postalCode);

            QJsonObject dst;
            dst["street"]     = QString::fromStdString(pkg.destination().street);
            dst["city"]       = QString::fromStdString(pkg.destination().city);
            dst["country"]    = QString::fromStdString(pkg.destination().country);
            dst["postalCode"] = QString::fromStdString(pkg.destination().postalCode);

            QJsonObject log;
            log["importDate"]         = helpers::dateToString(pkg.logistics().importDate);
            log["expectedExportDate"] = helpers::dateToString(pkg.logistics().expectedExportDate);
            log["importVehicle"]      = QString::fromStdString(pkg.logistics().importVehicle);
            log["exportVehicle"]      = QString::fromStdString(pkg.logistics().exportVehicle);
            log["containerId"]        = QString::fromStdString(pkg.logistics().containerId);

            QJsonObject loc;
            loc["zone"]  = QString::fromStdString(pkg.location().zone);
            loc["aisle"] = QString::fromStdString(pkg.location().aisle);
            loc["shelf"] = pkg.location().shelf;
            loc["slot"]  = pkg.location().slot;

            QJsonObject obj;
            obj["id"]          = QString::fromStdString(pkg.id());
            obj["state"]       = helpers::stateIdToString(pkg.currentStateId());
            obj["metadata"]    = meta;
            obj["source"]      = src;
            obj["destination"] = dst;
            obj["logistics"]   = log;
            obj["location"]    = loc;
            return obj;
        }

        domain::Package pkgFromJsonObj(const QJsonObject& obj)
        {
            const QJsonObject metaObj = obj["metadata"].toObject();
            const QJsonObject dimObj  = metaObj["dimensions"].toObject();
            const QJsonObject srcObj  = obj["source"].toObject();
            const QJsonObject dstObj  = obj["destination"].toObject();
            const QJsonObject logObj  = obj["logistics"].toObject();
            const QJsonObject locObj  = obj["location"].toObject();

            domain::PackageMetadata meta{
                metaObj["name"].toString().toStdString(),
                helpers::categoryFromString(metaObj["category"].toString()),
                metaObj["weight"].toDouble(),
                domain::Dimension{
                    dimObj["length"].toDouble(),
                    dimObj["width"].toDouble(),
                    dimObj["height"].toDouble()
                },
                metaObj["cost"].toDouble(),
                metaObj["description"].toString().toStdString()
            };
            domain::Address src{
                srcObj["street"].toString().toStdString(),
                srcObj["city"].toString().toStdString(),
                srcObj["country"].toString().toStdString(),
                srcObj["postalCode"].toString().toStdString()
            };
            domain::Address dst{
                dstObj["street"].toString().toStdString(),
                dstObj["city"].toString().toStdString(),
                dstObj["country"].toString().toStdString(),
                dstObj["postalCode"].toString().toStdString()
            };
            domain::LogisticsInfo log{
                helpers::dateFromString(logObj["importDate"].toString()),
                helpers::dateFromString(logObj["expectedExportDate"].toString()),
                logObj["importVehicle"].toString().toStdString(),
                logObj["exportVehicle"].toString().toStdString(),
                logObj["containerId"].toString().toStdString()
            };
            domain::StorageLocation loc{
                locObj["zone"].toString().toStdString(),
                locObj["aisle"].toString().toStdString(),
                locObj["shelf"].toInt(),
                locObj["slot"].toInt()
            };
            return domain::Package::load(
                obj["id"].toString().toStdString(),
                std::move(meta), std::move(src), std::move(dst),
                std::move(log), std::move(loc),
                helpers::stateIdFromString(obj["state"].toString()));
        }

        // RFC 4180 CSV quoting
        QString sqlCsvQuote(const QString& field)
        {
            if (!field.contains(',') && !field.contains('"') && !field.contains('\n'))
                return field;
            return '"' + QString(field).replace('"', """""")
                                       + '"';
        }

        constexpr const char* SQL_CSV_HEADER =
            "id,state,name,category,weight,dimLength,dimWidth,dimHeight,cost,description,"
            "importDate,expectedExportDate,importVehicle,exportVehicle,containerId,"
            "srcStreet,srcCity,srcCountry,srcPostal,"
            "dstStreet,dstCity,dstCountry,dstPostal,"
            "zone,aisle,shelf,slot";

        QStringList sqlParseCsvLine(const QString& line)
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
                        if (i + 1 < line.size() && line[i + 1] == '"')
                        { current += '"'; ++i; }
                        else
                        { inQuotes = false; }
                    }
                    else { current += ch; }
                }
                else
                {
                    if (ch == '"') { inQuotes = true; }
                    else if (ch == ',') { tokens.append(current); current.clear(); }
                    else { current += ch; }
                }
            }
            tokens.append(current);
            return tokens;
        }

        /**
         * @brief  Resolve the effective state for an imported package.
         *
         *  If the package's importDate is still in the future, it cannot
         *  physically be in the warehouse yet, so we override whatever state
         *  was stored in the file and return OnRoute. For all other cases the
         *  file's state is preserved as-is.
         *
         * @param  stateId    State read from the CSV/JSON file.
         * @param  importDate The package's scheduled import date.
         * @return The state that should actually be persisted.
         */
        domain::PackageStateId resolveImportedStateId(
            domain::PackageStateId stateId,
            const domain::Date&    importDate)
        {
            const auto today = std::chrono::floor<std::chrono::days>(
                std::chrono::system_clock::now());
            if (importDate > today)
                return domain::PackageStateId::OnRoute;
            return stateId;
        }
    } // anonymous namespace

    void SqlitePackageRepository::exportToJson(const std::string& filePath) const
    {
        const auto packages = getAll();
        QJsonArray array;
        for (const auto& pkg : packages)
            array.append(pkgToJsonObj(pkg));

        QJsonDocument doc{ array };
        QFile file{ QString::fromStdString(filePath) };
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            throw std::runtime_error(
                "SqlitePackageRepository::exportToJson - cannot open file: " + filePath);
        file.write(doc.toJson(QJsonDocument::Indented));
    }

    void SqlitePackageRepository::importFromJson(const std::string& filePath)
    {
        QFile file{ QString::fromStdString(filePath) };
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            throw std::runtime_error(
                "SqlitePackageRepository::importFromJson - cannot open file: " + filePath);

        const QByteArray raw = file.readAll();
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);

        if (parseError.error != QJsonParseError::NoError)
            throw std::runtime_error(
                "SqlitePackageRepository::importFromJson - JSON parse error: " +
                parseError.errorString().toStdString());

        if (!doc.isArray())
            throw std::runtime_error(
                "SqlitePackageRepository::importFromJson - root element must be a JSON array");

        for (const QJsonValue& val : doc.array())
        {
            domain::Package pkg = pkgFromJsonObj(val.toObject());

            // If importDate is in the future, force state back to OnRoute.
            const auto effectiveState = resolveImportedStateId(
                pkg.currentStateId(), pkg.logistics().importDate);
            if (effectiveState != pkg.currentStateId())
                pkg.transitionTo(std::make_unique<domain::OnRouteState>());

            const std::string id = pkg.id();
            if (getById(id).has_value())
                update(pkg);
            else
                add(pkg);
        }
    }

    void SqlitePackageRepository::exportToCsv(const std::string& filePath) const
    {
        QFile file{ QString::fromStdString(filePath) };
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            throw std::runtime_error(
                "SqlitePackageRepository::exportToCsv - cannot open file: " + filePath);

        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);
        out << SQL_CSV_HEADER << "\n";

        for (const auto& pkg : getAll())
        {
            const auto& meta = pkg.metadata();
            const auto& log  = pkg.logistics();
            const auto& loc  = pkg.location();
            const auto& src  = pkg.source();
            const auto& dst  = pkg.destination();

            out << sqlCsvQuote(QString::fromStdString(pkg.id()))                        << ','
                << sqlCsvQuote(helpers::stateIdToString(pkg.currentStateId()))          << ','
                << sqlCsvQuote(QString::fromStdString(meta.name))                       << ','
                << sqlCsvQuote(helpers::categoryToString(meta.category))                << ','
                << meta.weight                                                          << ','
                << meta.dimensions.length                                               << ','
                << meta.dimensions.width                                                << ','
                << meta.dimensions.height                                               << ','
                << meta.cost                                                            << ','
                << sqlCsvQuote(QString::fromStdString(meta.description))               << ','
                << sqlCsvQuote(helpers::dateToString(log.importDate))                   << ','
                << sqlCsvQuote(helpers::dateToString(log.expectedExportDate))           << ','
                << sqlCsvQuote(QString::fromStdString(log.importVehicle))               << ','
                << sqlCsvQuote(QString::fromStdString(log.exportVehicle))               << ','
                << sqlCsvQuote(QString::fromStdString(log.containerId))                 << ','
                << sqlCsvQuote(QString::fromStdString(src.street))                      << ','
                << sqlCsvQuote(QString::fromStdString(src.city))                        << ','
                << sqlCsvQuote(QString::fromStdString(src.country))                     << ','
                << sqlCsvQuote(QString::fromStdString(src.postalCode))                  << ','
                << sqlCsvQuote(QString::fromStdString(dst.street))                      << ','
                << sqlCsvQuote(QString::fromStdString(dst.city))                        << ','
                << sqlCsvQuote(QString::fromStdString(dst.country))                     << ','
                << sqlCsvQuote(QString::fromStdString(dst.postalCode))                  << ','
                << sqlCsvQuote(QString::fromStdString(loc.zone))                        << ','
                << sqlCsvQuote(QString::fromStdString(loc.aisle))                       << ','
                << loc.shelf                                                            << ','
                << loc.slot                                                             << '\n';
        }
    }

    void SqlitePackageRepository::importFromCsv(const std::string& filePath)
    {
        QFile file{ QString::fromStdString(filePath) };
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            throw std::runtime_error(
                "SqlitePackageRepository::importFromCsv - cannot open file: " + filePath);

        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);

        if (!in.atEnd()) in.readLine(); // skip header

        int lineNumber = 1;
        while (!in.atEnd())
        {
            ++lineNumber;
            const QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;

            const QStringList f = sqlParseCsvLine(line);
            if (f.size() < 27)
                throw std::runtime_error(
                    "SqlitePackageRepository::importFromCsv - expected 27 columns at line " +
                    std::to_string(lineNumber));

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

            const auto effectiveState = resolveImportedStateId(
                helpers::stateIdFromString(f[1]), log.importDate);

            domain::Package pkg = domain::Package::load(
                f[0].toStdString(),
                std::move(meta), std::move(src), std::move(dst),
                std::move(log), std::move(loc),
                effectiveState
            );

            if (getById(pkg.id()).has_value())
                update(pkg);
            else
                add(pkg);
        }
    }

    // --Query--

    std::vector<domain::Package> SqlitePackageRepository::findByCriteria(
        const domain::PackageQueryCriteria& criteria) const
    {
        const QString sql = "SELECT * FROM packages" + buildWhereClause(criteria);

        QSqlQuery query{ m_connection.handle() };
        query.prepare(sql);
        bindWhereClause(query, criteria);

        if (!query.exec())
            throw std::runtime_error(
                "SqlitePackageRepository::findByCriteria - query failed: " +
                query.lastError().text().toStdString());

        std::vector<domain::Package> result;
        while (query.next())
            result.push_back(packageFromRecord(query));

        return result;
    }

    // --Query building--

    QString SqlitePackageRepository::buildWhereClause(const domain::PackageQueryCriteria& c)
    {
        QStringList clauses;

        if (c.name.has_value())               clauses << "LOWER(package_name) LIKE :nameKeyword";
        if (c.state.has_value())              clauses << "state = :state";
        if (c.category.has_value())           clauses << "category = :category";
        if (c.minWeight.has_value())          clauses << "weight >= :minWeight";
        if (c.maxWeight.has_value())          clauses << "weight <= :maxWeight";
        if (c.zone.has_value())               clauses << "zone = :zone";
        if (c.containerId.has_value())        clauses << "container_id = :containerId";
        if (c.descriptionKeyword.has_value()) clauses << "LOWER(description) LIKE :descriptionKeyword";
        if (c.overdueOnly)                    clauses << "expected_export_date <= :today";
        if (c.lateOnly)                       clauses << "import_date <= :today";
        if (c.importedToday)                  clauses << "import_date = :today";
        if (c.exportDueToday)                 clauses << "expected_export_date = :today";
        if (c.importDate.has_value())  clauses << "import_date = :importDateFilter";
        if (c.exportDate.has_value())  clauses << "expected_export_date = :exportDateFilter";

        if (clauses.isEmpty())
            return QString{};

        return " WHERE " + clauses.join(" AND ");
    }

    // --Query binding--

    void SqlitePackageRepository::bindWhereClause(QSqlQuery& query,
                                                   const domain::PackageQueryCriteria& c)
    {
        if (c.name.has_value())
        {
            const QString keyword = QString::fromStdString(*c.name).toLower();
            query.bindValue(":nameKeyword", "%" + keyword + "%");
        }
        if (c.state.has_value())
            query.bindValue(":state", helpers::stateIdToString(*c.state));
        if (c.category.has_value())
            query.bindValue(":category", helpers::categoryToString(*c.category));
        if (c.minWeight.has_value())
            query.bindValue(":minWeight", *c.minWeight);
        if (c.maxWeight.has_value())
            query.bindValue(":maxWeight", *c.maxWeight);
        if (c.zone.has_value())
            query.bindValue(":zone", QString::fromStdString(*c.zone));
        if (c.containerId.has_value())
            query.bindValue(":containerId", QString::fromStdString(*c.containerId));
        if (c.descriptionKeyword.has_value())
        {
            const QString keyword = QString::fromStdString(*c.descriptionKeyword).toLower();
            query.bindValue(":descriptionKeyword", "%" + keyword + "%");
        }
        if (c.overdueOnly || c.lateOnly || c.importedToday || c.exportDueToday)
            query.bindValue(":today", helpers::todayAsString());

        if (c.importDate.has_value()) {
            auto d = c.importDate.value();
            QString dateStr = QString("%1-%2-%3")
                .arg(static_cast<int>(d.year()), 4, 10, QChar('0'))
                .arg(static_cast<unsigned>(d.month()), 2, 10, QChar('0'))
                .arg(static_cast<unsigned>(d.day()), 2, 10, QChar('0'));

            query.bindValue(":importDateFilter", dateStr);
        }

        if (c.exportDate.has_value()) {
            auto d = c.exportDate.value();
            QString dateStr = QString("%1-%2-%3")
                .arg(static_cast<int>(d.year()), 4, 10, QChar('0'))
                .arg(static_cast<unsigned>(d.month()), 2, 10, QChar('0'))
                .arg(static_cast<unsigned>(d.day()), 2, 10, QChar('0'));

            query.bindValue(":exportDateFilter", dateStr);
        }
    }

    // --Row mapping--

    domain::Package SqlitePackageRepository::packageFromRecord(const QSqlQuery& query)
    {
        domain::PackageMetadata metadata{
            query.value("package_name").toString().toStdString(),
            helpers::categoryFromString(query.value("category").toString()),
            query.value("weight").toDouble(),
            domain::Dimension{
                query.value("dim_length").toDouble(),
                query.value("dim_width").toDouble(),
                query.value("dim_height").toDouble()
            },
            query.value("cost").toDouble(),
            query.value("description").toString().toStdString()
        };

        domain::Address source{
            query.value("src_street").toString().toStdString(),
            query.value("src_city").toString().toStdString(),
            query.value("src_country").toString().toStdString(),
            query.value("src_postal").toString().toStdString()
        };

        domain::Address destination{
            query.value("dst_street").toString().toStdString(),
            query.value("dst_city").toString().toStdString(),
            query.value("dst_country").toString().toStdString(),
            query.value("dst_postal").toString().toStdString()
        };

        const QVariant containerId = query.value("container_id");

        domain::LogisticsInfo logistics{
            helpers::dateFromString(query.value("import_date").toString()),
            helpers::dateFromString(query.value("expected_export_date").toString()),
            query.value("import_vehicle").toString().toStdString(),
            query.value("export_vehicle").toString().toStdString(),
            containerId.isNull() ? std::string{} : containerId.toString().toStdString()
        };

        domain::StorageLocation location{
            query.value("zone").toString().toStdString(),
            query.value("aisle").toString().toStdString(),
            query.value("shelf").toInt(),
            query.value("slot").toInt()
        };

        return domain::Package::load(
            query.value("id").toString().toStdString(),
            std::move(metadata),
            std::move(source),
            std::move(destination),
            std::move(logistics),
            std::move(location),
            helpers::stateIdFromString(query.value("state").toString())
        );
    }

    void SqlitePackageRepository::bindPackageFields(QSqlQuery& query, const domain::Package& pkg)
    {
        const auto& meta     = pkg.metadata();
        const auto& src      = pkg.source();
        const auto& dst      = pkg.destination();
        const auto& logistics = pkg.logistics();
        const auto& location  = pkg.location();

        query.bindValue(":id",          QString::fromStdString(pkg.id()));
        query.bindValue(":state",       helpers::stateIdToString(pkg.currentStateId()));
        query.bindValue(":packageName", QString::fromStdString(meta.name));
        query.bindValue(":category",    helpers::categoryToString(meta.category));
        query.bindValue(":weight",      meta.weight);
        query.bindValue(":dimLength",   meta.dimensions.length);
        query.bindValue(":dimWidth",    meta.dimensions.width);
        query.bindValue(":dimHeight",   meta.dimensions.height);
        query.bindValue(":cost",        meta.cost);
        query.bindValue(":description", QString::fromStdString(meta.description));

        query.bindValue(":srcStreet",  QString::fromStdString(src.street));
        query.bindValue(":srcCity",    QString::fromStdString(src.city));
        query.bindValue(":srcCountry", QString::fromStdString(src.country));
        query.bindValue(":srcPostal",  QString::fromStdString(src.postalCode));

        query.bindValue(":dstStreet",  QString::fromStdString(dst.street));
        query.bindValue(":dstCity",    QString::fromStdString(dst.city));
        query.bindValue(":dstCountry", QString::fromStdString(dst.country));
        query.bindValue(":dstPostal",  QString::fromStdString(dst.postalCode));

        query.bindValue(":importDate",         helpers::dateToString(logistics.importDate));
        query.bindValue(":expectedExportDate",  helpers::dateToString(logistics.expectedExportDate));
        query.bindValue(":importVehicle",       QString::fromStdString(logistics.importVehicle));
        query.bindValue(":exportVehicle",       QString::fromStdString(logistics.exportVehicle));

        // An empty containerId means "not assigned to a container" and is
        // stored as SQL NULL rather than an empty string. There is no FOREIGN
        // KEY on this column yet (see schema.sql header note), but storing NULL
        // now keeps the column's meaning correct and requires no migration once
        // the FK constraint is added later.
        if (logistics.containerId.empty())
            query.bindValue(":containerId", QVariant(QMetaType::fromType<QString>()));
        else
            query.bindValue(":containerId", QString::fromStdString(logistics.containerId));

        query.bindValue(":zone",  QString::fromStdString(location.zone));
        query.bindValue(":aisle", QString::fromStdString(location.aisle));
        query.bindValue(":shelf", location.shelf);
        query.bindValue(":slot",  location.slot);
    }
}
