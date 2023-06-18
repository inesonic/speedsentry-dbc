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
* This header defines the \ref CustomerCapabilitiesManager class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef CUSTOMER_CAPABILITIES_MANAGER_H
#define CUSTOMER_CAPABILITIES_MANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "rest_helpers.h"

class CustomersCapabilities;
class CustomerSecrets;
class ServerAdministrator;

/**
 * Class that support a set of REST endpoints used to manage customer capabilities.
 */
class CustomerCapabilitiesManager:public QObject {
    Q_OBJECT

    public:
        /**
         * Path used to get information about a specific customer's capabilities.
         */
        static const QString customerCapabilitiesGetPath;

        /**
         * Path used to create a new customer or update an existing customer, defining their allowed capabilities.
         */
        static const QString customerCapabilitiesUpdatePath;

        /**
         * Path used to delete a customer.
         */
        static const QString customerCapabilitiesDeletePath;

        /**
         * Path used to purge multiple customers from the system.
         */
        static const QString customerCapabilitiesPurgePath;

        /**
         * Path used to obtain a list of all customer's capabilities.
         */
        static const QString customerCapabilitiesListPath;

        /**
         * Path used to obtain customer REST secret data.
         */
        static const QString customerGetSecretPath;

        /**
         * Path used to reset customer REST secret data.
         */
        static const QString customerResetSecretPath;

        /**
         * Path used to pause/resume a customer.
         */
        static const QString customerCapabilitiesPausePath;

        /**
         * Constructor
         *
         * \param[in] restApiServer                    The REST API server instance.
         *
         * \param[in] customersCapabilitiesDatabaseApi Class used to manage customer's capabilities in the database.
         *
         * \param[in] customerSecretsDatabaseApi       Class used to manage customer secrets.
         *
         * \param[in] serverAdministrator              The server administrator used to issue commands to the polling
         *                                             servers.
         *
         * \param[in[ secret                           The incoming data secret.
         *
         * \param[in] parent                           Pointer to the parent object.
         */
        CustomerCapabilitiesManager(
            RestApiInV1::Server*   restApiServer,
            CustomersCapabilities* customerCapabilitiesDatabaseApi,
            CustomerSecrets*       customerSecretsDatabaseApi,
            ServerAdministrator*   ServerAdministrator,
            const QByteArray&      secret,
            QObject*               parent = nullptr
        );

        ~CustomerCapabilitiesManager() override;

        /**
         * Method you can use to set the Inesonic authentication secret.
         *
         * \param[in] newSecret The new secret to be used.  The secret must be the length prescribed by the
         *                      constant \ref inesonicSecretLength.
         */
        void setSecret(const QByteArray& newSecret);

    private:
        /**
         * The customer/get handler.
         */
        class CustomerCapabilitiesGet:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                          The secret to use for this handler.
                 *
                 * \param[in] customerCapabilitiesDatabaseApi Class used to manage customer capabilities in the
                 *                                            database.
                 */
                CustomerCapabilitiesGet(
                    const QByteArray&      secret,
                    CustomersCapabilities* customerCapabilitiesDatabaseApi
                );

