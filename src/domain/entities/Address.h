/**
 * @file   Address.h
 * @brief  Value object representing a postal address within the domain.
 *
 * @author Do Minh Khang
 * @date   2026-06-09
 */

#pragma once

#include <string>

namespace wms::domain
{
    /**
     * @struct Address
     * @brief  Represents a physical postal address used by the domain model.
     *
     * All fields are plain std::string to avoid Qt dependencies at the domain
     * boundary. Callers must ensure fields are populated with validated data.
     */
    struct Address
    {
        std::string street;
        std::string city;
        std::string country;
        std::string postalCode;
    };
}