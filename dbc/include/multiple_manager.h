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
* This header defines the \ref MultipleManager class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef MULTIPLE_MANAGER_H
#define MULTIPLE_MANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "monitor.h"
#include "monitors.h"
#include "rest_helpers.h"

class HostSchemes;
class Monitors;
class Events;
class LatencyPlotter;

/**
 * Class that support a set of REST endpoints used to obtain multiple data elements.
 */
class MultipleManager:public QObject {
    Q_OBJECT

    public:
        /**
         * Path used to get multiple information elements for a customer in a single REST API call.
         */
        static const QString multipleListPath;

        /**
         * Constructor
         *
         * \param[in] restApiServer         The REST API server instance.
         *
         * \param[in] hostSchemeDatabaseApi Class used to manage hosts and schemes.
         *
         * \param[in] monitorDatabaseApi    Class used to manage the customer's monitors.
         *
         * \param[in] eventDatabaseApi      Class used to manage events and status.
         *
         * \param[in] latencyPlotter        Plotter used to generate latency plots.
         *
         * \param[in[ secret                The incoming data secret.
         *
         * \param[in] parent                Pointer to the parent object.
         */
        MultipleManager(
            RestApiInV1::Server*   restApiServer,
            HostSchemes*           hostSchemeDatabaseApi,
            Monitors*              monitorDatabaseApi,
            Events*                eventDatabaseApi,
            LatencyPlotter*        latencyPlotter,
            const QByteArray&      secret,
            QObject*               parent = nullptr
        );

        ~MultipleManager() override;

        /**
         * Method you can use to set the Inesonic authentication secret.
         *
         * \param[in] newSecret The new secret to be used.  The secret must be the length prescribed by the
         *                      constant \ref inesonicSecretLength.
         */
        void setSecret(const QByteArray& newSecret);

    private:
        /**
         * The multiple/list handler.
         */
        class MultipleList:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                The secret to use for this handler.
                 *
                 * \param[in] hostSchemeDatabaseApi Class used to manage hosts and schemes.
                 *
                 * \param[in] monitorDatabaseApi    Class used to manage the customer's monitors.
                 *
                 * \param[in] eventDatabaseApi      Class used to manage events and status.
                 */
                MultipleList(
                    const QByteArray& secret,
                    HostSchemes*      hostSchemeDatabaseApi,
                    Monitors*         monitorDatabaseApi,
                    Events*           eventDatabaseApi
                );

                ~MultipleList() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path      The request path.
                 *
                 * \param[in] request   The request data encoded as a JSON document.
                 *
                 * \param[in] threadId  The ID used to uniquely identify this thread while in flight.
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
                 * The current host/scheme database API.
                 */
                HostSchemes* currentHostSchemes;

                /**
                 * The current monitors database API.
                 */
                Monitors* currentMonitors;

                /**
                 * The current event database API.
                 */
                Events* currentEvents;
        };

        /**
         * The multiple/list handler.
         */
        MultipleList multipleList;
};

#endif