                ~CustomerCapabilitiesGet() override;

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
                 * The current customer capabilities database API.
                 */
                CustomersCapabilities* currentCustomersCapabilities;
        };

        /**
         * The customer/create handler.
         */
        class CustomerCapabilitiesUpdate:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                          The secret to use for this handler.
                 *
                 * \param[in] customerCapabilitiesDatabaseApi Class used to manage customer capabilities in the
                 *                                            database.
                 *
                 * \param[in] serverAdministrator             The server administrator.
                 *
                 * \param[in] customerSecretsDatabaseApi      Class used to manage customer secrets.
                 */
                CustomerCapabilitiesUpdate(
                    const QByteArray&      secret,
                    CustomersCapabilities* customerCapabilitiesDatabaseApi,
                    ServerAdministrator*   serverAdministrator,
                    CustomerSecrets*       customerSecretsDatabaseApi
                );

                ~CustomerCapabilitiesUpdate() override;

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
                 * The current customer capabilities database API.
                 */
                CustomersCapabilities* currentCustomersCapabilities;

                /**
                 * The server administrator used to command the polling servers.
                 */
                ServerAdministrator* currentServerAdministrator;

                /**
                 * The customer secrets database API.
                 */
                CustomerSecrets* currentCustomerSecrets;
        };

        /**
         * The customer/delete handler.
         */
        class CustomerCapabilitiesDelete:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                          The secret to use for this handler.
                 *
                 * \param[in] customerCapabilitiesDatabaseApi Class used to manage customer capabilities in the
                 *                                            database.
                 *
                 * \param[in] serverAdministrator             The server administrator.
                 */
                CustomerCapabilitiesDelete(
                    const QByteArray&      secret,
                    CustomersCapabilities* customerCapabilitiesDatabaseApi,
                    ServerAdministrator*   serverAdministrator
                );

                ~CustomerCapabilitiesDelete() override;

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
                 * The current customer capabilities database API.
                 */
                CustomersCapabilities* currentCustomersCapabilities;

                /**
                 * The server administrator used to command the polling servers.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The customer/purge handler.
         */
        class CustomerCapabilitiesPurge:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                          The secret to use for this handler.
                 *
                 * \param[in] customerCapabilitiesDatabaseApi Class used to manage customer capabilities in the
                 *                                            database.
                 *
                 * \param[in] serverAdministrator             The server administrator.
                 */
                CustomerCapabilitiesPurge(
                    const QByteArray&      secret,
                    CustomersCapabilities* customerCapabilitiesDatabaseApi,
                    ServerAdministrator*   serverAdministrator
                );

                ~CustomerCapabilitiesPurge() override;

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
                 * The current customer capabilities database API.
                 */
                CustomersCapabilities* currentCustomersCapabilities;

                /**
                 * The server administrator used to command the polling servers.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The customer/list handler.
         */
        class CustomerCapabilitiesList:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                          The secret to use for this handler.
                 *
                 * \param[in] customerCapabilitiesDatabaseApi Class used to manage customer capabilities in the
                 *                                            database.
                 */
                CustomerCapabilitiesList(
                    const QByteArray&      secret,
                    CustomersCapabilities* customerCapabilitiesDatabaseApi
                );

                ~CustomerCapabilitiesList() override;

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
                 * The current customer capabilities database API.
                 */
                CustomersCapabilities* currentCustomersCapabilities;
        };

        /**
         * The customer/get_secret handler.
         */
        class CustomerGetSecret:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                     The secret to use for this handler.
                 *
                 * \param[in] customerSecretsDatabaseApi Class used to manage customer secrets.
                 */
                CustomerGetSecret(const QByteArray& secret, CustomerSecrets* customerSecretsDatabaseApi);

                ~CustomerGetSecret() override;

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
                 * The customer secrets database API.
                 */
                CustomerSecrets* currentCustomerSecrets;
        };

        /**
         * The customer/reset_secret handler.
         */
        class CustomerResetSecret:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                     The secret to use for this handler.
                 *
                 * \param[in] customerSecretsDatabaseApi Class used to manage customer secrets.
                 */
                CustomerResetSecret(const QByteArray& secret, CustomerSecrets* customerSecretsDatabaseApi);

                ~CustomerResetSecret() override;

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
                 * The customer secrets database API.
                 */
                CustomerSecrets* currentCustomerSecrets;
        };

        /**
         * The customer/pause handler.
         */
        class CustomerCapabilitiesPause:public RestApiInV1::InesonicRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret              The secret to use for this handler.
                 *
                 * \param[in] serverAdministrator The server administrator used to send the request.
                 */
                CustomerCapabilitiesPause(
                    const QByteArray&    secret,
                    ServerAdministrator* serverAdministrator
                );

                ~CustomerCapabilitiesPause() override;

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
                 * The server administrator used to command the polling servers.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The customer/get handler.
         */
        CustomerCapabilitiesGet customerCapabilitiesGet;

        /**
         * The customer/create handler.
         */
        CustomerCapabilitiesUpdate customerCapabilitiesUpdate;

        /**
         * The customer/delete handler.
         */
        CustomerCapabilitiesDelete customerCapabilitiesDelete;

        /**
         * The customer/purge handler.
         */
        CustomerCapabilitiesPurge customerCapabilitiesPurge;

        /**
         * The customer/list handler.
         */
        CustomerCapabilitiesList customerCapabilitiesList;

        /**
         * The customer/get_secret handler.
         */
        CustomerGetSecret customerGetSecret;

        /**
         * The customer/reset_secret handler.
         */
        CustomerResetSecret customerResetSecret;

        /**
         * The customer/pause handler.
         */
        CustomerCapabilitiesPause customerCapabilitiesPause;
};

#endif
