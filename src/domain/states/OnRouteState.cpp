/**
 * @file   OnRouteState.cpp
 * @brief  Implementation of OnRouteState.
 *
 * @author Do Minh Khang
 * @date   2026-06-10
 *
 * @update
 * @author Huynh Phuc Nguyen
 * @date   2026-07-19
 * @changelog
 *   - Implement clone() override required by the updated IPackageState contract.
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-07-30
 * @changelog
 *   - Add body implementation for handle(pkg)
 *   - All related function across the codebase MIGHT change based on this
 */

#include "domain/states/OnRouteState.h"
#include "domain/entities/Package.h"
#include "domain/entities/Package.h"
#include "domain/states/MissingState.h"

#include <chrono>
#include <memory>

namespace wms::domain
{

    void OnRouteState::handle(Package& pkg)
    {
        const auto today = std::chrono::year_month_day{
            std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())
        };
        const auto dueDate = pkg.logistics().importDate;

        // Transition to MissingState if today is past the import date.
        if (today >= dueDate)
        {
            pkg.transitionTo(std::make_unique<MissingState>());
        }
    }

    std::string_view OnRouteState::getStateLabel() const
    {
        return "On Route";
    }

    PackageStateId OnRouteState::stateId() const
    {
        return PackageStateId::OnRoute;
    }

    std::unique_ptr<IPackageState> OnRouteState::clone() const
    {
        return std::make_unique<OnRouteState>(*this);
    }

}
