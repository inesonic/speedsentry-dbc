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
* This header implements the \ref CustomerMappingManager class.
***********************************************************************************************************************/

#include <QObject>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "customer_mapping.h"
#include "customer_mapping_manager.h"
#include "server_administrator.h"

/***********************************************************************************************************************
* CustomerMappingManager::CustomerMappingGet
*/

CustomerMappingManager::CustomerMappingGet::CustomerMappingGet(
        const QByteArray& secret,
        CustomerMapping*  customerMappingDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentCustomerMapping(
        customerMappingDatabaseApi
    ) {}


CustomerMappingManager::CustomerMappingGet::~CustomerMappingGet() {}


RestApiInV1::JsonResponse CustomerMappingManager::CustomerMappingGet::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 1 && object.contains("customer_id")) {
            QJsonObject responseObject;

            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble >= 1 && customerIdDouble <= 0xFFFFFFFF) {
                CustomerMapping::CustomerId customerId = static_cast<CustomerMapping::CustomerId>(customerIdDouble);

                CustomerMapping::Mapping mapping = currentCustomerMapping->mapping(customerId, threadId);
                if (mapping.isValid()) {
                    QJsonObject mappingObject;
                    mappingObject.insert("primary_server", static_cast<double>(mapping.primaryServerId()));

                    QJsonArray serverList;
                    for (  CustomerMapping::Mapping::const_iterator it = mapping.constBegin(), end = mapping.constEnd()
                         ; it != end
                         ; ++it
                        ) {
                        serverList.append(static_cast<double>(*it));
                    }

                    mappingObject.insert("servers", serverList);

                    responseObject.insert("status" , "OK");
                    responseObject.insert("mapping", mappingObject);
                } else {
                    responseObject.insert("status", "no mapping");
                }
            } else {
                responseObject.insert("status", "invalid customer ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerMappingManager::CustomerMappingUpdate
*/

CustomerMappingManager::CustomerMappingUpdate::CustomerMappingUpdate(
        const QByteArray& secret,
        CustomerMapping*  customerMappingDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentCustomerMapping(
        customerMappingDatabaseApi
    ) {}


CustomerMappingManager::CustomerMappingUpdate::~CustomerMappingUpdate() {}


RestApiInV1::JsonResponse CustomerMappingManager::CustomerMappingUpdate::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 2 && object.contains("customer_id") && object.contains("mapping")) {
            QJsonObject responseObject;

            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble >= 1 && customerIdDouble <= 0xFFFFFFFF) {
                CustomerMapping::CustomerId customerId = static_cast<CustomerMapping::CustomerId>(customerIdDouble);

                QJsonValue mappingValue = object.value("mapping");
                if (mappingValue.isArray()) {
                    QJsonArray mappingArray = mappingValue.toArray();

                    CustomerMapping::ServerId  primaryServerId = CustomerMapping::invalidServerId;
                    CustomerMapping::ServerSet servers;

                    bool                       success = true;
                    QJsonArray::const_iterator it      = mappingArray.constBegin();
                    QJsonArray::const_iterator end     = mappingArray.constEnd();

                    while (success && it != end) {
                        int serverIdInt = it->toInt(-1);
                        if (serverIdInt >= 1 && serverIdInt <= 0xFFFF) {
                            CustomerMapping::ServerId serverId = static_cast<CustomerMapping::ServerId>(serverIdInt);
                            if (primaryServerId == CustomerMapping::invalidServerId) {
                                primaryServerId = serverId;
                            }

                            servers.insert(serverId);
                            ++it;
                        } else {
                            success = false;
                        }
                    }

                    if (success) {
                        CustomerMapping::Mapping mapping(primaryServerId, servers);
                        success = currentCustomerMapping->updateMapping(customerId, mapping, threadId);
                        if (success) {
                            responseObject.insert("status", "OK");
                        } else {
                            responseObject.insert("status", "failed to create new mapping");
                        }
                    } else {
                        responseObject.insert("status", "invalid mapping");
                    }
                } else {
                    responseObject.insert("status", "expected array");
                }
            } else {
                responseObject.insert("status", "invalid customer ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerMappingManager::CustomerMappingCustomerActivate
*/

CustomerMappingManager::CustomerMappingCustomerActivate::CustomerMappingCustomerActivate(
        const QByteArray&    secret,
        ServerAdministrator* serverAdministratorApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServerAdministrator(
        serverAdministratorApi
    ) {}


CustomerMappingManager::CustomerMappingCustomerActivate::~CustomerMappingCustomerActivate() {}


RestApiInV1::JsonResponse CustomerMappingManager::CustomerMappingCustomerActivate::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 1 && object.contains("customer_id")) {
            QJsonObject responseObject;

            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble >= 1 && customerIdDouble <= 0xFFFFFFFF) {
                CustomerMapping::CustomerId customerId = static_cast<CustomerMapping::CustomerId>(customerIdDouble);

                bool success = currentServerAdministrator->activateCustomer(customerId, threadId);
                if (success) {
                    responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "unknown customer ID");
                }
            } else {
                responseObject.insert("status", "invalid customer ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerMappingManager::CustomerMappingCustomerDeactivate
*/

CustomerMappingManager::CustomerMappingCustomerDeactivate::CustomerMappingCustomerDeactivate(
        const QByteArray&    secret,
        ServerAdministrator* serverAdministratorApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServerAdministrator(
        serverAdministratorApi
    ) {}


CustomerMappingManager::CustomerMappingCustomerDeactivate::~CustomerMappingCustomerDeactivate() {}


RestApiInV1::JsonResponse CustomerMappingManager::CustomerMappingCustomerDeactivate::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 1 && object.contains("customer_id")) {
            QJsonObject responseObject;

            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble >= 1 && customerIdDouble <= 0xFFFFFFFF) {
                CustomerMapping::CustomerId customerId = static_cast<CustomerMapping::CustomerId>(customerIdDouble);

                bool success = currentServerAdministrator->deactivateCustomer(customerId, threadId);
                if (success) {
                    responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "failed to deactivate");
                }
            } else {
                responseObject.insert("status", "invalid customer ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerMappingManager::CustomerMappingList
*/

CustomerMappingManager::CustomerMappingList::CustomerMappingList(
        const QByteArray& secret,
        CustomerMapping*          customerMappingDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentCustomerMapping(
        customerMappingDatabaseApi
    ) {}


CustomerMappingManager::CustomerMappingList::~CustomerMappingList() {}


RestApiInV1::JsonResponse CustomerMappingManager::CustomerMappingList::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() <= 1) {
            bool                      success  = true;
            CustomerMapping::ServerId serverId = CustomerMapping::invalidServerId;
            if (object.contains("server_id")) {
                int serverIdInt = object.value("server_id").toInt(-1);
                if (serverIdInt >= 1 && serverIdInt <= 0xFFFF) {
                    serverId = static_cast<CustomerMapping::ServerId>(serverIdInt);
                } else {
                    success = false;

                    QJsonObject responseObject;
                    responseObject.insert("status", "invalid server ID");
                    response = RestApiInV1::JsonResponse(responseObject);
                }
            }

            if (success) {
                QJsonObject responseObject;

                CustomerMapping::MappingsByCustomerId mappings = currentCustomerMapping->mappings(serverId, threadId);
                QJsonObject mappingsObject;
                for (  CustomerMapping::MappingsByCustomerId::const_iterator
                           mappingsInterator = mappings.constBegin(),
                           mappingsEndIterator = mappings.constEnd()
                     ; mappingsInterator != mappingsEndIterator
                     ; ++mappingsInterator
                    ) {
                    CustomerMapping::CustomerId     customerId = mappingsInterator.key();
                    const CustomerMapping::Mapping& mapping    = mappingsInterator.value();

                    QJsonObject mappingObject;
                    QJsonArray  serverList;
                    for (  CustomerMapping::Mapping::const_iterator serverIterator    = mapping.constBegin(),
                                                                    serverEndIterator = mapping.constEnd()
                         ; serverIterator != serverEndIterator
                         ; ++serverIterator
                        ) {
                        serverList.append(*serverIterator);
                    }

                    mappingObject.insert("primary_server", mapping.primaryServerId());
                    mappingObject.insert("servers", serverList);

                    mappingsObject.insert(QString::number(customerId), mappingObject);
                }

                responseObject.insert("status", "OK");
                responseObject.insert("mappings", mappingsObject);

                response = RestApiInV1::JsonResponse(responseObject);
            }
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerMappingManager
*/

const QString CustomerMappingManager::mappingGetPath("/mapping/get");
const QString CustomerMappingManager::mappingUpdatePath("/mapping/update");
const QString CustomerMappingManager::mappingCustomerActivatePath("/mapping/customer/activate");
const QString CustomerMappingManager::mappingCustomerDeactivatePath("/mapping/customer/deactivate");
const QString CustomerMappingManager::mappingListPath("/mapping/list");

CustomerMappingManager::CustomerMappingManager(
        RestApiInV1::Server* restApiServer,
        CustomerMapping*     customerMappingDatabaseApi,
        ServerAdministrator* serverAdministratorApi,
        const QByteArray&    secret,
        QObject*             parent
    ):QObject(
        parent
    ),mappingGet(
        secret,
        customerMappingDatabaseApi
    ),mappingUpdate(
        secret,
        customerMappingDatabaseApi
    ),mappingCustomerActivate(
        secret,
        serverAdministratorApi
    ),mappingCustomerDeactivate(
        secret,
        serverAdministratorApi
    ),mappingList(
        secret,
        customerMappingDatabaseApi
    ) {
    restApiServer->registerHandler(&mappingGet, RestApiInV1::Handler::Method::POST, mappingGetPath);
    restApiServer->registerHandler(&mappingUpdate, RestApiInV1::Handler::Method::POST, mappingUpdatePath);
    restApiServer->registerHandler(
        &mappingCustomerActivate,
        RestApiInV1::Handler::Method::POST,
        mappingCustomerActivatePath
    );
    restApiServer->registerHandler(
        &mappingCustomerDeactivate,
        RestApiInV1::Handler::Method::POST,
        mappingCustomerDeactivatePath
    );
    restApiServer->registerHandler(&mappingList, RestApiInV1::Handler::Method::POST, mappingListPath);
}


CustomerMappingManager::~CustomerMappingManager() {}


void CustomerMappingManager::setSecret(const QByteArray& newSecret) {
    mappingGet.setSecret(newSecret);
    mappingUpdate.setSecret(newSecret);
    mappingCustomerActivate.setSecret(newSecret);
    mappingCustomerDeactivate.setSecret(newSecret);
    mappingList.setSecret(newSecret);
}
