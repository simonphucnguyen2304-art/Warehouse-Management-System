/**
 * @file   Package.h
 * @brief  Core domain entity representing a single warehouse package.
 *
 *  Package is the central class of the entire system. It owns:
 *   - A set of immutable value objects describing what the package is,
 *     where it came from/is going, and where it sits in the warehouse.
 *   - A polymorphic state object (via unique_ptr<IPackageState>) that
 *     represents its current lifecycle stage.
 *
 *  Design rules:
 *   - No Qt types anywhere in this file. Qt lives in repository/ and gui/.
 *   - State can only be changed through transitionTo() - never directly.
 *   - Package is movable and copyable; copies carry the same state label
 *     but own independent state objects.
 *
 * @author Do Minh Khang
 * @date   2026-06-10
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-06-24
 * @changelog
 *   - Make the construction function private
 *   - Add public static create() and load() to call construction function
 */

#pragma once

#include "domain/entities/PackageMetadata.h"
#include "domain/entities/Address.h"
#include "domain/entities/LogisticsInfo.h"
#include "domain/entities/StorageLocation.h"
#include "domain/states/IPackageState.h"
#include "domain/states/PackageStateId.h"

#include <memory>
#include <string>

namespace wms::domain
{

    /**
     * @brief  Represents a single package managed by the warehouse.
     *
     *  Owns all descriptive data (metadata, addresses, logistics, location)
     *  as value members and owns its current state via unique_ptr.
     *
     *  The class is copyable. Copying a Package produces an independent object
     *  with a fresh state instance of the same type - see the copy constructor.
     */
    class Package
    {
    public:
        /**
         * @brief  Create a new package with a generated UUID.
         *         Use this when the user adds a package through the GUI.
         */
        static Package create(PackageMetadata metadata,
            Address source,
            Address destination,
            LogisticsInfo logistics,
            StorageLocation location);

        /**
         * @brief  Restore a package from persistent storage.
         *         Preserves the original ID and state.
         *         Use this only in JsonPackageRepository::packageFromJson().
         */
        static Package load(std::string existingId,
            PackageMetadata metadata,
            Address source,
            Address destination,
            LogisticsInfo logistics,
            StorageLocation location,
            PackageStateId stateId);

        /**
         * @brief  Create a new package with a generated UUID in a chosen initial state.
         *
         *  Like create(), but allows specifying an initial state other than
         *  OnRoute. Intended for GUI flows where staff register a package
         *  that is already in a known state (e.g. InStorage on arrival).
         *
         * @param  stateId  The initial lifecycle state for the package.
         */
        static Package createWithState(PackageMetadata metadata,
            Address source,
            Address destination,
            LogisticsInfo logistics,
            StorageLocation location,
            PackageStateId stateId);

        // --Rule of five--
        // Package owns a unique_ptr<IPackageState>, so we must define or
        // declare all five special members explicitly.
        ~Package() = default;
        Package(const Package& other);
        Package& operator=(const Package& other);
        Package(Package&& other) noexcept = default;
        Package& operator=(Package&& other) noexcept = default;

        // --Identity--

        /**
         * @brief  Returns the unique identifier for this package (UUID string).
         */
        const std::string& id() const;

        // --Value object accessors--

        /**
         * @brief  Returns the package's descriptive metadata (category, weight, etc.).
         */
        const PackageMetadata& metadata() const;

        /**
         * @brief  Returns the origin address.
         */
        const Address& source() const;

        /**
         * @brief  Returns the delivery destination address.
         */
        const Address& destination() const;

        /**
         * @brief  Returns logistics information (dates, vehicles, container).
         */
        const LogisticsInfo& logistics() const;

        /**
         * @brief  Returns the physical storage location inside the warehouse.
         */
        const StorageLocation& location() const;

        // --Setters for mutable fields--
        // Metadata and addresses are set once at creation, but logistics and
        // location change during the package's warehouse lifetime.

