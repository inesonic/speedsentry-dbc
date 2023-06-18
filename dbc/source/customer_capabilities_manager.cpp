/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref CustomerCapabilitiesManager class.
***********************************************************************************************************************/

#include <QObject>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "customer_secrets.h"
#include "server_administrator.h"
#include "customers_capabilities.h"
#include "customer_capabilities_manager.h"

/***********************************************************************************************************************
* CustomerCapabilitiesManager::CustomerCapabilitiesGet
*/

CustomerCapabilitiesManager::CustomerCapabilitiesGet::CustomerCapabilitiesGet(
        const QByteArray& secret,
        CustomersCapabilities*          customerCapabilitiesDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentCustomersCapabilities(
        customerCapabilitiesDatabaseApi
    ) {}


CustomerCapabilitiesManager::CustomerCapabilitiesGet::~CustomerCapabilitiesGet() {}


RestApiInV1::JsonResponse CustomerCapabilitiesManager::CustomerCapabilitiesGet::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() >= 1 && object.contains("customer_id")) {
            QJsonObject responseObject;

            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0) {
                CustomerCapabilities::CustomerId customerId = static_cast<CustomerCapabilities::CustomerId>(
                    customerIdDouble
                );

                CustomerCapabilities capabilities = currentCustomersCapabilities->getCustomerCapabilities(
                    customerId,
                    threadId
                );

                if (capabilities.isValid()) {
                    responseObject.insert("status", "OK");
                    responseObject.insert("customer", convertToJson(capabilities, true, true, true));
                } else {
                    responseObject.insert("status", "failed, unknown customer ID");
                }
            } else {
                responseObject.insert("status", "failed, invalid customer ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerCapabilitiesManager::CustomerCapabilitiesUpdate
*/

CustomerCapabilitiesManager::CustomerCapabilitiesUpdate::CustomerCapabilitiesUpdate(
        const QByteArray&      secret,
        CustomersCapabilities* customerCapabilitiesDatabaseApi,
        ServerAdministrator*   serverAdministrator,
        CustomerSecrets*       customerSecretsDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentCustomersCapabilities(
        customerCapabilitiesDatabaseApi
    ),currentServerAdministrator(
        serverAdministrator
    ),currentCustomerSecrets(
        customerSecretsDatabaseApi
    ) {}


CustomerCapabilitiesManager::CustomerCapabilitiesUpdate::~CustomerCapabilitiesUpdate() {}


RestApiInV1::JsonResponse CustomerCapabilitiesManager::CustomerCapabilitiesUpdate::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() >= 1 && object.contains("customer_id")) {
            QJsonObject responseObject;

            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0) {
                CustomerCapabilities::CustomerId customerId = static_cast<CustomerCapabilities::CustomerId>(
                    customerIdDouble
                );

                unsigned numberFields                     = 1;
                int      maximumNumberMonitors            = 10;
                int      pollingInterval                  = 120;
                int      expirationDays                   = 180;
                bool     customerActive                   = false;
                bool     multiRegionChecking              = false;
                bool     supportsWordPress                = false;
                bool     supportsRestApi                  = false;
                bool     supportsContentChecking          = false;
                bool     supportsKeywordChecking          = false;
                bool     supportsPostMethod               = false;
                bool     supportsLatencyTracking          = false;
                bool     supportsSslExpirationChecking    = false;
                bool     supportsPingBasedPolling         = false;
                bool     supportsBlacklistChecking        = false;
                bool     supportsDomainExpirationChecking = false;
                bool     supportsMaintenanceMode          = false;
                bool     supportsRollups                  = false;

                if (object.contains("maximum_number_monitors")) {
                    maximumNumberMonitors = object.value("maximum_number_monitors").toInt(-1);
                    ++numberFields;
                }

                if (object.contains("polling_interval")) {
                    pollingInterval = object.value("polling_interval").toInt(-1);
                    ++numberFields;
                }

                if (object.contains("expiration_days")) {
                    expirationDays = object.value("expiration_days").toInt(-1);
                    ++numberFields;
                }

                if (object.contains("multi_region_checking")) {
                    multiRegionChecking = object.value("multi_region_checking").toBool(false);
                    ++numberFields;
                }

                if (object.contains("customer_active")) {
                    customerActive = object.value("customer_active").toBool(false);
                    ++numberFields;
                }

                if (object.contains("supports_wordpress")) {
                    supportsWordPress = object.value("supports_wordpress").toBool(false);
                    ++numberFields;
                }

                if (object.contains("supports_rest_api")) {
                    supportsRestApi = object.value("supports_rest_api").toBool(false);
                    ++numberFields;
                }

                if (object.contains("supports_content_checking")) {
                    supportsContentChecking = object.value("supports_content_checking").toBool(false);
                    ++numberFields;
                }

                if (object.contains("supports_keyword_checking")) {
                    supportsKeywordChecking = object.value("supports_keyword_checking").toBool(false);
                    ++numberFields;
                }

                if (object.contains("supports_post_method")) {
                    supportsPostMethod = object.value("supports_post_method").toBool(false);
                    ++numberFields;
                }

                if (object.contains("supports_latency_tracking")) {
                    supportsLatencyTracking = object.value("supports_latency_tracking").toBool(false);
                    ++numberFields;
                }

                if (object.contains("supports_ssl_expiration_checking")) {
                    supportsSslExpirationChecking = object.value("supports_ssl_expiration_checking").toBool(false);
                    ++numberFields;
                }

                if (object.contains("supports_ping_based_polling")) {
                    supportsPingBasedPolling = object.value("supports_ping_based_polling").toBool(false);
                    ++numberFields;
                }

                if (object.contains("supports_blacklist_checking")) {
                    supportsBlacklistChecking = object.value("supports_blacklist_checking").toBool(false);
                    ++numberFields;
                }

                if (object.contains("supports_domain_expiration_checking")) {
                    supportsDomainExpirationChecking = object.value(
                        "supports_domain_expiration_checking"
                    ).toBool(false);
                    ++numberFields;
                }

                if (object.contains("supports_maintenance_mode")) {
                    supportsMaintenanceMode = object.value("supports_maintenance_mode").toBool(false);
                    ++numberFields;
                }

                if (object.contains("supports_rollups")) {
                    supportsRollups = object.value("supports_rollups").toBool(false);
                    ++numberFields;
                }

                if (maximumNumberMonitors > 0) {
                    if (expirationDays >= 0) {
                        CustomerCapabilities customerCapabilities = CustomerCapabilities(
                            customerId,
                            maximumNumberMonitors,
                            pollingInterval,
                            expirationDays,
                            customerActive,
                            multiRegionChecking,
                            supportsWordPress,
                            supportsRestApi,
                            supportsContentChecking,
                            supportsKeywordChecking,
                            supportsPostMethod,
                            supportsLatencyTracking,
                            supportsSslExpirationChecking,
                            supportsPingBasedPolling,
                            supportsBlacklistChecking,
                            supportsDomainExpirationChecking,
                            supportsMaintenanceMode,
                            supportsRollups,
                            false // paused
                        );

                        bool success = currentCustomersCapabilities->updateCustomerCapabilities(
                            customerCapabilities,
                            threadId
                        );

                        if (success) {
                            if (supportsRestApi || supportsWordPress) {
                                CustomerSecret secret = currentCustomerSecrets->getCustomerSecret(
                                    customerId,
                                    true,
                                    threadId
                                );

                                if (secret.isInvalid()) {
                                    currentCustomerSecrets->updateCustomerSecret(customerId, threadId);
                                }
                            }

                            if (customerActive) {
                                currentServerAdministrator->activateCustomer(customerId, threadId);
                            } else {
                                currentServerAdministrator->deactivateCustomer(customerId, threadId);
                            }

                            responseObject.insert("status", "OK");
                            responseObject.insert(
                                "customer",
                                convertToJson(customerCapabilities, true, true, true)
                            );

                            response = RestApiInV1::JsonResponse(responseObject);
                        } else {
                            responseObject.insert("status", "failed, could not update customer capabilities");
                        }
                    } else {
                        responseObject.insert("status", "failed, invalid expiration period");
                    }
                } else {
                    responseObject.insert("status", "failed, invalid number of monitors");
                }
            } else {
                responseObject.insert("status", "failed, invalid customer ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerCapabilitiesManager::CustomerCapabilitiesDelete
*/

CustomerCapabilitiesManager::CustomerCapabilitiesDelete::CustomerCapabilitiesDelete(
        const QByteArray&      secret,
        CustomersCapabilities* customerCapabilitiesDatabaseApi,
        ServerAdministrator*   serverAdministrator
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentCustomersCapabilities(
        customerCapabilitiesDatabaseApi
    ),currentServerAdministrator(
        serverAdministrator
    ) {}


CustomerCapabilitiesManager::CustomerCapabilitiesDelete::~CustomerCapabilitiesDelete() {}


RestApiInV1::JsonResponse CustomerCapabilitiesManager::CustomerCapabilitiesDelete::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() >= 1 && object.contains("customer_id")) {
            QJsonObject responseObject;

            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0) {
                CustomerCapabilities::CustomerId customerId = static_cast<CustomerCapabilities::CustomerId>(
                    customerIdDouble
                );

                bool success = currentServerAdministrator->deactivateCustomer(customerId, threadId);
                if (success) {
                    success = currentCustomersCapabilities->deleteCustomerCapabilities(customerId, threadId);
                    if (success) {
                        responseObject.insert("status", "OK");
                    } else {
                        responseObject.insert("status", "failed, could not delete customer");
                    }
                } else {
                    responseObject.insert("status", "failed, could not deactivate customer");
                }
            } else {
                responseObject.insert("status", "failed, invalid customer ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerCapabilitiesManager::CustomerCapabilitiesPurge
*/

CustomerCapabilitiesManager::CustomerCapabilitiesPurge::CustomerCapabilitiesPurge(
        const QByteArray&      secret,
        CustomersCapabilities* customerCapabilitiesDatabaseApi,
        ServerAdministrator*   serverAdministrator
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentCustomersCapabilities(
        customerCapabilitiesDatabaseApi
    ),currentServerAdministrator(
        serverAdministrator
    ) {}


CustomerCapabilitiesManager::CustomerCapabilitiesPurge::~CustomerCapabilitiesPurge() {}


RestApiInV1::JsonResponse CustomerCapabilitiesManager::CustomerCapabilitiesPurge::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isArray()) {
        QJsonObject   responseObject;

        QJsonArray    array             = request.array();
        unsigned long numberCustomerIds = static_cast<unsigned long>(array.size());
        unsigned long index             = 0;
        bool          success           = true;

        CustomersCapabilities::CustomerIdSet customerIds;
        while (success && index < numberCustomerIds) {
            double customerIdDouble = array.at(index).toDouble(-1.0);
            if (customerIdDouble >= 1.0 && customerIdDouble <= 0xFFFFFFFF) {
                CustomerCapabilities::CustomerId customerId = static_cast<CustomerCapabilities::CustomerId>(
                    customerIdDouble
                );
                success = currentServerAdministrator->deactivateCustomer(customerId, threadId);
                if (success) {
                    customerIds.insert(customerId);
                    ++index;
                } else {
                    responseObject.insert(
                        "status",
                        QString("failed, could not deactivate customer %1)invalid customer ID").arg(customerId)
                    );
                }
            } else {
                success = false;
                responseObject.insert("status", "failed, invalid customer ID");
            }
        }

        if (success) {
            if (static_cast<unsigned long>(customerIds.size()) == numberCustomerIds) {
                success = currentCustomersCapabilities->purgeCustomerCapabilities(customerIds, threadId);
                if (success) {
                    responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "failed");
                }
            } else {
                responseObject.insert("status", "failed, duplicate customer ID");
            }
        }

        response = RestApiInV1::JsonResponse(responseObject);
    }

    return response;
}

/***********************************************************************************************************************
* CustomerCapabilitiesManager::CustomerCapabilitiesList
*/

CustomerCapabilitiesManager::CustomerCapabilitiesList::CustomerCapabilitiesList(
        const QByteArray& secret,
        CustomersCapabilities*          customerCapabilitiesDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentCustomersCapabilities(
        customerCapabilitiesDatabaseApi
    ) {}


CustomerCapabilitiesManager::CustomerCapabilitiesList::~CustomerCapabilitiesList() {}


RestApiInV1::JsonResponse CustomerCapabilitiesManager::CustomerCapabilitiesList::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 0) {
            CustomersCapabilities::CapabilitiesByCustomerId
                ccbyid = currentCustomersCapabilities->getAllCustomerCapabilities(threadId);

            QJsonObject resultObject;
            for (  CustomersCapabilities::CapabilitiesByCustomerId::const_iterator
                       it = ccbyid.constBegin(), end = ccbyid.constEnd()
                 ; it != end
                 ; ++it
                ) {
                resultObject.insert(QString::number(it.key()), convertToJson(it.value(), true, true, true));
            }

            QJsonObject responseObject;
            responseObject.insert("status", "OK");
            responseObject.insert("customers", resultObject);

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerCapabilitiesManager::CustomerGetSecret
*/

CustomerCapabilitiesManager::CustomerGetSecret::CustomerGetSecret(
        const QByteArray& secret,
        CustomerSecrets*  customerSecretsDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentCustomerSecrets(
        customerSecretsDatabaseApi
    ) {}


CustomerCapabilitiesManager::CustomerGetSecret::~CustomerGetSecret() {}


RestApiInV1::JsonResponse CustomerCapabilitiesManager::CustomerGetSecret::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() >= 1 && object.contains("customer_id")) {
            QJsonObject responseObject;

            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0) {
                CustomerCapabilities::CustomerId customerId = static_cast<CustomerCapabilities::CustomerId>(
                    customerIdDouble
                );

                CustomerSecret customerSecret = currentCustomerSecrets->getCustomerSecret(customerId, true, threadId);
                if (customerSecret.isValid()) {
                    QByteArray     base64Encoded  = customerSecret.customerSecret().toBase64();

                    std::uint64_t customerIdentifier       = currentCustomerSecrets->toCustomerIdentifier(customerId);
                    QString       customerIdentifierString = QString::number(customerIdentifier, 16);
                    while (customerIdentifierString.length() != 16) {
                        customerIdentifierString.prepend(QChar('0'));
                    }

                    QJsonObject resultObject;
                    resultObject.insert("identifier", customerIdentifierString);
                    resultObject.insert("secret", QString::fromUtf8(base64Encoded));

                    responseObject.insert("status", "OK");
                    responseObject.insert("customer", resultObject);
                } else {
                    responseObject.insert("status", "failed, unknown customer ID");
                }
            } else {
                responseObject.insert("status", "failed, invalid customer ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerCapabilitiesManager::CustomerResetSecret
*/

CustomerCapabilitiesManager::CustomerResetSecret::CustomerResetSecret(
        const QByteArray& secret,
        CustomerSecrets*  customerSecretsDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentCustomerSecrets(
        customerSecretsDatabaseApi
    ) {}


CustomerCapabilitiesManager::CustomerResetSecret::~CustomerResetSecret() {}


RestApiInV1::JsonResponse CustomerCapabilitiesManager::CustomerResetSecret::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() >= 1 && object.contains("customer_id")) {
            QJsonObject responseObject;

            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0) {
                CustomerCapabilities::CustomerId customerId = static_cast<CustomerCapabilities::CustomerId>(
                    customerIdDouble
                );

                CustomerSecret customerSecret = currentCustomerSecrets->updateCustomerSecret(customerId, threadId);
                if (customerSecret.isValid()) {
                    responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "failed, unknown customer ID");
                }
            } else {
                responseObject.insert("status", "failed, invalid customer ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerCapabilitiesManager::CustomerCapabilitiesPause
*/

CustomerCapabilitiesManager::CustomerCapabilitiesPause::CustomerCapabilitiesPause(
        const QByteArray&    secret,
        ServerAdministrator* serverAdministrator
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServerAdministrator(
        serverAdministrator
    ) {}


CustomerCapabilitiesManager::CustomerCapabilitiesPause::~CustomerCapabilitiesPause() {}


RestApiInV1::JsonResponse CustomerCapabilitiesManager::CustomerCapabilitiesPause::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 2 && object.contains("customer_id") && object.contains("pause")) {
            QJsonObject responseObject;

            bool   nowPaused        = object.value("pause").toBool();
            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0) {
                CustomerCapabilities::CustomerId customerId = static_cast<CustomerCapabilities::CustomerId>(
                    customerIdDouble
                );

                bool success = currentServerAdministrator->setPaused(customerId, nowPaused, threadId);
                if (success) {
                    responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "failed, unable to update");
                }
            } else {
                responseObject.insert("status", "failed, invalid customer ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* CustomerCapabilitiesManager
*/

const QString CustomerCapabilitiesManager::customerCapabilitiesGetPath("/customer/get");
const QString CustomerCapabilitiesManager::customerCapabilitiesUpdatePath("/customer/create");
const QString CustomerCapabilitiesManager::customerCapabilitiesDeletePath("/customer/delete");
const QString CustomerCapabilitiesManager::customerCapabilitiesPurgePath("/customer/purge");
const QString CustomerCapabilitiesManager::customerCapabilitiesListPath("/customer/list");
const QString CustomerCapabilitiesManager::customerGetSecretPath("/customer/get_secret");
const QString CustomerCapabilitiesManager::customerResetSecretPath("/customer/reset_secret");
const QString CustomerCapabilitiesManager::customerCapabilitiesPausePath("/customer/pause");

CustomerCapabilitiesManager::CustomerCapabilitiesManager(
        RestApiInV1::Server*   restApiServer,
        CustomersCapabilities* customerCapabilitiesDatabaseApi,
        CustomerSecrets*       customerSecretsDatabaseApi,
        ServerAdministrator*   serverAdministrator,
        const QByteArray&      secret,
        QObject*               parent
    ):QObject(
        parent
    ),customerCapabilitiesGet(
        secret,
        customerCapabilitiesDatabaseApi
    ),customerCapabilitiesUpdate(
        secret,
        customerCapabilitiesDatabaseApi,
        serverAdministrator,
        customerSecretsDatabaseApi
    ),customerCapabilitiesDelete(
        secret,
        customerCapabilitiesDatabaseApi,
        serverAdministrator
    ),customerCapabilitiesPurge(
        secret,
        customerCapabilitiesDatabaseApi,
        serverAdministrator
    ),customerCapabilitiesList(
        secret,
        customerCapabilitiesDatabaseApi
    ),customerGetSecret(
        secret,
        customerSecretsDatabaseApi
    ),customerResetSecret(
        secret,
        customerSecretsDatabaseApi
    ),customerCapabilitiesPause(
        secret,
        serverAdministrator
    ) {
    restApiServer->registerHandler(
        &customerCapabilitiesGet,
        RestApiInV1::Handler::Method::POST,
        customerCapabilitiesGetPath
    );
    restApiServer->registerHandler(
        &customerCapabilitiesUpdate,
        RestApiInV1::Handler::Method::POST,
        customerCapabilitiesUpdatePath
    );
    restApiServer->registerHandler(
        &customerCapabilitiesDelete,
        RestApiInV1::Handler::Method::POST,
        customerCapabilitiesDeletePath
    );
    restApiServer->registerHandler(
        &customerCapabilitiesPurge,
        RestApiInV1::Handler::Method::POST,
        customerCapabilitiesPurgePath
    );
    restApiServer->registerHandler(
        &customerCapabilitiesList,
        RestApiInV1::Handler::Method::POST,
        customerCapabilitiesListPath
    );
    restApiServer->registerHandler(
        &customerGetSecret,
        RestApiInV1::Handler::Method::POST,
        customerGetSecretPath
    );
    restApiServer->registerHandler(
        &customerResetSecret,
        RestApiInV1::Handler::Method::POST,
        customerResetSecretPath
    );
    restApiServer->registerHandler(
        &customerCapabilitiesPause,
        RestApiInV1::Handler::Method::POST,
        customerCapabilitiesPausePath
    );
}


CustomerCapabilitiesManager::~CustomerCapabilitiesManager() {}


void CustomerCapabilitiesManager::setSecret(const QByteArray& newSecret) {
    customerCapabilitiesGet.setSecret(newSecret);
    customerCapabilitiesUpdate.setSecret(newSecret);
    customerCapabilitiesDelete.setSecret(newSecret);
    customerCapabilitiesPurge.setSecret(newSecret);
    customerCapabilitiesList.setSecret(newSecret);
    customerGetSecret.setSecret(newSecret);
    customerResetSecret.setSecret(newSecret);
    customerCapabilitiesPause.setSecret(newSecret);
}
