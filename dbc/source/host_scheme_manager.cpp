/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref HostSchemeManager class.
***********************************************************************************************************************/

#include <QObject>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "host_scheme.h"
#include "host_schemes.h"
#include "monitor_updater.h"
#include "host_scheme_manager.h"

/***********************************************************************************************************************
* HostSchemeManager::HostSchemeGet
*/

HostSchemeManager::HostSchemeGet::HostSchemeGet(
        const QByteArray& secret,
        HostSchemes*      hostSchemeDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentHostSchemes(
        hostSchemeDatabaseApi
    ) {}


HostSchemeManager::HostSchemeGet::~HostSchemeGet() {}


RestApiInV1::JsonResponse HostSchemeManager::HostSchemeGet::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 1 && object.contains("host_scheme_id")) {
            QJsonObject responseObject;

            double hostSchemeIdDouble = object.value("host_scheme_id").toDouble(-1);
            if (hostSchemeIdDouble > 0 && hostSchemeIdDouble <= 0xFFFFFFFF) {
                HostScheme::HostSchemeId hostSchemeId = static_cast<HostScheme::HostSchemeId>(hostSchemeIdDouble);
                HostScheme hs = currentHostSchemes->getHostScheme(hostSchemeId, threadId);
                if (hs.isValid()) {
                    responseObject = convertToJson(hs, true);
                    responseObject.insert("status", "OK");
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
* HostSchemeManager::HostSchemeCreate
*/

HostSchemeManager::HostSchemeCreate::HostSchemeCreate(
        const QByteArray& secret,
        HostSchemes*      hostSchemeDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentHostSchemes(
        hostSchemeDatabaseApi
    ) {}


HostSchemeManager::HostSchemeCreate::~HostSchemeCreate() {}


RestApiInV1::JsonResponse HostSchemeManager::HostSchemeCreate::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 2 && object.contains("customer_id") && object.contains("url")) {
            QString urlString        = object.value("url").toString();
            double  customerIdDouble = object.value("customer_id").toDouble(-1);

            QJsonObject responseObject;

            if (customerIdDouble > 0 && customerIdDouble < 0xFFFFFFFF) {
                HostScheme::CustomerId customerId = static_cast<HostScheme::CustomerId>(customerIdDouble);
                QUrl    url(urlString);

                if (url.scheme() == QString("http")  ||
                    url.scheme() == QString("https") ||
                    url.scheme() == QString("ftp")   ||
                    url.scheme() == QString("sftp")     ) {
                    if (url.path().isEmpty() && !url.hasQuery() && !url.hasFragment()) {
                        HostScheme hs = currentHostSchemes->createHostScheme(customerId, url, threadId);
                        if (hs.isValid()) {
                            responseObject = convertToJson(hs, true);
                            responseObject.insert("status", "OK");
                        } else {
                            responseObject.insert("status", "failed to create host/scheme");
                        }
                    } else {
                        responseObject.insert("status", "url can-not have path, query, or fragment");
                    }
                } else {
                    responseObject.insert("status", "invalid scheme");
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
* HostSchemeManager::HostSchemeModify
*/

HostSchemeManager::HostSchemeModify::HostSchemeModify(
        const QByteArray& secret,
        HostSchemes*      hostSchemeDatabaseApi,
        MonitorUpdater*   monitorUpdater
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentHostSchemes(
        hostSchemeDatabaseApi
    ),currentMonitorUpdater(
        monitorUpdater
    ) {}


HostSchemeManager::HostSchemeModify::~HostSchemeModify() {}


RestApiInV1::JsonResponse HostSchemeManager::HostSchemeModify::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() >= 2 && object.contains("host_scheme_id")) {
            QJsonObject responseObject;

            double hostSchemeIdDouble = object.value("host_scheme_id").toDouble(-1);
            if (hostSchemeIdDouble > 0 && hostSchemeIdDouble <= 0xFFFFFFFF) {
                HostScheme::HostSchemeId hostSchemeId = static_cast<HostScheme::HostSchemeId>(hostSchemeIdDouble);
                HostScheme hostScheme = currentHostSchemes->getHostScheme(hostSchemeId, threadId);
                if (hostScheme.isValid()) {
                    bool                   success    = true;
                    HostScheme::CustomerId customerId = HostScheme::invalidCustomerId;
                    QUrl                   url;

                    if (object.contains("customer_id")) {
                        double customerIdDouble = object.value("customer_id").toDouble(-1);
                        if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                            customerId = static_cast<HostScheme::CustomerId>(customerIdDouble);
                        } else {
                            success = false;
                            responseObject.insert("status", "invalid customer ID");
                        }
                    }

                    if (success && object.contains("url")) {
                        QString urlString = object.value("url").toString();
                        url = QUrl(urlString);

                        if (url.scheme() == QString("http")  ||
                            url.scheme() == QString("https") ||
                            url.scheme() == QString("ftp")   ||
                            url.scheme() == QString("sftp")     ) {
                            if (!url.path().isEmpty() || url.hasQuery() || url.hasFragment()) {
                                success = false;
                                responseObject.insert("status", "url can-not have path, query, or fragment");
                            }
                        } else {
                            success = false;
                            responseObject.insert("status", "invalid scheme");
                        }
                    }

                    if (success) {
                        if (url.isValid()) {
                            hostScheme.setUrl(url);
                        }

                        if (customerId != HostScheme::invalidCustomerId) {
                            hostScheme.setCustomerId(customerId);
                        }

                        success = currentMonitorUpdater->modifyHostScheme(hostScheme, threadId);
                        if (success) {
                            responseObject = convertToJson(hostScheme, true);
                            responseObject.insert("status", "OK");
                        } else {
                            responseObject.insert("status", "failed to modify host/scheme");
                        }
                    }
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
* HostSchemeManager::HostSchemeCertificate
*/

HostSchemeManager::HostSchemeCertificate::HostSchemeCertificate(
        const QByteArray& secret,
        HostSchemes*      hostSchemeDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentHostSchemes(
        hostSchemeDatabaseApi
    ) {}


HostSchemeManager::HostSchemeCertificate::~HostSchemeCertificate() {}


RestApiInV1::JsonResponse HostSchemeManager::HostSchemeCertificate::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 3                      &&
            object.contains("monitor_id")           &&
            object.contains("host_scheme_id")       &&
            object.contains("expiration_timestamp")    ) {
            QJsonObject responseObject;

            double hostSchemeIdDouble = object.value("host_scheme_id").toDouble(-1);
            if (hostSchemeIdDouble > 0 && hostSchemeIdDouble <= 0xFFFFFFFF) {
                HostScheme::HostSchemeId hostSchemeId = static_cast<HostScheme::HostSchemeId>(hostSchemeIdDouble);
                HostScheme               hostScheme   = currentHostSchemes->getHostScheme(hostSchemeId, threadId);
                if (hostScheme.isValid()) {
                    double expirationTimestampDouble = object.value("expiration_timestamp").toDouble(-1);
                    if (expirationTimestampDouble > 0) {
                        unsigned long long timestamp = static_cast<unsigned long long>(expirationTimestampDouble);
                        hostScheme.setSslExpirationTimestamp(timestamp);
                        currentHostSchemes->modifyHostScheme(hostScheme, threadId);

                        responseObject.insert("status", "OK");
                    } else {
                        responseObject.insert("status", "invalid expiration timestamp.");
                    }
                } else {
                    responseObject.insert("status", "OK");
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
* HostSchemeManager::HostSchemeDelete
*/

HostSchemeManager::HostSchemeDelete::HostSchemeDelete(
        const QByteArray& secret,
        HostSchemes*      hostSchemeDatabaseApi,
        MonitorUpdater*   monitorUpdater
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentHostSchemes(
        hostSchemeDatabaseApi
    ),currentMonitorUpdater(
        monitorUpdater
    ) {}


HostSchemeManager::HostSchemeDelete::~HostSchemeDelete() {}


RestApiInV1::JsonResponse HostSchemeManager::HostSchemeDelete::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 1) {
            if (object.contains("host_scheme_id")) {
                QJsonObject responseObject;

                double hostSchemeIdDouble = object.value("host_scheme_id").toDouble(-1);
                if (hostSchemeIdDouble > 0 && hostSchemeIdDouble <= 0xFFFFFFFF) {
                    HostScheme::HostSchemeId hostSchemeId = static_cast<HostScheme::HostSchemeId>(hostSchemeIdDouble);
                    HostScheme hs = currentHostSchemes->getHostScheme(hostSchemeId, threadId);
                    if (hs.isValid()) {
                        bool success = currentHostSchemes->deleteHostScheme(hs, threadId);
                        if (success) {
                            responseObject.insert("status", "OK");
                        } else {
                            responseObject.insert("status", "failed to delete host/scheme");
                        }
                    } else {
                        responseObject.insert("status", "unknown host/scheme ID");
                    }
                } else {
                    responseObject.insert("status", "invalid host/scheme ID");
                }

                response = RestApiInV1::JsonResponse(responseObject);
            } else if (object.contains("customer_id")) {
                QJsonObject responseObject;

                double customerIdDouble = object.value("customer_id").toDouble(-1);
                if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                    HostScheme::CustomerId customerId = static_cast<HostScheme::CustomerId>(customerIdDouble);
                    bool success = currentMonitorUpdater->deleteCustomer(customerId, threadId);
                    if (success) {
                        responseObject.insert("status", "OK");
                    } else {
                        responseObject.insert("status", "failed to delete host/schemes for customer");
                    }
                } else {
                    responseObject.insert("status", "invalid customer ID");
                }

                response = RestApiInV1::JsonResponse(responseObject);
            }
        }
    }

    return response;
}

/***********************************************************************************************************************
* HostSchemeManager::HostSchemeList
*/

HostSchemeManager::HostSchemeList::HostSchemeList(
        const QByteArray& secret,
        HostSchemes*      hostSchemeDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentHostSchemes(
        hostSchemeDatabaseApi
    ) {}


HostSchemeManager::HostSchemeList::~HostSchemeList() {}


RestApiInV1::JsonResponse HostSchemeManager::HostSchemeList::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object  = request.object();
        bool        success = true;

        HostScheme::CustomerId customerId = HostScheme::invalidCustomerId;
        if (object.size() == 1 && object.contains("customer_id")) {
            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                customerId = static_cast<HostScheme::CustomerId>(customerIdDouble);
            } else {
                success = false;

                QJsonObject responseObject;
                responseObject.insert("status", "invalid customer ID");

                response = RestApiInV1::JsonResponse(responseObject);
            }
        } else if (object.size() > 1) {
            success = false;
        }

        if (success) {
            HostSchemes::HostSchemeHash hostSchemes = currentHostSchemes->getHostSchemes(customerId, threadId);

            QJsonObject responseObject;
            responseObject.insert("status", "OK");
            responseObject.insert("data", convertToJson(hostSchemes, true));

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* HostSchemeManager
*/

const QString HostSchemeManager::hostSchemeGetPath("/host_scheme/get");
const QString HostSchemeManager::hostSchemeCreatePath("/host_scheme/create");
const QString HostSchemeManager::hostSchemeModifyPath("/host_scheme/modify");
const QString HostSchemeManager::hostSchemeCertificatePath("/host_scheme/certificate");
const QString HostSchemeManager::hostSchemeDeletePath("/host_scheme/delete");
const QString HostSchemeManager::hostSchemeListPath("/host_scheme/list");

HostSchemeManager::HostSchemeManager(
        RestApiInV1::Server* restApiServer,
        HostSchemes*         hostSchemeDatabaseApi,
        MonitorUpdater*      monitorUpdater,
        const QByteArray&    secret,
        QObject*             parent
    ):QObject(
        parent
    ),hostSchemeGet(
        secret,
        hostSchemeDatabaseApi
    ),hostSchemeCreate(
        secret,
        hostSchemeDatabaseApi
    ),hostSchemeModify(
        secret,
        hostSchemeDatabaseApi,
        monitorUpdater
    ),hostSchemeCertificate(
        secret,
        hostSchemeDatabaseApi
    ),hostSchemeDelete(
        secret,
        hostSchemeDatabaseApi,
        monitorUpdater
    ),hostSchemeList(
        secret,
        hostSchemeDatabaseApi
    ) {
    restApiServer->registerHandler(
        &hostSchemeGet,
        RestApiInV1::Handler::Method::POST,
        hostSchemeGetPath
    );

    restApiServer->registerHandler(
        &hostSchemeCreate,
        RestApiInV1::Handler::Method::POST,
        hostSchemeCreatePath
    );

    restApiServer->registerHandler(
        &hostSchemeModify,
        RestApiInV1::Handler::Method::POST,
        hostSchemeModifyPath
    );

    restApiServer->registerHandler(
        &hostSchemeCertificate,
        RestApiInV1::Handler::Method::POST,
        hostSchemeCertificatePath
    );

    restApiServer->registerHandler(
        &hostSchemeDelete,
        RestApiInV1::Handler::Method::POST,
        hostSchemeDeletePath
    );

    restApiServer->registerHandler(
        &hostSchemeList,
        RestApiInV1::Handler::Method::POST,
        hostSchemeListPath
    );
}


HostSchemeManager::~HostSchemeManager() {}


void HostSchemeManager::setSecret(const QByteArray& newSecret) {
    hostSchemeGet.setSecret(newSecret);
    hostSchemeCreate.setSecret(newSecret);
    hostSchemeModify.setSecret(newSecret);
    hostSchemeCertificate.setSecret(newSecret);
    hostSchemeDelete.setSecret(newSecret);
    hostSchemeList.setSecret(newSecret);
}