        /**
         * @brief  Updates the package's logistics information.
         * @param  logistics  New logistics data to replace the current values.
         */
        void setLogistics(LogisticsInfo logistics);

        /**
         * @brief  Updates the physical storage location.
         * @param  location  New location inside the warehouse.
         */
        void setLocation(StorageLocation location);

        /**
         * @brief  Updates the package metadata.
         * @param  metadata  Replacement metadata (e.g. corrected weight or category).
         */
        void setMetadata(PackageMetadata metadata);

        // --State pattern--

        /**
         * @brief  Replace the current state with a new one.
         *
         *  Takes ownership of the new state via move semantics. The old state
         *  is destroyed immediately. Only this method may mutate m_state -
         *  no external code should access m_state directly.
         *
         *  Usage:
         *  @code
         *      pkg.transitionTo(std::make_unique<InStorageState>());
         *  @endcode
         *
         * @param  newState  The state to transition into. Must not be nullptr.
         * @throws std::invalid_argument if newState is nullptr.
         */
        void transitionTo(std::unique_ptr<IPackageState> newState);

        /**
         * @brief  Returns the stable enum identity of the current state.
         *
         *  Use this for serialisation and filter predicates. Avoids dynamic_cast.
         *
         * @return The PackageStateId of the currently active state.
         */
        PackageStateId currentStateId() const;

        /**
         * @brief  Returns a read-only reference to the current state object.
         *
         *  Use this to call getStateLabel() for display, or to invoke handle()
         *  from WarehouseManager's periodic check.
         *
         * @return Const reference to the active IPackageState.
         */
        const IPackageState& currentState() const;

        /**
         * @brief  Calls handle() on the current state, passing this package.
         *
         *  WarehouseManager calls this periodically (driven by QTimer) to let
         *  states perform automatic transitions (e.g. InStorage → Overdue).
         *  Declared here so the manager does not need to call state().handle()
         *  directly - keeping the state encapsulated inside Package.
         */
        void handleCurrentState();

        // --Convenience helpers--

        /**
         * @brief  Returns true if the package is currently in InStorageState.
         *
         *  Shorthand used by WarehouseManager's overdue scan loop.
         */
        bool isInStorage() const;

        /**
         * @brief  Returns true if the package is currently in OverdueState.
         */
        bool isOverdue() const;

        /**
         * @brief  Returns true if the package is currently in MissingState.
         */
        bool isMissing() const;

    private:
        std::string m_id;
        PackageMetadata m_metadata;
        Address m_source;
        Address m_destination;
        LogisticsInfo m_logistics;
        StorageLocation m_location;

        std::unique_ptr<IPackageState> m_state;

        // --Construction--

        /**
         * @brief  Construct a fully initialised Package in OnRouteState.
         *
         *  This is the only way to create a Package. The private constructor
         *   only being called by public create() or load()
         *
         * @param  id           Unique ID
         * @param  metadata     Descriptive properties (category, weight, etc.).
         * @param  source       Origin address.
         * @param  destination  Delivery address.
         * @param  logistics    Dates, vehicles, and container information.
         * @param  location     Physical position inside the warehouse.
         * @param  stateId      The current state of Package
         */
        Package(std::string id,
            PackageMetadata metadata,
            Address source,
            Address destination,
            LogisticsInfo logistics,
            StorageLocation location,
            PackageStateId stateId);
        // --Private helpers--

        /**
         * @brief  Generates a new UUID v4 string for use as a package ID.
         * @return A randomly generated UUID string, e.g. "550e8400-e29b-41d4-...".
         */
        static std::string generateUuid();

        /**
         * @brief  Constructs a fresh state object from a PackageStateId enum value.
         *
         *  Used by the copy constructor to clone the state without knowing the
         *  concrete type. Also used by JsonPackageRepository when deserialising.
         *
         * @param  id  The state to instantiate.
         * @return A new heap-allocated state wrapped in unique_ptr.
         */
        static std::unique_ptr<IPackageState> makeStateFromId(PackageStateId id);
    };

}