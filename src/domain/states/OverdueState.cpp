/**
 * @file   OverdueState.cpp
 * @brief  Implementation of OverdueState.
 *
 * @author Do Minh Khang
 * @date   2026-06-10
 *
 * @update
 * @author Huynh Phuc Nguyen
 * @date   2026-07-19
 * @changelog
 *   - Implement clone() override required by the updated IPackageState contract.
 */

#include "domain/states/OverdueState.h"
#include "domain/entities/Package.h"

namespace wms::domain
{

    void OverdueState::handle(Package& /*pkg*/)
    {
        // Already in the most severe automatically-detectable state.
        // No further automatic transition - staff must intervene:
        //   - Dispatch the package → DispatchedState
        //   - Mark it lost       → MissingState
    }

    std::string_view OverdueState::getStateLabel() const
    {
        return "Overdue";
    }

    PackageStateId OverdueState::stateId() const
    {
        return PackageStateId::Overdue;
    }

    std::unique_ptr<IPackageState> OverdueState::clone() const
    {
        return std::make_unique<OverdueState>(*this);
    }

}
