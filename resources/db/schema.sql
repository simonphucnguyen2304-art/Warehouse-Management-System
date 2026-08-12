-- =============================================================================
-- @file:    schema.sql
-- @brief:   SQLite schema for the Packages Warehouse Management System.
--
-- @author:  Do Minh Khang
-- @date:    2026-07-18
--
-- @update
-- @author:  Huynh Phuc Nguyen
-- @date:    2026-07-19
-- @changelog:
--   - Add CREATE INDEX IF NOT EXISTS idx_packages_import_date on import_date.
--
-- Scope of this revision:
--  - Only the "packages" table is defined here. Container and Employee are
--    planned future modules and are intentionally NOT created in this file,
--    so this change stays reviewable as "the SQL package repository" rather
--    than mixing in schema for features that don't exist yet.
--  - container_id is stored as a plain TEXT column with no FOREIGN KEY
--    constraint for now, since a "containers" table does not exist yet.
--    When the Container module is implemented, add
--    "REFERENCES containers(id) ON DELETE SET NULL" to this column - SQLite
--    cannot add a FK to an existing column via ALTER TABLE, so that change
--    is expected to rebuild this table the same way any SQLite migration
--    would (see DatabaseConnection's class comment for where that step
--    belongs once a migration mechanism is introduced).
--  - The statement uses "CREATE TABLE IF NOT EXISTS" so this file can be
--    safely re-applied on every application startup (see DatabaseConnection).
--  - "PRAGMA foreign_keys = ON;" is applied by DatabaseConnection on every
--    connection before this file runs. It has no visible effect yet since
--    no FOREIGN KEY is declared below, but is kept on so behaviour does not
--    silently change once the Container migration above adds one.
-- =============================================================================

CREATE TABLE IF NOT EXISTS packages (
    id                    TEXT PRIMARY KEY,
    state                 TEXT NOT NULL,   -- OnRoute | InStorage | Dispatched | Missing | Overdue

    -- PackageMetadata
    package_name          TEXT NOT NULL DEFAULT '',
    category              TEXT NOT NULL,   -- Standard | Fragile | Perishable | Hazmat | Oversized | Liquid
    weight                REAL NOT NULL,
    dim_length            REAL NOT NULL,
    dim_width             REAL NOT NULL,
    dim_height            REAL NOT NULL,
    cost                  REAL NOT NULL,
    description           TEXT,

    -- Address: source
    src_street            TEXT,
    src_city              TEXT,
    src_country           TEXT,
    src_postal            TEXT,

    -- Address: destination
    dst_street            TEXT,
    dst_city              TEXT,
    dst_country           TEXT,
    dst_postal            TEXT,

    -- LogisticsInfo
    import_date           TEXT NOT NULL,   -- "YYYY-MM-DD"
    expected_export_date  TEXT NOT NULL,   -- "YYYY-MM-DD"
    import_vehicle        TEXT,
    export_vehicle        TEXT,
    container_id          TEXT,            -- Not yet a FK - see file header note above

    -- StorageLocation
    zone                  TEXT NOT NULL,
    aisle                 TEXT NOT NULL,
    shelf                 INTEGER NOT NULL,
    slot                  INTEGER NOT NULL
);

-- One index per column PackageQueryCriteria can filter on, so
-- SqlitePackageRepository::findByCriteria() never falls back to a full
-- table scan for a supported filter combination.
CREATE INDEX IF NOT EXISTS idx_packages_name         ON packages(package_name);
CREATE INDEX IF NOT EXISTS idx_packages_state        ON packages(state);
CREATE INDEX IF NOT EXISTS idx_packages_category     ON packages(category);
CREATE INDEX IF NOT EXISTS idx_packages_zone         ON packages(zone);
CREATE INDEX IF NOT EXISTS idx_packages_container    ON packages(container_id);
CREATE INDEX IF NOT EXISTS idx_packages_import_date  ON packages(import_date);
CREATE INDEX IF NOT EXISTS idx_packages_export_date  ON packages(expected_export_date);
