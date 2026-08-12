/**
 * @file   SqlitePackageRepository.h
 * @brief  SQLite-backed implementation of IPackageRepository.
 *
 * @author Do Minh Khang
 * @date   2026-07-18
 *
 * @update
 * @author Huynh Phuc Nguyen
 * @date   2026-07-19
 * @changelog
 *   - Remove private categoryToString / categoryFromString / stateIdToString /
 *     stateIdFromString / dateToString / dateFromString / todayAsString
 *     declarations. These helpers are now provided by RepositoryHelpers.h and
 *     called directly in the .cpp file, eliminating the duplication that
 *     previously existed between this class and JsonPackageRepository.
 *
 * @update
 * @author Nguyen Viet Bach
 * @date   2026-07-25
 * @changelog
 *   - Added exportToJson / importFromJson / exportToCsv / importFromCsv
 *     overrides implementing the new IPackageRepository bulk I/O contract.
 *     Each method fetches all rows via getAll(), then delegates to the same
 *     serialisation logic shared with JsonPackageRepository.
 *
 * This file (along with DatabaseConnection.h) is one of the files outside
 * gui/ that is allowed to use Qt (QSqlQuery, QString). All other layers
 * must remain Qt-free.
 *
 * Table: packages (see resources/db/schema.sql for full DDL). Value objects
 * with no independent identity (Address, LogisticsInfo, StorageLocation,
 * PackageMetadata) are flattened into columns on this single table since
 * they are always owned 1:1 by exactly one Package row.
 */

#pragma once

#include "repository/IPackageRepository.h"
#include "repository/DatabaseConnection.h"

#include <QSqlQuery>
#include <QString>

namespace wms::repository
{
    /**
     * @class SqlitePackageRepository
     * @brief Loads, saves, and queries packages against a SQLite database.
     *
     * Unlike JsonPackageRepository, this implementation keeps no in-memory
     * cache - every call issues a query directly against the database, so
     * save()/load() are no-ops (SQLite already persists on each write).
     * This also means findByCriteria() benefits from the indexes declared
     * in schema.sql instead of scanning every row in C++.
     *
     * Enum/date serialisation is handled by the shared helpers in
     * RepositoryHelpers.h, keeping the wire format consistent with
     * JsonPackageRepository without code duplication.
     */
    class SqlitePackageRepository : public IPackageRepository
    {
    public:
        /**
         * @brief  Construct against an already-open database connection.
         * @param  connection  Non-owning reference to a live DatabaseConnection.
         *                     The connection must outlive this repository.
         */
        explicit SqlitePackageRepository(const DatabaseConnection& connection);

        // --IPackageRepository--
        std::vector<domain::Package>   getAll()   const override;
        std::optional<domain::Package> getById(const std::string& id) const override;
        void add(domain::Package package)        override;
        void update(domain::Package package)     override;
        void remove(const std::string& id)       override;
        void save()                              override;
        void load()                              override;

        // --Bulk I/O--
        void exportToJson (const std::string& filePath) const override;
        void importFromJson(const std::string& filePath)       override;
        void exportToCsv  (const std::string& filePath) const override;
        void importFromCsv(const std::string& filePath)        override;

        /**
         * @brief  Query packages matching every set field of @p criteria.
         *
         *  Builds a single parameterised SQL statement with one WHERE clause
         *  fragment per set field (unset optional fields are omitted entirely,
         *  not compared against NULL). All bindings use named placeholders to
         *  prevent SQL injection.
         *
         * @param  criteria  Filter fields; unset fields are not constrained.
         * @return All matching packages, order not guaranteed - callers that
         *         need a stable order should sort the result.
         */
        std::vector<domain::Package> findByCriteria(
            const domain::PackageQueryCriteria& criteria) const override;

    private:
        const DatabaseConnection& m_connection;

        // --Row mapping--

        /**
         * @brief  Maps the current row of an executed QSqlQuery to a domain::Package.
         */
        static domain::Package packageFromRecord(const QSqlQuery& query);

        /**
         * @brief  Binds every column of @p pkg onto @p query using named placeholders.
         */
        static void bindPackageFields(QSqlQuery& query, const domain::Package& pkg);

        // --Query building--

        /**
         * @brief  Builds the "WHERE ..." clause text (empty string if unfiltered).
         *
         *  Bind values are applied separately in findByCriteria() after
         *  query.prepare(), matching Qt's prepared-statement API. Must be kept
         *  in sync with bindWhereClause() by hand - update both together when
         *  PackageQueryCriteria gains a new field.
         */
        static QString buildWhereClause(const domain::PackageQueryCriteria& criteria);

        /**
         * @brief  Binds every placeholder that buildWhereClause() may have emitted.
         */
        static void bindWhereClause(QSqlQuery& query,
                                    const domain::PackageQueryCriteria& criteria);
    };

}
