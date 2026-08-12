/**
 * @file   RepositoryHelpers.h
 * @brief  Shared serialisation helpers for repository implementations.
 *
 *  Provides inline free functions for converting domain enum values and
 *  date types to/from their string representations. These conversions are
 *  identical across JsonPackageRepository and SqlitePackageRepository;
 *  centralising them here eliminates the duplication and ensures both
 *  backends always produce the same wire format.
 *
 *  All functions are header-only (inline) so no separate .cpp is needed.
 *  Include this file only from repository implementation files (.cpp), not
 *  from headers, to keep Qt types (QString) out of the domain boundary.
 *
 * @author Huynh Phuc Nguyen
 * @date   2026-07-19
 */

#pragma once

#include "domain/entities/Category.h"
#include "domain/entities/Date.h"
#include "domain/states/PackageStateId.h"

#include <QString>
#include <QStringList>

#include <chrono>
#include <stdexcept>

namespace wms::repository::helpers
{
    // -------------------------------------------------------------------------
    // Category <-> string
    // -------------------------------------------------------------------------

    /**
     * @brief  Convert a Category enum value to its canonical string token.
     * @param  c  The category to convert.
     * @return One of: "Standard", "Fragile", "Perishable", "Hazmat",
     *         "Oversized", "Liquid". Falls back to "Standard" for unknown values.
     */
    inline QString categoryToString(domain::Category c)
    {
        switch (c)
        {
        case domain::Category::Standard:   return "Standard";
        case domain::Category::Fragile:    return "Fragile";
        case domain::Category::Perishable: return "Perishable";
        case domain::Category::Hazmat:     return "Hazmat";
        case domain::Category::Oversized:  return "Oversized";
        case domain::Category::Liquid:     return "Liquid";
        }
        return "Standard";
    }

    /**
     * @brief  Parse a category string token back to a Category enum value.
     * @param  s  One of the canonical tokens produced by categoryToString().
     * @return The matching Category, or Category::Standard for unrecognised input.
     */
    inline domain::Category categoryFromString(const QString& s)
    {
        if (s == "Fragile")    return domain::Category::Fragile;
        if (s == "Perishable") return domain::Category::Perishable;
        if (s == "Hazmat")     return domain::Category::Hazmat;
        if (s == "Oversized")  return domain::Category::Oversized;
        if (s == "Liquid")     return domain::Category::Liquid;
        return domain::Category::Standard;
    }

    // -------------------------------------------------------------------------
    // PackageStateId <-> string
    // -------------------------------------------------------------------------

    /**
     * @brief  Convert a PackageStateId enum value to its canonical string token.
     * @param  id  The state identifier to convert.
     * @return One of: "OnRoute", "InStorage", "Dispatched", "Missing", "Overdue".
     *         Falls back to "OnRoute" for unknown values.
     */
    inline QString stateIdToString(domain::PackageStateId id)
    {
        switch (id)
        {
        case domain::PackageStateId::OnRoute:    return "OnRoute";
        case domain::PackageStateId::InStorage:  return "InStorage";
        case domain::PackageStateId::Dispatched: return "Dispatched";
        case domain::PackageStateId::Missing:    return "Missing";
        case domain::PackageStateId::Overdue:    return "Overdue";
        }
        return "OnRoute";
    }

    /**
     * @brief  Parse a state string token back to a PackageStateId enum value.
     * @param  s  One of the canonical tokens produced by stateIdToString().
     * @return The matching PackageStateId, or PackageStateId::OnRoute for
     *         unrecognised input.
     */
    inline domain::PackageStateId stateIdFromString(const QString& s)
    {
        if (s == "InStorage")  return domain::PackageStateId::InStorage;
        if (s == "Dispatched") return domain::PackageStateId::Dispatched;
        if (s == "Missing")    return domain::PackageStateId::Missing;
        if (s == "Overdue")    return domain::PackageStateId::Overdue;
        return domain::PackageStateId::OnRoute;
    }

    // -------------------------------------------------------------------------
    // Date <-> string  ("YYYY-MM-DD")
    // -------------------------------------------------------------------------

    /**
     * @brief  Format a domain::Date as an ISO-8601 date string "YYYY-MM-DD".
     * @param  d  The date to format.
     * @return Zero-padded string, e.g. "2026-07-19".
     */
    inline QString dateToString(const domain::Date& d)
    {
        return QString("%1-%2-%3")
            .arg(static_cast<int>(d.year()), 4, 10, QChar('0'))
            .arg(static_cast<unsigned>(d.month()), 2, 10, QChar('0'))
            .arg(static_cast<unsigned>(d.day()), 2, 10, QChar('0'));
    }

    /**
     * @brief  Parse an ISO-8601 date string "YYYY-MM-DD" to a domain::Date.
     * @param  s  String produced by dateToString().
     * @return The parsed date.
     * @throws std::runtime_error if the string cannot be split into three parts.
     */
    inline domain::Date dateFromString(const QString& s)
    {
        const QStringList parts = s.split('-');
        if (parts.size() != 3)
            throw std::runtime_error(
                "RepositoryHelpers::dateFromString - invalid date string: " +
                s.toStdString());

        return domain::Date{
            std::chrono::year{ parts[0].toInt() },
            std::chrono::month{ static_cast<unsigned>(parts[1].toUInt()) },
            std::chrono::day{ static_cast<unsigned>(parts[2].toUInt()) }
        };
    }

    /**
     * @brief  Return today's date formatted as "YYYY-MM-DD".
     *
     *  Convenience wrapper used by SqlitePackageRepository when binding
     *  date-relative WHERE clauses (importedToday, exportDueToday, overdueOnly).
     *
     * @return Today's date string.
     */
    inline QString todayAsString()
    {
        const auto today = std::chrono::floor<std::chrono::days>(
            std::chrono::system_clock::now());
        return dateToString(domain::Date{ today });
    }

} // namespace wms::repository::helpers
