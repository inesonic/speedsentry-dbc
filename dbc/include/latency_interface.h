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
* This header defines the \ref LatencyInterface class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef LATENCY_INTERFACE_H
#define LATENCY_INTERFACE_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QHash>
#include <QList>
#include <QMutex>

#include <cstdint>

#include "customer_capabilities.h"
#include "monitor.h"
#include "server.h"
#include "short_latency_entry.h"
#include "latency_entry.h"

class QTimer;
class DatabaseManager;

/**
 * Class used to cache customer data and flush data to the database in bulk.  You can also query data entries by
 * customer ID.
 *
 * This class assumes we have a table defined as:
 *
 *     CREATE TABLE latency_seconds (
 *         monitor_id  INTEGER  UNSIGNED NOT NULL,
 *         server_id   SMALLINT UNSIGNED NOT NULL,
 *         timestamp   INTEGER  UNSIGNED NOT NULL,
 *         latency     INTEGER  UNSIGNED NOT NULL,
 *         KEY latency_seconds_constraint_1 (monitor_id),
 *         CONSTRAINT latency_seconds_constraint_1 FOREIGN KEY (monitor_id)
 *             REFERENCES monitor (monitor_id)
 *             ON DELETE CASCADE
 *             ON UPDATE NO ACTION
 *         KEY latency_seconds_constraint_2 (server_id),
 *         CONSTRAINT latency_seconds_constraint_2 FOREIGN KEY (server_id)
 *             REFERENCES servers (server_id)
 *             ON DELETE CASCADE
 *             ON UPDATE NO ACTION
 *     ) ENGINE=InnoDB DEFAULT CHARSET=latin1;
 */
class LatencyInterface:public QThread {
    Q_OBJECT

    public:
        /**
         * Type used to represent a monitor ID.
         */
        typedef LatencyEntry::MonitorId MonitorId;

        /**
         * Type used to represent a Zoran timestamp.
         */
        typedef LatencyEntry::ZoranTimeStamp ZoranTimeStamp;

        /**
         * Value used to represent a server ID.
         */
        typedef LatencyEntry::ServerId ServerId;

        /**
         * Value used to represent a latency, in microseconds.
         */
        typedef LatencyEntry::LatencyMicroseconds LatencyMicroseconds;

        /**
         * Type used to track a list of short latency entries.
         */
        typedef QList<LatencyEntry> LatencyEntryList;

        /**
         * Constructor
         *
         * \param[in] databaseManager The database manager tracking customer data.
         *
         * \param[in] connectionId    An integer value used to mange the database connection unique.
         *
         * \param[in] parent          Pointer to the parent object.
         */
        LatencyInterface(DatabaseManager* databaseManager, unsigned connectionId, QObject* parent = nullptr);

        ~LatencyInterface() override;

    public slots:
        /**
         * Slot you can trigger to add a new entry for a cutomer.
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
            MonitorId           monitorId,
            ServerId            serverId,
            ZoranTimeStamp      zoranTimestamp,
            LatencyMicroseconds latencyMicroseconds
        );

        /**
         * Slot you can trigger to add a new data entry for a customer.
         *
         * \param[in] latencyEntry The latency entry to be added.
         */
        void addEntry(const LatencyEntry& latencyEntry);

        /**
         * Slot you can trigger to add a collection of new data entries for a customer.
         *
         * \param[in] latencyEntries The latency entries to be added.
         */
        void addEntries(const LatencyEntryList& latencyEntries);

        /**
         * Method that starts this thread if it's not actively running.
         */
        void receivedEntries();

    protected:
        /**
         * Method that runs this thread.
         */
        void run() override;

    private:
        /**
         * Method that is called to flush entries.
         */
        void performFlush();

        /**
         * Time to sleep while the incoming data queue is empty, in seconds.
         */
        static const unsigned queueCheckInterval;

        /**
         * Number of queue check cycles before forcing a flush.
         */
        static const unsigned numberCyclesBeforeForcedCommit;

        /**
         * The ideal maximum number of entries we allow before we trigger a flush.
         */
        static const unsigned long maximumNumberCachedEntries;

        /**
         * The maximum number of rows per database transaction.
         */
        static const unsigned long maximumRowsPerTransaction;

        /**
         * The interval to wait before retrying a database operation.
         */
        static const unsigned retryIntervalMilliseconds;

        /**
         * Mutex used to gate changes to the incoming entries.
         */
        QMutex incomingEntriesMutex;

        /**
         * Table of incoming cached entries by slug and customer.
         */
        LatencyEntryList* currentIncomingEntries;

        /**
         * Table of in process cached entries by slug and customer.
         */
        LatencyEntryList* currentInProcessEntries;

        /**
         * The current database manager.
         */
        DatabaseManager* currentDatabaseManager;

        /**
         * The unique connection identifier for this connection.
         */
        unsigned currentConnectionId;

        /**
         * Flag that is set to indicate that we should shutdown the background thread.
         */
        bool shutdownRequested;
};

#endif
