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
* This header defines the \ref LatencyInterfaceManager class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef LATENCY_INTERFACE_MANAGER_H
#define LATENCY_INTERFACE_MANAGER_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QPair>
#include <QSqlDatabase>

#include <cstdint>
#include <limits>

#include "customer_capabilities.h"
#include "customers_capabilities.h"
#include "region.h"
#include "server.h"
#include "short_latency_entry.h"
#include "latency_entry.h"
#include "latency_interface.h"
#include "aggregated_latency_entry.h"

class QTimer;
class DatabaseManager;
class LatencyAggregator;

/**
 * Class used to manage a collection of latency interface classes.  This class exists to allow for greater
 * parallelism while writing to our database backend.
 */
class LatencyInterfaceManager:public QObject {
    Q_OBJECT

    public:
        /**
         * Type used to represent a specific region by region ID.
         */
        typedef std::uint16_t RegionId;

        /**
         * Type used to track a list of short latency entries.
         */
        typedef LatencyInterface::LatencyEntryList LatencyEntryList;

        /**
         * List of aggregated latency entries.
         */
        typedef QList<AggregatedLatencyEntry> AggregatedLatencyEntryList;

        /**
         * Type used to return a combination of recent and aggregated latency entries.
         */
        typedef QPair<LatencyEntryList, AggregatedLatencyEntryList> LatencyEntryLists;

        /**
         * Constructor
         *
         * \param[in] databaseManager The database manager class we use to create database instances.
         *
         * \param[in] parent          Pointer to the parent object.
         */
        LatencyInterfaceManager(DatabaseManager* databaseManager, QObject* parent = nullptr);

        ~LatencyInterfaceManager() override;

        /**
         * Method you can use to obtain a pointer to a data interface by region ID.
         *
         * \param[in] regionId The region ID of the region we want a data interface for.
         */
        LatencyInterface* getLatencyInterface(RegionId regionId);

        /**
         * Method you can use to obtain latency information by customer, by region, by server, or by monitor.
         *
         * \param[in] customerId     The ID of the customer requesting this data.  An invalid customer ID indicates all
         *                           customers.
         *
         * \param[in] hostSchemeId   The host/scheme ID of the host scheme we wish latency information for.  An invalid
         *                           host/scheme ID indicates the monitor ID should be used.
         *
         * \param[in] monitorId      The monitor ID of the monitor we wish latency information for.  An invalid monitor
         *                           ID indicates all monitors.
         *
         * \param[in] regionId       The region ID of the desired region.  An invalid region ID means all regions.
         *
         * \param[in] serverId       The server ID of the server we want latency data from.  An invalid server ID
         *                           indicates all servers.
         *
         * \param[in] startTimestamp The starting timestamp (inclusive) that we want information for.
         *
         * \param[in] endTimestamp   The ending timestamp (inclusive) that we want information for.
         *
         * \param[in] threadId       The optional thread ID of the thread we're operating under.
         *
         * \return Returns a pair of latency entries holding regular and aggregated data.
         */
        LatencyEntryLists getLatencyEntries(
            CustomerCapabilities::CustomerId customerId,
            HostScheme::HostSchemeId         hostSchemeId,
            LatencyEntry::MonitorId          monitorId,
            Region::RegionId                 regionId,
            Server::ServerId                 serverId,
            unsigned long long               startTimestamp = 0,
            unsigned long long               endTimestamp = std::numeric_limits<unsigned long long>::max(),
            unsigned                         threadId = 0
        );

        /**
         * Method you can use to obtain statistics about latency over a given time period.
         *
         * \param[in] customerId     The ID of the customer requesting this data.  An invalid customer ID indicates all
         *                           customers.
         *
         * \param[in] hostSchemeId   The host/scheme ID of the host scheme we wish latency information for.  An invalid
         *                           host/scheme ID indicates the monitor ID should be used.
         *
         * \param[in] monitorId      The monitor ID of the monitor we wish latency information for.  An invalid monitor
         *                           ID indicates all monitors.
         *
         * \param[in] regionId       The region ID of the desired region.  An invalid region ID means all regions.
         *
         * \param[in] serverId       The server ID of the server we want latency data from.  An invalid server ID
         *                           indicates all servers.
         *
         * \param[in] startTimestamp The starting timestamp (inclusive) that we want information for.
         *
         * \param[in] endTimestamp   The ending timestamp (inclusive) that we want information for.
         *
         * \param[in] threadId       The optional thread ID of the thread we're operating under.
         *
         * \return Returns an aggregated latency entry holding the captured statistics.
         */
        AggregatedLatencyEntry getLatencyStatistics(
            CustomerCapabilities::CustomerId customerId,
            HostScheme::HostSchemeId         hostSchemeId,
            LatencyEntry::MonitorId          monitorId,
            Region::RegionId                 regionId,
            Server::ServerId                 serverId,
            unsigned long long               startTimestamp = 0,
            unsigned long long               endTimestamp = std::numeric_limits<unsigned long long>::max(),
            unsigned                         threadId = 0
        );

