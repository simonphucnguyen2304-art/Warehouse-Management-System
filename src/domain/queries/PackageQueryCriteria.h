/**
 * @file   PackageQueryCriteria.h
 * @brief  Unified filter criteria for querying packages.
 *
 *  Replaces a one-method-per-filter repository API (findByState,
 *  findByZone, ...) with a single findByCriteria(const
 *  PackageQueryCriteria&) method on IPackageRepository. Every field is
 *  optional; unset fields are not applied as constraints. Concrete
 *  repositories translate this struct into whatever query mechanism fits
 *  their backing store - SqlitePackageRepository builds a parameterised SQL
 *  WHERE clause; JsonPackageRepository evaluates it in-memory against its
 *  QSqlDatabase-free store. Callers see one identical interface either way.
 *
 * @author Do Minh Khang
 * @date   2026-07-14
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-07-31
 * * @changelog
 *   - Add lateOnly for query
 * 
 * @update
 * @author Duong Anh Hao
 * @date   2026-08-02
 * @changelog
 *   - Include "domain/entities/Date.h".
 *   - Add importDate and exportDate properties (std::optional<wms::domain::Date>) 
 *      to support custom date filtering from UI
 */

#pragma once

#include "domain/entities/Category.h"
#include "domain/states/PackageStateId.h"
#include "domain/entities/Date.h"

#include <optional>
#include <string>

namespace wms::domain
{
    /**
     * @struct PackageQueryCriteria
     * @brief  Composable, optional filter fields for package queries.
     *
     * All fields default to std::nullopt (or false for overdueOnly),
     * meaning "do not filter on this field." Multiple set fields combine
     * with an implicit logical AND. A default-constructed criteria matches
     * every package, equivalent to calling getAll().
     */
    struct PackageQueryCriteria
    {
        std::optional<std::string> name;
        std::optional<PackageStateId> state;
        std::optional<Category> category;
        std::optional<double> minWeight;
        std::optional<double> maxWeight;
        std::optional<std::string> zone;
        std::optional<std::string> containerId;
        std::optional<std::string> descriptionKeyword;
        std::optional<wms::domain::Date> importDate;
        std::optional<wms::domain::Date> exportDate;

        bool overdueOnly = false;
        bool lateOnly = false;
        bool importedToday = false;
        bool exportDueToday = false;

        
    };
}
