/**
 * @file   Dimension.h
 * @brief  Physical dimensions of an object (length, width, height).
 *
 * @author Do Minh Khang
 * @date   2026-06-09
 */

#pragma once

namespace wms::domain
{
    /**
     * @struct Dimension
     * @brief  Represents the physical size of a package or object.
     *
     * Values represent linear measurements (e.g., meters). Callers are
     * responsible for using consistent units across the system.
     */
    struct Dimension
    {
        double length;
        double width;
        double height;
    };
}