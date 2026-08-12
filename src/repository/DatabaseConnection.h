/**
 * @file   DatabaseConnection.h
 * @brief  Opens and owns the single SQLite connection shared by every
 *         Sqlite*Repository.
 *
 *  Constructed once in main() and passed by const reference into each SQL
 *  repository's constructor - the same constructor-injection style already
 *  used for JsonPackageRepository and WarehouseManager. Deliberately NOT a
 *  Singleton: main() owns the one instance by construction, which keeps the
 *  class itself trivially testable (a unit test can construct its own
 *  DatabaseConnection against ":memory:" with a unique connection name).
 *
 *  Scope note: only SqlitePackageRepository consumes this class today.
 *  Future modules are expected to reuse the same connection the same way -
 *  add a "SqliteXxxRepository(const DatabaseConnection&)" constructor
 *  mirroring SqlitePackageRepository's, and extend resources/db/schema.sql
 *  with that module's own CREATE TABLE statement. No change to this class
 *  should be needed to support that.
 *
 * @author Do Minh Khang
 * @date   2026-07-18
 *
 * This file (along with JsonPackageRepository.h and SqlitePackageRepository.h)
 * is one of the files outside gui/ that is allowed to use Qt (QSqlDatabase,
 * QString). All other layers must remain Qt-free.
 */

#pragma once

#include <QSqlDatabase>
#include <QString>

namespace wms::repository
{
    /**
     * @class  DatabaseConnection
     * @brief  RAII wrapper around a named QSqlDatabase connection.
     *
     * Opens the connection and applies resources/db/schema.sql on
     * construction. The underlying QSqlDatabase connection is closed and
     * removed from Qt's connection registry on destruction.
     */
    class DatabaseConnection
    {
    public:
        /**
         * @brief  Opens a SQLite connection and applies the schema file.
         *
         * @param  databaseFilePath  Path to the .db file, or ":memory:" for
         *                           an in-memory database (used by tests).
         * @param  schemaFilePath    Path to the .sql file containing DDL
         *                           statements to apply on every startup.
         *                           CREATE statements should use "IF NOT
         *                           EXISTS"; ALTER TABLE ADD COLUMN
         *                           statements are also safe to leave here
         *                           permanently, since applySchema() treats
         *                           a resulting "duplicate column name"
         *                           error as already-applied rather than
         *                           fatal.
         * @param  connectionName    Unique Qt connection name. Must be
         *                           unique per open connection within the
         *                           process - tests should pass a fresh
         *                           name per test to run in isolation.
         * @throws std::runtime_error if the connection cannot be opened or
         *         the schema file cannot be read/executed.
         */
        DatabaseConnection(const QString& databaseFilePath,
            const QString& schemaFilePath,
            const QString& connectionName = "wms_connection");

        // --Rule of five--
        // Owns a Qt connection registered under a unique name. Copying
        // would create a dangling alias to the same underlying connection
        // name, and moving is unsafe once other repositories may already
        // hold a reference to this instance, so both are disabled.
        ~DatabaseConnection();
        DatabaseConnection(const DatabaseConnection&) = delete;
        DatabaseConnection& operator=(const DatabaseConnection&) = delete;
        DatabaseConnection(DatabaseConnection&&) = delete;
        DatabaseConnection& operator=(DatabaseConnection&&) = delete;

        /**
         * @brief  Returns the live QSqlDatabase handle for building queries.
         */
        QSqlDatabase handle() const;

        /**
         * @brief  Returns the Qt connection name this instance owns.
         */
        const QString& connectionName() const;

    private:
        QString m_connectionName;

        /**
         * @brief  Reads @p schemaFilePath and executes each statement
         *         against the open connection.
         * @throws std::runtime_error on read failure or SQL execution error.
         */
        void applySchema(const QString& schemaFilePath);
    };
}
