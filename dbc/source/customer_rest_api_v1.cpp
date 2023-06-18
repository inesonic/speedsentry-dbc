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
* This header implements the \ref CustomerRestApiV1 class.
***********************************************************************************************************************/

#include <QObject>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QImage>
#include <QBuffer>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_customer_rest_handler.h>
#include <rest_api_in_v1_inesonic_customer_binary_rest_handler.h>

#include "customer_authenticator.h"
#include "customer_capabilities.h"
#include "customers_capabilities.h"
#include "host_scheme.h"
#include "host_schemes.h"
#include "monitor.h"
#include "monitors.h"
#include "monitor_updater.h"
#include "region.h"
#include "regions.h"
#include "server.h"
#include "servers.h"
#include "event.h"
#include "events.h"
#include "event_processor.h"
#include "latency_entry.h"
#include "aggregated_latency_entry.h"
#include "latency_interface_manager.h"
#include "plot_mailbox.h"
#include "latency_plotter.h"
#include "resource.h"
#include "active_resources.h"
#include "resources.h"
#include "resource_plotter.h"
#include "server_administrator.h"
#include "rest_helpers.h"
#include "customer_rest_api_v1.h"

/***********************************************************************************************************************
* CustomerRestApiV1::CustomerCapabilitiesGet
*/

CustomerRestApiV1::CapabilitiesGet::CapabilitiesGet(
        CustomerAuthenticator* customerAuthenticator,
        CustomersCapabilities* customerCapabilitiesDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),customersCapabilities(
        customerCapabilitiesDatabaseApi
    ) {}


CustomerRestApiV1::CapabilitiesGet::~CapabilitiesGet() {}


RestApiInV1::JsonResponse CustomerRestApiV1::CapabilitiesGet::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& /* request */,
        unsigned             threadId
    ) {
    QJsonObject responseObject;
    CustomerCapabilities capabilities = customersCapabilities->getCustomerCapabilities(
        static_cast<CustomerCapabilities::CustomerId>(customerId),
        false,
        threadId
    );

    if (capabilities.isValid()) {
        responseObject.insert("status", "OK");
        responseObject.insert("capabilities", convertToJson(capabilities, false, false, false));
    } else {
        responseObject.insert("status", "failed, no capabilities identified");
    }

    return RestApiInV1::JsonResponse(responseObject);
}

/***********************************************************************************************************************
* CustomerRestApiV1::HostsGet
*/

