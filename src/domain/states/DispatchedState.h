/**
 * @file   DispatchedState.h
 * @brief  State representing a package that has left the warehouse.
 *
 *  This is a terminal state under normal operation. Once a package is
 *  dispatched it is no longer physically present in the warehouse and
 *  should not appear in active storage queries.
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
     * @brief  The package has been dispatched and has left the warehouse.
     *
     *  handle() is a no-op - a dispatched package requires no further
     *  automatic action from the system. No transition originates here
     *  under normal operation.
     */
    class DispatchedState final : public IPackageState
    {
    public:
        /**
         * @brief  No automatic action once the package has been dispatched.
         * @param  pkg  The owning package (unused in this state).
         */
        void handle(Package& pkg) override;

        /**
         * @brief  Returns "Dispatched".
         */
        std::string_view getStateLabel() const override;

        /**
         * @brief  Returns PackageStateId::Dispatched.
         */
        PackageStateId stateId() const override;

        /**
         * @brief  Returns an independent copy of this state.
         */
        std::unique_ptr<IPackageState> clone() const override;
    };

}
