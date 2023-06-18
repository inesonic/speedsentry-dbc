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
* This header defines the \ref OutboundRestApiFactory class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef OUTBOUND_REST_API_FACTORY_H
#define OUTBOUND_REST_API_FACTORY_H

#include <QObject>
#include <QHash>
#include <QMutex>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "host_scheme.h"
#include "outbound_rest_api.h"

/**
 * Class that provides a factory and cache of \ref OutboundRestApi instances that are tied to specific systems.
 *
 * To use, instantiate an instance and then call the \ref startReporting method (or trigger it).
 */
class OutboundRestApiFactory:public QObject {
    Q_OBJECT

    public:
        /**
         * The server scheme to be used for servers.
         */
        enum class Scheme {
            /**
             * Indicates HTTP scheme.
             */
            HTTP,

            /**
             * Indicates HTTPS scheme.
             */
            HTTPS
        };

        /**
         * The default user agent string.
         */
        static const QString defaultUserAgent;

        /**
         * The default slug to use to determine time deltas between us and a remote server.
         */
        static const QString defaultTimeDeltaSlug;

        /**
         * Constructor
         *
         * \param[in] networkAccessManager The network settings manager.
         *
         * \param[in] scheme               The scheme to be used.
         *
         * \param[in] port                 The port to be used.
         *
         * \param[in] timeDeltaSlug        The URL used to determine time deltas.
         *
         * \param[in] parent               Pointer to the parent object.
         */
        OutboundRestApiFactory(
            QNetworkAccessManager* networkAccessManager,
            Scheme                 scheme,
            unsigned short         port,
            const QString&         timeDeltaSlug = defaultTimeDeltaSlug,
            QObject*               parent = Q_NULLPTR
        );

        /**
         * Method you can use to set the default secret.
         *
         * \param[in] newDefaultSecret The new default secret to be used.
         */
        void setDefaultSecret(const QByteArray& newDefaultSecret);

        /**
         * Method you can use to get the current network access manager.
         *
         * \return Returns a pointer to the current network access manager.
         */
        QNetworkAccessManager* networkAccessManager() const;

        /**
         * Method you can use to set the scheme to use for servers.
         *
         * \param[in] newScheme The new server scheme.
         */
        void setScheme(Scheme newScheme);

        /**
         * Method you can use to determine the current scheme to use for servers.
         *
         * \return Returns the current server's scheme and host.
         */
        Scheme scheme() const;

        /**
         * Method you can use to set the port used by the polling servers.
         *
         * \param[in] newPort The new port number.  A port number of zero will cause the port to be omitted from the
         *                    created authority.
         */
        void setPort(unsigned short newPort);

        /**
         * Method you can use to obtain the port used by the polling servers.
         *
         * \return Returns the port to be used by the polling servers.
         */
        unsigned short port() const;

        /**
         * Method you can use to set the user agent string.
         *
         * \param[in] newUserAgent The new user agent string.
         */
        void setUserAgent(const QString& newUserAgent);

        /**
         * Method you can use to obtain the current user agent string.
         *
         * \return Returns the current user agent string
         */
        const QString& userAgent() const;

        /**
         * Method you can use to set the server's time delta slug.
         *
         * \param[in] newTimeDeltaSlug The slug to use to measure time deltas.
         */
        void setTimeDeltaSlug(const QString& newTimeDeltaSlug);

        /**
         * Method you can use to obtain the current time delta slug.
         *
         * \return Returns the currently selected time delta slug.
         */
        const QString& timeDeltaSlug() const;

        /**
         * Method you can use to send a message to a remote host.
         *
         * \param[in] serverIdentifier The identifier used to address this server.
         *
         * \param[in] endpoint         The endpoint to send the message to.
         *
         * \param[in] message          The message to be sent.
         *
         * \param[in] logText          Text to be dumped to stdout on successful completion.
         */
        void postMessage(
            const QString&       serverIdentifier,
            const QString&       endpoint,
            const QJsonDocument& message,
            const QString&       logText
        );

        /**
         * Method you can use to send a message to a remote host.
         *
         * \param[in] serverIdentifier The identifier used to address this server.
         *
         * \param[in] endpoint         The endpoint to send the message to.
         *
         * \param[in] message          The message to be sent.
         *
         * \param[in] logText          Text to be dumped to stdout on successful completion.
         */
        void postMessage(
            const QString&      serverIdentifier,
            const QString&      endpoint,
            const QJsonObject&  message,
            const QString&      logText
        );

        /**
         * Method you can use to send a message to a remote host.  This version will trigger a callback to an old
         * style Qt slot.
         *
         * \param[in] serverIdentifier The identifier used to address this server.
         *
         * \param[in] endpoint         The endpoint to send the message to.
         *
         * \param[in] message          The message to be sent.
         *
         * \param[in] logText          Text to be dumped to stdout on successful completion.
         *
         * \param[in] context          Context data send to the receiver.
         *
         * \param[in] receiver         The class receiving the response.
         *
         * \param[in] slot             The receiver slot.
         */
        void postMessage(
            const QString&       serverIdentifier,
            const QString&       endpoint,
            const QJsonDocument& message,
            const QString&       logText,
            void*                context,
            QObject*             receiver,
            const char*          slot
        );

