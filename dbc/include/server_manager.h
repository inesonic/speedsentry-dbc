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
* This header defines the \ref ServerManager class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

class ServerAdministrator;
class Regions;

/**
 * Class that support a set of REST endpoints used to manage servers.
 */
class ServerManager:public QObject {
    Q_OBJECT

    public:
        /**
         * Path used to get information about a specific server.
         */
        static const QString serverGetPath;

        /**
         * Path used to create or register a new server in the database.
         */
        static const QString serverCreatePath;

        /**
         * Path used to modify an existing server's information.
         */
        static const QString serverModifyPath;

        /**
         * Path used to delete an individual server from the database.
         */
        static const QString serverDeletePath;

        /**
         * Path used to obtain a list of all servers or servers by region or status.
         */
        static const QString serverListPath;

        /**
         * Path used to set a server's status to active.
         */
        static const QString serverActivatePath;

        /**
         * Path used to set a server's status to inactive.
         */
        static const QString serverDeactivatePath;

        /**
         * Path used to command a server to start.
         */
        static const QString serverStartPath;

        /**
         * Path used to reassign work owned by a server and set status to inactive.
         */
        static const QString serverReassignPath;

        /**
         * Path used to redistribute work across servers in a given region.
         */
        static const QString serverRedistributePath;

        /**
         * Constructor
         *
         * \param[in] restApiServer       The REST API server instance.
         *
         * \param[in] serverAdministrator Class used to manage servers entries.
         *
         * \param[in] regionDatabaseApi   Class used to manage servers entries in the database.
         *
         * \param[in[ secret              The incoming data secret.
         *
         * \param[in] parent              Pointer to the parent object.
         */
        ServerManager(
            RestApiInV1::Server* restApiServer,
            ServerAdministrator* serverAdministrator,
            Regions*             regionDatabaseApi,
            const QByteArray&    secret,
            QObject*             parent = nullptr
        );

        ~ServerManager() override;

        /**
         * Method you can use to set the Inesonic authentication secret.
         *
         * \param[in] newSecret The new secret to be used.  The secret must be the length prescribed by the
         *                      constant \ref inesonicSecretLength.
         */
        void setSecret(const QByteArray& newSecret);

    private:
        /**
         * The server/get handler.
         */
        class ServerGet:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret              The secret to use for this handler.
                 *
                 * \param[in] serverAdministrator Class used to manage servers entries.
                 */
                ServerGet(const QByteArray& secret, ServerAdministrator* serverAdministrator);

                ~ServerGet() override;

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
                 * The current servers database API.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The server/create handler.
         */
        class ServerCreate:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret              The secret to use for this handler.
                 *
                 * \param[in] serverAdministrator Class used to manage servers entries.
                 *
                 * \param[in] regionDatabaseApi   Class used to validate region IDs.
                 */
                ServerCreate(
                    const QByteArray&    secret,
                    ServerAdministrator* serverAdministrator,
                    Regions*             regionDatabaseApi
                );

                ~ServerCreate() override;

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
                 * The current servers database API.
                 */
                ServerAdministrator* currentServerAdministrator;

                /**
                 * The current regions database API.
                 */
                Regions* currentRegions;
        };

        /**
         * The server/modify handler.
         */
        class ServerModify:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret              The secret to use for this handler.
                 *
                 * \param[in] serverAdministrator Class used to manage servers entries.
                 *
                 * \param[in] regionDatabaseApi   Class used to validate region IDs.
                 */
                ServerModify(
                    const QByteArray&    secret,
                    ServerAdministrator* serverAdministrator,
                    Regions*             regionDatabaseApi);

                ~ServerModify() override;

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
                 * The current servers database API.
                 */
                ServerAdministrator* currentServerAdministrator;

                /**
                 * The current regions database API.
                 */
                Regions* currentRegions;
        };

        /**
         * The server/delete handler.
         */
        class ServerDelete:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret              The secret to use for this handler.
                 *
                 * \param[in] serverAdministrator Class used to manage servers entries.
                 */
                ServerDelete(const QByteArray& secret, ServerAdministrator* serverAdministrator);

                ~ServerDelete() override;

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
                 * The current servers database API.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The server/list handler.
         */
        class ServerList:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret              The secret to use for this handler.
                 *
                 * \param[in] serverAdministrator Class used to manage servers entries.
                 */
                ServerList(const QByteArray& secret, ServerAdministrator* serverAdministrator);

                ~ServerList() override;

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
                 * The current servers database API.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The server/activate handler.
         */
        class ServerActivate:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret              The secret to use for this handler.
                 *
                 * \param[in] serverAdministrator Class used to manage servers entries.
                 */
                ServerActivate(const QByteArray& secret, ServerAdministrator* serverAdministrator);

                ~ServerActivate() override;

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
                 * The current servers database API.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The server/deactivate handler.
         */
        class ServerDeactivate:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret              The secret to use for this handler.
                 *
                 * \param[in] serverAdministrator Class used to manage servers entries.
                 */
                ServerDeactivate(const QByteArray& secret, ServerAdministrator* serverAdministrator);

                ~ServerDeactivate() override;

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
                 * The current servers database API.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The server/start handler.
         */
        class ServerStart:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret              The secret to use for this handler.
                 *
                 * \param[in] serverAdministrator Class used to manage servers entries.
                 */
                ServerStart(const QByteArray& secret, ServerAdministrator* serverAdministrator);

                ~ServerStart() override;

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
                 * The current servers database API.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The server/reassign handler.
         */
        class ServerReassign:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret              The secret to use for this handler.
                 *
                 * \param[in] serverAdministrator Class used to manage servers entrie.
                 */
                ServerReassign(const QByteArray& secret, ServerAdministrator* serverAdministrator);

                ~ServerReassign() override;

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
                 * The current servers database API.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The server/redistribute handler.
         */
        class ServerRedistribute:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret              The secret to use for this handler.
                 *
                 * \param[in] serverAdministrator Class used to manage servers entries.
                 */
                ServerRedistribute(const QByteArray& secret, ServerAdministrator* serverAdministrator);

                ~ServerRedistribute() override;

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
                 * The current servers database API.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The server/get handler.
         */
        ServerGet serverGet;

        /**
         * The server/create handler.
         */
        ServerCreate serverCreate;

        /**
         * The server/modify handler.
         */
        ServerModify serverModify;

        /**
         * The server/delete handler.
         */
        ServerDelete serverDelete;

        /**
         * The server/list handler.
         */
        ServerList serverList;

        /**
         * The server/activate handler.
         */
        ServerActivate serverActivate;

        /**
         * The server/deactivate handler.
         */
        ServerDeactivate serverDeactivate;

        /**
         * The server/start handler.
         */
        ServerStart serverStart;

        /**
         * The server/reassign handler.
         */
        ServerReassign serverReassign;

        /**
         * The server/redistribute handler.
         */
        ServerRedistribute serverRedistribute;
};

#endif
