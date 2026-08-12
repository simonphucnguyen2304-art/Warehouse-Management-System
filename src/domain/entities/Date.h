/**
 * @file   Date.h
 * @brief  Alias for a calendar date type used in the domain layer.
 *
 * @author Do Minh Khang
 * @date   2026-06-09
 */

#pragma once

#include <chrono>

namespace wms::domain
{
    /**
     * @brief  Domain alias for a calendar date without time of day.
     *
     * Uses std::chrono::year_month_day to represent a calendar date and
     * intentionally avoids Qt types to keep the domain layer pure C++.
     */
    using Date = std::chrono::year_month_day;
}