/**
 * @file   Package.cpp
 * @brief  Implementation of the Package domain entity.
 *
 * @author Do Minh Khang
 * @date   2026-06-10
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-06-24
 * @changelog
 *   - Re-implement contruction
 *   - Implement create() and load()
 * 
 * @update
 * @author Huynh Phuc Nguyen
 * @date   2026-09-19
 * @changelog
 *   - Copy constructor and operator now use state clone()
 */

#include "domain/entities/Package.h"

#include "domain/states/OnRouteState.h"
#include "domain/states/InStorageState.h"
#include "domain/states/DispatchedState.h"
#include "domain/states/MissingState.h"
#include "domain/states/OverdueState.h"

#include <random>
#include <sstream>
#include <iomanip>
#include <stdexcept>


namespace wms::domain
{

    Package Package::create(PackageMetadata metadata,
        Address source,
        Address destination,
        LogisticsInfo logistics,
        StorageLocation location)
    {
        if (logistics.importDate > logistics.expectedExportDate)
        {
            throw std::invalid_argument(
                "Package: expectedExportDate must be >= importDate"
            );
        }
        
        return Package{
            generateUuid(),               // fresh UUID
            std::move(metadata),
            std::move(source),
            std::move(destination),
            std::move(logistics),
            std::move(location),
            PackageStateId::OnRoute       // always starts OnRoute
        };
    }

    Package Package::load(std::string existingId,
        PackageMetadata metadata,
        Address source,
        Address destination,
        LogisticsInfo logistics,
        StorageLocation location,
        PackageStateId stateId)
    {
        return Package{
            std::move(existingId),        // preserved from file
            std::move(metadata),
            std::move(source),
            std::move(destination),
            std::move(logistics),
            std::move(location),
            stateId                       // preserved from file
        };
    }

    Package Package::createWithState(PackageMetadata metadata,
        Address source,
        Address destination,
        LogisticsInfo logistics,
        StorageLocation location,
        PackageStateId stateId)
    {
        return Package{
            generateUuid(),
            std::move(metadata),
            std::move(source),
            std::move(destination),
            std::move(logistics),
            std::move(location),
            stateId
        };
    }

    // --Construction--

    Package::Package(std::string id,
        PackageMetadata metadata,
        Address source,
        Address destination,
        LogisticsInfo logistics,
        StorageLocation location,
        PackageStateId stateId)
        : m_id{ std::move(id) }
        , m_metadata{ std::move(metadata) }
        , m_source{ std::move(source) }
        , m_destination{ std::move(destination) }
        , m_logistics{ std::move(logistics) }
        , m_location{ std::move(location) }
        , m_state{ makeStateFromId(stateId) }
    {
        // Note: date-order validation (importDate <= expectedExportDate) is
        // intentionally NOT repeated here. This constructor is called by both
        // create() – which validates before delegating – and load() – which
        // restores persisted packages whose dates may have been updated at
        // runtime (e.g. importDate set to today on early arrival). Repeating
        // the check here would reject valid in-flight state mutations.
    }

    // --Rule of five--

    Package::Package(const Package& other)
        : m_id{ other.m_id }
        , m_metadata{ other.m_metadata }
        , m_source{ other.m_source }
        , m_destination{ other.m_destination }
        , m_logistics{ other.m_logistics }
        , m_location{ other.m_location }
        // unique_ptr is not copyable, so we cannot use = default here.
        // Instead, reconstruct a fresh state of the same type from the enum id.
        // The new state is independent - modifying the copy's state does not
        // affect the original.
        , m_state{ other.m_state->clone() }
    {
    }

    Package& Package::operator=(const Package& other)
    {
        // Guard against self-assignment (p = p).
        if (this == &other)
            return *this;

        m_id = other.m_id;
        m_metadata = other.m_metadata;
        m_source = other.m_source;
        m_destination = other.m_destination;
        m_logistics = other.m_logistics;
        m_location = other.m_location;
        m_state = other.m_state->clone();

        return *this;
    }