    public slots:
        /**
         * Slot you can trigger to add a new entry for a cutomer.
         *
         * \param[in] regionId           The region ID of the region we want a data interface for.
         *
         * \param[in] monitorId          The ID of the monitor tied to this entry.
         *
         * \param[in] serverId           The ID of the region server where this measurement was taken.
         *
         * \param[in] zoranTimestamp     The timestamp relative to the start of the Zoran epoch.
         *
         * \param[in] latencyMillisconds The latency measurement, in microseconds.
         */
        void addEntry(
            RegionId                          regionId,
            LatencyEntry::MonitorId           monitorId,
            LatencyEntry::ServerId            serverId,
            LatencyEntry::ZoranTimeStamp      zoranTimestamp,
            LatencyEntry::LatencyMicroseconds latencyMicroseconds
        );

        /**
         * Slot you can trigger to add a new data entry for a customer.
         *
         * \param[in] regionId     The region ID of the region we want a data interface for.
         *
         * \param[in] latencyEntry The latency entry to be added.
         */
        void addEntry(RegionId regionId, const LatencyEntry& latencyEntry);

        /**
         * Slot you can trigger to add a collection of new data entries for a customer.
         *
         * \param[in] regionId       The region ID of the region we want a data interface for.
         *
         * \param[in] latencyEntries The latency entries to be added.
         */
        void addEntries(RegionId regionId, const LatencyInterface::LatencyEntryList& latencyEntries);

        /**
         * Slot you can trigger to delete latency entries for a user.
         *
         * \param[in] customerIds The customer IDs of the users to have entries deleted for.
         *
         * \param[in] threadId    The ID of the thread to be used to delete these entries.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool deleteByCustomerId(const CustomersCapabilities::CustomerIdSet& customerIds, unsigned threadId);

        /**
         * Method you can use to set the input and output table and table type.
         *
         * \param[in] inputTableMaximumAge   The maximum age of input entries, in seconds.
         *
         * \param[in] resamplePeriod         The period to run this aggregator, in seconds.
         *
         * \param[in] expungePeriod          The maximum age for any entry before being expunged.
         *
         * \param[in] inputAlreadyAggregated If true, then the input table will contain additional entries for mean and
         *                                   variance.
         */
        void setParameters(
            unsigned long  inputTableMaximumAge,
            unsigned long  resamplePeriod,
            unsigned long  expungePeriod,
            bool           inputAggregated
        );

    private:
        /**
         * Method that gets raw latency entries.
         *
         * \param[out]    success        Flag holding true on exit if successful.
         *
         * \param[in,out] database       The database instance to be used.
         *
         * \param[in]     customerId     The ID of the customer requesting this data.  An invalid customer ID indicates
         *                               all customers.
         *
         * \param[in]     hostSchemeId   The host/scheme ID of the host scheme we wish latency information for.  An
         *                               invalid host/scheme ID indicates the monitor ID should be used.
         *
         * \param[in]     monitorId      The monitor ID of the monitor we wish latency information for.  An invalid
         *                               monitor ID indicates all monitors.
         *
         * \param[in]     regionId       The region ID of the desired region.  An invalid region ID means all regions.
         *
         * \param[in]     serverId       The server ID of the server we want latency data from.  An invalid server ID
         *                               indicates all servers.
         *
         * \param[in]     startTimestamp The starting timestamp (inclusive) that we want information for.
         *
         * \param[in]     endTimestamp   The ending timestamp (inclusive) that we want information for.
         *
         * \return Returns a list of captured \ref LatencyEntry instances.
         */
        LatencyEntryList getRawEntries(
            bool&                            success,
            QSqlDatabase&                    database,
            CustomerCapabilities::CustomerId customerId,
            HostScheme::HostSchemeId         hostSchemeId,
            LatencyEntry::MonitorId          monitorId,
            Region::RegionId                 regionId,
            Server::ServerId                 serverId,
            unsigned long long               startTimestamp,
            unsigned long long               endTimestamp
        );

