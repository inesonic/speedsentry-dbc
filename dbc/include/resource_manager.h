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
* This header defines the \ref ResourceManager class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>
#include <rest_api_in_v1_inesonic_binary_rest_handler.h>

#include "rest_helpers.h"

class Resources;
class ResourcePlotter;

/**
 * Class that support a set of REST endpoints used to manage resources.
 */
class ResourceManager:public QObject {
    Q_OBJECT

    public:
        /**
         * Path used to check if resource data is available.
         */
        static const QString resourceAvailablePath;

        /**
         * Path used to create a new resource.
         */
        static const QString resourceCreatePath;

        /**
         * Path used to obtain a list of all resources.
         */
        static const QString resourceListPath;

        /**
         * Path used to purge older resources.
         */
        static const QString resourcePurgePath;

        /**
         * Path used to plot resource data.
         */
        static const QString resourcePlotPath;

        /**
         * Constructor
         *
         * \param[in] restApiServer       The REST API server instance.
         *
         * \param[in] resourceDatabaseApi Class used to manage resources entries in the database.
         *
         * \param[in] resourcePlotter     Class used to plot resource data.
         *
         * \param[in[ secret              The incoming data secret.
         *
         * \param[in] parent              Pointer to the parent object.
         */
        ResourceManager(
            RestApiInV1::Server* restApiServer,
            Resources*           resourceDatabaseApi,
            ResourcePlotter*     resourcePlotter,
            const QByteArray&    secret,
            QObject*             parent = nullptr
        );

        ~ResourceManager() override;

        /**
         * Method you can use to set the Inesonic authentication secret.
         *
         * \param[in] newSecret The new secret to be used.  The secret must be the length prescribed by the
         *                      constant \ref inesonicSecretLength.
         */
        void setSecret(const QByteArray& newSecret);

    private:
        /**
         * The resource/available handler.
         */
        class ResourceAvailable:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret            The secret to use for this handler.
                 *
                 * \param[in] resourceDatabaseApi Class used to manage resources entries in the database.
                 */
                ResourceAvailable(const QByteArray& secret, Resources* resourceDatabaseApi);

                ~ResourceAvailable() override;

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
                 * The current resource database API.
                 */
                Resources* currentResources;
        };

        /**
         * The resource/create handler.
         */
        class ResourceCreate:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret            The secret to use for this handler.
                 *
                 * \param[in] resourceDatabaseApi Class used to manage resources entries in the database.
                 */
                ResourceCreate(const QByteArray& secret, Resources* resourceDatabaseApi);

                ~ResourceCreate() override;

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
                 * The current resource database API.
                 */
                Resources* currentResources;
        };

        /**
         * The resource/list handler.
         */
        class ResourceList:public RestApiInV1::InesonicRestHandler, public RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret            The secret to use for this handler.
                 *
                 * \param[in] resourceDatabaseApi Class used to manage resources entries in the database.
                 */
                ResourceList(const QByteArray& secret, Resources* resourceDatabaseApi);

                ~ResourceList() override;

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
                 * The current resource database API.
                 */
                Resources* currentResources;
        };

        /**
         * The resource/purge handler.
         */
        class ResourcePurge:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret            The secret to use for this handler.
                 *
                 * \param[in] resourceDatabaseApi Class used to manage resources entries in the database.
                 */
                ResourcePurge(const QByteArray& secret, Resources* resourceDatabaseApi);

                ~ResourcePurge() override;

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
                 * The current resource database API.
                 */
                Resources* currentResources;
        };

        /**
         * The resource/plot handler.
         */
        class ResourcePlot:public RestApiInV1::InesonicBinaryRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret          The secret to use for this handler.
                 *
                 * \param[in] resourcePlotter Class used to generate plots of resource data.
                 */
                ResourcePlot(const QByteArray& secret, ResourcePlotter* resourcePlotter);

                ~ResourcePlot() override;

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
                 * The current resource plotter API.
                 */
                ResourcePlotter* currentResourcePlotter;
        };

        /**
         * The resource/available handler.
         */
        ResourceAvailable resourceAvailable;

        /**
         * The resource/create handler.
         */
        ResourceCreate resourceCreate;

        /**
         * The resource/list handler.
         */
        ResourceList resourceList;

        /**
         * The resource/purge handler.
         */
        ResourcePurge resourcePurge;

        /**
         * The resource/plot handler.
         */
        ResourcePlot resourcePlot;
};

#endif
