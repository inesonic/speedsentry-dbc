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
* This header defines the \ref HostSchemeManager class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef HOST_SCHEME_MANAGER_H
#define HOST_SCHEME_MANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "rest_helpers.h"

class HostSchemes;
class MonitorUpdater;

/**
 * Class that support a set of REST endpoints used to manage hosts and schemes.
 */
class HostSchemeManager:public QObject {
    Q_OBJECT

    public:
        /**
         * Path used to get information about a specific host/scheme.
         */
        static const QString hostSchemeGetPath;

        /**
         * Path used to create a new host/scheme.
         */
        static const QString hostSchemeCreatePath;

        /**
         * Path used to modify an existing host/scheme.
         */
        static const QString hostSchemeModifyPath;

        /**
         * Path used to update just certificate data for a host/scheme.
         */
        static const QString hostSchemeCertificatePath;

        /**
         * Path used to delete an individual host/scheme.
         */
        static const QString hostSchemeDeletePath;

        /**
         * Path used to obtain a list of all hosts/schemes.
         */
        static const QString hostSchemeListPath;

        /**
         * Constructor
         *
         * \param[in] restApiServer          The REST API server instance.
         *
         * \param[in] hostSchemesDatabaseApi Class used to manage host/scheme entries in the database.
         *
         * \param[in] monitorUpdater         Class that updates customer monitors.
         *
         * \param[in[ secret                 The incoming data secret.
         *
         * \param[in] parent                 Pointer to the parent object.
         */
        HostSchemeManager(
            RestApiInV1::Server* restApiServer,
            HostSchemes*         hostSchemesDatabaseApi,
            MonitorUpdater*      monitorUpdater,
            const QByteArray&    secret,
            QObject*             parent = nullptr
        );

        ~HostSchemeManager() override;

        /**
         * Method you can use to set the Inesonic authentication secret.
         *
         * \param[in] newSecret The new secret to be used.  The secret must be the length prescribed by the
         *                      constant \ref inesonicSecretLength.
         */
        void setSecret(const QByteArray& newSecret);

    private:
        /**
         * The host_scheme/get handler.
         */
        class HostSchemeGet:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                The secret to use for this handler.
                 *
                 * \param[in] hostSchemeDatabaseApi Class used to manage host/scheme entries in the database.
                 */
                HostSchemeGet(const QByteArray& secret, HostSchemes* hostSchemeDatabaseApi);

                ~HostSchemeGet() override;

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
                 * The current host/scheme database API.
                 */
                HostSchemes* currentHostSchemes;
        };

        /**
         * The host_scheme/create handler.
         */
        class HostSchemeCreate:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                The secret to use for this handler.
                 *
                 * \param[in] hostSchemeDatabaseApi Class used to manage host/scheme entries in the database.
                 */
                HostSchemeCreate(const QByteArray& secret, HostSchemes* hostSchemeDatabaseApi);

                ~HostSchemeCreate() override;

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
                 * The current host/scheme database API.
                 */
                HostSchemes* currentHostSchemes;
        };

        /**
         * The host_scheme/modify handler.
         */
        class HostSchemeModify:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                The secret to use for this handler.
                 *
                 * \param[in] hostSchemeDatabaseApi Class used to manage host/scheme entries in the database.
                 *
                 * \param[in] monitorUpdater        Class that updates customer monitors.
                 */
                HostSchemeModify(
                    const QByteArray& secret,
                    HostSchemes*      hostSchemeDatabaseApi,
                    MonitorUpdater*   monitorUpdater
                );

                ~HostSchemeModify() override;

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
                 * The current host/scheme database API.
                 */
                HostSchemes* currentHostSchemes;

                /**
                 * The monitor updater engine.
                 */
                MonitorUpdater* currentMonitorUpdater;
        };

        /**
         * The host_scheme/certificate handler.
         */
        class HostSchemeCertificate:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                The secret to use for this handler.
                 *
                 * \param[in] hostSchemeDatabaseApi Class used to manage host/scheme entries in the database.
                 */
                HostSchemeCertificate(const QByteArray& secret, HostSchemes* hostSchemeDatabaseApi);

                ~HostSchemeCertificate() override;

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
                 * The current host/scheme database API.
                 */
                HostSchemes* currentHostSchemes;
        };

        /**
         * The host_scheme/delete handler.
         */
        class HostSchemeDelete:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                The secret to use for this handler.
                 *
                 * \param[in] hostSchemeDatabaseApi Class used to manage host/scheme entries in the database.
                 *
                 * \param[in] monitorUpdater        Class that updates customer monitors.
                 */
                HostSchemeDelete(
                    const QByteArray& secret,
                    HostSchemes*      hostSchemeDatabaseApi,
                    MonitorUpdater*   monitorUpdater
                );

                ~HostSchemeDelete() override;

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
                 * The current host/scheme database API.
                 */
                HostSchemes* currentHostSchemes;

                /**
                 * The monitor updater engine.
                 */
                MonitorUpdater* currentMonitorUpdater;
        };

        /**
         * The host_scheme/list handler.
         */
        class HostSchemeList:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                The secret to use for this handler.
                 *
                 * \param[in] hostSchemeDatabaseApi Class used to manage host/scheme entries in the database.
                 */
                HostSchemeList(const QByteArray& secret, HostSchemes* hostSchemeDatabaseApi);

                ~HostSchemeList() override;

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
                 * The current host/scheme database API.
                 */
                HostSchemes* currentHostSchemes;
        };

        /**
         * The host_scheme/get handler.
         */
        HostSchemeGet hostSchemeGet;

        /**
         * The host_scheme/create handler.
         */
        HostSchemeCreate hostSchemeCreate;

        /**
         * The host_scheme/modify handler.
         */
        HostSchemeModify hostSchemeModify;

        /**
         * The host_scheme/certificate handler.
         */
        HostSchemeCertificate hostSchemeCertificate;

        /**
         * The host_scheme/delete handler.
         */
        HostSchemeDelete hostSchemeDelete;

        /**
         * The host_scheme/list handler.
         */
        HostSchemeList hostSchemeList;
};

#endif