    // --Identity--

    const std::string& Package::id() const
    {
        return m_id;
    }

    // --Value object accessors--

    const PackageMetadata& Package::metadata() const
    {
        return m_metadata;
    }

    const Address& Package::source() const
    {
        return m_source;
    }

    const Address& Package::destination() const
    {
        return m_destination;
    }

    const LogisticsInfo& Package::logistics() const
    {
        return m_logistics;
    }

    const StorageLocation& Package::location() const
    {
        return m_location;
    }

    // --Setters--

    void Package::setLogistics(LogisticsInfo logistics)
    {
        m_logistics = std::move(logistics);
    }

    void Package::setLocation(StorageLocation location)
    {
        m_location = std::move(location);
    }

    void Package::setMetadata(PackageMetadata metadata)
    {
        m_metadata = std::move(metadata);
    }

    // --State pattern--

    void Package::transitionTo(std::unique_ptr<IPackageState> newState)
    {
        if (!newState)
            throw std::invalid_argument("Package::transitionTo - newState must not be nullptr");

        m_state = std::move(newState);
    }

    PackageStateId Package::currentStateId() const
    {
        return m_state->stateId();
    }

    const IPackageState& Package::currentState() const
    {
        return *m_state;
    }

    void Package::handleCurrentState()
    {
        // Delegates to the active state. The state may call transitionTo()
        // on this package if an automatic transition is required
        // (e.g. InStorageState detecting an overdue date).
        m_state->handle(*this);
    }

    // --Convenience helpers--

    bool Package::isInStorage() const
    {
        return m_state->stateId() == PackageStateId::InStorage;
    }

    bool Package::isOverdue() const
    {
        return m_state->stateId() == PackageStateId::Overdue;
    }

    bool Package::isMissing() const
    {
        return m_state->stateId() == PackageStateId::Missing;
    }

    // --Private helpers--

    std::string Package::generateUuid()
    {
        // UUID v4: 128 random bits formatted as 8-4-4-4-12 hex digits.
        // Initialized once per thread via thread-local static PRNG to avoid
        // expensive re-initialization and improve cache locality. The Mersenne
        // Twister is seeded once on first call per thread from hardware entropy.
        static thread_local std::mt19937 gen(
            []() {
                std::random_device rd;
                return rd();
            }()
        );

        std::uniform_int_distribution<> dis(0, 15);
        std::uniform_int_distribution<> dis2(8, 11); // variant bits

        std::ostringstream oss;
        oss << std::hex;

        for (int i = 0; i < 8; ++i) oss << dis(gen);
        oss << '-';
        for (int i = 0; i < 4; ++i) oss << dis(gen);
        oss << '-';
        oss << '4';                                    // version 4
        for (int i = 0; i < 3; ++i) oss << dis(gen);
        oss << '-';
        oss << dis2(gen);                              // variant
        for (int i = 0; i < 3; ++i) oss << dis(gen);
        oss << '-';
        for (int i = 0; i < 12; ++i) oss << dis(gen);

        return oss.str();
    }

    std::unique_ptr<IPackageState> Package::makeStateFromId(PackageStateId id)
    {
        // Exhaustive switch - if a new state is added to PackageStateId,
        // the compiler will warn about the missing case here.
        switch (id)
        {
        case PackageStateId::OnRoute: return std::make_unique<OnRouteState>();
        case PackageStateId::InStorage: return std::make_unique<InStorageState>();
        case PackageStateId::Dispatched: return std::make_unique<DispatchedState>();
        case PackageStateId::Missing: return std::make_unique<MissingState>();
        case PackageStateId::Overdue: return std::make_unique<OverdueState>();
        }

        // Unreachable if the switch is exhaustive, but satisfies the compiler
        // on configurations that don't treat non-exhaustive switches as errors.
        throw std::invalid_argument("Package::makeStateFromId - unknown PackageStateId");
    }

}