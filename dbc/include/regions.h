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
* This header defines the \ref Regions class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef REGIONS_H
#define REGIONS_H

#include <QObject>
#include <QString>
#include <QHash>

#include <cstdint>

#include "region.h"
#include "sql_helpers.h"

class QTimer;
class DatabaseManager;

/**
 * Class used to read and write information about a region.  THis class expects a table named "regions" with the
 * following definition:
 *
 *     CREATE TABLE regions (
 *         region_id SMALLINT NOT NULL AUTO_INCREMENT,
 *         region_name VARCHAR(64) NOT NULL,
 *         PRIMARY KEY (region_id)
 *     ) ENGINE=InnoDB DEFAULT CHARSET=latin1
 */
class Regions:public QObject, private SqlHelpers {
    Q_OBJECT

    public:
        /**
         * Type used to represent a region ID.
         */
        typedef Region::RegionId RegionId;

        /**
         * Type used to represent a collection of regions.
         */
        typedef QHash<RegionId, Region> RegionHash;

        /**
         * Constructor
         *
         * \param[in] databaseManager The database manager used to fetch information about a region.
         *
         * \param[in] parent          Pointer to the parent object.
         */
        Regions(DatabaseManager* databaseManager, QObject* parent = nullptr);

        ~Regions() override;

        /**
         * Method you can use to get a region by region ID.
         *
         * \param[in] regionId The region ID of the desired region.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the \ref Region instance for the region.  An invalid region is returned if the region ID
         *         isn't valid.
         */
        Region getRegion(RegionId regionId, unsigned threadId = 0) const;

        /**
         * Method you can use to add a new region to the database.
         *
         * \param[in] regionName A descriptive text for the new region.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the newly created \ref Region instance.  An invalid region is returned if the region text
         *         already exists in the database.
         */
        Region createRegion(const QString& regionName, unsigned threadId = 0);

        /**
         * Method you can use to update a region stored in the database.
         *
         * \param[in] region   The modified region instance.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool modifyRegion(const Region& region, unsigned threadId = 0);

        /**
         * Method you can use to delete a region from the database.
         *
         * \param[in] region The region to be deleted.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool deleteRegion(const Region& region, unsigned threadId = 0);

        /**
         * Method you can use to obtain all the known regions.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         */
        RegionHash getAllRegions(unsigned threadId = 0);

    private:
        /**
         * The underlying database manager instance.
         */
        DatabaseManager* currentDatabaseManager;
};

#endif
