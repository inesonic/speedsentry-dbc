/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref LatencyEntry class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef LATENCY_ENTRY_H
#define LATENCY_ENTRY_H

#include <cstdint>

#include "customer_capabilities.h"
#include "monitor.h"
#include "server.h"
#include "short_latency_entry.h"

/**
 * Class used to track a single latency entry.
 */
class LatencyEntry:public ShortLatencyEntry {
    public:
        /**
         * Type used to represent a monitor index.
         */
        typedef Monitor::MonitorId MonitorId;

        /**
         * Type used to represent a server code.
         */
        typedef Server::ServerId ServerId;

        constexpr LatencyEntry():
            currentMonitorId(0),
            currentServerId(0) {}

        /**
         * Constructor
         *
         * \param[in] monitorId          The ID of the monitor tied to this entry.
         *
         * \param[in] serverId           The ID of the region server where this measurement was taken.
         *
         * \param[in] zoranTimestamp     The timestamp relative to the start of the Zoran epoch.
         *
         * \param[in] latencyMillisconds The latency measurement, in microseconds.
         */
        constexpr LatencyEntry(
                MonitorId           monitorId,
                ServerId            serverId,
                ZoranTimeStamp      zoranTimestamp,
                LatencyMicroseconds latencyMicroseconds
            ):ShortLatencyEntry(
                zoranTimestamp,
                latencyMicroseconds
            ),currentMonitorId(
                monitorId
            ),currentServerId(
                serverId
            ) {}

        /**
         * Copy constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        constexpr LatencyEntry(
                const LatencyEntry& other
            ):ShortLatencyEntry(
                other
            ),currentMonitorId(
                other.currentMonitorId
            ),currentServerId(
                other.currentServerId
            ) {}

        ~LatencyEntry() = default;

        /**
         * Method you can use to obtain the monitor ID for this monitor.
         *
         * \return Returns the slug ID.
         */
        inline MonitorId monitorId() const {
            return currentMonitorId;
        }

        /**
         * Method you can use to obtain the server ID.
         *
         * \return Returns the server ID of the server that took this measurement.
         */
        inline ServerId serverId() const {
            return currentServerId;
        }

        /**
         * Assignment operator.
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        inline LatencyEntry& operator=(const LatencyEntry& other) {
            ShortLatencyEntry::operator=(other);

            currentMonitorId = other.currentMonitorId;
            currentServerId  = other.currentServerId;

            return *this;
        }

    private:
        /**
         * The monitor ID of the monitor tied to this entry.
         */
        MonitorId currentMonitorId;

        /**
         * The server ID of the server that recorded this measurement.
         */
        ServerId currentServerId;
};

#endif