        /**
         * Method you can use to send a message to a remote host.  This version will trigger a callback to an old
         * style Qt slot.
         *
         * \param[in] serverIdentifier The identifier used to address this server.
         *
         * \param[in] endpoint         The endpoint to send the message to.
         *
         * \param[in] message          The message to be sent.
         *
         * \param[in] logText          Text to be dumped to stdout on successful completion.
         *
         * \param[in] context          Context data send to the receiver.
         *
         * \param[in] receiver         The class receiving the response.
         *
         * \param[in] slot             The receiver slot.
         */
        void postMessage(
            const QString&      serverIdentifier,
            const QString&      endpoint,
            const QJsonObject&  message,
            const QString&      logText,
            void*               context,
            QObject*            receiver,
            const char*         slot
        );

        /**
         * Method you can use to send a message to a remote host.
         *
         * \param[in] serverIdentifier The identifier used to address this server.
         *
         * \param[in] endpoint         The endpoint to send the message to.
         *
         * \param[in] logText          Text to be dumped to stdout on successful completion.
         */
        void postMessage(const QString& serverIdentifier, const QString& endpoint, const QString& logText);

        /**
         * Method you can use to send a message to a remote host.  This version will trigger a callback to an old
         * style Qt slot.
         *
         * \param[in] serverIdentifier The identifier used to address this server.
         *
         * \param[in] endpoint         The endpoint to send the message to.
         *
         * \param[in] logText          Text to be dumped to stdout on successful completion.
         *
         * \param[in] context          Context data send to the receiver.
         *
         * \param[in] receiver         The class receiving the response.
         *
         * \param[in] slot             The receiver slot.
         */
        void postMessage(
            const QString& serverIdentifier,
            const QString& endpoint,
            const QString& logText,
            void*          context,
            QObject*       receiver,
            const char*    slot
        );

    signals:
        /**
         * Signal that queues up a request to send a message to a remote host.  This version will trigger a callback to
         * an old style Qt slot.
         *
         * \param[in] serverIdentifier The identifier used to address this server.
         *
         * \param[out] endpoint        The endpoint to send the message to.
         *
         * \param[out] message         The message to be sent.
         *
         * \param[out] logText         Text to be dumped to stdout on successful completion.
         *
         * \param[out] context         Context data send to the receiver.
         *
         * \param[out] receiver        The class receiving the response.
         *
         * \param[out] slot            The receiver slot.
         */
        void postMessageRequested(
            const QString&       serverIdentifier,
            const QString&       endpoint,
            const QJsonDocument& message,
            const QString&       logText,
            void*                context,
            QObject*             receiver,
            const char*          slot
        );

    private slots:
        /**
         * Slot that performs the work of posting messages.
         *
         * \param[in] serverIdentifier The identifier used to address this server.
         *
         * \param[in] endpoint         The endpoint to send the message to.
         *
         * \param[in] message          The message to be sent.
         *
         * \param[in] logText          Text to be dumped to stdout on successful completion.
         *
         * \param[in] context          Context data send to the receiver.
         *
         * \param[in] receiver         The class receiving the response.
         *
         * \param[in] slot             The receiver slot.
         */
        void performPostMessage(
            const QString&       serverIdentifier,
            const QString&       endpoint,
            const QJsonDocument& message,
            const QString&       logText,
            void*                context,
            QObject*             receiver,
            const char*          slot
        );

        /**
         * Slot that is triggered to expunge an outbound REST API.
         *
         * \param[in] outboundRestApi The outbound REST API to be expunged.
         */
        void expungeOutboundRestApi(OutboundRestApi* outboundRestApi);

    private:
        /**
         * Method you can use to obtain a server instance for a given host address.
         *
         * \param[in] serverIdentifier The identifier used to address this server.
         *
         * \return Returns a server instance tied to this host address.
         */
        OutboundRestApi* outboundRestApi(const QString& serverIdentifier);

        /**
         * Method that is triggered by a server to indicate that it should be expunged.
         *
         * \param[in] schemeAndHost The server scheme and host.
         */
        void expungeServer(const QUrl& schemeAndHost);

        /**
         * Mutex used to keep all operations fully thread safe.
         */
        QMutex accessMutex;

        /**
         * Hash table of servers by server identifier.
         */
        QHash<QString, OutboundRestApi*> outboundRestApiByServerIdentifier;

        /**
         * Hash table of host addresses by server.
         */
        QHash<OutboundRestApi*, QString> serverIdentifierByOutboundRestApi;

        /**
         * The network access manager to be used by our servers.
         */
        QNetworkAccessManager* currentNetworkAccessManager;

        /**
         * The scheme to apply to server identifier.
         */
        Scheme currentScheme;

        /**
         * The port to apply to the polling server host addresses.
         */
        unsigned short currentPort;

        /**
         * The default time delta slug to use for time correction.
         */
        QString currentTimeDeltaSlug;

        /**
         * The user agent string to use.
         */
        QString currentUserAgent;

        /**
         * The default secret to use with new servers.
         */
        QByteArray currentDefaultSecret;
};

#endif
