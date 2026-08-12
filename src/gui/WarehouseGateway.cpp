/**
 * @file   WarehouseGateway.cpp
 * @brief  Implementation of WarehouseGateway.
 *
 * @author Do Minh Khang
 * @date   2026-07-23

 * @update
 * @author Nguyen Viet Bach
 * @date   2026-07-24
 * @changelog
 *   - Added exportDataJson / importDataJson / exportDataCsv / importDataCsv
 *     forwarding methods. Import variants emit packagesChanged() after the
 *     underlying import completes, so the GUI automatically refreshes.
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-07-31
 * @changelog
 *   - Implement checkLatePackages().
 */

#include "gui/WarehouseGateway.h"

namespace wms::gui
{
    // --Construction--

    WarehouseGateway::WarehouseGateway(service::WarehouseManager* manager, QObject* parent)
        : QObject{ parent }
        , m_manager{ manager }
    {
    }

    // --Reads--

    std::vector<domain::Package> WarehouseGateway::getAllPackages() const
    {
        return m_manager->getAllPackages();
    }

    std::vector<domain::Package> WarehouseGateway::queryPackages(
        const domain::PackageQueryCriteria& criteria) const
    {
        return m_manager->queryPackages(criteria);
    }

    domain::Package WarehouseGateway::getPackage(const std::string& id) const
    {
        return m_manager->getPackage(id);
    }

    service::WarehouseManager::DailyTodoList WarehouseGateway::getDailyTodoList() const
    {
        return m_manager->getDailyTodoList();
    }

    std::vector<domain::Package> WarehouseGateway::getOverdue() const
    {
        return m_manager->getOverdue();
    }

    std::vector<domain::Package> WarehouseGateway::getMissing() const
    {
        return m_manager->getMissing();
    }

    // --Mutations--

    void WarehouseGateway::addPackage(domain::Package package)
    {
        m_manager->addPackage(std::move(package));
        emit packagesChanged();
    }

    void WarehouseGateway::updatePackage(domain::Package package)
    {
        m_manager->updatePackage(std::move(package));
        emit packagesChanged();
    }

    void WarehouseGateway::removePackage(const std::string& id)
    {
        m_manager->removePackage(id);
        emit packagesChanged();
    }

    void WarehouseGateway::receivePackage(const std::string& id)
    {
        m_manager->receivePackage(id);
        emit packagesChanged();
    }

    void WarehouseGateway::dispatchPackage(const std::string& id)
    {
        m_manager->dispatchPackage(id);
        emit packagesChanged();
    }

    void WarehouseGateway::markMissing(const std::string& id)
    {
        m_manager->markMissing(id);
        emit packagesChanged();
    }

    void WarehouseGateway::markFound(const std::string& id)
    {
        m_manager->markFound(id);
        emit packagesChanged();
    }

    int WarehouseGateway::checkOverduePackages()
    {
        const int count = m_manager->checkOverduePackages();
        if (count > 0)
            emit packagesChanged();
        return count;
    }

    int WarehouseGateway::checkLatePackages()
    {
        const int count = m_manager->checkLatePackages();
        // Always emit so the UI refreshes even when count == 0.
        // (The user clicked the button explicitly, so a refresh is expected
        // regardless of whether any package actually changed state.)
        emit packagesChanged();
        return count;
    }

    void WarehouseGateway::save()
    {
        m_manager->save();
    }

    void WarehouseGateway::load()
    {
        m_manager->load();
        emit packagesChanged();
    }

    // --Bulk I/O--

    void WarehouseGateway::exportDataJson(const std::string& filePath) const
    {
        m_manager->exportDataJson(filePath);
        // Read-only: no data changed, no signal needed.
    }

    void WarehouseGateway::importDataJson(const std::string& filePath)
    {
        m_manager->importDataJson(filePath);
        emit packagesChanged(); // new packages may have been merged
    }

    void WarehouseGateway::exportDataCsv(const std::string& filePath) const
    {
        m_manager->exportDataCsv(filePath);
        // Read-only: no data changed, no signal needed.
    }

    void WarehouseGateway::importDataCsv(const std::string& filePath)
    {
        m_manager->importDataCsv(filePath);
        emit packagesChanged(); // new packages may have been merged
    }
}