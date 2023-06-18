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
* This header defines the \ref RestHelpers class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef REST_HELPERS_H
#define REST_HELPERS_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "host_scheme.h"
#include "host_schemes.h"
#include "monitor.h"
#include "monitors.h"
#include "server.h"
#include "servers.h"
#include "customer_capabilities.h"
#include "event.h"
#include "events.h"
#include "latency_entry.h"
#include "aggregated_latency_entry.h"
#include "latency_interface_manager.h"
#include "monitor_updater.h"
#include "resource.h"
#include "resources.h"

/**
 * Class that provides a handful of helper method to parse REST requests and generate responses.
 */
class RestHelpers {
    public:
        /**
         * Method used to convert a host/scheme to a JSON object.
         *
         * \param[in] hostScheme        The host/scheme to convert to JSON.
         *
         * \param[in] includeCustomerId If true, the customer ID will be included.  If false, the customer ID will be
         *                              excluded.
         *
         * \return Returns the monitor object converted to JSON.
         */
        static QJsonObject convertToJson(const HostScheme& hostScheme, bool includeCustomerId);

        /**
         * Method used to convert a host/scheme to a JSON object.
         *
         * \param[in] hostSchemes       The host/schemes to convert to JSON.
         *
         * \param[in] includeCustomerId If true, the customer ID will be included.  If false, the customer ID will be
         *                              excluded.
         *
         * \return Returns the monitor object converted to JSON.
         */
        static QJsonObject convertToJson(const HostSchemes::HostSchemeHash& hostSchemes, bool includeCustomerId);

        /**
         * Method used to convert a monitor to a JSON object.
         *
         * \param[in] monitor             The monitor to be converted.
         *
         * \param[in] includeCustomerId   If true, the customer ID will be included.  If false, the customer Id will
         *                                be excluded.
         *
         * \param[in] includeUserOrdering If true, then user ordering will be included in the output.
         *
         * \return Returns the monitor object converted to JSON.
         */
        static QJsonObject convertToJson(const Monitor& monitor, bool includeCustomerId, bool includeUserOrdering);

        /**
         * Method used to convert a monitor to a JSON object.
         *
         * \param[in] monitor             The monitor to be converted.
         *
         * \param[in] url                 A URL field to be included.
         *
         * \param[in] includeCustomerId   If true, the customer ID will be included.  If false, the customer Id will
         *                                be excluded.
         *
         * \param[in] includeUserOrdering If true, then user ordering will be included in the output.
         *
         * \return Returns the monitor object converted to JSON.
         */
        static QJsonObject convertToJson(
            const Monitor& monitor,
            const QUrl&    url,
            bool           includeCustomerId,
            bool           includeUserOrdering
        );

        /**
         * Method used to convert a monitor to a JSON object.
         *
         * \param[in] monitor           The monitor to be converted.
         *
         * \param[in] includeCustomerId If true, the customer ID will be included.  If false, the customer Id will
         *                              be excluded.
         *
         * \return Returns the monitor object converted to JSON.
         */
        static QJsonObject convertToJson(const Monitors::MonitorList& monitor, bool includeCustomerId);

        /**
         * Method that parses a JSON object into a list of monitor update entries.
         *
         * \param[out] errors      A list of generated error messages.
         *
         * \param[in]  monitorData The JSON monitor data to be parsed.
         *
         * \param[out] ok          A pointer to an optional boolean value holding true on success or false on error.
         *
         * \return returns the generated list of monitor entries.  An empty list will be returned on error.
         */
        static MonitorUpdater::MonitorEntries convertToMonitorEntries(
            MonitorUpdater::Errors& errors,
            const QJsonArray&       monitorData
        );

        /**
         * Method that parses a JSON object into a list of monitor update entries.
         *
         * \param[out] errors      A list of generated error messages.
         *
         * \param[in]  monitorData The JSON monitor data to be parsed.
         *
         * \param[out] ok          A pointer to an optional boolean value holding true on success or false on error.
         *
         * \return returns the generated list of monitor entries.  An empty list will be returned on error.
         */
        static MonitorUpdater::MonitorEntries convertToMonitorEntries(
            MonitorUpdater::Errors& errors,
            const QJsonObject&      monitorData
        );

