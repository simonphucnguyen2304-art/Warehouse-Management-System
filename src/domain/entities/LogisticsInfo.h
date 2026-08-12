/**
 * @file   LogisticsInfo.h
 * @brief  Logistics-related timestamps and transport identifiers for a package.
 *
 * @author Do Minh Khang
 * @date   2026-06-09
 */

#pragma once

#include "domain/entities/Date.h"

#include <string>

namespace wms::domain
{
    /**
     * @struct LogisticsInfo
     * @brief  Encapsulates inbound/outbound dates and transport identifiers.
     *
     * This value object is domain-only and intentionally uses `Date` and
     * `std::string` to avoid Qt dependencies at the domain boundary.
     * It is embedded in `Package` metadata for scheduling, auditing, and
     * export/import reconciliation.
     */
    struct LogisticsInfo
    {
        Date importDate;
        Date expectedExportDate;
        std::string importVehicle;
        std::string exportVehicle;
        std::string containerId;
    };
}