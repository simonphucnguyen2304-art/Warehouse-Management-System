/**
 * @file   StorageLocation.h
 * @brief  Value object representing a physical location within the warehouse.
 *
 * @author Do Minh Khang
 * @date   2026-06-09
 */

#pragma once

#include <string>

namespace wms::domain
{

	/**
	 * @struct StorageLocation
	 * @brief  Immutable value object encapsulating zone, aisle, shelf, and slot
	 *         coordinates within the warehouse structure.
	 *
	 * All fields are required and must be populated by the caller. This struct
	 * is typically embedded within Package metadata to record where a package is
	 * physically stored.
	 */
	struct StorageLocation
	{
		std::string zone;
		std::string aisle;
		int shelf;
		int slot;
	};

}