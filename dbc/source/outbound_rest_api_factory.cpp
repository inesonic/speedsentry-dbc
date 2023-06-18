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
* This header implements the \ref OutboundRestApiFactory class.
***********************************************************************************************************************/

#include <QObject>
#include <QUrl>
#include <QHash>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QMutex>
#include <QMutexLocker>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <rest_api_out_v1_server.h>
#include <rest_api_out_v1_inesonic_rest_handler.h>

#include "log.h"
#include "metatypes.h"
#include "servers.h"
#include "outbound_rest_api.h"
#include "outbound_rest_api_factory.h"

const QString  OutboundRestApiFactory::defaultUserAgent("InesonicBot");
const QString  OutboundRestApiFactory::defaultTimeDeltaSlug("/td");

OutboundRestApiFactory::OutboundRestApiFactory(
        QNetworkAccessManager* networkAccessManager,
        Scheme                 scheme,
        unsigned short         port,
        const QString&         timeDeltaSlug,
        QObject*               parent
    ):QObject(
        parent
    ),currentNetworkAccessManager(
        networkAccessManager
    ),currentScheme(
        scheme
    ),currentPort(
        port
    ),currentTimeDeltaSlug(
        timeDeltaSlug
    ),currentUserAgent(
        RestApiOutV1::Server::defaultUserAgent
    ) {
    connect(this, &OutboundRestApiFactory::postMessageRequested, this, &OutboundRestApiFactory::performPostMessage);
}


OutboundRestApi::~OutboundRestApi() {}


void OutboundRestApiFactory::setDefaultSecret(const QByteArray& newDefaultSecret) {
    currentDefaultSecret = newDefaultSecret;
}


QNetworkAccessManager* OutboundRestApiFactory::networkAccessManager() const {
    return currentNetworkAccessManager;
}


void OutboundRestApiFactory::setScheme(OutboundRestApiFactory::Scheme newScheme) {
    currentScheme = newScheme;
}


OutboundRestApiFactory::Scheme OutboundRestApiFactory::scheme() const {
    return currentScheme;
}


void OutboundRestApiFactory::setPort(unsigned short newPort) {
    currentPort = newPort;
}


unsigned short OutboundRestApiFactory::port() const {
    return currentPort;
}


void OutboundRestApiFactory::setUserAgent(const QString& newUserAgent) {
    currentUserAgent = newUserAgent;
}


const QString& OutboundRestApiFactory::userAgent() const {
    return currentUserAgent;
}


void OutboundRestApiFactory::setTimeDeltaSlug(const QString& newTimeDeltaSlug) {
    currentTimeDeltaSlug = newTimeDeltaSlug;
}


const QString& OutboundRestApiFactory::timeDeltaSlug() const {
    return currentTimeDeltaSlug;
}


void OutboundRestApiFactory::postMessage(
        const QString&       serverIdentifier,
        const QString&       endpoint,
        const QJsonDocument& message,
        const QString&       logText
    ) {
    postMessage(serverIdentifier, endpoint, message, logText, nullptr, nullptr, "");
}


void OutboundRestApiFactory::postMessage(
        const QString&      serverIdentifier,
        const QString&      endpoint,
        const QJsonObject&  message,
        const QString&      logText
    ) {
    postMessage(serverIdentifier, endpoint, QJsonDocument(message), logText);
}


void OutboundRestApiFactory::postMessage(
        const QString&       serverIdentifier,
        const QString&       endpoint,
        const QJsonDocument& message,
        const QString&       logText,
        void*                context,
        QObject*             receiver,
        const char*          slot
    ) {
    emit postMessageRequested(serverIdentifier, endpoint, message, logText, context, receiver, slot);
}


void OutboundRestApiFactory::postMessage(
        const QString&      serverIdentifier,
        const QString&      endpoint,
        const QJsonObject&  message,
        const QString&      logText,
        void*               context,
        QObject*            receiver,
        const char*         slot
    ) {
    postMessage(serverIdentifier, endpoint, QJsonDocument(message), logText, context, receiver, slot);
}


void OutboundRestApiFactory::postMessage(
        const QString& serverIdentifier,
        const QString& endpoint,
        const QString& logText
    ) {
    postMessage(serverIdentifier, endpoint, QJsonObject(), logText);
}


void OutboundRestApiFactory::postMessage(
        const QString&       serverIdentifier,
        const QString&       endpoint,
        const QString&       logText,
        void*                context,
        QObject*             receiver,
        const char*          slot
    ) {
    postMessage(serverIdentifier, endpoint, QJsonObject(), logText, context, receiver, slot);
}


void OutboundRestApiFactory::performPostMessage(
        const QString&       serverIdentifier,
        const QString&       endpoint,
        const QJsonDocument& message,
        const QString&       logText,
        void*                context,
        QObject*             receiver,
        const char*          slot
    ) {
    OutboundRestApi* api = outboundRestApi(serverIdentifier);
    api->postMessage(endpoint, message, logText, context, receiver, slot);
}


void OutboundRestApiFactory::expungeOutboundRestApi(OutboundRestApi* outboundRestApi) {
    QMutexLocker locker(&accessMutex);
    QString serverIdentifier = serverIdentifierByOutboundRestApi.value(outboundRestApi);
    outboundRestApiByServerIdentifier.remove(serverIdentifier);
    serverIdentifierByOutboundRestApi.remove(outboundRestApi);

    outboundRestApi->deleteLater();
}


OutboundRestApi* OutboundRestApiFactory::outboundRestApi(const QString& serverIdentifier) {
    QMutexLocker locker(&accessMutex);

    OutboundRestApi* result = outboundRestApiByServerIdentifier.value(serverIdentifier, nullptr);
    if (result == nullptr) {
        QString urlString = QString(currentScheme == Scheme::HTTP ? "http://" : "https://") + serverIdentifier;
        QUrl url(urlString);
        if (!url.isValid()) {
            logWrite(QString("Invalid URL %1").arg(urlString), true);
        }

        result = new OutboundRestApi(currentNetworkAccessManager, url, currentTimeDeltaSlug, true, this);
        result->setDefaultSecret(currentDefaultSecret);
        result->setUserAgent(currentUserAgent);

        connect(
            result,
            &OutboundRestApi::performGarbageCollection,
            this,
            &OutboundRestApiFactory::expungeOutboundRestApi
        );

        outboundRestApiByServerIdentifier.insert(serverIdentifier, result);
        serverIdentifierByOutboundRestApi.insert(result, serverIdentifier);
    }

    return result;
}