        /**
         * Method that parses a JSON object into a single monitor update entry.
         *
         * \param[out] errors       A list of generated error messages.
         *
         * \param[in]  monitorData  The JSON monitor data to be parsed.
         *
         * \param[in]  userOrdering The user ordering used to identify this entry.
         *
         * \param[out] ok           A pointer to an optional boolean value holding true on success or false on error.
         *
         * \return returns the generated list of monitor entries.  An empty list will be returned on error.
         */
        static MonitorUpdater::Entry convertToMonitorEntry(
            MonitorUpdater::Errors&      errors,
            const QJsonObject&           monitorData,
            MonitorUpdater::UserOrdering userOrdering,
            bool*                        ok = nullptr
        );

        /**
         * Method that converts an errors list to a JSON response.
         *
         * \param[in] errors A list of \ref MonitorUpdater::Error instances.
         *
         * \return returns a QJSonObject instance holding the response.
         */
        static QJsonObject convertToJson(const MonitorUpdater::Errors& error);

        /**
         * Method that converts converts customer capabilities to a JSON response.
         *
         * \param[in] customerCapabilities         The customer capabilities to convert.
         *
         * \param[in] includeCustomerId            If true, then the customer_id field will be included.
         *
         * \param[in] includeBlacklistField        If true, then the blacklist field will be included.
         *
         * \param[in] includeDomainExpirationField If true, then the domain expiration field will be included.
         *
         * \return returns a QJSonObject instance holding the response.
         */
        static QJsonObject convertToJson(
            const CustomerCapabilities& customerCapabilities,
            bool                        includeCustomerId,
            bool                        includeBlacklistField,
            bool                        includeDomainExpirationField
        );

        /**
         * Method that converts an event to a JSON response.
         *
         * \param[in] event             The event to be converted.
         *
         * \param[in] includeCustomerId If true, the customer ID will be included in the response.  If false, the
         *                              customer ID will be excluded from the response.
         *
         * \param[in] includeHash       If true, the event hash will be included.  If false, the event hash will be
         *                              excluded.
         *
         * \return Returns a QJsonObject instance holding the response.
         */
        static QJsonObject convertToJson(const Event& event, bool includeCustomerId, bool includeHash);

        /**
         * Method that converts a list of events to a JSON response.
         *
         * \param[in] events            The event list to be converted.
         *
         * \param[in] includeCustomerId If true, the customer ID will be included in the response.  If false, the
         *                              customer ID will be excluded from the response.
         *
         * \param[in] includeHash       If true, the event hash will be included.  If false, the event hash will be
         *                              excluded.
         *
         * \return Returns a QJsonObject instance holding the response.
         */
        static QJsonArray convertToJson(const Events::EventList& event, bool includeCustomerId, bool includeHash);

        /**
         * Method that converts a latency entry to a JSON response.
         *
         * \param[in] entry             The entry to be converted.
         *
         * \param[in] serversById       A hash of server instances by server ID.
         *
         * \param[in] monitorsById      A hash of monitor instances by monitor ID.
         *
         * \param[in] includeServerId   If true, the server ID will be included in the response.  If false, the server
         *                              ID will be excluded from the response.
         *
         * \param[in] includeRegionId   If true, the region ID will be included in the response.  If false, the region
         *                              ID will be excluded from the response.
         *
         * \param[in] includeCustomerId If true, the customer ID will be included in the response.  If false, the
         *                              customer ID will be excluded from the response.
         *
         * \return Returns a QJsonObject instance holding the response.
         */
        static QJsonObject convertToJson(
            const LatencyEntry&           entry,
            const Servers::ServersById&   serversById,
            const Monitors::MonitorsById& monitorsById,
            bool                          includeServerId,
            bool                          includeRegionId,
            bool                          includeCustomerId
        );

