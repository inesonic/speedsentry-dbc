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
* This header defines the \ref MonitorManager class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef MONITOR_MANAGER_H
#define MONITOR_MANAGER_H

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

class MonitorUpdater;

/**
 * Class that support a set of REST endpoints used to manage monitors.
 */
class MonitorManager:public QObject {
    Q_OBJECT

    public:
        /**
         * Path used to get information about a specific monitor.
         */
        static const QString monitorGetPath;

        /**
         * Path used to delete an individual monitor.
         */
        static const QString monitorDeletePath;

        /**
         * Path used to obtain a list of all monitors.
         */
        static const QString monitorListPath;

        /**
         * Path used to update monitors for a given customer.
         */
        static const QString monitorUpdatePath;

        /**
         * Constructor
         *
         * \param[in] restApiServer       The REST API server instance.
         *
         * \param[in] monitorDatabaseApi  Class used to manage monitors entries in the database.
         *
         * \param[in] customerDatabaseApi Class used to manage the customer data.
         *
         * \param[in] monitorUpdater      Class that updates customer monitors.
         *
         * \param[in[ secret              The incoming data secret.
         *
         * \param[in] parent              Pointer to the parent object.
         */
        MonitorManager(
            RestApiInV1::Server*   restApiServer,
            Monitors*              monitorDatabaseApi,
            CustomersCapabilities* customerDatabaseApi,
            MonitorUpdater*        monitorUpdater,
            const QByteArray&      secret,
            QObject*               parent = nullptr
        );

        ~MonitorManager() override;

        /**
         * Method you can use to set the Inesonic authentication secret.
         *
         * \param[in] newSecret The new secret to be used.  The secret must be the length prescribed by the
         *                      constant \ref inesonicSecretLength.
         */
        void setSecret(const QByteArray& newSecret);

    private:
        /**
         * The monitor/get handler.
         */
        class MonitorGet:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret             The secret to use for this handler.
                 *
                 * \param[in] monitorDatabaseApi Class used to manage monitors entries in the database.
                 */
                MonitorGet(const QByteArray& secret, Monitors* monitorDatabaseApi);

                ~MonitorGet() override;

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
                 * The current monitor database API.
                 */
                Monitors* currentMonitors;
        };

        /**
         * The monitor/delete handler.
         */
        class MonitorDelete:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret             The secret to use for this handler.
                 *
                 * \param[in] monitorDatabaseApi Class used to manage monitors entries in the database.
                 *
                 * \param[in] monitorUpdater     Class that updates customer monitors.
                 */
                MonitorDelete(const QByteArray& secret, Monitors* monitorDatabaseApi, MonitorUpdater* monitorUpdater);

                ~MonitorDelete() override;

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
                 * The current monitor database API.
                 */
                Monitors* currentMonitors;

                /**
                 * The monitor updater engine.
                 */
                MonitorUpdater* currentMonitorUpdater;
        };

        /**
         * The monitor/list handler.
         */
        class MonitorList:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret             The secret to use for this handler.
                 *
                 * \param[in] monitorDatabaseApi Class used to manage monitors entries in the database.
                 */
                MonitorList(const QByteArray& secret, Monitors* monitorDatabaseApi);

                ~MonitorList() override;

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
                 * The current monitor database API.
                 */
                Monitors* currentMonitors;
        };

        /**
         * The monitor/update handler.
         */
        class MonitorUpdate:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret              The secret to use for this handler.
                 *
                 * \param[in] monitorDatabaseApi  Class used to manage monitors entries in the database.
                 *
                 * \param[in] customerDatabaseApi Class used to manage the customer data.
                 *
                 * \param[in] monitorUpdater      Class that updates customer monitors.
                 */
                MonitorUpdate(
                    const QByteArray&      secret,
                    Monitors*              monitorDatabaseApi,
                    CustomersCapabilities* customerDatabaseApi,
                    MonitorUpdater*        monitorUpdater
                );

                ~MonitorUpdate() override;

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
                 * The current monitor database API.
                 */
                Monitors* currentMonitors;

                /**
                 * The current customer database API.
                 */
                CustomersCapabilities* currentCustomersCapabilities;

                /**
                 * The monitor updater engine.
                 */
                MonitorUpdater* currentMonitorUpdater;
        };

        /**
         * The monitor/get handler.
         */
        MonitorGet monitorGet;

        /**
         * The monitor/delete handler.
         */
        MonitorDelete monitorDelete;

        /**
         * The monitor/list handler.
         */
        MonitorList monitorList;

        /**
         * The monitor/update handler.
         */
        MonitorUpdate monitorUpdate;
};

#endif
