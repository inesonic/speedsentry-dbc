/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 - 2023 Inesonic, LLC.
*
* GNU Public License, Version 3:
*   This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
*   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
*   version.
*   
*   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
*   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
*   details.
*   
*   You should have received a copy of the GNU General Public License along with this program.  If not, see
*   <https://www.gnu.org/licenses/>.
********************************************************************************************************************//**
* \file
*
* This header defines the \ref Region class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef REGION_H
#define REGION_H

#include <QString>

#include <cstdint>

class Regions;

/**
 * Trivial class used to hold information about a specific region.
 */
class Region {
    friend class Regions;

    public:
        /**
         * Value used to represent a region ID.
         */
        typedef std::uint16_t RegionId;

        /**
         * Value used to indicate an invalid region ID.
         */
        static constexpr RegionId invalidRegionId = 0;

    private:
        /**
         * Constructor.
         *
         * \param[in] regionId   The ID used to identify this region.
         *
         * \param[in] regionName The name used to describe this region.
         */
        inline Region(
                RegionId       regionId,
                const QString& regionName
            ):currentRegionId(
                regionId
            ),currentRegionName(
                regionName
            ) {}

    public:
        inline Region():currentRegionId(invalidRegionId) {}

        /**
         * Copy constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline Region(
                const Region& other
            ):currentRegionId(
                other.currentRegionId
            ),currentRegionName(
                other.currentRegionName
            ) {}

        /**
         * Move constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline Region(
                Region&& other
            ):currentRegionId(
                other.currentRegionId
            ),currentRegionName(
                other.currentRegionName
            ) {}

        ~Region() = default;

        /**
         * Method you can use to determine if this region instance is valid.
         *
         * \return Returns True if the region instance is valid.  Returns false if the region instance is invalid.
         */
        inline bool isValid() const {
            return currentRegionId != invalidRegionId;
        }

        /**
         * Method you can use to determine if this region instance is invalid.
         *
         * \return Returns True if the region instance is invalid.  Returns false if the region instance is valid.
         */
        inline bool isInvalid() const {
            return !isValid();
        }

        /**
         * Method you can use to obtain the region ID.
         *
         * \return Returns the region ID for this region.
         */
        inline RegionId regionId() const {
            return currentRegionId;
        }

        /**
         * Method you can use to obtain the region descriptive text.
         *
         * \return Returns the region name.
         */
        inline const QString& regionName() const {
            return currentRegionName;
        }

        /**
         * Method you can use to change the stored region name.
         *
         * \param[in[ newRegionName The new name for this region.
         */
        inline void setRegionName(const QString& newRegionName) {
            currentRegionName = newRegionName;
        }

        /**
         * Assignment operator.
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        inline Region& operator=(const Region& other) {
            currentRegionId   = other.currentRegionId;
            currentRegionName = other.currentRegionName;

            return *this;
        }

        /**
         * Assignment operator (move semantics)
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        inline Region& operator=(Region&& other) {
            currentRegionId   = other.currentRegionId;
            currentRegionName = other.currentRegionName;

            return *this;
        }

    private:
        /**
         * The current region ID.
         */
        RegionId currentRegionId;

        /**
         * The current region name.
         */
        QString currentRegionName;
};

#endif
