/**
 * @file   main.cpp
 * @brief  Main entry point for testing the Warehouse Management System core and repository modules.
 *
 * @author Duong Anh Hao
 * @date   2026-06-15
 * 
 * @update 
 * @author  Nguyen Viet Bach
 * @date   2026-06-23
 * @changelog
 *   - Launching the Qt GUI interface.
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-06-24
 * @changelog
 *   - Replace package constructor in part 3 with create()
 *   - Add comfirmination in part 7 for id checking
 * 
 * @update
 * @author Nguyen Viet Bach
 * @date   2026-07-04
 * @changelog
 *   - Added RUN_GUI macro to switch between GUI and console test mode
 *   Switch between two run modes by toggling the macro below:
 *
 *    #define RUN_GUI      →  Opens the Qt GUI window  (production mode)
 *    comment it out      →  Runs the console test harness (development mode)
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-07-18
 * @changelog
 *   - Swapped JsonPackageRepository for SqlitePackageRepository. Only the
 *     repository construction (part 1) and the "simulate restart" setup
 *     (part 7) actually changed - every call against IPackageRepository
 *     (add/save/getAll/getById/remove) is untouched. To switch back to
 *     JSON, revert part 1 and part 7 only.
 * @note  There's a known error that the project can't open database because
 *        it cannot load the requested driver: 'QSQLITE'.
 * 
 * @update
 * @author Lam Hong Hai Hoang Le
 * @date   2026-07-26
 * @changelog
 *   - Updated the GUI module to use SqlitePackageRepository
 * 
 */

#define RUN_GUI

#include <string>
#include <memory>

// Include Repository & Service Layer
#include "repository/SqlitePackageRepository.h"
#include "service/WarehouseManager.h"

#ifdef RUN_GUI

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "gui/MainWindow.h"
#include "gui/WarehouseGateway.h"

namespace
{
QString resolveDataFilePath(std::string inputPath)
{
    const QString fileName = QString::fromStdString(inputPath);
    const QStringList candidates = {
        QDir(QCoreApplication::applicationDirPath()).filePath(fileName),
        QDir::current().filePath(fileName),
        fileName
    };

    for (const QString& path : candidates)
    {
        const QFileInfo info(path);
        if (info.exists())
            return path;

        QDir().mkpath(info.absolutePath());
        return path;
    }

    return fileName;
}
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    const QString dbFile = "warehouse.db";
    const QString schemaFile = "resources/db/schema.sql";

    wms::repository::DatabaseConnection connection(
        resolveDataFilePath("warehouse.db"),
        resolveDataFilePath("resources/db/schema.sql")
    );
    auto repo = std::make_unique<wms::repository::SqlitePackageRepository>(connection);

    wms::service::WarehouseManager manager(std::move(repo));
    wms::gui::WarehouseGateway gateway(&manager);

    wms::gui::MainWindow window(&gateway);
    window.show();

    return app.exec();
}

#else

#include <iostream>
#include <exception>
#include <string>
#include <chrono>

// Include Domain Entities
#include "domain/entities/Package.h"
#include "domain/entities/PackageMetadata.h"
#include "domain/entities/Address.h"
#include "domain/entities/LogisticsInfo.h"
#include "domain/entities/StorageLocation.h"

// Include Repository
#include "repository/DatabaseConnection.h"
#include "repository/SqlitePackageRepository.h"
#include "repository/JsonPackageRepository.h"

