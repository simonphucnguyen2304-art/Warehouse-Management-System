/**
 * @file   Category.h
 * @brief  Enumeration of package categories used for handling rules.
 *
 * @author Do Minh Khang
 * @date   2026-06-09
 */

#pragma once

namespace wms::domain
{
    /**
     * @enum Category
     * @brief Classifies packages for handling, storage, and transport logic.
     *
     * Uses enum class to prevent implicit conversions and to provide strong
     * typing across the domain layer.
     */
    enum class Category
    {
        Standard,
        Fragile,
        Perishable,
        Hazmat,
        Oversized,
        Liquid
    };
}