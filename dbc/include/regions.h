/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
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
