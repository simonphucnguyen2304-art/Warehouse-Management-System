/**
 * @file   WarehouseGateway.h
 * @brief  Qt-side Observer Subject wrapping WarehouseManager for the GUI layer.
 *
 *  WarehouseManager (service/) is deliberately Qt-free - no QObject, no
 *  signals - so it stays unit-testable and portable without pulling Qt into
 *  the service layer. WarehouseGateway is the one place that boundary gets
 *  crossed: every read/write call is forwarded to the wrapped
 *  WarehouseManager unchanged, and every method that mutates package state
 *  emits packagesChanged() immediately after the mutation succeeds.
 *
 *  This makes WarehouseGateway the Subject in the Observer relationship
 *  between the data layer and the GUI's views. The key structural guarantee
 *  it provides over the previous design (MainWindow manually calling a
 *  persistAndRefresh() helper after each action): there is no call path
 *  through this class that mutates data without also emitting the signal.
 *
 *  Views become Observers by connecting to packagesChanged(). A view does
 *  not know which specific action caused the change, does not know about
 *  any other view, and is not told what changed - it decides for itself
 *  what to re-fetch when notified (see MainWindow::onPackagesChanged() for
 *  the current fan-out to each page).
 *
 * @author Do Minh Khang
 * @date   2026-07-23
 *
 * @update
 * @author Nguyen Viet Bach
 * @date   2026-07-20
 * @changelog
 *   - Added exportDataJson / importDataJson / exportDataCsv / importDataCsv
 *     forwarding methods. Import variants emit packagesChanged() after the
 *     operation so all views refresh without extra wiring in MainWindow.
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-07-31
 * @changelog
 *   - Wire up the new checkLatePackages() from manager. The new one is
 *     replicated from checkOverduePackages since both share nearly identical
 *     structure.
 */

#pragma once

#include "service/WarehouseManager.h"
#include "domain/queries/PackageQueryCriteria.h"

#include <QObject>

#include <string>
#include <vector>

namespace wms::gui
{
    /**
     * @class  WarehouseGateway
     * @brief  Observer Subject: notifies the GUI whenever package data changes.
     *
     * Non-owning wrapper around a WarehouseManager.
     */
    class WarehouseGateway : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief  Wrap an existing WarehouseManager.
         * @param  manager  Must outlive this gateway. Not owned.
         * @param  parent   Standard QObject parent for Qt's ownership tree.
         */
        explicit WarehouseGateway(service::WarehouseManager* manager, QObject* parent = nullptr);

        // --Reads--
        // No notification fired - these don't mutate

        /** @brief Forwards to WarehouseManager::getAllPackages(). */
        std::vector<domain::Package> getAllPackages() const;

        /** @brief Forwards to WarehouseManager::queryPackages(). */
        std::vector<domain::Package> queryPackages(const domain::PackageQueryCriteria& criteria) const;

        /** @brief Forwards to WarehouseManager::getPackage(). */
        domain::Package getPackage(const std::string& id) const;

        /** @brief Forwards to WarehouseManager::getDailyTodoList(). */
        service::WarehouseManager::DailyTodoList getDailyTodoList() const;

        /** @brief Forwards to WarehouseManager::getOverdue(). */
        std::vector<domain::Package> getOverdue() const;

        /** @brief Forwards to WarehouseManager::getMissing(). */
        std::vector<domain::Package> getMissing() const;

        // --Mutations--
        // Each method below calls into WarehouseManager first, then emits
        // packagesChanged(). If the WarehouseManager call throws (e.g. an
        // invalid state transition), the exception propagates to the caller
        // unchanged - the emit line is never reached, so views never
        // refresh in response to a rejected action. Callers (MainWindow's
        // showOperationError()) keep catching exactly as before.

        /** @brief Forwards to WarehouseManager::addPackage(), then notifies. */
        void addPackage(domain::Package package);

        /** @brief Forwards to WarehouseManager::updatePackage(), then notifies. */
        void updatePackage(domain::Package package);

        /** @brief Forwards to WarehouseManager::removePackage(), then notifies. */
        void removePackage(const std::string& id);

        /** @brief Forwards to WarehouseManager::receivePackage(), then notifies. */
        void receivePackage(const std::string& id);

        /** @brief Forwards to WarehouseManager::dispatchPackage(), then notifies. */
        void dispatchPackage(const std::string& id);

        /** @brief Forwards to WarehouseManager::markMissing(), then notifies. */
        void markMissing(const std::string& id);

        /** @brief Forwards to WarehouseManager::markFound(), then notifies. */
        void markFound(const std::string& id);

        /**
         * @brief  Scan for newly-overdue packages.
         *
         *  Only emits packagesChanged() if at least one package actually
         *  transitioned - matches the previous behaviour where MainWindow
         *  only refreshed when checkOverduePackages() returned a non-zero
         *  count.
         *
         * @return Number of packages transitioned to OverdueState.
         */
        int checkOverduePackages();

        /**
         * @brief  Scan for newly-missing packages.
         *
         *  Replicate the checkOverduePackages() for missing checking.
         *
         * @return Number of packages transitioned to OverdueState.
         */
        int checkLatePackages();

        /**
         * @brief  Forwards to WarehouseManager::save(). No-op on the SQLite
         *         backend (every mutation already commits directly); kept
         *         for interface parity so a future swap back to the JSON
         *         repository does not require touching call sites.
         *         Does not emit packagesChanged() - saving does not change
         *         what is queryable.
         */
        void save();

        /**
         * @brief  Forwards to WarehouseManager::load(), then emits
         *         packagesChanged(). No-op on the SQLite backend, but on
         *         the JSON backend this discards in-memory state and
         *         re-reads from disk, which views must refresh in response
         *         to - unlike save(), this can change what is queryable.
         */
        void load();

        // --Bulk I/O--

        /** @brief Forwards to WarehouseManager::exportDataJson(). Read-only; no signal emitted. */
        void exportDataJson(const std::string& filePath) const;

        /**
         * @brief  Forwards to WarehouseManager::importDataJson().
         *         Emits packagesChanged() on success so all views refresh.
         */
        void importDataJson(const std::string& filePath);

        /** @brief Forwards to WarehouseManager::exportDataCsv(). Read-only; no signal emitted. */
        void exportDataCsv(const std::string& filePath) const;

        /**
         * @brief  Forwards to WarehouseManager::importDataCsv().
         *         Emits packagesChanged() on success so all views refresh.
         */
        void importDataCsv(const std::string& filePath);

    signals:
        /**
         * @brief  Fired after any mutation above completes without throwing.
         *
         *  Deliberately one un-parameterised signal rather than one signal
         *  per action (packageAdded, packageRemoved, ...): every current
         *  view responds to a change by re-fetching its own slice of data
         *  via queryPackages()/getDailyTodoList(), not by patching a
         *  specific row in place, so a more granular signal has nothing to
         *  buy today. Split this into typed signals only when a view needs
         *  to react differently depending on what changed - e.g. an
         *  in-place row update instead of a full re-fetch, or a toast
         *  naming the affected package.
         */
        void packagesChanged();

    private:
        service::WarehouseManager* m_manager;
    };
}