/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref MonitorManager class.
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
#include "customer_capabilities.h"
#include "customers_capabilities.h"
#include "monitors.h"
#include "monitor_updater.h"
#include "rest_helpers.h"
#include "monitor_manager.h"

/***********************************************************************************************************************
* MonitorManager::MonitorGet
*/

MonitorManager::MonitorGet::MonitorGet(
        const QByteArray& secret,
        Monitors*         monitorDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentMonitors(
        monitorDatabaseApi
    ) {}


MonitorManager::MonitorGet::~MonitorGet() {}


RestApiInV1::JsonResponse MonitorManager::MonitorGet::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 1 && object.contains("monitor_id")) {
            QJsonObject responseObject;

            double monitorIdDouble = object.value("monitor_id").toDouble(-1);
            if (monitorIdDouble > 0 && monitorIdDouble <= 0xFFFFFFFF) {
                Monitor::MonitorId monitorId = static_cast<Monitor::MonitorId>(monitorIdDouble);
                Monitor monitor = currentMonitors->getMonitor(monitorId, threadId);

                if (monitor.isValid()) {
                    responseObject.insert("status", "OK");
                    responseObject.insert("monitor", convertToJson(monitor, true, true));
                } else {
                    responseObject.insert("status", "unknown monitor ID");
                }
            } else {
                responseObject.insert("status", "invalid monitor ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* MonitorManager::MonitorDelete
*/

MonitorManager::MonitorDelete::MonitorDelete(
        const QByteArray& secret,
        Monitors*         monitorDatabaseApi,
        MonitorUpdater*   monitorUpdater
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentMonitors(
        monitorDatabaseApi
    ),currentMonitorUpdater(
        monitorUpdater
    ) {}


MonitorManager::MonitorDelete::~MonitorDelete() {}


RestApiInV1::JsonResponse MonitorManager::MonitorDelete::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 1 && object.contains("monitor_id")) {
            double monitorIdDouble = object.value("monitor_id").toDouble(-1);
            if (monitorIdDouble > 0 && monitorIdDouble <= 0xFFFFFFFF) {
                Monitor::MonitorId monitorId = static_cast<Monitor::MonitorId>(monitorIdDouble);
                Monitor monitor = currentMonitors->getMonitor(monitorId, threadId);

                QJsonObject responseObject;
                if (monitor.isValid()) {
                    bool success = currentMonitorUpdater->deleteMonitor(monitor, threadId);
                    if (success) {
                        logWrite(QString("Deleted monitor %1").arg(monitorId), false);
                        responseObject.insert("status", "OK");
                    } else {
                        responseObject.insert("status", "delete failed");
                    }
                } else {
                    responseObject.insert("status", "invalid monitor ID");
                }

                response = RestApiInV1::JsonResponse(QJsonDocument(responseObject));
            }
        } else if (object.size() == 1 && object.contains("customer_id")) {
            QJsonObject responseObject;

            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                Monitor::MonitorId customerId = static_cast<Monitor::MonitorId>(customerIdDouble);
                bool success = currentMonitorUpdater->deleteCustomer(customerId, threadId);
                if (success) {
                    logWrite(QString("Deleted customer %1 via monitor/delete").arg(customerId), false);
                    responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "delete failed");
                }
            } else {
                responseObject.insert("status", "invalid monitor ID");
            }

            response = RestApiInV1::JsonResponse(QJsonDocument(responseObject));
        }
    }

    return response;
}

/***********************************************************************************************************************
* MonitorManager::MonitorList
*/