        /**
         * Method that converts a latency entry to a JSON response.
         *
         * \param[in] entry             The entry to be converted.
         *
         * \param[in] serversById       A hash of server instances by server ID.
         *
         * \param[in] monitorsById      A hash of monitor instances by monitor ID.
         *
         * \param[in] includeServerId   If true, the server ID will be included in the response.  If false, the server
         *                              ID will be excluded from the response.
         *
         * \param[in] includeRegionId   If true, the region ID will be included in the response.  If false, the region
         *                              ID will be excluded from the response.
         *
         * \param[in] includeCustomerId If true, the customer ID will be included in the response.  If false, the
         *                              customer ID will be excluded from the response.
         *
         * \return Returns a QJsonObject instance holding the response.
         */
        static QJsonObject convertToJson(
            const AggregatedLatencyEntry& entry,
            const Servers::ServersById&   serversById,
            const Monitors::MonitorsById& monitorsById,
            bool                          includeServerId,
            bool                          includeRegionId,
            bool                          includeCustomerId
        );

        /**
         * Method that converts a list of latency entries to a JSON response.
         *
         * \param[in] entries           The entries to be converted.
         *
         * \param[in] serversById       A hash of server instances by server ID.
         *
         * \param[in] monitorsById      A hash of monitor instances by monitor ID.
         *
         * \param[in] includeServerId   If true, the server ID will be included in the response.  If false, the server
         *                              ID will be excluded from the response.
         *
         * \param[in] includeRegionId   If true, the region ID will be included in the response.  If false, the region
         *                              ID will be excluded from the response.
         *
         * \param[in] includeCustomerId If true, the customer ID will be included in the response.  If false, the
         *                              customer ID will be excluded from the response.
         *
         * \return Returns a QJsonObject instance holding the response.
         */
        static QJsonArray convertToJson(
            const LatencyInterfaceManager::LatencyEntryList& entries,
            const Servers::ServersById&                     serversById,
            const Monitors::MonitorsById&                    monitorsById,
            bool                                             includeServerId,
            bool                                             includeRegionId,
            bool                                             includeCustomerId
        );

        /**
         * Method that converts a list of latency entries to a JSON response.
         *
         * \param[in] entries           The entries to be converted.
         *
         * \param[in] serversById       A hash of server instances by server ID.
         *
         * \param[in] monitorsById      A hash of monitor instances by monitor ID.
         *
         * \param[in] includeServerId   If true, the server ID will be included in the response.  If false, the server
         *                              ID will be excluded from the response.
         *
         * \param[in] includeRegionId   If true, the region ID will be included in the response.  If false, the region
         *                              ID will be excluded from the response.
         *
         * \param[in] includeCustomerId If true, the customer ID will be included in the response.  If false, the
         *                              customer ID will be excluded from the response.
         *
         * \return Returns a QJsonObject instance holding the response.
         */
        static QJsonArray convertToJson(
            const LatencyInterfaceManager::AggregatedLatencyEntryList& entries,
            const Servers::ServersById&                                serversById,
            const Monitors::MonitorsById&                              monitorsById,
            bool                                                       includeServerId,
            bool                                                       includeRegionId,
            bool                                                       includeCustomerId
        );

        /**
         * Method that converts a single resource entry to JSON.
         *
         * \param[in] resource          The resource to be converted.
         *
         * \param[in] includeCustomerId If true, then the customer ID should be included.
         *
         * \return Returns a JSON dictionary holding the converted resource entry.
         */
        static QJsonObject convertToJson(const Resource& resource, bool includeCustomerId);

        /**
         * Method that converts a collection of resources to JSON.
         *
         * \param[in] resources         The list of resources to be converted.
         *
         * \param[in] fullEntry         If true, then every entry should be complete.
         *
         * \param[in] includeCustomerId If true, then the customer ID should be included.
         *
         * \return Returns a JSON dictionary holding the converted resource entry.
         */
        static QJsonArray convertToJson(
            const Resources::ResourceList& resourcess,
            bool                           fullEntry,
            bool                           includeCustomerId
        );
};

#endif
