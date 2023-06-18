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
* This header defines the \ref RegionManager class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef REGION_MANAGER_H
#define REGION_MANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

class Regions;

/**
 * Class that support a set of REST endpoints used to manage regions.
 */
class RegionManager:public QObject {
    Q_OBJECT

    public:
        /**
         * Path used to get information about a specific region.
         */
        static const QString regionGetPath;

        /**
         * Path used to create a new region.
         */
        static const QString regionCreatePath;

        /**
         * Path used to modify an existing region's name.
         */
        static const QString regionModifyPath;

        /**
         * Path used to delete an individual region.
         */
        static const QString regionDeletePath;

        /**
         * Path used to obtain a list of all regions.
         */
        static const QString regionListPath;

        /**
         * Constructor
         *
         * \param[in] restApiServer     The REST API server instance.
         *
         * \param[in] regionDatabaseApi Class used to manage regions entries in the database.
         *
         * \param[in[ secret            The incoming data secret.
         *
         * \param[in] parent            Pointer to the parent object.
         */
        RegionManager(
            RestApiInV1::Server* restApiServer,
            Regions*             regionDatabaseApi,
            const QByteArray&    secret,
            QObject*             parent = nullptr
        );

        ~RegionManager() override;

        /**
         * Method you can use to set the Inesonic authentication secret.
         *
         * \param[in] newSecret The new secret to be used.  The secret must be the length prescribed by the
         *                      constant \ref inesonicSecretLength.
         */
        void setSecret(const QByteArray& newSecret);

    private:
        /**
         * The region/get handler.
         */
        class RegionGet:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret            The secret to use for this handler.
                 *
                 * \param[in] regionDatabaseApi Class used to manage regions entries in the database.
                 */
                RegionGet(const QByteArray& secret, Regions* regionDatabaseApi);

                ~RegionGet() override;

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
                Regions* currentRegions;
        };

        /**
         * The region/create handler.
         */
        class RegionCreate:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret            The secret to use for this handler.
                 *
                 * \param[in] regionDatabaseApi Class used to manage regions entries in the database.
                 */
                RegionCreate(const QByteArray& secret, Regions* regionDatabaseApi);

                ~RegionCreate() override;

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
                Regions* currentRegions;
        };

        /**
         * The region/modify handler.
         */
        class RegionModify:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret            The secret to use for this handler.
                 *
                 * \param[in] regionDatabaseApi Class used to manage regions entries in the database.
                 */
                RegionModify(const QByteArray& secret, Regions* regionDatabaseApi);

                ~RegionModify() override;

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
                Regions* currentRegions;
        };

        /**
         * The region/delete handler.
         */
        class RegionDelete:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret            The secret to use for this handler.
                 *
                 * \param[in] regionDatabaseApi Class used to manage regions entries in the database.
                 */
                RegionDelete(const QByteArray& secret, Regions* regionDatabaseApi);

                ~RegionDelete() override;

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
                Regions* currentRegions;
        };

        /**
         * The region/list handler.
         */
        class RegionList:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret            The secret to use for this handler.
                 *
                 * \param[in] regionDatabaseApi Class used to manage regions entries in the database.
                 */
                RegionList(const QByteArray& secret, Regions* regionDatabaseApi);

                ~RegionList() override;

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
                Regions* currentRegions;
        };

        /**
         * The region/get handler.
         */
        RegionGet regionGet;

        /**
         * The region/create handler.
         */
        RegionCreate regionCreate;

        /**
         * The region/modify handler.
         */
        RegionModify regionModify;

        /**
         * The region/delete handler.
         */
        RegionDelete regionDelete;

        /**
         * The region/list handler.
         */
        RegionList regionList;
};

#endif
