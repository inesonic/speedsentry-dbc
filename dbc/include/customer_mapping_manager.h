/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref CustomerMappingManager class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef CUSTOMER_MAPPING_MANAGER_H
#define CUSTOMER_MAPPING_MANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

class CustomerMapping;
class ServerAdministrator;

/**
 * Class that support a set of REST endpoints used to manage customer/server mappings.
 */
class CustomerMappingManager:public QObject {
    Q_OBJECT

    public:
        /**
         * Path used to get information about a specific customer mapping.
         */
        static const QString mappingGetPath;

        /**
         * Path used to create or update a customer mapping.
         */
        static const QString mappingUpdatePath;

        /**
         * Path used to activate a customers settingsm updating polling servers.
         */
        static const QString mappingCustomerActivatePath;

        /**
         * Path used to deactivate a customer, updating polling servers.
         */
        static const QString mappingCustomerDeactivatePath;

        /**
         * Path used to obtain a list of all customer mappings.
         */
        static const QString mappingListPath;

        /**
         * Constructor
         *
         * \param[in] restApiServer              The REST API server instance.
         *
         * \param[in] customerMappingDatabaseApi Class used to manage customer mapping entries in the database.
         *
         * \param[in] serverAdministratorApi     The server administration API.
         *
         * \param[in[ secret                     The incoming data secret.
         *
         * \param[in] parent                     Pointer to the parent object.
         */
        CustomerMappingManager(
            RestApiInV1::Server* restApiServer,
            CustomerMapping*     customerMappingDatabaseApi,
            ServerAdministrator* serverAdministratorApi,
            const QByteArray&    secret,
            QObject*             parent = nullptr
        );

        ~CustomerMappingManager() override;

        /**
         * Method you can use to set the Inesonic authentication secret.
         *
         * \param[in] newSecret The new secret to be used.  The secret must be the length prescribed by the
         *                      constant \ref inesonicSecretLength.
         */
        void setSecret(const QByteArray& newSecret);

    private:
        /**
         * The mapping/get handler.
         */
        class CustomerMappingGet:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                     The secret to use for this handler.
                 *
                 * \param[in] customerMappingDatabaseApi Class used to manage customer mapping entries in the database.
                 */
                CustomerMappingGet(const QByteArray& secret, CustomerMapping* customerMappingDatabaseApi);

                ~CustomerMappingGet() override;

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
                 * The current customer mapping database API.
                 */
                CustomerMapping* currentCustomerMapping;
        };

        /**
         * The mapping/update handler.
         */
        class CustomerMappingUpdate:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                     The secret to use for this handler.
                 *
                 * \param[in] customerMappingDatabaseApi Class used to manage customer mapping entries in the database.
                 */
                CustomerMappingUpdate(const QByteArray& secret, CustomerMapping* customerMappingDatabaseApi);

                ~CustomerMappingUpdate() override;

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
                 * The current customer mapping database API.
                 */
                CustomerMapping* currentCustomerMapping;
        };

        /**
         * The mapping/customer/activate handler.
         */
        class CustomerMappingCustomerActivate:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                 The secret to use for this handler.
                 *
                 * \param[in] serverAdministratorApi The server administration API.
                 */
                CustomerMappingCustomerActivate(const QByteArray& secret, ServerAdministrator* serverAdministratorApi);

                ~CustomerMappingCustomerActivate() override;

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
                 * The current server administrator.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The mapping/customer/deactivate handler.
         */
        class CustomerMappingCustomerDeactivate:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                 The secret to use for this handler.
                 *
                 * \param[in] serverAdministratorApi The server administration API.
                 */
                CustomerMappingCustomerDeactivate(const QByteArray& secret, ServerAdministrator* serverAdministratorApi);

                ~CustomerMappingCustomerDeactivate() override;

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
                 * The current server administrator.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The mapping/list handler.
         */
        class CustomerMappingList:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret                     The secret to use for this handler.
                 *
                 * \param[in] customerMappingDatabaseApi Class used to manage customer mapping entries in the database.
                 */
                CustomerMappingList(const QByteArray& secret, CustomerMapping* customerMappingDatabaseApi);

                ~CustomerMappingList() override;

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
                 * The current customer mapping database API.
                 */
                CustomerMapping* currentCustomerMapping;
        };

        /**
         * The mapping/get handler.
         */
        CustomerMappingGet mappingGet;

        /**
         * The mapping/update handler.
         */
        CustomerMappingUpdate mappingUpdate;

        /**
         * The mapping/customer/activate handler.
         */
        CustomerMappingCustomerActivate mappingCustomerActivate;

        /**
         * The mapping/customer/deactivate handler.
         */
        CustomerMappingCustomerDeactivate mappingCustomerDeactivate;

        /**
         * The mapping/list handler.
         */
        CustomerMappingList mappingList;
};

#endif
