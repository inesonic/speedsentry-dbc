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
* This header defines the \ref Server class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef SERVER_H
#define SERVER_H

#include <QString>

#include <cstdint>

#include "region.h"

class Servers;

/**
 * Trivial class used to hold information about a specific server.
 */
class Server {
    friend class Servers;

    public:
        /**
         * Value used to represent a server ID.
         */
        typedef std::uint16_t ServerId;

        /**
         * Value used to indicate an invalid server ID.
         */
        static const ServerId invalidServerId;

        /**
         * Enumeration of supported server status codes.
         */
        enum class Status : std::uint8_t {
            /**
             * Indicates all or unknown status.  This value will never be used by a server and is used to either
             * indicate we want all possible status or that the server's status has not yet been determined.
             */
            ALL_UNKNOWN = 0,

            /**
             * Indicates the server is active.
             */
            ACTIVE = 1,

            /**
             * Indicates the server is inactive.
             */
            INACTIVE = 2,

            /**
             * Indicates the server is defunct and no longer exists.
             */
            DEFUNCT = 3,

            /**
             * Indicates the number of server status values.
             */
            NUMBER_VALUES
        };

    private:
        /**
         * Constructor.
         *
         * \param[in] serverId          The ID used to identif this server.  This value will be unique for every
         *                              server.
         *
         * \param[in] regionId          The ID used to identify the region where this server resides.
         *
         * \param[in] identifier        A string identifier for this server.
         *
         * \param[in] status            The reported server status.
         *
         * \param[in] monitorsPerSecond The number of monitors that this server is processing.
         *
         * \param[in] cpuLoading        The CPU loading last reported by the server.
         *
         * \param[in] memoryLoading     The memory utilization last reported by the server.
         */
        inline Server(
                ServerId         serverId,
                Region::RegionId regionId,
                const QString&   identifier,
                Status           status,
                float            monitorsPerSecond,
                float            cpuLoading,
                float            memoryLoading
            ):currentServerId(
                serverId
            ),currentRegionId(
                regionId
            ),currentIdentifier(
                identifier
            ),currentStatus(
                status
            ),currentMonitorsPerSecond(
                monitorsPerSecond
            ),currentCpuLoading(
                cpuLoading
            ),currentMemoryLoading(
                memoryLoading
            ) {}

    public:
        inline Server() {
            currentServerId          = invalidServerId;
            currentRegionId          = Region::invalidRegionId;
            currentStatus            = Status::DEFUNCT;
            currentMonitorsPerSecond = 0;
            currentCpuLoading        = 0;
            currentMemoryLoading     = 0;
        }

        /**
         * Copy constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline Server(
                const Server& other
            ):currentServerId(
                other.currentServerId
            ),currentRegionId(
                other.currentRegionId
            ),currentIdentifier(
                other.currentIdentifier
            ),currentStatus(
                other.currentStatus
            ),currentMonitorsPerSecond(
                other.currentMonitorsPerSecond
            ),currentCpuLoading(
                other.currentCpuLoading
            ),currentMemoryLoading(
                other.currentMemoryLoading
            ) {}

        /**
         * Move constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline Server(
                Server&& other
            ):currentServerId(
                other.currentServerId
            ),currentRegionId(
                other.currentRegionId
            ),currentIdentifier(
                other.currentIdentifier
            ),currentStatus(
                other.currentStatus
            ),currentMonitorsPerSecond(
                other.currentMonitorsPerSecond
            ),currentCpuLoading(
                other.currentCpuLoading
            ),currentMemoryLoading(
                other.currentMemoryLoading
            ) {}

        ~Server() = default;

        /**
         * Method you can use to determine if this server instance is valid.
         *
         * \return Returns True if the server instance is valid.  Returns false if the server instance is invalid.
         */
        inline bool isValid() const {
            return currentServerId != invalidServerId;
        }

        /**
         * Method you can use to determine if this server instance is invalid.
         *
         * \return Returns True if the server instance is invalid.  Returns false if the server instance is valid.
         */
        inline bool isInvalid() const {
            return !isValid();
        }

        /**
         * Method you can use to get the server ID.
         *
         * \return Returns the server ID.
         */
        inline ServerId serverId() const {
            return currentServerId;
        }

        /**
         * Method you can use to get the region where this server resides.
         *
         * \return Returns the server's region ID.
         */
        inline Region::RegionId regionId() const {
            return currentRegionId;
        }

