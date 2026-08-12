/**
 * @file   InStorageState.h
 * @brief  State representing a package physically present in the warehouse.
 *
 *  The package has been received and assigned a StorageLocation.
 *  This is the primary "active" state - most warehouse operations
 *  (filtering, zone queries, overdue checks) target packages in this state.
 *
 * @author Do Minh Khang
 * @date   2026-06-10
 *
 * @update
 * @author Huynh Phuc Nguyen
 * @date   2026-07-19
 * @changelog
 *   - Add clone() override to satisfy the new IPackageState contract.
 */

#pragma once

#include "domain/states/IPackageState.h"

namespace wms::domain
{

    /**
     * @brief  The package is physically present and stored in the warehouse.
     *
     *  handle() checks whether the package is past its expected export date.
     *  If today's date exceeds expectedExportDate, it automatically transitions
     *  the package to OverdueState. This check is called periodically by the
     *  WarehouseManager's QTimer.
     *
     *  THREAD SAFETY: handle() must only be called from WarehouseManager's
     *  periodic check loop (single-threaded via QTimer). Concurrent calls to
     *  handle() on the same Package from multiple threads are not supported
     *  and will cause race conditions in state transitions.
     */
    class InStorageState final : public IPackageState
    {
    public:
        /**
         * @brief  Check for overdue condition and transition if necessary.
         *
         *  Compares today's date against pkg.logistics().expectedExportDate.
         *  Transitions to OverdueState if the date has passed.
         *
         * @param  pkg  The owning package. May be transitioned inside this call.
         */
        void handle(Package& pkg) override;

        /**
         * @brief  Returns "In Storage".
         */
        std::string_view getStateLabel() const override;

        /**
         * @brief  Returns PackageStateId::InStorage.
         */
        PackageStateId stateId() const override;

        /**
         * @brief  Returns an independent copy of this state.
         */
        std::unique_ptr<IPackageState> clone() const override;
    };

}
