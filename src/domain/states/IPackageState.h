/**
 * @file   IPackageState.h
 * @brief  Abstract interface for all concrete package lifecycle states.
 *
 *  Every state the Package can be in must implement this interface.
 *  Concrete states are stateless - they carry no member data.
 *  Any context a state needs is passed in through handle().
 *
 * @author Do Minh Khang
 * @date   2026-06-10
 *
 * @update
 * @author Huynh Phuc Nguyen
 * @date   2026-07-19
 * @changelog
 *   - Add clone() pure virtual method to the interface contract.
 *     Package's copy constructor previously reconstructed state via
 *     makeStateFromId(), which silently loses any data if a future concrete
 *     state is ever made stateful. clone() makes each state responsible for
 *     copying itself, so the contract is enforced at compile time: adding a
 *     stateful concrete state without overriding clone() is a build error,
 *     not a silent runtime data loss.
 *   - Remove reference to PackageFilter::byState() in stateId() doc comment;
 *     filtering is now handled exclusively through PackageQueryCriteria.
 */

#pragma once

#include "domain/states/PackageStateId.h"

#include <memory>
#include <string_view>

// Forward declaration - avoids a circular include between Package and IPackageState.
// Package.h includes IPackageState.h, so IPackageState.h must not include Package.h.
// Package.h will be included in the .cpp file of each concrete State for the handle()
// parameter, not in the header.
namespace wms::domain { class Package; }

namespace wms::domain
{

    /**
     * @class  IPackageState
     * @brief  Interface that every concrete package state must implement.
     *
     *  Defines the four responsibilities of a state:
     *   - handle()        : act on the owning Package (e.g. check conditions,
     *                       trigger an automatic transition).
     *   - getStateLabel() : provide a human-readable display string for the GUI.
     *   - stateId()       : return the stable enum identity for serialisation
     *                       and switch-based logic.
     *   - clone()         : produce an independent copy of this state object,
     *                       used by Package's copy constructor.
     */
    class IPackageState
    {
    public:
        // --Construction--
        IPackageState() = default;

        // --Rule of five--
        // States are stateless value-like objects. Copying or moving them
        // is safe and the compiler-generated defaults are correct.
        IPackageState(const IPackageState&) = default;
        IPackageState& operator=(const IPackageState&) = default;
        IPackageState(IPackageState&&) = default;
        IPackageState& operator=(IPackageState&&) = default;

        /**
         * @brief  Virtual destructor - required for safe polymorphic deletion
         *         through a base-class pointer.
         */
        virtual ~IPackageState() = default;

        // --Interface contract--

        /**
         * @brief  Perform state-specific logic on the owning package.
         *
         *  Called by WarehouseManager on periodic checks or explicit triggers.
         *  A state may call pkg.transitionTo() inside this method to drive an
         *  automatic transition (e.g. InStorageState detecting a past-due date).
         *
         * @param  pkg  The Package that owns this state. Passed by reference so
         *              the state can inspect or modify it, including transitioning.
         */
        virtual void handle(Package& pkg) = 0;

        /**
         * @brief  Return a human-readable label shown in the GUI table.
         *
         *  Returns string_view into a compile-time string literal - no allocation.
         *  The GUI layer converts this to QString at the Qt boundary.
         *
         * @return A stable, null-terminated string_view, e.g. "In Storage".
         */
        virtual std::string_view getStateLabel() const = 0;

        /**
         * @brief  Return the stable enum identity of this state.
         *
         *  Used for serialisation and switch-based dispatch. Avoids dynamic_cast
         *  entirely - callers compare the returned enum value rather than
         *  down-casting the pointer.
         *
         * @return The PackageStateId value corresponding to this state.
         */
        virtual PackageStateId stateId() const = 0;

        /**
         * @brief  Produce an independent heap-allocated copy of this state.
         *
         *  Used by Package's copy constructor instead of makeStateFromId(), so
         *  that each concrete state is responsible for cloning itself. This
         *  enforces the stateless contract at compile time: a future concrete
         *  state that carries member data must override clone() to copy that
         *  data, rather than relying on makeStateFromId() silently dropping it.
         *
         *  All current concrete states are stateless, so their clone()
         *  implementations are trivially:
         *  @code
         *      return std::make_unique<ConcreteState>(*this);
         *  @endcode
         *
         * @return A new unique_ptr owning a copy of this state.
         */
        virtual std::unique_ptr<IPackageState> clone() const = 0;
    };

}
