/**
 * @file   OnRouteState.h
 * @brief  State representing a package in transit to the warehouse.
 *
 *  This is the initial state of every newly created package.
 *  The package has been registered in the system but has not yet
 *  been physically received at the warehouse.
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
     * @brief  The package is on its way to the warehouse and not yet received.
     *
     *  handle() is a no-op in this state - no automatic transition occurs
     *  while the package is in transit. The transition to InStorageState
     *  is triggered explicitly by warehouse staff when the package arrives.
     */
    class OnRouteState final : public IPackageState
    {
    public:
        /**
         * @brief  No automatic action while the package is in transit.
         * @param  pkg  The owning package (unused in this state).
         */
        void handle(Package& pkg) override;

        /**
         * @brief  Returns "On Route".
         */
        std::string_view getStateLabel() const override;

        /**
         * @brief  Returns PackageStateId::OnRoute.
         */
        PackageStateId stateId() const override;

        /**
         * @brief  Returns an independent copy of this state.
         */
        std::unique_ptr<IPackageState> clone() const override;
    };

}
