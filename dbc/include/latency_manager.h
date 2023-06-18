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
* This header defines the \ref LatencyManager class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef LATENCY_MANAGER_H
#define LATENCY_MANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>
#include <rest_api_in_v1_inesonic_binary_rest_handler.h>

#include "rest_helpers.h"

class Servers;
class LatencyInterfaceManager;
class LatencyPlotter;

/**
 * Class that support a set of REST endpoints used to manage customer latency data.
 */
class LatencyManager:public QObject {
    Q_OBJECT

    public:
        /**
         * Path used to record latency information in bulk.
         */
        static const QString latencyRecordPath;

        /**
         * Path used to obtain latency information in bulk.
         */
        static const QString latencyGetPath;

        /**
         * Path used to delete latency information in bulk.
         */
        static const QString latencyPurgePath;

        /**
         * Path used to obtain latency information in plot format.
         */
        static const QString latencyPlotPath;

        /**
         * Path used to obtain latency information as statistics.
         */
        static const QString latencyStatisticsPath;

        /**
         * Constructor
         *
         * \param[in] restApiServer      The REST API server instance.
         *
         * \param[in] latencyDatabaseApi Class used to manage regions entries in the database.
         *
         * \param[in] serverDatabaseApi  Class used to manage our servers.
         *
         * \param[in] monitorDatabaseApi Class used to manage our monitors.
         *
         * \param[in] latencyPlotter     Class used to generate latency plots.
         *
         * \param[in[ secret             The incoming data secret.
         *
         * \param[in] parent             Pointer to the parent object.
         */
        LatencyManager(
            RestApiInV1::Server*     restApiServer,
            LatencyInterfaceManager* latencyDatabaseApi,
            Servers*                 serverDatabaseApi,
            Monitors*                monitorDatabaseApi,
            LatencyPlotter*          latencyPlotter,
            const QByteArray&        secret,
            QObject*                 parent = nullptr
        );

        ~LatencyManager() override;

        /**
         * Method you can use to set the Inesonic authentication secret.
         *
         * \param[in] newSecret The new secret to be used.  The secret must be the length prescribed by the
         *                      constant \ref inesonicSecretLength.
         */
        void setSecret(const QByteArray& newSecret);

    private:
        /**
         * The latency/record handler.
         */
        class LatencyRecord:public RestApiInV1::InesonicBinaryRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret             The secret to use for this handler.
                 *
                 * \param[in] latencyDatabaseApi Class used to manage regions entries in the database.
                 *
                 * \param[in] serverDatabaseApi  Class used to manage our servers.
                 */
                LatencyRecord(
                    const QByteArray&        secret,
                    LatencyInterfaceManager* latencyDatabaseApi,
                    Servers*                 serverDatabaseApi
                );

                ~LatencyRecord() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path     The request path.
                 *
                 * \param[in] request  The request data in raw binary format.
                 *
                 * \param[in] threadId The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return.  You can use either \ref BinaryResponse or \ref JsonResponse to
                 *         encode the data in your favorite format.
                 */
                RestApiInV1::Response* processAuthenticatedRequest(
                    const QString&    path,
                    const QByteArray& request,
                    unsigned          threadId
                ) override;

            private:
                /**
                 * The server maximum identifier length, in bytes.
                 */
                static constexpr unsigned maximumIdentifierLength = 48;

                /**
                 * Structure that defines our latency record header.  Note that this structure is also defined in the
                 * polling_server project with a structure that must match this one.
                 */
                struct Header {
                    /**
                     * A header version code value.  Currently ignored and set to 0.
                     */
                    std::uint16_t version;

                    /**
                     * The identifier for this server, encoded in UTF-8 format.
                     */
                    std::uint8_t identifier[maximumIdentifierLength];

                    /**
                     * The monitor service rate for this service.  The value holds the number of monitors serviced per
                     * second.  Value is in unsigned 24.8 notation.
                     */
                    std::uint32_t monitorsPerSecond;

                    /**
                     * The CPU loading reported as a value between 0 and 65535 where 0 is 0% and 65535 is 1600%.
                     */
                    std::uint16_t cpuLoading;

                    /**
                     * The memory loading reported as a value between 0 and 65535 where 0 is 0% and 65535 is 100%.
                     */
                    std::uint16_t memoryLoading;

                    /**
                     * The current server status.
                     */
                    std::uint8_t serverStatusCode;

