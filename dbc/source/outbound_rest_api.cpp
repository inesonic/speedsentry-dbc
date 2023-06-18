/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref OutboundRestApi class.
***********************************************************************************************************************/

#include <QObject>
#include <QCoreApplication>
#include <QUrl>
#include <QQueue>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <rest_api_out_v1_server.h>
#include <rest_api_out_v1_inesonic_rest_handler.h>

#include "log.h"
#include "outbound_rest_api.h"

OutboundRestApi::OutboundRestApi(
        QNetworkAccessManager* networkAccessManager,
        const QUrl&            authority,
        const QString&         timeDeltaSlug,
        bool                   garbageCollect,
        QObject*               parent
    ):RestApiOutV1::Server(
        networkAccessManager,
        authority,
        timeDeltaSlug,
        parent
    ),currentPerformGarbageCollection(
        garbageCollect
    ) {
    activeRequest = nullptr;

    connect(&eventTimer, &QTimer::timeout, this, &OutboundRestApi::startNextAction);
    connect(this, &OutboundRestApi::sendMessage, this, &OutboundRestApi::processSendMessage);

    eventTimer.setSingleShot(true);
    if (garbageCollect) {
        eventTimer.start(maximumIdleTimeSeconds * 1000);
        timerAction = TimerAction::GARBAGE_COLLECTION;
    } else {
        timerAction = TimerAction::NONE;
    }
}


void OutboundRestApi::postMessage(const QString& endpoint, const QJsonDocument& message, const QString& logText) {
    timerAction = TimerAction::NONE;
    emit sendMessage(endpoint, message, logText, nullptr, nullptr, QString());
}


void OutboundRestApi::postMessage(
        const QString&      endpoint,
        const QJsonObject&  message,
        const QString&      logText
    ) {
    postMessage(endpoint, QJsonDocument(message), logText);
}


void OutboundRestApi::postMessage(
        const QString&       endpoint,
        const QJsonDocument& message,
        const QString&       logText,
        void*                context,
        QObject*             receiver,
        const char*          slot
    ) {
    timerAction = TimerAction::NONE;
    emit sendMessage(endpoint, message, logText, context, receiver, QString::fromLocal8Bit(slot));
}


void OutboundRestApi::postMessage(
        const QString&      endpoint,
        const QJsonObject&  message,
        const QString&      logText,
        void*               context,
        QObject*            receiver,
        const char*         slot
    ) {
    postMessage(endpoint, QJsonDocument(message), logText, context, receiver, slot);
}


void OutboundRestApi::postMessage(const QString& endpoint, const QString& logText) {
    postMessage(endpoint, QJsonObject(), logText);
}


void OutboundRestApi::postMessage(
        const QString&       endpoint,
        const QString&       logText,
        void*                context,
        QObject*             receiver,
        const char*          slot
    ) {
    postMessage(endpoint, QJsonObject(), logText, context, receiver, slot);
}


void OutboundRestApi::processSendMessage(
        const QString&       endpoint,
        const QJsonDocument& message,
        const QString&       logText,
        void*                context,
        QObject*             receiver,
        const QString&       slot
    ) {
    pendingRequests.enqueue(Request(endpoint, message, logText, context, receiver, slot));
    if (activeRequest == nullptr) {
        startSend(pendingRequests.head());
    }
}


void OutboundRestApi::startNextAction() {
    switch (timerAction) {
        case TimerAction::NONE: {
            break;
        }

        case TimerAction::RETRY: {
            startSend((pendingRequests.head()));
            break;
        }

        case TimerAction::CLEANUP: {
            Q_ASSERT(!pendingRequests.isEmpty());

            const Request& request   = pendingRequests.head();
            QObject*       receiver = request.receiver();

            Q_ASSERT(receiver != nullptr);
            disconnect(this, SIGNAL(sendCallback(void*)), receiver, SLOT(sendCallback(void*)));

            startNext();

            break;
        }

        case TimerAction::GARBAGE_COLLECTION: {
            Q_ASSERT(pendingRequests.isEmpty());
            emit performGarbageCollection(this);
            break;
        }
    }
}


void OutboundRestApi::jsonResponse(const QJsonDocument& jsonData) {
    const Request& request = pendingRequests.head();

    QObject* receiver = request.receiver();
    if (receiver != nullptr) {
        connect(this, SIGNAL(sendCallback(void*)), receiver, SLOT(sendCallback(void*)));
        timerAction = TimerAction::CLEANUP;
        eventTimer.start(10); // Give 10mSec for the event to propagate.

        emit sendCallback(request.context());
    }

    if (jsonData.isObject()) {
        QJsonObject object = jsonData.object();
        QString     status = object.value("status").toString();
        if (status == QString("OK")) {
            logWrite(
                QString("Sent message to %1%2: %3")
                .arg(schemeAndHost().toString(), request.endpoint(), request.logText()),
                false
            );
        }
    }

    activeRequest->deleteLater();
    if (receiver == nullptr) {
        startNext();
    } else {
        activeRequest = nullptr;
    }
}


void OutboundRestApi::requestFailed(const QString& errorString) {
    const Request& request = pendingRequests.head();
    logWrite(
        QString("Failed to send to %1%2:%3 -- Retrying in %4 seconds.")
        .arg(schemeAndHost().toString(), request.endpoint(), errorString)
        .arg(retryInterval),
        true
    );

    timerAction = TimerAction::RETRY;
    eventTimer.start(retryInterval * 1000);

    activeRequest->deleteLater();
    activeRequest = nullptr;
}


void OutboundRestApi::startSend(const OutboundRestApi::Request& request) {
    activeRequest = new RestApiOutV1::InesonicRestHandler(this, this);

    connect(activeRequest, &RestApiOutV1::InesonicRestHandler::jsonResponse, this, &OutboundRestApi::jsonResponse);
    connect(activeRequest, &RestApiOutV1::InesonicRestHandler::requestFailed, this, &OutboundRestApi::requestFailed);

    activeRequest->post(request.endpoint(), request.message());
}


void OutboundRestApi::startNext() {
    pendingRequests.dequeue();

    if (!pendingRequests.isEmpty()) {
        startSend(pendingRequests.head());
    } else {
        activeRequest = nullptr;

        if (currentPerformGarbageCollection) {
            timerAction = TimerAction::GARBAGE_COLLECTION;
            eventTimer.start(maximumIdleTimeSeconds * 1000);
        } else {
            timerAction = TimerAction::NONE;
        }
    }
}
