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
* This header defines the \ref EventManager class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "rest_helpers.h"

class Events;
class Monitors;
class EventProcessor;

/**
 * Class that support a set of REST endpoints used to manage regions.
 */
class EventManager:public QObject {
    Q_OBJECT

    public:
        /**
         * Path used to report about an event.
         */
        static const QString eventReportPath;

        /**
         * Path used to obtain status about a monitor or all monitors associated with a customer.
         */
        static const QString eventStatusPath;

        /**
         * Path used to obtain a history of events.
         */
        static const QString eventGetPath;

        /**
         * Constructor
         *
         * \param[in] restApiServer      The REST API server instance.
         *
         * \param[in] eventDatabaseApi   Class used to manage event entries in the database.
         *
         * \param[in] monitorDatabaseApi Class used to manage monitors entries in the database.
         *
         * \param[in] eventProcessor     The event processor.  Used to propagate events.
         *
         * \param[in[ secret             The incoming data secret.
         *
         * \param[in] parent             Pointer to the parent object.
         */
        EventManager(
            RestApiInV1::Server* restApiServer,
            Events*              eventDatabaseApi,
            Monitors*            monitorDatabaseApi,
            EventProcessor*      eventProcessor,
            const QByteArray&    secret,
            QObject*             parent = nullptr
        );

        ~EventManager() override;

        /**
         * Method you can use to set the Inesonic authentication secret.
         *
         * \param[in] newSecret The new secret to be used.  The secret must be the length prescribed by the
         *                      constant \ref inesonicSecretLength.
         */
        void setSecret(const QByteArray& newSecret);

    private:
        /**
         * The event/report handler.
         */
        class EventReport:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret             The secret to use for this handler.
                 *
                 * \param[in] eventProcessor     Class used to propagate events.
                 *
                 * \param[in] monitorDatabaseApi Class used to manage monitors entries in the database.
                 *
                 * \param[in] eventProcessor     Class used to propagate events.
                 */
                EventReport(const QByteArray& secret, EventProcessor* eventDatabaseApi, Monitors* monitorDatabaseApi);

                ~EventReport() override;

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
                 * The current event processor API.
                 */
                EventProcessor* currentEventProcessor;

                /**
                 * The current monitor database API.
                 */
                Monitors* currentMonitors;
        };

        /**
         * The event/status handler.
         */
        class EventStatus:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret             The secret to use for this handler.
                 *
                 * \param[in] eventnDatabaseApi  Class used to manage events entries in the database.
                 *
                 * \param[in] monitorDatabaseApi Class used to manage monitors entries in the database.
                 */
                EventStatus(const QByteArray& secret, Events* eventDatabaseApi, Monitors* monitorDatabaseApi);

                ~EventStatus() override;

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
                 * The current event database API.
                 */
                Events* currentEvents;

                /**
                 * The current monitor database API.
                 */
                Monitors* currentMonitors;
        };

        /**
         * The event/get handler.
         */
        class EventGet:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret            The secret to use for this handler.
                 *
                 * \param[in] regionDatabaseApi Class used to manage regions entries in the database.
                 */
                EventGet(const QByteArray& secret, Events* regionDatabaseApi);

                ~EventGet() override;

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
                 * The current event database API.
                 */
                Events* currentEvents;
        };

        /**
         * The event/get handler.
         */
        EventReport eventReport;

        /**
         * The event/status handler.
         */
        EventStatus eventStatus;

        /**
         * The event/get handler.
         */
        EventGet eventGet;
};

#endif