        /**
         * Method you can use to change the server's region ID.
         *
         * \param[in] newRegionId The new region ID.
         */
        inline void setRegionId(Region::RegionId newRegionId) {
            currentRegionId = newRegionId;
        }

        /**
         * Method you can use to get the server's identifier string.
         *
         * \return Returns the server's identifier string.
         */
        inline const QString& identifier() const {
            return currentIdentifier;
        }

        /**
         * Method you can use to change the server's identifier string.
         *
         * \param[in] newIdentifier The new server identifier string.
         */
        inline void setIdentifier(const QString& newIdentifier) {
            currentIdentifier = newIdentifier;
        }

        /**
         * Method you can use to get the server's last known status.
         *
         * \return Returns the server's last known status.
         */
        inline Status status() const {
            return currentStatus;
        }

        /**
         * Method you can use to change the server's status.
         *
         * \param[in] newStatus The new server status.
         */
        inline void setStatus(Status newStatus) {
            currentStatus = newStatus;
        }

        /**
         * Method you can use to get the server's last reported monitor service rate.
         *
         * \return Returns the server's last reported monitor service rate.
         */
        inline float monitorsPerSecond() const {
            return currentMonitorsPerSecond;
        }

        /**
         * Method you can use to set the server's current monitor service rate.
         *
         * \param[in] newMonitorsPerSecond The new server monitor service rate.
         */
        inline void setMonitorsPerSecond(float newMonitorsPerSecond) {
            currentMonitorsPerSecond = newMonitorsPerSecond;
        }

        /**
         * Method you can use to get the server's last reported CPU loading.
         *
         * \return Returns the server's last reported CPU loading.
         */
        inline float cpuLoading() const {
            return currentCpuLoading;
        }

        /**
         * Method you can use to set the server's reported CPU loading.
         *
         * \param[in] newCpuLoading The new CPU loading reported by the server.
         */
        inline void setCpuLoading(float newCpuLoading) {
            currentCpuLoading = newCpuLoading;
        }

        /**
         * Method you can use to get the server's last reported memory loading.
         *
         * \return Returns the server's last reported memory loading.
         */
        inline float memoryLoading() const {
            return currentMemoryLoading;
        }

        /**
         * Method you can use to set the server's reported memory loading.
         *
         * \param[in] newMemoryLoading The new server memory loading.
         */
        inline void setMemoryLoading(float newMemoryLoading) {
            currentMemoryLoading = newMemoryLoading;
        }

        /**
         * Assignment operator.
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        inline Server& operator=(const Server& other) {
            currentServerId          = other.currentServerId;
            currentRegionId          = other.currentRegionId;
            currentIdentifier        = other.currentIdentifier;
            currentStatus            = other.currentStatus;
            currentMonitorsPerSecond = other.currentMonitorsPerSecond;
            currentCpuLoading        = other.currentCpuLoading;
            currentMemoryLoading     = other.currentMemoryLoading;

            return *this;
        }

        /**
         * Assignment operator (move semantics).
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        inline Server& operator=(Server&& other) {
            currentServerId          = other.currentServerId;
            currentRegionId          = other.currentRegionId;
            currentIdentifier        = other.currentIdentifier;
            currentStatus            = other.currentStatus;
            currentMonitorsPerSecond = other.currentMonitorsPerSecond;
            currentCpuLoading        = other.currentCpuLoading;
            currentMemoryLoading     = other.currentMemoryLoading;

            return *this;
        }

        /**
         * Method that converts a server status value to a string.
         *
         * \param[in] status  The status value to convert.
         *
         * \return Returns the status value converted to an upper case string.
         */
        static QString toString(Status status);

        /**
         * Method that converts a string to a server status value.
         *
         * \param[in[ str     The string to be converted.  Value can be upper or lower case.
         *
         * \param[in] success An optional pointer to a boolean value what will be set to true on success or false on
         *                    error.
         */
        static Status toStatus(const QString& str, bool* success = nullptr);

    private:
        /**
         * The current server ID of this server.
         */
        ServerId currentServerId;

        /**
         * The current region ID for the region where this server resides.
         */
        Region::RegionId currentRegionId;

        /**
         * The current server identifier string.
         */
        QString currentIdentifier;

        /**
         * The current server status.
         */
        Status currentStatus;

        /**
         * The last reported monitor service rate.
         */
        float currentMonitorsPerSecond;

        /**
         * The last reported server CPU loading.
         */
        float currentCpuLoading;

        /**
         * The last reported server memory loading.
         */
        float currentMemoryLoading;
};

#endif
