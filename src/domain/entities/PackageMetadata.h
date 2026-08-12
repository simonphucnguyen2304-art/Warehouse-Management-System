/**
 * @file   PackageMetadata.h
 * @brief  Metadata describing package characteristics used across the domain.
 *
 * @author Do Minh Khang
 * @date   2026-06-09
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-07-11
 * @changelog
 *   - Add package name
 */

#pragma once

#include "domain/entities/Category.h"
#include "domain/entities/Dimension.h"

#include <string>

namespace wms::domain
{
    /**
     * @struct PackageMetadata
     * @brief  Aggregates descriptive attributes of a package.
     *
     * The struct is a plain value object used by the domain layer for pricing,
     * handling, and storage decisions. It intentionally uses std::string and
     * other domain types to remain Qt-free.
     */
    struct PackageMetadata
    {
        std::string name;
        Category category;
        double weight;
        Dimension dimensions;
        double cost;
        std::string description;
    };
}