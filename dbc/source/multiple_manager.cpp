/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref MultipleManager class.
***********************************************************************************************************************/

#include <QObject>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "log.h"
#include "host_schemes.h"
#include "monitors.h"
#include "events.h"
#include "latency_plotter.h"
#include "rest_helpers.h"
#include "multiple_manager.h"

/***********************************************************************************************************************
* MultipleManager::MultipleList
*/

MultipleManager::MultipleList::MultipleList(
        const QByteArray& secret,
        HostSchemes*      hostSchemeDatabaseApi,
        Monitors*         monitorDatabaseApi,
        Events*           eventDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentHostSchemes(
        hostSchemeDatabaseApi
    ),currentMonitors(
        monitorDatabaseApi
    ),currentEvents(
        eventDatabaseApi
    ) {}


MultipleManager::MultipleList::~MultipleList() {}


RestApiInV1::JsonResponse MultipleManager::MultipleList::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject requestObject = request.object();
        if (requestObject.contains("customer_id")) {
            QJsonObject responseObject;

            double customerIdDouble = requestObject.value("customer_id").toDouble(-1);
            if (customerIdDouble >= 1 && customerIdDouble <= 0xFFFFFFFF) {
                CustomerCapabilities::CustomerId customerId = static_cast<CustomerCapabilities::CustomerId>(
                    customerIdDouble
                );

                HostSchemes::HostSchemeHash customerHostSchemes = currentHostSchemes->getHostSchemes(
                    static_cast<HostSchemes::CustomerId>(customerId),
                    threadId
                );

                Monitors::MonitorList monitors = currentMonitors->getMonitorsByUserOrder(
                    static_cast<CustomerCapabilities::CustomerId>(customerId),
                    threadId
                );

                Events::MonitorStatusByMonitorId monitorStatusMultiple = currentEvents->monitorStatusByCustomerId(
                    static_cast<CustomerCapabilities::CustomerId>(customerId),
                    threadId
                );

                Events::EventList events = currentEvents->getEventsByCustomer(
                    static_cast<CustomerCapabilities::CustomerId>(customerId),
                    0,
                    static_cast<unsigned long long>(-1),
                    threadId
                );

                QJsonObject statusObject;
                for (  Events::MonitorStatusByMonitorId::const_iterator it  = monitorStatusMultiple.constBegin(),
                                                                        end = monitorStatusMultiple.constEnd()
                     ; it != end
                     ; ++it
                    ) {
                    statusObject.insert(QString::number(it.key()), Monitor::toString(it.value()).toLower());
                }

                responseObject.insert("status", "OK");
                responseObject.insert("host_schemes", convertToJson(customerHostSchemes, false));
                responseObject.insert("monitors", convertToJson(monitors, false));
                responseObject.insert("events", convertToJson(events, false, false));
                responseObject.insert("monitor_status", statusObject);
            } else {
                responseObject.insert("status", "failed");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* MultipleManager
*/

const QString MultipleManager::multipleListPath("/multiple/list");

MultipleManager::MultipleManager(
        RestApiInV1::Server* restApiServer,
        HostSchemes*         hostSchemeDatabaseApi,
        Monitors*            monitorDatabaseApi,
        Events*              eventDatabaseApi,
        LatencyPlotter*      /* latencyPlotter */,
        const QByteArray&    secret,
        QObject*             parent
    ):QObject(
        parent
    ),multipleList(
        secret,
        hostSchemeDatabaseApi,
        monitorDatabaseApi,
        eventDatabaseApi
    ) {
    restApiServer->registerHandler(&multipleList, RestApiInV1::Handler::Method::POST, multipleListPath);
}


MultipleManager::~MultipleManager() {}


void MultipleManager::setSecret(const QByteArray& newSecret) {
    multipleList.setSecret(newSecret);
}