MonitorManager::MonitorList::MonitorList(
        const QByteArray& secret,
        Monitors*         monitorDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentMonitors(
        monitorDatabaseApi
    ) {}


MonitorManager::MonitorList::~MonitorList() {}


RestApiInV1::JsonResponse MonitorManager::MonitorList::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 0) {
            Monitors::MonitorList monitors = currentMonitors->getMonitorsByUserOrder(
                Monitors::invalidCustomerId,
                threadId
            );

            QJsonObject responseObject;
            responseObject.insert("status", "OK");
            responseObject.insert("data", convertToJson(monitors, true));

            response = RestApiInV1::JsonResponse(responseObject);
        } else if (object.size() == 1 && object.contains("customer_id")) {
            QJsonObject responseObject;

            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                Monitor::MonitorId customerId = static_cast<Monitor::MonitorId>(customerIdDouble);

                Monitors::MonitorList monitors = currentMonitors->getMonitorsByUserOrder(customerId, threadId);

                responseObject.insert("status", "OK");
                responseObject.insert("data", convertToJson(monitors, true));
            } else {
                responseObject.insert("status", "invalid customer ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* MonitorManager::MonitorUpdate
*/

MonitorManager::MonitorUpdate::MonitorUpdate(
        const QByteArray&      secret,
        Monitors*              monitorDatabaseApi,
        CustomersCapabilities* customerDatabaseApi,
        MonitorUpdater*        monitorUpdater
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentMonitors(
        monitorDatabaseApi
    ),currentCustomersCapabilities(
        customerDatabaseApi
    ),currentMonitorUpdater(
        monitorUpdater
    ) {}


MonitorManager::MonitorUpdate::~MonitorUpdate() {}


RestApiInV1::JsonResponse MonitorManager::MonitorUpdate::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 2 && object.contains("customer_id") && object.contains("data")) {
            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                Monitor::MonitorId customerId = static_cast<Monitor::MonitorId>(customerIdDouble);
                CustomerCapabilities capabilities = currentCustomersCapabilities->getCustomerCapabilities(
                    customerId,
                    false,
                    threadId
                );

                if (capabilities.isValid()) {
                    QJsonValue monitorDataValue = object.value("data");

                    MonitorUpdater::Errors         errors;
                    MonitorUpdater::MonitorEntries monitorEntries;
                    if (monitorDataValue.isArray()) {
                        monitorEntries = convertToMonitorEntries(errors, monitorDataValue.toArray());
                    } else if (monitorDataValue.isObject()) {
                        monitorEntries = convertToMonitorEntries(errors, monitorDataValue.toObject());
                    }

                    if (errors.isEmpty()) {
                        errors = currentMonitorUpdater->update(capabilities, monitorEntries, threadId);
                        if (errors.isEmpty()) {
                            logWrite(QString("Updated monitor settings for customer %1").arg(customerId), false);
                        }
                    }

                    response = RestApiInV1::JsonResponse(convertToJson(errors));
                } else {
                    QJsonObject responseObject;
                    responseObject.insert("status", "unknown customer ID");
                    response = RestApiInV1::JsonResponse(responseObject);
                }
            } else {
                QJsonObject responseObject;
                responseObject.insert("status", "invalid customer ID");
                response = RestApiInV1::JsonResponse(responseObject);
            }
        }
    }

    return response;
}

/***********************************************************************************************************************
* MonitorManager
*/

const QString MonitorManager::monitorGetPath("/monitor/get");
const QString MonitorManager::monitorDeletePath("/monitor/delete");
const QString MonitorManager::monitorListPath("/monitor/list");
const QString MonitorManager::monitorUpdatePath("/monitor/update");

MonitorManager::MonitorManager(
        RestApiInV1::Server*   restApiServer,
        Monitors*              monitorDatabaseApi,
        CustomersCapabilities* customerDatabaseApi,
        MonitorUpdater*        monitorUpdater,
        const QByteArray&      secret,
        QObject*               parent
    ):QObject(
        parent
    ),monitorGet(
        secret,
        monitorDatabaseApi
    ),monitorDelete(
        secret,
        monitorDatabaseApi,
        monitorUpdater
    ),monitorList(
        secret,
        monitorDatabaseApi
    ),monitorUpdate(
        secret,
        monitorDatabaseApi,
        customerDatabaseApi,
        monitorUpdater
    ) {
    restApiServer->registerHandler(&monitorGet, RestApiInV1::Handler::Method::POST, monitorGetPath);
    restApiServer->registerHandler(&monitorDelete, RestApiInV1::Handler::Method::POST, monitorDeletePath);
    restApiServer->registerHandler(&monitorList, RestApiInV1::Handler::Method::POST, monitorListPath);
    restApiServer->registerHandler(&monitorUpdate, RestApiInV1::Handler::Method::POST, monitorUpdatePath);
}


MonitorManager::~MonitorManager() {}


void MonitorManager::setSecret(const QByteArray& newSecret) {
    monitorGet.setSecret(newSecret);
    monitorDelete.setSecret(newSecret);
    monitorList.setSecret(newSecret);
    monitorUpdate.setSecret(newSecret);
}