                    /**
                     * Reserved for future use.  Fill with zeros.
                     */
                    std::uint8_t spare[64 - (2 + maximumIdentifierLength + 4 + 2 + 2 + 1)];
                } __attribute__((packed));

                /**
                 * Structure that defines our latency entry.  Note that this structure is also defined in the
                 * polling_server project with a with a structure that must match this one.
                 */
                struct Entry {
                    /**
                     * The monitor ID.
                     */
                    std::uint32_t monitorId;

                    /**
                     * The Zoran timestamp.
                     */
                    std::uint32_t timestamp;

                    /**
                     * The latency in microseconds
                     */
                    std::uint32_t latencyMicroseconds;
                } __attribute__((packed));

                /**
                 * The current region database API.
                 */
                LatencyInterfaceManager* currentLatencyInterfaceManager;

                /**
                 * The current servers database API.
                 */
                Servers* currentServers;
        };

        /**
         * The latency/get handler.
         */
        class LatencyGet:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret             The secret to use for this handler.
                 *
                 * \param[in] latencyDatabaseApi Class used to manage regions entries in the database.
                 *
                 * \param[in] serverDatabaseApi  Class used to manage our servers.
                 *
                 * \param[in] monitorDatabaseApi Class used to manage our monitors.
                 */
                LatencyGet(
                    const QByteArray&        secret,
                    LatencyInterfaceManager* latencyDatabaseApi,
                    Servers*                 serverDatabaseApi,
                    Monitors*                monitorDatabaseApi
                );

                ~LatencyGet() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path     The request path.
                 *
                 * \param[in] request  The request data encoded as a JSON document.
                 *
                 * \param[in] threadId The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current region database API.
                 */
                LatencyInterfaceManager* currentLatencyInterfaceManager;

                /**
                 * The current servers database API.
                 */
                Servers* currentServers;

                /**
                 * The current monitors database API.
                 */
                Monitors* currentMonitors;
        };

        /**
         * The latency/purge handler.
         */
        class LatencyPurge:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret             The secret to use for this handler.
                 *
                 * \param[in] latencyDatabaseApi Class used to manage regions entries in the database.
                 */
                LatencyPurge(const QByteArray& secret, LatencyInterfaceManager* latencyDatabaseApi);

                ~LatencyPurge() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path     The request path.
                 *
                 * \param[in] request  The request data encoded as a JSON document.
                 *
                 * \param[in] threadId The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current region database API.
                 */
                LatencyInterfaceManager* currentLatencyInterfaceManager;
        };

        /**
         * The latency/plot handler.
         */
        class LatencyPlot:public RestApiInV1::InesonicBinaryRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret         The secret to use for this handler.
                 *
                 * \param[in] latencyPlotter Class used to generate latency plots.
                 */
                LatencyPlot(const QByteArray& secret, LatencyPlotter* latencyPlotter);

                ~LatencyPlot() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path     The request path.
                 *
                 * \param[in] request  The request data encoded as a JSON document.
                 *
                 * \param[in] threadId The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::Response* processAuthenticatedRequest(
                    const QString&    path,
                    const QByteArray& request,
                    unsigned          threadId
                ) override;

            private:
                /**
                 * The current latency plotter.
                 */
                LatencyPlotter* currentLatencyPlotter;
        };

        /**
         * The latency/statistics handler.
         */
        class LatencyStatistics:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret             The secret to use for this handler.
                 *
                 * \param[in] latencyDatabaseApi Class used to manage regions entries in the database.
                 */
                LatencyStatistics(const QByteArray& secret, LatencyInterfaceManager* latencyDatabaseApi);

                ~LatencyStatistics() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path     The request path.
                 *
                 * \param[in] request  The request data encoded as a JSON document.
                 *
                 * \param[in] threadId The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current latency plotter.
                 */
                LatencyInterfaceManager* currentLatencyInterfaceManager;
        };

        /**
         * The latency/record handler.
         */
        LatencyRecord latencyRecord;

        /**
         * The latency/get handler.
         */
        LatencyGet latencyGet;

        /**
         * The latency/purge handler.
         */
        LatencyPurge latencyPurge;

        /**
         * The latency/plot handler.
         */
        LatencyPlot latencyPlot;

        /**
         * The latency/statistics handler.
         */
        LatencyStatistics latencyStatistics;
};

#endif