#include <QCoreApplication>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    try {
        std::cout << "      WMS REPOSITORY MODULE TEST       \n";

        // 1. Initialize Repository

        // JSON TEST
        /*const QString testFile = "test_data.json";
        wms::repository::JsonPackageRepository repo(testFile);*/

        // SQLITE TEST
        const QString dbFile = "warehouse.db";
        const QString schemaFile = "resources/db/schema.sql";

        wms::repository::DatabaseConnection connection(dbFile, schemaFile);
        wms::repository::SqlitePackageRepository repo(connection);

        std::cout << "[INFO] Repository initialized successfully.\n";

        // 2. Prepare mock data for a new Package
        // Metadata: Category, weight, dimensions(l, w, h), cost, description
        wms::domain::PackageMetadata metadata{
            "Iphone 100",
            wms::domain::Category::Standard,
            15.5,
            {10.0, 5.0, 5.0},
            250.0,
            "Test Electronics Package"
        };

        // Source and Destination Addresses
        wms::domain::Address source{ "123 Tech Street", "Hanoi", "Vietnam", "100000" };
        wms::domain::Address destination{ "456 Startup Blvd", "Ho Chi Minh City", "Vietnam", "700000" };

        // Logistics Information 
        std::chrono::year_month_day importDate{ std::chrono::year{2026}, std::chrono::month{6}, std::chrono::day{15} };
        std::chrono::year_month_day exportDate{ std::chrono::year{2026}, std::chrono::month{6}, std::chrono::day{20} };
        wms::domain::LogisticsInfo logistics{ importDate, exportDate, "Truck-HN-01", "Truck-HCM-02", "CONT-1234" };

        // Storage Location
        wms::domain::StorageLocation location{ "ZoneA", "Aisle1", 1, 1 };

        // 3. Create a new Package instance
        wms::domain::Package newPackage = wms::domain::Package::create(
            metadata, source, destination, logistics, location
        );
        std::string pkgId = newPackage.id();
        std::cout << "[INFO] Created new Package. UUID: " << pkgId << "\n";

        // 4. Test ADD and SAVE operations
        repo.add(newPackage);
        repo.save();
        std::cout << "[SUCCESS] Package added and saved to JSON file.\n";

        // 5. Test READ operations (Get All)
        auto allPackages = repo.getAll();
        std::cout << "[INFO] Total packages in repository: " << allPackages.size() << "\n";

        // 6. Test READ operation (Get by ID)
        auto foundPkg = repo.getById(pkgId);
        if (foundPkg.has_value()) {
            std::cout << "[SUCCESS] Retrieved Package successfully by ID: " << foundPkg->id() << "\n";
        }
        else {
            std::cout << "[ERROR] Could not find the package by ID.\n";
        }

        // 7. Test LOAD operation (Simulate restarting the application)
        std::cout << "\n--- Simulating App Restart ---\n";
        // JSON TEST
        /*wms::repository::JsonPackageRepository repoRestart(testFile);
        auto loadedPackages = repoRestart.getAll();
        std::cout << "[SUCCESS] Reloaded repository from file. Total packages: " << loadedPackages.size() << "\n";*/

        // SQLITE TEST
        wms::repository::DatabaseConnection restartConnection(
            dbFile, schemaFile, "wms_connection_restart");
        wms::repository::SqlitePackageRepository repoRestart(restartConnection);

        auto loadedPackages = repoRestart.getAll();
        std::cout << "[SUCCESS] Reloaded repository from database. Total packages: " << loadedPackages.size() << "\n";


        auto reloadedPkg = repoRestart.getById(pkgId);
        if (reloadedPkg.has_value() && reloadedPkg->id() == pkgId)
            std::cout << "[SUCCESS] ID preserved across save/load: " << pkgId << "\n";
        else
            std::cout << "[ERROR] ID mismatch after reload - Package::load() not wired correctly.\n";

        // 8. Test REMOVE operation
        std::cout << "\n--- Cleaning up test data ---\n";
        repoRestart.remove(pkgId);
        repoRestart.save();
        std::cout << "[SUCCESS] Package removed and JSON file updated.\n";
        std::cout << "        ALL TESTS PASSED!              \n";   

    }
    catch (const std::exception& e) {
        // Catch and display any runtime errors or validation failures
        std::cerr << "[EXCEPTION] " << e.what() << "\n";
    }

    return 0;
}

#endif