/**
 * @file   DatabaseConnection.cpp
 * @brief  Implementation of DatabaseConnection.
 *
 * @author Do Minh Khang
 * @date   2026-07-18
 */

#include "repository/DatabaseConnection.h"

#include <QFile>
#include <QIODevice>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QTextStream>

#include <stdexcept>

namespace wms::repository
{
    // --Construction--

    DatabaseConnection::DatabaseConnection(const QString& databaseFilePath,
        const QString& schemaFilePath,
        const QString& connectionName)
        : m_connectionName{ connectionName }
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
        db.setDatabaseName(databaseFilePath);

        if (!db.open())
        {
            throw std::runtime_error(
                "DatabaseConnection - failed to open database: " +
                db.lastError().text().toStdString());
        }

        QSqlQuery pragma{ db };
        if (!pragma.exec("PRAGMA foreign_keys = ON;"))
        {
            throw std::runtime_error(
                "DatabaseConnection - failed to enable foreign keys: " +
                pragma.lastError().text().toStdString());
        }

        applySchema(schemaFilePath);
    }

    DatabaseConnection::~DatabaseConnection()
    {
        // Close and remove the connection from Qt's registry so the
        // connection name can be reused (important for repeated test runs).
        {
            QSqlDatabase db = QSqlDatabase::database(m_connectionName);
            db.close();
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    // --Accessors--

    QSqlDatabase DatabaseConnection::handle() const
    {
        return QSqlDatabase::database(m_connectionName);
    }

    const QString& DatabaseConnection::connectionName() const
    {
        return m_connectionName;
    }

    // --Private helpers--

    void DatabaseConnection::applySchema(const QString& schemaFilePath)
    {
        QFile file{ schemaFilePath };
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            throw std::runtime_error(
                "DatabaseConnection::applySchema - cannot open schema file: " +
                schemaFilePath.toStdString());
        }

        const QString rawContents = QTextStream{ &file }.readAll();

        // Strip "--" line comments BEFORE splitting on ';'. Without this, a
        // semicolon written inside a comment gets treated as a statement
        // terminator.
        QStringList codeOnlyLines;
        for (const QString& line : rawContents.split('\n'))
        {
            const int commentStart = line.indexOf("--");
            codeOnlyLines << (commentStart >= 0 ? line.left(commentStart) : line);
        }
        const QString contents = codeOnlyLines.join('\n');

        // Statements are separated by ';'.
        const QStringList statements = contents.split(';', Qt::SkipEmptyParts);

        QSqlQuery query{ handle() };
        for (const QString& raw : statements)
        {
            const QString statement = raw.trimmed();
            if (statement.isEmpty())
                continue;

            if (!query.exec(statement))
            {
                // SQLite has no "ALTER TABLE ... ADD COLUMN IF NOT EXISTS".
                // Column-adding migrations therefore fail with "duplicate
                // column name" on every run after the first, on databases
                // that already have the column. Treating that one error as
                // non-fatal is what lets an ALTER TABLE statement stay in
                // this file permanently and be re-applied safely alongside
                // the CREATE TABLE IF NOT EXISTS statements on every
                // startup, instead of needing a separate migration runner.
                // Any other failure (bad syntax, missing table, etc.) still
                // throws, since silently swallowing those would hide real
                // schema bugs.
                const QString error = query.lastError().text();
                if (error.contains("duplicate column name", Qt::CaseInsensitive))
                    continue;

                throw std::runtime_error(
                    "DatabaseConnection::applySchema - statement failed: " +
                    error.toStdString());
            }
        }
    }
}