        /**
         * Method that gets aggregated latency entries.
         *
         * \param[out]    success        Flag holding true on exit if successful.
         *
         * \param[in,out] database       The database instance to be used.
         *
         * \param[in]     customerId     The ID of the customer requesting this data.  An invalid customer ID indicates
         *                               all customers.
         *
         * \param[in]     hostSchemeId   The host/scheme ID of the host scheme we wish latency information for.  An
         *                               invalid host/scheme ID indicates the monitor ID should be used.
         *
         * \param[in]     monitorId      The monitor ID of the monitor we wish latency information for.  An invalid
         *                               monitor ID indicates all monitors.
         *
         * \param[in]     regionId       The region ID of the desired region.  An invalid region ID means all regions.
         *
         * \param[in]     serverId       The server ID of the server we want latency data from.  An invalid server ID
         *                               indicates all servers.
         *
         * \param[in]     startTimestamp The starting timestamp (inclusive) that we want information for.
         *
         * \param[in]     endTimestamp   The ending timestamp (inclusive) that we want information for.
         *
         * \return Returns a list of captured \ref LatencyEntry instances.
         */
        AggregatedLatencyEntryList getAggregatedEntries(
            bool&                            success,
            QSqlDatabase&                    database,
            CustomerCapabilities::CustomerId customerId,
            HostScheme::HostSchemeId         hostSchemeId,
            LatencyEntry::MonitorId          monitorId,
            Region::RegionId                 regionId,
            Server::ServerId                 serverId,
            unsigned long long               startTimestamp,
            unsigned long long               endTimestamp
        );

        /**
         * Method that gets raw latency entry statistics.
         *
         * \param[out]    success        Flag holding true on exit if successful.
         *
         * \param[in,out] database       The database instance to be used.
         *
         * \param[in]     customerId     The ID of the customer requesting this data.  An invalid customer ID indicates
         *                               all customers.
         *
         * \param[in]     hostSchemeId   The host/scheme ID of the host scheme we wish latency information for.  An
         *                               invalid host/scheme ID indicates the monitor ID should be used.
         *
         * \param[in]     monitorId      The monitor ID of the monitor we wish latency information for.  An invalid
         *                               monitor ID indicates all monitors.
         *
         * \param[in]     regionId       The region ID of the desired region.  An invalid region ID means all regions.
         *
         * \param[in]     serverId       The server ID of the server we want latency data from.  An invalid server ID
         *                               indicates all servers.
         *
         * \param[in]     startTimestamp The starting timestamp (inclusive) that we want information for.
         *
         * \param[in]     endTimestamp   The ending timestamp (inclusive) that we want information for.
         *
         * \return Returns a list of captured \ref LatencyEntry instances.
         */
        AggregatedLatencyEntry getRawEntryStatistics(
            bool&                            success,
            QSqlDatabase&                    database,
            CustomerCapabilities::CustomerId customerId,
            HostScheme::HostSchemeId         hostSchemeId,
            LatencyEntry::MonitorId          monitorId,
            Region::RegionId                 regionId,
            Server::ServerId                 serverId,
            unsigned long long               startTimestamp,
            unsigned long long               endTimestamp
        );

        /**
         * Method that builds a select query based on a set of constraints.
         *
         * \param[in] tableName      The name of the table to build the query for.
         *
         * \param[in] customerId     The ID of the customer requesting this data.  An invalid customer ID indicates all
         *                           customers.
         *
         * \param[in] hostSchemeId   The host/scheme ID of the host scheme we wish latency information for.  An invalid
         *                           host/scheme ID indicates the monitor ID should be used.
         *
         * \param[in] monitorId      The monitor ID of the monitor we wish latency information for.  An invalid monitor
         *                           ID indicates all monitors.
         *
         * \param[in] regionId       The region ID of the desired region.  An invalid region ID means all regions.
         *
         * \param[in] serverId       The server ID of the server we want latency data from.  An invalid server ID
         *                           indicates all servers.
         *
         * \param[in] startTimestamp The starting timestamp (inclusive) that we want information for.
         *
         * \param[in] endTimestamp   The ending timestamp (inclusive) that we want information for.
         *
         * \param[in] selectClause   The select clause.
         */
        QString buildQueryString(
            const QString&                   tableName,
            CustomerCapabilities::CustomerId customerId,
            HostScheme::HostSchemeId         hostSchemeId,
            LatencyEntry::MonitorId          monitorId,
            Region::RegionId                 regionId,
            Server::ServerId                 serverId,
            unsigned long long               startTimestamp,
            unsigned long long               endTimestamp,
            const QString&                   selectClause = QString("*")
        );

        /**
         * Method that converts a Unix timestamp to a Zoran timestamp with capping.
         *
         * \param[in] unixTimestamp The Unix timestamp to be converted.
         *
         * \return Returns the Zoran timestamp.
         */
        LatencyEntry::ZoranTimeStamp toZoranTimestamp(unsigned long long unixTimestamp);

        /**
         * The underlying database manager.
         */
        DatabaseManager* currentDatabaseManager;

        /**
         * The latency data aggregator.
         */
        LatencyAggregator* currentLatencyAggregator;

        /**
         * Mutex used to control access to the data interface by region table.
         */
        QMutex accessMutex;

        /**
         * Table of data interfaces by region.
         */
        QHash<RegionId, LatencyInterface*> dataInterfacesByRegion;
};

#endif