CustomerRestApiV1::HostsGet::HostsGet(
        CustomerAuthenticator* customerAuthenticator,
        HostSchemes*           hostSchemeDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentHostSchemes(
        hostSchemeDatabaseApi
    ) {}


CustomerRestApiV1::HostsGet::~HostsGet() {}


RestApiInV1::JsonResponse CustomerRestApiV1::HostsGet::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject requestObject = request.object();
        if (requestObject.contains("host_scheme_id")) {
            QJsonObject responseObject;

            double hostSchemeIdDouble = requestObject.value("host_scheme_id").toDouble(-1);
            if (hostSchemeIdDouble > 0 && hostSchemeIdDouble <= 0xFFFFFFFF) {
                HostScheme::HostSchemeId hostSchemeId = static_cast<HostScheme::HostSchemeId>(hostSchemeIdDouble);
                HostScheme hostScheme = currentHostSchemes->getHostScheme(hostSchemeId, threadId);

                if (hostScheme.isValid() && hostScheme.customerId() == customerId) {
                    responseObject.insert("status", "OK");
                    responseObject.insert("host_scheme", convertToJson(hostScheme, false));
                } else {
                   responseObject.insert("status", "unknown host/scheme ID");
                }
            } else {
                responseObject.insert("status", "invalid host/scheme ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerRestApiV1::HostsList
*/

CustomerRestApiV1::HostsList::HostsList(
        CustomerAuthenticator* customerAuthenticator,
        HostSchemes*           hostSchemeDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentHostSchemes(
        hostSchemeDatabaseApi
    ) {}


CustomerRestApiV1::HostsList::~HostsList() {}


RestApiInV1::JsonResponse CustomerRestApiV1::HostsList::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& /* request */,
        unsigned             threadId
    ) {
    QJsonObject responseObject;

    HostSchemes::HostSchemeHash customerHostSchemes = currentHostSchemes->getHostSchemes(
        static_cast<HostSchemes::CustomerId>(customerId),
        threadId
    );

    responseObject.insert("status", "OK");
    responseObject.insert("host_schemes", convertToJson(customerHostSchemes, false));

    return RestApiInV1::JsonResponse(responseObject);
}

/***********************************************************************************************************************
* CustomerRestApiV1::MonitorsGet
*/

CustomerRestApiV1::MonitorsGet::MonitorsGet(
        CustomerAuthenticator* customerAuthenticator,
        Monitors*              monitorDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentMonitors(
        monitorDatabaseApi
    ) {}


CustomerRestApiV1::MonitorsGet::~MonitorsGet() {}


RestApiInV1::JsonResponse CustomerRestApiV1::MonitorsGet::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject requestObject = request.object();
        if (requestObject.contains("monitor_id")) {
            QJsonObject responseObject;

            double monitorIdDouble = requestObject.value("monitor_id").toDouble(-1);
            if (monitorIdDouble > 0 && monitorIdDouble <= 0xFFFFFFFF) {
                Monitor::MonitorId monitorId = static_cast<Monitor::MonitorId>(monitorIdDouble);
                Monitor monitor = currentMonitors->getMonitor(monitorId, threadId);
                if (monitor.isValid() && monitor.customerId() == customerId) {
                    responseObject.insert("status", "OK");
                    responseObject.insert("monitor", convertToJson(monitor, false, true));
                } else {
                    responseObject.insert("status", "failed, unknown monitor ID");
                }
            } else {
                responseObject.insert("status", "failed, invalid monitor ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerRestApiV1::MonitorsList
*/

CustomerRestApiV1::MonitorsList::MonitorsList(
        CustomerAuthenticator* customerAuthenticator,
        Monitors*              monitorDatabaseApi,
        HostSchemes*           hostSchemeDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentMonitors(
        monitorDatabaseApi
    ),currentHostSchemes(
        hostSchemeDatabaseApi
    ) {}


CustomerRestApiV1::MonitorsList::~MonitorsList() {}


RestApiInV1::JsonResponse CustomerRestApiV1::MonitorsList::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    QString orderBy("monitor_id");
    bool    validRequest = true;
    if (request.isObject()) {
        QJsonObject requestObject = request.object();
        if (requestObject.size() == 1 && requestObject.contains("order_by")) {
            orderBy = requestObject.value("order_by").toString().toLower().replace('-', '_');
        } else {
            validRequest = false;
        }
    }

    if (validRequest) {
        QJsonObject responseObject;

        enum class OrderBy {
            MONITOR_ID,
            USER_ORDERING,
            URL,
            INVALID
        };

        OrderBy orderByValue = OrderBy::INVALID;
        if (orderBy == QString("monitor_id")) {
            orderByValue = OrderBy::MONITOR_ID;
        } else if (orderBy == QString("user_ordering")) {
            orderByValue = OrderBy::USER_ORDERING;
        } else if (orderBy == QString("url")) {
            orderByValue = OrderBy::URL;
        }

        if (orderByValue != OrderBy::INVALID) {
            Monitors::MonitorList monitors = currentMonitors->getMonitorsByUserOrder(
                static_cast<CustomerCapabilities::CustomerId>(customerId),
                threadId
            );

            HostSchemes::HostSchemeHash hostSchemesById = currentHostSchemes->getHostSchemes(
                static_cast<CustomerCapabilities::CustomerId>(customerId),
                threadId
            );

            QJsonObject monitorsObject;
            for (  Monitors::MonitorList::const_iterator it = monitors.constBegin(), end = monitors.constEnd()
                 ; it != end
                 ; ++it
                ) {
                const Monitor&    monitor    = *it;
                const HostScheme& hostScheme = hostSchemesById.value(monitor.hostSchemeId());

                QUrl url(hostScheme.url());
                url.setPath(monitor.path());

                switch (orderByValue) {
                    case OrderBy::MONITOR_ID: {
                        monitorsObject.insert(
                            QString::number(monitor.monitorId()),
                            convertToJson(monitor, url, false, true)
                        );

                        break;
                    }

                    case OrderBy::USER_ORDERING: {
                        monitorsObject.insert(
                            QString::number(monitor.userOrdering()),
                            convertToJson(monitor, url, false, true)
                        );

                        break;
                    }

                    case OrderBy::URL: {
                        QJsonObject newEntry = convertToJson(monitor, url, false, true);

                        QString urlString = url.toString();
                        if (monitorsObject.contains(urlString)) {
                            QJsonValue currentValue = monitorsObject.value(urlString);
                            if (currentValue.isArray()) {
                                QJsonArray arrayValue = currentValue.toArray();
                                arrayValue.append(newEntry);
                                monitorsObject.insert(urlString, arrayValue);
                            } else {
                                QJsonArray arrayValue;
                                arrayValue.append(currentValue);
                                arrayValue.append(newEntry);
                                monitorsObject.insert(urlString, arrayValue);
                            }
                        } else {
                            monitorsObject.insert(urlString, newEntry);
                        }

                        break;
                    }

                    case OrderBy::INVALID: {
                        Q_ASSERT(false);
                        break;
                    }

                    default: {
                        Q_ASSERT(false);
                        break;
                    }
                }
            }

            responseObject.insert("status", "OK");
            responseObject.insert("monitors", monitorsObject);
        } else {
            responseObject.insert("status", "failed, invalid order_by value.");
        }

        response = RestApiInV1::JsonResponse(responseObject);
    }

    return response;
}

/***********************************************************************************************************************
* CustomerRestApiV1::MonitorsUpdate
*/

CustomerRestApiV1::MonitorsUpdate::MonitorsUpdate(
        CustomerAuthenticator* customerAuthenticator,
        CustomersCapabilities* customersCapabilitiesDatabaseApi,
        MonitorUpdater*        monitorUpdater
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentCustomersCapabilities(
        customersCapabilitiesDatabaseApi
    ),currentMonitorUpdater(
        monitorUpdater
    ) {}


CustomerRestApiV1::MonitorsUpdate::~MonitorsUpdate() {}


RestApiInV1::JsonResponse CustomerRestApiV1::MonitorsUpdate::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isArray()) {
        QJsonObject responseObject;

        QJsonArray           requestArray         = request.array();
        unsigned             numberMonitorEntries = static_cast<unsigned>(requestArray.size());
        CustomerCapabilities capabilities         = currentCustomersCapabilities->getCustomerCapabilities(
            static_cast<CustomerCapabilities::CustomerId>(customerId),
            false,
            threadId
        );

        if (numberMonitorEntries <= capabilities.maximumNumberMonitors()) {
            MonitorUpdater::MonitorEntries monitorEntries;
            MonitorUpdater::Errors         errors;
            bool                           success = true;

            monitorEntries.reserve(numberMonitorEntries);

            for (Monitor::UserOrdering userOrdering = 0 ; userOrdering<numberMonitorEntries ; ++userOrdering) {
                QJsonValue value = requestArray.at(userOrdering);
                if (value.isObject()) {
                    QJsonObject monitorObject = value.toObject();
                    bool        ok;
                    MonitorUpdater::Entry entry = convertToMonitorEntry(errors, monitorObject, userOrdering, &ok);
                    if (ok) {
                        monitorEntries.append(entry);
                    } else {
                        success = false;
                    }
                } else {
                    errors.append(MonitorUpdater::Error(userOrdering, QString("dictionary object required")));
                    success = false;
                }
            }

            if (success) {
                MonitorUpdater::Errors moreErrors = currentMonitorUpdater->update(
                    capabilities,
                    monitorEntries,
                    threadId
                );

                success = moreErrors.isEmpty();
                errors.append(moreErrors);
            }

            if (success) {
                responseObject.insert("status", "OK");
            } else {
                responseObject.insert("status", "failed, see errors");
            }

            if (!errors.isEmpty()) {
                responseObject.insert("errors", convertToJson(errors));
            }
        } else {
            responseObject.insert(
                "status",
                QString("failed, subscription limited to %1 monitors.").arg(capabilities.maximumNumberMonitors())
            );
        }

        response = RestApiInV1::JsonResponse(responseObject);
    }

    return response;
}

/***********************************************************************************************************************
* CustomerRestApiV1::RegionsGet
*/

CustomerRestApiV1::RegionsGet::RegionsGet(
        CustomerAuthenticator* customerAuthenticator,
        Regions*               regionDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentRegions(
        regionDatabaseApi
    ) {}


CustomerRestApiV1::RegionsGet::~RegionsGet() {}


RestApiInV1::JsonResponse CustomerRestApiV1::RegionsGet::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        /* customerId */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject requestObject = request.object();
        if (requestObject.contains("region_id")) {
            QJsonObject responseObject;

            int regionIdInt = requestObject.value("region_id").toInt(-1);
            if (regionIdInt > 0 && regionIdInt <= 0xFFFF) {
                Region::RegionId regionId = static_cast<Region::RegionId>(regionIdInt);
                Region           region   = currentRegions->getRegion(regionId, threadId);

                if (region.isValid()) {
                    QJsonObject regionObject;
                    regionObject.insert("region_id", region.regionId());
                    regionObject.insert("description", region.regionName());
                    responseObject.insert("status", "OK");
                    responseObject.insert("region", regionObject);
                } else {
                    responseObject.insert("status", "unknown region ID");
                }
            } else {
                responseObject.insert("status", "invalid region ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerRestApiV1::RegionsList
*/

CustomerRestApiV1::RegionsList::RegionsList(
        CustomerAuthenticator* customerAuthenticator,
        Regions*               regionDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentRegions(
        regionDatabaseApi
    ) {}


CustomerRestApiV1::RegionsList::~RegionsList() {}


RestApiInV1::JsonResponse CustomerRestApiV1::RegionsList::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        /* customerId */,
        const QJsonDocument& /* request */,
        unsigned             threadId
    ) {
    QJsonObject responseObject;

    Regions::RegionHash regions = currentRegions->getAllRegions(threadId);

    QJsonObject regionsObject;
    for (Regions::RegionHash::const_iterator it=regions.constBegin(),end=regions.constEnd() ; it!=end ; ++it) {
        QJsonObject regionObject;
        regionObject.insert("region_id", it.value().regionId());
        regionObject.insert("description", it.value().regionName());

        regionsObject.insert(QString::number(it.key()), regionObject);
    }

    responseObject.insert("status", "OK");
    responseObject.insert("regions" , regionsObject);

    return RestApiInV1::JsonResponse(responseObject);
}

/***********************************************************************************************************************
* CustomerRestApiV1::EventsGet
*/

CustomerRestApiV1::EventsGet::EventsGet(
        CustomerAuthenticator* customerAuthenticator,
        Events*                eventDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentEvents(
        eventDatabaseApi
    ) {}


CustomerRestApiV1::EventsGet::~EventsGet() {}


RestApiInV1::JsonResponse CustomerRestApiV1::EventsGet::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject requestObject = request.object();
        if (requestObject.contains("event_id")) {
            QJsonObject responseObject;

            double eventIdDouble = requestObject.value("event_id").toDouble(-1);
            if (eventIdDouble > 0 && eventIdDouble <= 0xFFFFFFFF) {
                Event::EventId eventId = static_cast<Event::EventId>(eventIdDouble);
                Event          event   = currentEvents->getEvent(eventId, threadId);

                if (event.isValid() && event.customerId() == customerId) {
                    responseObject.insert("status", "OK");
                    responseObject.insert("event", convertToJson(event, false, false));
                } else {
                    responseObject.insert("status", "unknown event ID");
                }
            } else {
                responseObject.insert("status", "invalid event ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerRestApiV1::EventsList
*/

CustomerRestApiV1::EventsList::EventsList(
        CustomerAuthenticator* customerAuthenticator,
        Events*                eventDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentEvents(
        eventDatabaseApi
    ) {}


CustomerRestApiV1::EventsList::~EventsList() {}


RestApiInV1::JsonResponse CustomerRestApiV1::EventsList::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    QJsonObject responseObject;

    unsigned long long startTimestamp   = 0;
    unsigned long long endTimestamp     = static_cast<unsigned long long>(-1);
    bool               isValid          = true;
    bool               sendJsonResponse = true;

    if (request.isObject()) {
        QJsonObject object       = request.object();
        unsigned    numberFields = 0;

        if (object.contains("start_timestamp")) {
            double startTimestampDouble = object.value("start_timestamp").toDouble(-1);
            if (startTimestampDouble >= 0) {
                startTimestamp = static_cast<unsigned long long>(startTimestampDouble);
            } else {
                responseObject.insert("status", "failed, invalid start timestamp");
                isValid = false;
            }

            ++numberFields;
        }

        if (object.contains("end_timestamp")) {
            double endTimestampDouble = object.value("end_timestamp").toDouble(-1);
            if (endTimestampDouble >= 0) {
                endTimestamp = static_cast<unsigned long long>(endTimestampDouble);
            } else {
                responseObject.insert("status", "failed, invalid end timestamp");
                isValid = false;
            }

            ++numberFields;
        }

        if (numberFields != static_cast<unsigned>(object.size())) {
            sendJsonResponse = false;
            isValid          = false;
        }
    }

    if (isValid && sendJsonResponse) {
        Events::EventList events = currentEvents->getEventsByCustomer(
            static_cast<CustomerCapabilities::CustomerId>(customerId),
            startTimestamp,
            endTimestamp,
            threadId
        );

        responseObject.insert("status", "OK");
        responseObject.insert("events", convertToJson(events, false, false));
    }

    return   sendJsonResponse
           ? RestApiInV1::JsonResponse(responseObject)
           : RestApiInV1::JsonResponse(StatusCode::BAD_REQUEST);
}

/***********************************************************************************************************************
* CustomerRestApiV1::EventsCreate
*/

CustomerRestApiV1::EventsCreate::EventsCreate(
        CustomerAuthenticator* customerAuthenticator,
        Monitors*              monitorDatabaseApi,
        EventProcessor*        eventProcessor
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentMonitors(
        monitorDatabaseApi
    ),currentEventProcessor(
        eventProcessor
    ) {}


CustomerRestApiV1::EventsCreate::~EventsCreate() {}


RestApiInV1::JsonResponse CustomerRestApiV1::EventsCreate::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    QJsonObject responseObject;

    if (request.isObject()) {
        QJsonObject object = request.object();

        if (object.contains("message")) {
            QString message = object.value("message").toString().trimmed();

            if (!message.isEmpty()) {
                Monitor::MonitorId monitorId;
                double monitorIdDouble = object.value("monitor_id").toDouble(-1);
                if (monitorIdDouble > 0 && monitorIdDouble <= 0xFFFFFFFF) {
                    monitorId = static_cast<Monitor::MonitorId>(monitorIdDouble);
                    Monitor monitor = currentMonitors->getMonitor(monitorId, threadId);
                    if (monitor.customerId() != customerId) {
                        responseObject.insert("status", "failed, invalid monitor ID");
                        monitorId = Monitor::invalidMonitorId;
                    }
                } else {
                    Monitors::MonitorList monitors = currentMonitors->getMonitorsByUserOrder(
                        static_cast<CustomerCapabilities::CustomerId>(customerId),
                        threadId
                    );

                    if (!monitors.isEmpty()) {
                        monitorId = monitors.first().monitorId();
                    } else {
                        responseObject.insert("status", "failed, no monitors");
                        monitorId = Monitor::invalidMonitorId;
                    }
                }

                if (monitorId != Monitor::invalidMonitorId) {
                    unsigned typeIndex = 1;
                    if (object.contains("type")) {
                        typeIndex = static_cast<unsigned>(object.value("type").toInt(0));
                    }

                    Event::EventType eventType = Event::toCustomerEventType(typeIndex);
                    if (eventType != Event::EventType::INVALID) {
                        bool success = currentEventProcessor->reportEvent(
                            customerId,
                            monitorId,
                            QDateTime::currentSecsSinceEpoch(),
                            eventType,
                            EventProcessor::MonitorStatus::WORKING,
                            message,
                            QByteArray(),
                            threadId
                        );

                        if (success) {
                            responseObject.insert("status", "OK");
                        } else {
                            responseObject.insert("status", "failed, unable to post");
                        }
                    } else {
                        responseObject.insert("status", "failed, invalid event type");
                    }
                }
            } else {
                responseObject.insert("status", "failed, empty message");
            }
        } else {
            responseObject.insert("status", "failed, no message");
        }
    } else {
        responseObject.insert("status", "failed, dictionary expected");
    }

    return RestApiInV1::JsonResponse(responseObject);
}

/***********************************************************************************************************************
* CustomerRestApiV1::StatusGet
*/

CustomerRestApiV1::StatusGet::StatusGet(
        CustomerAuthenticator* customerAuthenticator,
        Monitors*              monitorDatabaseApi,
        Events*                eventDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentMonitors(
        monitorDatabaseApi
    ),currentEvents(
        eventDatabaseApi
    ) {}


CustomerRestApiV1::StatusGet::~StatusGet() {}


RestApiInV1::JsonResponse CustomerRestApiV1::StatusGet::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject requestObject = request.object();
        if (requestObject.contains("monitor_id")) {
            QJsonObject responseObject;

            double monitorIdDouble = requestObject.value("monitor_id").toDouble(-1);
            if (monitorIdDouble > 0 && monitorIdDouble <= 0xFFFFFFFF) {
                Event::EventId monitorId = static_cast<Monitor::MonitorId>(monitorIdDouble);
                Monitor        monitor   = currentMonitors->getMonitor(monitorId, threadId);
                if (monitor.isValid() && monitor.customerId() == customerId) {
                    Events::MonitorStatus monitorStatus = currentEvents->monitorStatus(monitorId, threadId);
                    responseObject.insert("status", "OK");
                    responseObject.insert("monitor_status", Monitor::toString(monitorStatus).toLower());
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
* CustomerRestApiV1::StatusList
*/

CustomerRestApiV1::StatusList::StatusList(
        CustomerAuthenticator* customerAuthenticator,
        Events*                eventDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentEvents(
        eventDatabaseApi
    ) {}


CustomerRestApiV1::StatusList::~StatusList() {}


RestApiInV1::JsonResponse CustomerRestApiV1::StatusList::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& /* request */,
        unsigned             threadId
    ) {
    QJsonObject responseObject;

    Events::MonitorStatusByMonitorId monitorStatus = currentEvents->monitorStatusByCustomerId(
        static_cast<CustomerCapabilities::CustomerId>(customerId),
        threadId
    );

    QJsonObject statusObject;
    for (  Events::MonitorStatusByMonitorId::const_iterator it  = monitorStatus.constBegin(),
                                                            end = monitorStatus.constEnd()
         ; it != end
         ; ++it
        ) {
        statusObject.insert(QString::number(it.key()), Monitor::toString(it.value()).toLower());
    }

    responseObject.insert("status", "OK");
    responseObject.insert("monitor_status", statusObject);

    return RestApiInV1::JsonResponse(responseObject);
}



/***********************************************************************************************************************
* CustomerRestApiV1::MultipleList
*/

CustomerRestApiV1::MultipleList::MultipleList(
        CustomerAuthenticator* customerAuthenticator,
        HostSchemes*           hostSchemeDatabaseApi,
        Monitors*              monitorDatabaseApi,
        Events*                eventDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentHostSchemes(
        hostSchemeDatabaseApi
    ),currentMonitors(
        monitorDatabaseApi
    ),currentEvents(
        eventDatabaseApi
    ) {}


CustomerRestApiV1::MultipleList::~MultipleList() {}


RestApiInV1::JsonResponse CustomerRestApiV1::MultipleList::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& /* request */,
        unsigned             threadId
    ) {
    QJsonObject responseObject;

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

    return RestApiInV1::JsonResponse(responseObject);
}

/***********************************************************************************************************************
* CustomerRestApiV1::LatencyList
*/

CustomerRestApiV1::LatencyList::LatencyList(
        CustomerAuthenticator*   customerAuthenticator,
        LatencyInterfaceManager* latencyInterfaceManager,
        Servers*                 serverDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentLatencyInterfaceManager(
        latencyInterfaceManager
    ),currentServers(
        serverDatabaseApi
    ) {}


CustomerRestApiV1::LatencyList::~LatencyList() {}


RestApiInV1::JsonResponse CustomerRestApiV1::LatencyList::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response;

    if (request.isObject()) {
        QJsonObject        responseObject;
        bool               success        = true;
        QJsonObject        object         = request.object();
        unsigned           numberFields   = 0;
        Monitor::MonitorId monitorId      = Monitor::invalidMonitorId;
        Region::RegionId   regionId       = Region::invalidRegionId;
        unsigned long long startTimestamp = 0;
        unsigned long long endTimestamp   = std::numeric_limits<unsigned long long>::max();

        if (object.contains("monitor_id")) {
            if (success) {
                double monitorIdDouble = object.value("monitor_id").toDouble(-1);
                if (monitorIdDouble > 0 && monitorIdDouble <= 0xFFFFFFFF) {
                    monitorId = static_cast<Monitor::MonitorId>(monitorIdDouble);
                } else {
                    success = false;
                    responseObject.insert("status", "failed, invalid monitor ID");
                }
            }

            ++numberFields;
        }

        if (object.contains("region_id")) {
            if (success) {
                double regionIdDouble = object.value("region_id").toDouble(-1);
                if (regionIdDouble > 0 && regionIdDouble <= 0xFFFF) {
                    regionId = static_cast<Server::ServerId>(regionIdDouble);
                } else {
                    success = false;
                    responseObject.insert("status", "failed, invalid region ID");
                }
            }

            ++numberFields;
        }

        if (object.contains("start_timestamp")) {
            if (success) {
                double startTimestampDouble = object.value("start_timestamp").toDouble(-1);
                if (startTimestampDouble >= 0) {
                    startTimestamp = static_cast<unsigned long long>(startTimestampDouble);
                } else {
                    success = false;
                    responseObject.insert("status", "failed, invalid start timestamp");
                }
            }

            ++numberFields;
        }

        if (object.contains("end_timestamp")) {
            if (success) {
                double endTimestampDouble = object.value("end_timestamp").toDouble(-1);
                if (endTimestampDouble >= 0) {
                    endTimestamp = static_cast<unsigned long long>(endTimestampDouble);
                } else {
                    success = false;
                    responseObject.insert("status", "failed, invalid end timestamp");
                }
            }

            ++numberFields;
        }

        if (numberFields == static_cast<unsigned>(object.size())) {
            LatencyInterfaceManager::LatencyEntryLists result = currentLatencyInterfaceManager->getLatencyEntries(
                static_cast<CustomerCapabilities::CustomerId>(customerId),
                HostScheme::invalidHostSchemeId,
                monitorId,
                regionId,
                Server::invalidServerId,
                startTimestamp,
                endTimestamp,
                threadId
            );

            LatencyInterfaceManager::LatencyEntryList           rawEntries        = result.first;
            LatencyInterfaceManager::AggregatedLatencyEntryList aggregatedEntries = result.second;

            Servers::ServersById   serversById  = currentServers->getServersById(threadId);
            Monitors::MonitorsById monitorsById;

            responseObject.insert("status", "OK");
            responseObject.insert("recent", convertToJson(rawEntries, serversById, monitorsById, false, true, false));
            responseObject.insert(
                "aggregated",
                convertToJson(
                    aggregatedEntries,
                    serversById,
                    monitorsById,
                    false,
                    true,
                    false
                )
            );

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerRestApiV1::LatencyPlot
*/

CustomerRestApiV1::LatencyPlot::LatencyPlot(
        CustomerAuthenticator* customerAuthenticator,
        LatencyPlotter*        latencyPlotter
    ):RestApiInV1::InesonicCustomerBinaryRestHandler(
        customerAuthenticator
    ),currentLatencyPlotter(
        latencyPlotter
    ) {}


CustomerRestApiV1::LatencyPlot::~LatencyPlot() {}


RestApiInV1::BinaryResponse CustomerRestApiV1::LatencyPlot::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::BinaryResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject              responseObject;
        bool                     success        = true;
        QJsonObject              object         = request.object();
        unsigned                 numberFields   = 0;
        HostScheme::HostSchemeId hostSchemeId   = HostScheme::invalidHostSchemeId;
        Monitor::MonitorId       monitorId      = Monitor::invalidMonitorId;
        Region::RegionId         regionId       = Region::invalidRegionId;
        unsigned long long       startTimestamp = 0;
        unsigned long long       endTimestamp   = std::numeric_limits<unsigned long long>::max();
        double                   minimumLatency = -1;
        double                   maximumLatency = -1;
        bool                     logScale       = false;
        unsigned                 width          = ::LatencyPlotter::defaultWidth;
        unsigned                 height         = ::LatencyPlotter::defaultHeight;
        QString                  plotType("history");
        QString                  plotFormat("PNG");
        QString                  title("Latency Over Time");
        QString                  xAxisLabel("Date/Time");
        QString                  yAxisLabel("Latency (seconds)");
        QString                  dateFormat("MMM dd yyyy - hh:mm");
        QString                  titleFont;
        QString                  axisTitleFont;
        QString                  axisLabelFont;

        if (object.contains("plot_type")) {
            plotType = object.value("plot_type").toString().toLower();
            if (plotType == "histogram") {
                title = QString("Latency Histogram");
                xAxisLabel = QString("Latency (seconds)");
                yAxisLabel = QString("Counts");
            }

            ++numberFields;
        }

        if (object.contains("host_scheme_id")) {
            if (success) {
                double hostSchemeIdDouble = object.value("host_scheme_id").toDouble(-1);
                if (hostSchemeIdDouble > 0 && hostSchemeIdDouble <= 0xFFFFFFFF) {
                    hostSchemeId = static_cast<HostScheme::HostSchemeId>(hostSchemeIdDouble);
                } else {
                    success = false;
                    responseObject.insert("status", "failed, invalid host/scheme ID");
                }
            }

            ++numberFields;
        }

        if (object.contains("monitor_id")) {
            if (success) {
                double monitorIdDouble = object.value("monitor_id").toDouble(-1);
                if (monitorIdDouble > 0 && monitorIdDouble <= 0xFFFFFFFF) {
                    monitorId = static_cast<Monitor::MonitorId>(monitorIdDouble);
                } else {
                    success = false;
                    responseObject.insert("status", "failed, invalid monitor ID");
                }
            }

            ++numberFields;
        }

        if (object.contains("region_id")) {
            if (success) {
                double regionIdDouble = object.value("region_id").toDouble(-1);
                if (regionIdDouble > 0 && regionIdDouble <= 0xFFFF) {
                    regionId = static_cast<Server::ServerId>(regionIdDouble);
                } else {
                    success = false;
                    responseObject.insert("status", "failed, invalid region ID");
                }
            }

            ++numberFields;
        }

        if (object.contains("start_timestamp")) {
            if (success) {
                double startTimestampDouble = object.value("start_timestamp").toDouble(-1);
                if (startTimestampDouble >= 0) {
                    startTimestamp = static_cast<unsigned long long>(startTimestampDouble);
                } else {
                    success = false;
                    responseObject.insert("status", "failed, invalid start timestamp");
                }
            }

            ++numberFields;
        }

        if (object.contains("end_timestamp")) {
            if (success) {
                double endTimestampDouble = object.value("end_timestamp").toDouble(-1);
                if (endTimestampDouble >= 0) {
                    endTimestamp = static_cast<unsigned long long>(endTimestampDouble);
                } else {
                    success = false;
                    responseObject.insert("status", "failed, invalid end timestamp");
                }
            }

            ++numberFields;
        }

        if (object.contains("title")) {
            title = object.value("title").toString();
            ++numberFields;
        }

        if (object.contains("x_axis_label")) {
            xAxisLabel = object.value("x_axis_label").toString();
            ++numberFields;
        }

        if (object.contains("y_axis_label")) {
            yAxisLabel = object.value("y_axis_label").toString();
            ++numberFields;
        }

        if (object.contains("date_format")) {
            dateFormat = object.value("date_format").toString();
            ++numberFields;
        }

        if (object.contains("title_font")) {
            titleFont = object.value("title_font").toString();
            ++numberFields;
        }

        if (object.contains("axis_title_font")) {
            axisTitleFont = object.value("axis_title_font").toString();
            ++numberFields;
        }

        if (object.contains("axis_label_font")) {
            axisLabelFont = object.value("axis_label_font").toString();
            ++numberFields;
        }

        if (object.contains("minimum_latency")) {
            minimumLatency = object.value("minimum_latency").toDouble(-1);
            ++numberFields;
        }

        if (object.contains("maximum_latency")) {
            maximumLatency = object.value("maximum_latency").toDouble(-1);
            ++numberFields;
        }

        if (object.contains("log_scale")) {
            logScale = object.value("log_scale").toBool();
            ++numberFields;
        }

        if (object.contains("width")) {
            int widthInteger = object.value("width").toInt(-1);
            if (widthInteger >= 100 && widthInteger <= 2048) {
                width = static_cast<unsigned>(widthInteger);
            } else {
                success = false;
                responseObject.insert("status", "failed, invalid width");
            }

            ++numberFields;
        }

        if (object.contains("height")) {
            int heightInteger = object.value("height").toInt(-1);
            if (heightInteger >= 100 && heightInteger <= 2048) {
                height = static_cast<unsigned>(heightInteger);
            } else {
                success = false;
                responseObject.insert("status", "failed, invalid height");
            }

            ++numberFields;
        }

        if (object.contains("format")) {
            plotFormat = object.value("format").toString().toUpper();
            ++numberFields;
        }

        if (numberFields == static_cast<unsigned>(object.size())) {
            if (success) {
                QImage image;

                if (plotType == "history") {
                    PlotMailbox& mailbox = currentLatencyPlotter->requestHistoryPlot(
                        threadId,
                        customerId,
                        regionId,
                        Server::invalidServerId, // Customers have limited visibility to specific servers.
                        hostSchemeId,
                        monitorId,
                        startTimestamp,
                        endTimestamp,
                        title,
                        xAxisLabel,
                        yAxisLabel,
                        dateFormat,
                        titleFont,
                        axisTitleFont,
                        axisLabelFont,
                        maximumLatency,
                        minimumLatency,
                        logScale,
                        width,
                        height
                    );

                    image = mailbox.waitForImage();
                } else if (plotType == "histogram") {
                    PlotMailbox& mailbox = currentLatencyPlotter->requestHistogramPlot(
                        threadId,
                        customerId,
                        regionId,
                        Server::invalidServerId, // Customers have limited visibility to specific servers.
                        hostSchemeId,
                        monitorId,
                        startTimestamp,
                        endTimestamp,
                        title,
                        xAxisLabel,
                        yAxisLabel,
                        titleFont,
                        axisTitleFont,
                        axisLabelFont,
                        maximumLatency,
                        minimumLatency,
                        width,
                        height
                    );

                    image = mailbox.waitForImage();
                } else {
                    success = false;
                    responseObject.insert("status", "invalid plot type");
                }

                if (success) {
                    QByteArray plotData;
                    QBuffer    buffer(&plotData);
                    buffer.open(QBuffer::OpenModeFlag::WriteOnly);

                    success = image.save(&buffer, plotFormat.toLocal8Bit().data());
                    if (!success) {
                        responseObject.insert("status", "failed, could not convert to image");
                    } else {
                        response = RestApiInV1::BinaryResponse(
                            QString("image/%1").arg(plotFormat).toLower().toUtf8(),
                            plotData
                        );
                    }
                }
            }

            if (!success) {
                response = RestApiInV1::BinaryResponse(
                    QByteArray("application/json"),
                    QJsonDocument(responseObject).toJson()
                );
            }
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerRestApiV1::CustomerPause
*/

CustomerRestApiV1::CustomerPause::CustomerPause(
        CustomerAuthenticator* customerAuthenticator,
        ServerAdministrator*   serverAdministrator
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentServerAdministrator(
        serverAdministrator
    ) {}


CustomerRestApiV1::CustomerPause::~CustomerPause() {}


RestApiInV1::JsonResponse CustomerRestApiV1::CustomerPause::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject requestObject = request.object();

        if (requestObject.size() == 1 && requestObject.contains("pause")) {
            QJsonObject responseObject;

            bool nowPaused = requestObject.value("pause").toBool();
            bool success   = currentServerAdministrator->setPaused(customerId, nowPaused, threadId);
            if (success) {
                responseObject.insert("status", "OK");
            } else {
                responseObject.insert("status", "failed");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerRestApiV1::ResourceAvailable
*/

CustomerRestApiV1::ResourceAvailable::ResourceAvailable(
        CustomerAuthenticator* customerAuthenticator,
        Resources*             resourceDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentResources(
        resourceDatabaseApi
    ) {}


CustomerRestApiV1::ResourceAvailable::~ResourceAvailable() {}


RestApiInV1::JsonResponse CustomerRestApiV1::ResourceAvailable::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& /* request */,
        unsigned             threadId
    ) {
    QJsonObject responseObject;

    ActiveResources     activeResources = currentResources->hasResourceData(customerId, threadId);
    Resource::ValueType valueType       = activeResources.nextValidValueType();

    QJsonArray valueTypes;
    while (valueType != ActiveResources::invalidValueType) {
        valueTypes.append(static_cast<int>(valueType));
        valueType = activeResources.nextValidValueType(valueType + 1);
    }

    responseObject.insert("status", "OK");
    responseObject.insert("value_types", valueTypes);

    return RestApiInV1::JsonResponse(QJsonDocument(responseObject));
}

/***********************************************************************************************************************
* CustomerRestApiV1::ResourceCreate
*/

CustomerRestApiV1::ResourceCreate::ResourceCreate(
        CustomerAuthenticator* customerAuthenticator,
        Resources*             resourceDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentResources(
        resourceDatabaseApi
    ) {}


CustomerRestApiV1::ResourceCreate::~ResourceCreate() {}


RestApiInV1::JsonResponse CustomerRestApiV1::ResourceCreate::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response;

    if (request.isObject()) {
        QJsonObject responseObject;
        bool        success = true;
        QJsonObject object  = request.object();
        if (object.size() >= 1 && object.contains("value")) {
            float               value        = object.value("value").toDouble(0);
            Resource::ValueType valueType    = 0;
            unsigned long long  timestamp    = 0;
            unsigned            numberFields = 1;

            if (object.contains("value_type")) {
                int valueTypeInt = object.value("value_type").toInt(-1);
                if (valueTypeInt >= 0 && valueTypeInt <= 255) {
                    valueType = static_cast<Resource::ValueType>(valueTypeInt);
                } else {
                    responseObject.insert("status", "failed, bad value type");
                    success = false;
                }

                ++numberFields;
            }

            if (object.contains("timestamp")) {
                double timestampDouble = object.value("timestamp").toDouble(-1);
                if (timestampDouble >= 0 && timestampDouble <= 0xFFFFFFFFFFFFFFFFULL) {
                    timestamp = static_cast<unsigned long long>(timestampDouble);
                } else {
                    responseObject.insert("status", "failed, bad timestamp");
                    success = false;
                }

                ++numberFields;
            } else {
                timestamp = QDateTime::currentSecsSinceEpoch();
            }

            if (success) {
                if (static_cast<unsigned>(object.size()) == numberFields) {
                    Resource resource = currentResources->recordResource(
                        customerId,
                        valueType,
                        value,
                        timestamp,
                        threadId
                    );
                    if (resource.isValid()) {
                        responseObject.insert("status", "OK");
                    } else {
                        responseObject.insert("status", "failed to create entry");
                    }
                } else {
                    responseObject.insert("status", "unexpected fields");
                }
            }
        } else {
            responseObject.insert("status", "failed, missing value field");
        }

        response = RestApiInV1::JsonResponse(QJsonDocument(responseObject));
    }

    return response;
}

/***********************************************************************************************************************
* CustomerRestApiV1::ResourceList
*/

CustomerRestApiV1::ResourceList::ResourceList(
        CustomerAuthenticator* customerAuthenticator,
        Resources*             resourceDatabaseApi
    ):RestApiInV1::InesonicCustomerRestHandler(
        customerAuthenticator
    ),currentResources(
        resourceDatabaseApi
    ) {}


CustomerRestApiV1::ResourceList::~ResourceList() {}


RestApiInV1::JsonResponse CustomerRestApiV1::ResourceList::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response;

    if (request.isObject()) {
        QJsonObject responseObject;
        QJsonObject object  = request.object();
        if (object.size() >= 1 && object.contains("value_type")) {
            int valueTypeInt = object.value("value_type").toInt(-1);
            if (valueTypeInt >= 0 && valueTypeInt <= 255) {
                Resource::ValueType valueType = static_cast<Resource::ValueType>(valueTypeInt);

                unsigned long long startTimestamp = 0;
                unsigned long long endTimestamp   = 0;
                unsigned           numberFields   = 1;
                bool               success        = true;

                if (object.contains("start_timestamp")) {
                    double startTimestampDouble = object.value("start_timestamp").toDouble(-1);
                    if (startTimestampDouble >= 0 && startTimestampDouble <= 0xFFFFFFFFFFFFFFFFULL) {
                        startTimestamp = static_cast<unsigned long long>(startTimestampDouble);
                    } else {
                        responseObject.insert("status", "failed, bad start timestamp");
                        success = false;
                    }

                    ++numberFields;
                }

                if (object.contains("end_timestamp")) {
                    double endTimestampDouble = object.value("end_timestamp").toDouble(-1);
                    if (endTimestampDouble >= 0 && endTimestampDouble <= 0xFFFFFFFFFFFFFFFFULL) {
                        endTimestamp = static_cast<unsigned long long>(endTimestampDouble);
                    } else {
                        responseObject.insert("status", "failed, bad end timestamp");
                        success = false;
                    }

                    ++numberFields;
                }

                if (success) {
                    if (numberFields == static_cast<unsigned>(object.size())) {
                        Resources::ResourceList resources = currentResources->getResources(
                            customerId,
                            valueType,
                            startTimestamp,
                            endTimestamp,
                            threadId
                        );

                        QJsonObject dataObject;
                        dataObject.insert("value_type", static_cast<int>(valueType));
                        dataObject.insert("resources", convertToJson(resources, false, false));

                        responseObject.insert("status", "OK");
                        responseObject.insert("data", dataObject);
                    } else {
                        responseObject.insert("status", "failed, invalid parameter");
                    }
                }
            } else {
                responseObject.insert("status", "failed, bad value_type");
            }
        } else {
            responseObject.insert("status", "failed, missing value_type parameter");
        }

        response = RestApiInV1::JsonResponse(QJsonDocument(responseObject));
    }

    return response;
}

/***********************************************************************************************************************
* CustomerRestApiV1::ResourcePlot
*/

CustomerRestApiV1::ResourcePlot::ResourcePlot(
        CustomerAuthenticator* customerAuthenticator,
        ResourcePlotter*       resourcePlotter
    ):RestApiInV1::InesonicCustomerBinaryRestHandler(
        customerAuthenticator
    ),currentResourcePlotter(
        resourcePlotter
    ) {}


CustomerRestApiV1::ResourcePlot::~ResourcePlot() {}


RestApiInV1::BinaryResponse CustomerRestApiV1::ResourcePlot::processAuthenticatedRequest(
        const QString&       /* path */,
        unsigned long        customerId,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::BinaryResponse response = nullptr;
    bool                        success  = true;
    QJsonObject                 responseObject;

    if (request.isObject()) {
        QJsonObject              object         = request.object();
        unsigned                 numberFields   = 0;
        Resource::ValueType      valueType      = 0;
        unsigned long long       startTimestamp = 0;
        unsigned long long       endTimestamp   = std::numeric_limits<unsigned long long>::max();
        float                    scaleFactor    = 1.0F;
        unsigned                 width          = ::ResourcePlotter::defaultWidth;
        unsigned                 height         = ::ResourcePlotter::defaultHeight;
        QString                  plotFormat("PNG");
        QString                  title("Resource Over Time");
        QString                  xAxisLabel("Date/Time");
        QString                  yAxisLabel("Value");
        QString                  dateFormat("MMM dd yyyy - hh:mm");
        QString                  titleFont;
        QString                  axisTitleFont;
        QString                  axisLabelFont;

        if (object.contains("value_type")) {
            int valueTypeInt = object.value("value_type").toInt(-1);
            if (valueTypeInt >= 0 && valueTypeInt <= 255) {
                valueType = static_cast<Resource::ValueType>(valueTypeInt);
            }

            ++numberFields;
        }

        if (object.contains("start_timestamp")) {
            if (success) {
                double startTimestampDouble = object.value("start_timestamp").toDouble(-1);
                if (startTimestampDouble >= 0) {
                    startTimestamp = static_cast<unsigned long long>(startTimestampDouble);
                } else {
                    success = false;
                    responseObject.insert("status", "failed, invalid start timestamp");
                }
            }

            ++numberFields;
        }

        if (object.contains("end_timestamp")) {
            if (success) {
                double endTimestampDouble = object.value("end_timestamp").toDouble(-1);
                if (endTimestampDouble >= 0) {
                    endTimestamp = static_cast<unsigned long long>(endTimestampDouble);
                } else {
                    success = false;
                    responseObject.insert("status", "failed, invalid end timestamp");
                }
            }

            ++numberFields;
        }

        if (object.contains("scale_factor")) {
            if (success) {
                scaleFactor = object.value("scale_factor").toDouble(0);
                if (scaleFactor == 0) {
                    success = false;
                    responseObject.insert("status", "failed, invalid scale factor");
                }
            }

            ++numberFields;
        }

        if (object.contains("title")) {
            title = object.value("title").toString();
            ++numberFields;
        }

        if (object.contains("x_axis_label")) {
            xAxisLabel = object.value("x_axis_label").toString();
            ++numberFields;
        }

        if (object.contains("y_axis_label")) {
            yAxisLabel = object.value("y_axis_label").toString();
            ++numberFields;
        }

        if (object.contains("date_format")) {
            dateFormat = object.value("date_format").toString();
            ++numberFields;
        }

        if (object.contains("title_font")) {
            titleFont = object.value("title_font").toString();
            ++numberFields;
        }

        if (object.contains("axis_title_font")) {
            axisTitleFont = object.value("axis_title_font").toString();
            ++numberFields;
        }

        if (object.contains("axis_label_font")) {
            axisLabelFont = object.value("axis_label_font").toString();
            ++numberFields;
        }

        if (object.contains("width")) {
            int widthInteger = object.value("width").toInt(-1);
            if (widthInteger >= 100 && widthInteger <= 2048) {
                width = static_cast<unsigned>(widthInteger);
            } else {
                success = false;
                responseObject.insert("status", "failed, invalid width");
            }

            ++numberFields;
        }

        if (object.contains("height")) {
            int heightInteger = object.value("height").toInt(-1);
            if (heightInteger >= 100 && heightInteger <= 2048) {
                height = static_cast<unsigned>(heightInteger);
            } else {
                success = false;
                responseObject.insert("status", "failed, invalid height");
            }

            ++numberFields;
        }

        if (object.contains("format")) {
            plotFormat = object.value("format").toString().toUpper();
            ++numberFields;
        }

        if (success) {
            if (numberFields == static_cast<unsigned>(object.size())) {
                PlotMailbox& mailbox = currentResourcePlotter->requestPlot(
                    threadId,
                    customerId,
                    valueType,
                    startTimestamp,
                    endTimestamp,
                    scaleFactor,
                    title,
                    xAxisLabel,
                    yAxisLabel,
                    dateFormat,
                    titleFont,
                    axisTitleFont,
                    axisLabelFont,
                    width,
                    height
                );

                QImage image = mailbox.waitForImage();

                QByteArray plotData;
                QBuffer    buffer(&plotData);
                buffer.open(QBuffer::OpenModeFlag::WriteOnly);

                success = image.save(&buffer, plotFormat.toLocal8Bit().data());
                if (!success) {
                    responseObject.insert("status", "failed, could not convert to image");
                } else {
                    response = RestApiInV1::BinaryResponse(
                        QString("image/%1").arg(plotFormat).toLower().toUtf8(),
                        plotData
                    );
                }
            }
        }
    } else {
        responseObject.insert("status", "invalid payload");
        success = false;
    }

    if (!success) {
        response = RestApiInV1::BinaryResponse(
            QByteArray("application/json"),
            QJsonDocument(responseObject).toJson()
        );
    }

    return response;
}

/***********************************************************************************************************************
* CustomerRestApiV1
*/

const QString CustomerRestApiV1::capabilitiesGetPath("/v1/capabilities/get");
const QString CustomerRestApiV1::hostsGetPath("/v1/hosts/get");
const QString CustomerRestApiV1::hostsListPath("/v1/hosts/list");
const QString CustomerRestApiV1::monitorsGetPath("/v1/monitors/get");
const QString CustomerRestApiV1::monitorsListPath("/v1/monitors/list");
const QString CustomerRestApiV1::monitorsUpdatePath("/v1/monitors/update");
const QString CustomerRestApiV1::regionsGetPath("/v1/regions/get");
const QString CustomerRestApiV1::regionsListPath("/v1/regions/list");
const QString CustomerRestApiV1::eventsGetPath("/v1/events/get");
const QString CustomerRestApiV1::eventsListPath("/v1/events/list");
const QString CustomerRestApiV1::eventsCreatePath("/v1/events/create");
const QString CustomerRestApiV1::statusGetPath("/v1/status/get");
const QString CustomerRestApiV1::statusListPath("/v1/status/list");
const QString CustomerRestApiV1::multipleListPath("/v1/multiple/list");
const QString CustomerRestApiV1::latencyListPath("/v1/latency/list");
const QString CustomerRestApiV1::latencyPlotPath("/v1/latency/plot");
const QString CustomerRestApiV1::customerPausePath("/v1/customer/pause");
const QString CustomerRestApiV1::resourceAvailablePath("/v1/resource/available");
const QString CustomerRestApiV1::resourceCreatePath("/v1/resource/create");
const QString CustomerRestApiV1::resourceListPath("/v1/resource/list");
const QString CustomerRestApiV1::resourcePlotPath("/v1/resource/plot");

CustomerRestApiV1::CustomerRestApiV1(
        RestApiInV1::Server*     restApiServer,
        CustomerAuthenticator*   wordPressCustomerAuthenticator,
        CustomerAuthenticator*   restCustomerAuthenticator,
        CustomersCapabilities*   customersCapabilitiesDatabaseApi,
        HostSchemes*             hostSchemeDatabaseApi,
        Monitors*                monitorDatabaseApi,
        MonitorUpdater*          monitorUpdater,
        Regions*                 regionDatabaseApi,
        Servers*                 serverDatabaseApi,
        Events*                  eventDatabaseApi,
        EventProcessor*          eventProcessor,
        LatencyInterfaceManager* latencyInterfaceManager,
        LatencyPlotter*          latencyPlotter,
        Resources*               resourceDatabaseApi,
        ResourcePlotter*         resourcePlotter,
        ServerAdministrator*     serverAdministrator,
        QObject*                 parent
    ):QObject(
        parent
    ),capabilitiesGet(
        wordPressCustomerAuthenticator,
        customersCapabilitiesDatabaseApi
    ),hostsGet(
        restCustomerAuthenticator,
        hostSchemeDatabaseApi
    ),hostsList(
        restCustomerAuthenticator,
        hostSchemeDatabaseApi
    ),monitorsGet(
        restCustomerAuthenticator,
        monitorDatabaseApi
    ),monitorsList(
        restCustomerAuthenticator,
        monitorDatabaseApi,
        hostSchemeDatabaseApi
    ),monitorsUpdate(
        restCustomerAuthenticator,
        customersCapabilitiesDatabaseApi,
        monitorUpdater
    ),regionsGet(
        restCustomerAuthenticator,
        regionDatabaseApi
    ),regionsList(
        restCustomerAuthenticator,
        regionDatabaseApi
    ),eventsGet(
        restCustomerAuthenticator,
        eventDatabaseApi
    ),eventsList(
        restCustomerAuthenticator,
        eventDatabaseApi
    ),eventsCreate(
        restCustomerAuthenticator,
        monitorDatabaseApi,
        eventProcessor
    ),statusGet(
        restCustomerAuthenticator,
        monitorDatabaseApi,
        eventDatabaseApi
    ),statusList(
        restCustomerAuthenticator,
        eventDatabaseApi
    ),multipleList(
        wordPressCustomerAuthenticator,
        hostSchemeDatabaseApi,
        monitorDatabaseApi,
        eventDatabaseApi
    ),latencyList(
        restCustomerAuthenticator,
        latencyInterfaceManager,
        serverDatabaseApi
    ),latencyPlot(
        wordPressCustomerAuthenticator,
        latencyPlotter
    ),customerPause(
        restCustomerAuthenticator,
        serverAdministrator
    ),resourceAvailable(
        restCustomerAuthenticator,
        resourceDatabaseApi
    ),resourceCreate(
        restCustomerAuthenticator,
        resourceDatabaseApi
    ),resourceList(
        restCustomerAuthenticator,
        resourceDatabaseApi
    ),resourcePlot(
        restCustomerAuthenticator,
        resourcePlotter
    ) {
    restApiServer->registerHandler(
        &capabilitiesGet,
        RestApiInV1::Handler::Method::POST,
        capabilitiesGetPath
    );
    restApiServer->registerHandler(
        &hostsGet,
        RestApiInV1::Handler::Method::POST,
        hostsGetPath
    );
    restApiServer->registerHandler(
        &hostsList,
        RestApiInV1::Handler::Method::POST,
        hostsListPath
    );
    restApiServer->registerHandler(
        &monitorsGet,
        RestApiInV1::Handler::Method::POST,
        monitorsGetPath
    );
    restApiServer->registerHandler(
        &monitorsList,
        RestApiInV1::Handler::Method::POST,
        monitorsListPath
    );
    restApiServer->registerHandler(
        &monitorsUpdate,
        RestApiInV1::Handler::Method::POST,
        monitorsUpdatePath
    );
    restApiServer->registerHandler(
        &regionsGet,
        RestApiInV1::Handler::Method::POST,
        regionsGetPath
    );
    restApiServer->registerHandler(
        &regionsList,
        RestApiInV1::Handler::Method::POST,
        regionsListPath
    );
    restApiServer->registerHandler(
        &eventsGet,
        RestApiInV1::Handler::Method::POST,
        eventsGetPath
    );
    restApiServer->registerHandler(
        &eventsList,
        RestApiInV1::Handler::Method::POST,
        eventsListPath
    );
    restApiServer->registerHandler(
        &eventsCreate,
        RestApiInV1::Handler::Method::POST,
        eventsCreatePath
    );
    restApiServer->registerHandler(
        &statusGet,
        RestApiInV1::Handler::Method::POST,
        statusGetPath
    );
    restApiServer->registerHandler(
        &statusList,
        RestApiInV1::Handler::Method::POST,
        statusListPath
    );
    restApiServer->registerHandler(
        &multipleList,
        RestApiInV1::Handler::Method::POST,
        multipleListPath
    );
    restApiServer->registerHandler(
        &latencyList,
        RestApiInV1::Handler::Method::POST,
        latencyListPath
    );
    restApiServer->registerHandler(
        &latencyPlot,
        RestApiInV1::Handler::Method::POST,
        latencyPlotPath
    );
    restApiServer->registerHandler(
        &customerPause,
        RestApiInV1::Handler::Method::POST,
        customerPausePath
    );
    restApiServer->registerHandler(
        &resourceAvailable,
        RestApiInV1::Handler::Method::POST,
        resourceAvailablePath
    );
    restApiServer->registerHandler(
        &resourceCreate,
        RestApiInV1::Handler::Method::POST,
        resourceCreatePath
    );
    restApiServer->registerHandler(
        &resourceList,
        RestApiInV1::Handler::Method::POST,
        resourceListPath
    );
    restApiServer->registerHandler(
        &resourcePlot,
        RestApiInV1::Handler::Method::POST,
        resourcePlotPath
    );
}


CustomerRestApiV1::~CustomerRestApiV1() {}
