/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref EventManager class.
***********************************************************************************************************************/

#include <QObject>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <limits>

#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "log.h"
#include "latency_entry.h"
#include "event.h"
#include "events.h"
#include "monitors.h"
#include "event_processor.h"
#include "event_manager.h"

/***********************************************************************************************************************
* EventManager::EventReport
*/

EventManager::EventReport::EventReport(
        const QByteArray& secret,
        EventProcessor*   eventProcessor,
        Monitors*         monitorDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentEventProcessor(
        eventProcessor
    ),currentMonitors(
        monitorDatabaseApi
    ) {}


EventManager::EventReport::~EventReport() {}


RestApiInV1::JsonResponse EventManager::EventReport::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.contains("monitor_id")       &&
            object.contains("timestamp")        &&
            object.contains("event_type")       &&
            object.contains("monitor_status")   &&
            object.contains("message")          &&
            (object.size() == 5            ||
             (object.size() == 6      &&
              object.contains("hash")    )    )    ) {
            QJsonObject responseObject;

            QByteArray hash;
            bool       success = true;
            if (object.size() == 6) {
                QString                      hashString = object.value("hash").toString();
                QByteArray::FromBase64Result hashResult = QByteArray::fromBase64Encoding(hashString.toLatin1());

                success = (hashResult.decodingStatus == QByteArray::Base64DecodingStatus::Ok);
                if (success) {
                    hash = hashResult.decoded;
                }
            }

            if (success) {
                QString message         = object.value("message").toString();
                double  monitorIdDouble = object.value("monitor_id").toDouble(-1);

                if (monitorIdDouble > 0 && monitorIdDouble < 0xFFFFFFFF) {
                    Events::MonitorId monitorId = static_cast<Events::MonitorId>(monitorIdDouble);
                    Monitor monitor = currentMonitors->getMonitor(monitorId, threadId);
                    if (monitor.isValid()) {
                        Events::CustomerId customerId      = monitor.customerId();
                        double             timestampDouble = object.value("timestamp").toDouble(-1);
                        if (timestampDouble >= LatencyEntry::startOfZoranEpoch                &&
                            timestampDouble <= (LatencyEntry::startOfZoranEpoch + 0xFFFFFFFF)    ) {
                            unsigned long long timestamp = static_cast<unsigned long long>(timestampDouble);
                            QString            eventTypeString = object.value("event_type").toString();
                            Events::EventType  eventType       = Event::toEventType(eventTypeString, &success);
                            if (success) {
                                QString monitorStatusString = object.value("monitor_status").toString();
                                Monitor::MonitorStatus monitorStatus = Monitor::toMonitorStatus(
                                    monitorStatusString,
                                    &success
                                );

                                if (success) {
                                    logWrite(
                                        QString(
                                            "Received event customer %1, type %2, monitor %3, timestamp %4, hash %5"
                                        ).arg(customerId)
                                         .arg(eventTypeString)
                                         .arg(monitorId)
                                         .arg(timestamp)
                                         .arg(QString::fromLocal8Bit(hash.toHex()))
                                    );
                                    success = currentEventProcessor->reportEvent(
                                        customerId,
                                        monitorId,
                                        timestamp,
                                        eventType,
                                        monitorStatus,
                                        message,
                                        hash,
                                        threadId
                                    );

                                    if (success) {
                                        responseObject.insert("status", "OK");
                                    } else {
                                        responseObject.insert("status", "failed to report event");
                                    }
                                } else {
                                    responseObject.insert("status", "failed, invalid monitor status");
                                }
                            } else {
                                responseObject.insert("status", "failed, invalid event type");
                            }
                        } else {
                            responseObject.insert("status", "failed, invalid timestamp");
                        }
                    } else {
                        // Polling server messages can be delayed until after we delete a monitor so responding with
                        // a failed response creates a race condition that introduces an infinte loop.  We address this
                        // by silently ignoring messages from the polling server for non-existent monitors.

                        logWrite(QString("Ignoring event for nonexistent monitor ID %1").arg(monitorId));
                        responseObject.insert("status", "OK");
                    }
                } else {
                    responseObject.insert("status", "failed, invalid monitor ID");
                }
            } else {
                responseObject.insert("status", "failed, invalid MD5 sum value");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* EventManager::EventStatus
*/

EventManager::EventStatus::EventStatus(
        const QByteArray& secret,
        Events*           eventDatabaseApi,
        Monitors*         monitorDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentEvents(
        eventDatabaseApi
    ),currentMonitors(
        monitorDatabaseApi
    ) {}


EventManager::EventStatus::~EventStatus() {}


RestApiInV1::JsonResponse EventManager::EventStatus::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject        responseObject;

        QJsonObject        object         = request.object();
        unsigned           numberFields   = 0;
        bool               success        = true;
        Events::CustomerId customerId     = Events::invalidCustomerId;
        Events::MonitorId  monitorId      = Monitor::invalidMonitorId;

        if (object.contains("monitor_id")) {
            double monitorIdDouble = object.value("monitor_id").toDouble(-1);
            if (monitorIdDouble > 0 && monitorIdDouble <= 0xFFFFFFFF) {
                monitorId = static_cast<Events::MonitorId>(monitorIdDouble);
            } else {
                success = false;
                responseObject.insert("status", "failed, invalid monitor ID");
            }

            ++numberFields;
        }

        if (object.contains("customer_id")) {
            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                customerId = static_cast<Events::MonitorId>(customerIdDouble);
            } else {
                success = false;
                responseObject.insert("status", "failed, invalid customer ID");
            }

            ++numberFields;
        }

        if (success) {
            if (numberFields == static_cast<unsigned>(object.size())) {
                if (customerId == Events::invalidCustomerId || monitorId == Monitor::invalidMonitorId) {
                    QJsonObject statusObject;
                    if (monitorId != Monitor::invalidMonitorId) {
                        Events::MonitorStatus monitorStatus = currentEvents->monitorStatus(monitorId, threadId);
                        QString               statusString  = Monitor::toString(monitorStatus);

                        statusObject.insert(QString::number(monitorId), statusString.toLower());
                    } else {
                        Monitors::MonitorList monitorList = currentMonitors->getMonitorsByUserOrder(
                            customerId,
                            threadId
                        );

                        for (  Monitors::MonitorList::const_iterator it  = monitorList.constBegin(),
                                                                     end = monitorList.constEnd()
                             ; it != end
                             ; ++it
                            ) {
                            Monitor::MonitorId    monitorId = it->monitorId();
                            Events::MonitorStatus monitorStatus = currentEvents->monitorStatus(monitorId, threadId);
                            QString               statusString  = Monitor::toString(monitorStatus);

                            statusObject.insert(QString::number(monitorId), statusString.toLower());
                        }
                    }

                    responseObject.insert("status", "OK");
                    responseObject.insert("monitors", statusObject);
                } else {
                    responseObject.insert("status", "failed, customer ID or monitor ID, not both");
                }

                response = RestApiInV1::JsonResponse(responseObject);
            } else {
                // Give the customer a bad response.
            }
        } else {
            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* EventManager::EventGet
*/

EventManager::EventGet::EventGet(
        const QByteArray& secret,
        Events*          eventDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentEvents(
        eventDatabaseApi
    ) {}


EventManager::EventGet::~EventGet() {}


RestApiInV1::JsonResponse EventManager::EventGet::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject        responseObject;

        QJsonObject        object         = request.object();
        unsigned           numberFields   = 0;
        bool               success        = true;
        Events::CustomerId customerId     = Events::invalidCustomerId;
        Events::MonitorId  monitorId      = Monitor::invalidMonitorId;
        unsigned long long startTimestamp = 0;
        unsigned long long endTimestamp   = std::numeric_limits<unsigned long long>::max();

        if (object.contains("monitor_id")) {
            double monitorIdDouble = object.value("monitor_id").toDouble(-1);
            if (monitorIdDouble > 0 && monitorIdDouble <= 0xFFFFFFFF) {
                monitorId = static_cast<Events::MonitorId>(monitorIdDouble);
            } else {
                success = false;
                responseObject.insert("status", "failed, invalid monitor ID");
            }

            ++numberFields;
        }

        if (object.contains("customer_id")) {
            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                customerId = static_cast<Events::MonitorId>(customerIdDouble);
            } else {
                success = false;
                responseObject.insert("status", "failed, invalid customer ID");
            }

            ++numberFields;
        }

        if (object.contains("start_timestamp")) {
            double startTimestampDouble = object.value("start_timestamp").toDouble(-1);
            if (startTimestampDouble >= 0) {
                startTimestamp = static_cast<Events::MonitorId>(startTimestampDouble);
            } else {
                success = false;
                responseObject.insert("status", "failed, invalid start timestamp");
            }

            ++numberFields;
        }

        if (object.contains("end_timestamp")) {
            double endTimestampDouble = object.value("end_timestamp").toDouble(-1);
            if (endTimestampDouble >= 0) {
                endTimestamp = static_cast<Events::MonitorId>(endTimestampDouble);
            } else {
                success = false;
                responseObject.insert("status", "failed, invalid end timestamp");
            }

            ++numberFields;
        }

        if (success) {
            if (numberFields == static_cast<unsigned>(object.size())) {
                if (customerId == Events::invalidCustomerId || monitorId == Monitor::invalidMonitorId) {
                    Events::EventList events;
                    if (monitorId != Monitor::invalidMonitorId) {
                        events = currentEvents->getEventsByMonitor(monitorId, startTimestamp, endTimestamp, threadId);
                    } else {
                        events = currentEvents->getEventsByCustomer(
                            customerId,
                            startTimestamp,
                            endTimestamp,
                            threadId
                        );
                    }

                    responseObject.insert("status", "OK");
                    responseObject.insert("events", convertToJson(events, true, true));
                } else {
                    responseObject.insert("status", "failed, customer ID or monitor ID, not both");
                }

                response = RestApiInV1::JsonResponse(responseObject);
            } else {
                // Give the customer a bad response.
            }
        } else {
            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* EventManager
*/

const QString EventManager::eventReportPath("/event/report");
const QString EventManager::eventStatusPath("/event/status");
const QString EventManager::eventGetPath("/event/get");

EventManager::EventManager(
        RestApiInV1::Server* restApiServer,
        Events*              eventDatabaseApi,
        Monitors*            monitorDatabaseApi,
        EventProcessor*      eventProcessor,
        const QByteArray&    secret,
        QObject*             parent
    ):QObject(
        parent
    ),eventReport(
        secret,
        eventProcessor,
        monitorDatabaseApi
    ),eventStatus(
        secret,
        eventDatabaseApi,
        monitorDatabaseApi
    ),eventGet(
        secret,
        eventDatabaseApi
    ) {
    restApiServer->registerHandler(&eventReport, RestApiInV1::Handler::Method::POST, eventReportPath);
    restApiServer->registerHandler(&eventStatus, RestApiInV1::Handler::Method::POST, eventStatusPath);
    restApiServer->registerHandler(&eventGet, RestApiInV1::Handler::Method::POST, eventGetPath);
}


EventManager::~EventManager() {}


void EventManager::setSecret(const QByteArray& newSecret) {
    eventReport.setSecret(newSecret);
    eventStatus.setSecret(newSecret);
    eventGet.setSecret(newSecret);
}
