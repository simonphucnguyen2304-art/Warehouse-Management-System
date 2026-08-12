/**
 * @file   PackageStateId.h
 * @brief  Stable numeric identity for each package lifecycle state.
 *
 *  Used for serialisation (JSON) and switch-based dispatch without
 *  resorting to dynamic_cast on the IPackageState interface.
 *
 * @author Do Minh Khang
 * @date   2026-06-10
 */

#pragma once

namespace wms::domain
{

    /**
     * @brief  Enumerates every state a package can be in during its lifecycle.
     *
     *  The underlying integer values are stored in JSON - never change them
     *  once data files exist, or existing saves will deserialise incorrectly.
     *  Add new states by appending at the end only.
     */
    enum class PackageStateId
    {
        OnRoute = 0,
        InStorage = 1,
        Dispatched = 2,
        Missing = 3,
        Overdue = 4,
    };

}
