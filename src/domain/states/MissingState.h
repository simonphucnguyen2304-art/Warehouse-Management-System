/**
 * @file   MissingState.h
 * @brief  State representing a package that cannot be located in the warehouse.
 *
 *  Set manually by warehouse staff when a physical audit fails to find the
 *  package at its recorded StorageLocation. Unlike DispatchedState, this is
 *  NOT terminal - a missing package can be found and returned to InStorageState.
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
     * @brief  The package cannot be located at its expected StorageLocation.
     *
     *  handle() is a no-op - the system cannot automatically resolve a missing
     *  package. Recovery is a manual operation: staff locate the package and
     *  explicitly transition it back to InStorageState (or another state).
     */
    class MissingState final : public IPackageState
    {
    public:
        /**
         * @brief  No automatic action for a missing package.
         * @param  pkg  The owning package (unused in this state).
         */
        void handle(Package& pkg) override;

        /**
         * @brief  Returns "Missing".
         */
        std::string_view getStateLabel() const override;

        /**
         * @brief  Returns PackageStateId::Missing.
         */
        PackageStateId stateId() const override;

        /**
         * @brief  Returns an independent copy of this state.
         */
        std::unique_ptr<IPackageState> clone() const override;
    };

}
