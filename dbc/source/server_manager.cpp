/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref ServerManager class.
***********************************************************************************************************************/

#include <QObject>
#include <QByteArray>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "log.h"
#include "region.h"
#include "regions.h"
#include "server.h"
#include "servers.h"
#include "server_administrator.h"
#include "server_manager.h"

/***********************************************************************************************************************
* ServerManager::ServerGet
*/

ServerManager::ServerGet::ServerGet(
        const QByteArray&    secret,
        ServerAdministrator* serverAdministrator
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServerAdministrator(
        serverAdministrator
    ) {}


ServerManager::ServerGet::~ServerGet() {}


RestApiInV1::JsonResponse ServerManager::ServerGet::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 1) {
            Server  server;
            QString statusResponse("OK");

            if (object.contains("server_id")) {
                int serverIdInteger = object.value("server_id").toInt(-1);
                if (serverIdInteger >= 0 && serverIdInteger <= 0xFFFF) {
                    Server::ServerId serverId = static_cast<Server::ServerId>(serverIdInteger);
                    server = currentServerAdministrator->getServer(serverId, threadId);
                    if (server.isInvalid()) {
                        statusResponse = QString("invalid server ID");
                    }
                } else {
                    statusResponse = QString("invalid server ID");
                }
            } else if (object.contains("identifier")) {
                QString serverIdentifier = object.value("identifier").toString();
                server = currentServerAdministrator->getServer(serverIdentifier, threadId);
                if (server.isInvalid()) {
                    statusResponse = QString("invalid address");
                }
            } else {
                statusResponse = QString("invalid request");
            }

            QJsonObject responseObject;
            responseObject.insert("status", statusResponse);

            if (server.isValid()) {
                responseObject.insert("server_id", server.serverId());
                responseObject.insert("region_id", server.regionId());
                responseObject.insert("identifier", server.identifier());
                responseObject.insert("server_status", Server::toString(server.status()).toLower());
                responseObject.insert("monitor_service_rate", static_cast<int>(server.monitorsPerSecond()));
                responseObject.insert("cpu_loading", server.cpuLoading());
                responseObject.insert("memory_loading", server.memoryLoading());
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* ServerManager::ServerCreate
*/

ServerManager::ServerCreate::ServerCreate(
        const QByteArray&    secret,
        ServerAdministrator* serverAdministrator,
        Regions*             regionDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServerAdministrator(
        serverAdministrator
    ),currentRegions(
        regionDatabaseApi
    ) {}


ServerManager::ServerCreate::~ServerCreate() {}


RestApiInV1::JsonResponse ServerManager::ServerCreate::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 2 && object.contains("region_id") && object.contains("identifier")) {
            QJsonObject responseObject;

            int regionIdInteger = object.value("region_id").toInt(-1);
            if (regionIdInteger >= 0 && regionIdInteger <= 0xFFFF) {
                Region::RegionId regionId         = static_cast<Region::RegionId>(regionIdInteger);
                QString          serverIdentifier = object.value("identifier").toString();

                Region region = currentRegions->getRegion(regionId, threadId);
                if (region.isValid()) {
                    Server server = currentServerAdministrator->createServer(
                        regionId,
                        serverIdentifier,
                        Server::Status::INACTIVE
                    );

                    if (server.isValid()) {
                        logWrite(
                            QString("Created server %1 identified by %2").arg(server.serverId()).arg(serverIdentifier),
                            false
                        );

                        responseObject.insert("status", "OK");
                        responseObject.insert("server_id", server.serverId());
                        responseObject.insert("region_id", server.regionId());
                        responseObject.insert("identifier", serverIdentifier);
                        responseObject.insert("server_status", Server::toString(server.status()).toLower());
                        responseObject.insert("monitor_service_rate", static_cast<int>(server.monitorsPerSecond()));
                        responseObject.insert("cpu_loading", server.cpuLoading());
                        responseObject.insert("memory_loading", server.memoryLoading());
                    } else {
                        responseObject.insert("status", "failed to create server");
                    }
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
* ServerManager::ServerModify
*/

ServerManager::ServerModify::ServerModify(
        const QByteArray&    secret,
        ServerAdministrator* serverAdministrator,
        Regions*             regionDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServerAdministrator(
        serverAdministrator
    ),currentRegions(
        regionDatabaseApi
    ) {}


ServerManager::ServerModify::~ServerModify() {}


RestApiInV1::JsonResponse ServerManager::ServerModify::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() >= 2 && object.contains("server_id")) {
            QJsonObject responseObject;

            int serverIdInteger = object.value("server_id").toInt(-1);
            if (serverIdInteger >= 0 && serverIdInteger <= 0xFFFF) {
                Server::ServerId serverId = static_cast<Server::ServerId>(serverIdInteger);
                Server           server   = currentServerAdministrator->getServer(serverId, threadId);
                if (server.isValid()) {
                    bool success  = true;
                    bool modified = false;
                    if (object.contains("region_id")) {
                        int regionIdInteger = object.value("region_id").toInt(-1);
                        if (regionIdInteger >= 0 && regionIdInteger <= 0xFFFF) {
                            Region::RegionId regionId = static_cast<Region::RegionId>(regionIdInteger);
                            Region region = currentRegions->getRegion(regionId, threadId);
                            if (region.isValid()) {
                                server.setRegionId(regionId);
                                modified = true;
                            } else {
                                responseObject.insert("status", "unknown region ID");
                                success = false;
                            }
                        } else {
                            responseObject.insert("status", "invalid region ID");
                            success = false;
                        }
                    }

                    if (success && object.contains("server_status")) {
                        QString        serverStatusString = object.value("server_status").toString();
                        Server::Status serverStatus       = Server::toStatus(serverStatusString, &success);

                        if (success) {
                            server.setStatus(serverStatus);
                            modified = true;
                        } else {
                            responseObject.insert("status", "invalid server status");
                        }
                    }

                    if (success && object.contains("identifier")) {
                        QString serverIdentifier = object.value("identifier").toString();
                        server.setIdentifier(serverIdentifier);
                        modified = true;
                    }

                    if (success && modified) {
                        success = currentServerAdministrator->modifyServer(server, threadId);
                        if (success) {
                            logWrite(
                                QString("Modified server %1 identified by %2")
                                .arg(server.serverId())
                                .arg(server.identifier()),
                                false
                            );

                            responseObject.insert("status", "OK");
                            responseObject.insert("server_id", server.serverId());
                            responseObject.insert("region_id", server.regionId());
                            responseObject.insert("identifier", server.identifier());
                            responseObject.insert("server_status", Server::toString(server.status()).toLower());
                            responseObject.insert("monitor_service_rate", static_cast<int>(server.monitorsPerSecond()));
                            responseObject.insert("cpu_loading", server.cpuLoading());
                            responseObject.insert("memory_loading", server.memoryLoading());
                        } else {
                            responseObject.insert("status", "failed to modify server.");
                        }
                    }
                } else {
                    responseObject.insert("status", "unknown server ID");
                }
            } else {
                responseObject.insert("status", "invalid server ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* ServerManager::ServerDelete
*/

ServerManager::ServerDelete::ServerDelete(
        const QByteArray&    secret,
        ServerAdministrator* serverAdministrator
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServerAdministrator(
        serverAdministrator
    ) {}


ServerManager::ServerDelete::~ServerDelete() {}


RestApiInV1::JsonResponse ServerManager::ServerDelete::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 1 && object.contains("server_id")) {
            QJsonObject responseObject;

            int serverIdInteger = object.value("server_id").toInt(-1);
            if (serverIdInteger >= 0 && serverIdInteger <= 0xFFFF) {
                Server::ServerId serverId = static_cast<Server::ServerId>(serverIdInteger);
                bool success = currentServerAdministrator->deleteServer(serverId, threadId);
                if (success) {
                    logWrite(QString("Deleted server %1").arg(serverId), false);
                     responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "unknown server ID");
                }
            } else {
                responseObject.insert("status", "invalid server ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* ServerManager::ServerList
*/

ServerManager::ServerList::ServerList(
        const QByteArray&    secret,
        ServerAdministrator* serverAdministrator
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServerAdministrator(
        serverAdministrator
    ) {}


ServerManager::ServerList::~ServerList() {}


RestApiInV1::JsonResponse ServerManager::ServerList::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();

        QJsonObject responseObject;
        bool        success = true;

        Region::RegionId regionId     = Region::invalidRegionId;
        Server::Status   serverStatus = Server::Status::ALL_UNKNOWN;

        if (object.contains("region_id")) {
            int regionIdInteger = object.value("region_id").toInt(-1);
            if (regionIdInteger >= 0 && regionIdInteger <= 0xFFFF) {
                regionId = static_cast<Region::RegionId>(regionIdInteger);
            } else {
                responseObject.insert("status", "invalid region ID");
                success = false;
            }
        }

        if (object.contains("server_status")) {
            QString serverStatusString = object.value("server_status").toString();
            serverStatus = Server::toStatus(serverStatusString, &success);

            if (!success) {
                responseObject.insert("status", "invalid server status");
            }
        }

        if (success) {
            Servers::ServerList serverList = currentServerAdministrator->getServers(regionId, serverStatus, threadId);
            QJsonArray serversList;
            for (  Servers::ServerList::const_iterator it = serverList.constBegin(), end = serverList.constEnd()
                 ; it!=end
                 ; ++it
                ) {
                const Server& server = *it;
                QJsonObject serverObject;

                serverObject.insert("server_id", server.serverId());
                serverObject.insert("region_id", server.regionId());
                serverObject.insert("identifier", server.identifier());
                serverObject.insert("server_status", Server::toString(server.status()).toLower());
                serverObject.insert("monitor_service_rate", static_cast<int>(server.monitorsPerSecond()));
                serverObject.insert("cpu_loading", server.cpuLoading());
                serverObject.insert("memory_loading", server.memoryLoading());

                serversList.append(serverObject);
            }

            responseObject.insert("status", "OK");
            responseObject.insert("servers", serversList);
        }

        response = RestApiInV1::JsonResponse(responseObject);
    }

    return response;
}

/***********************************************************************************************************************
* ServerManager::ServerActivate
*/

ServerManager::ServerActivate::ServerActivate(
        const QByteArray&    secret,
        ServerAdministrator* serverAdministrator
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServerAdministrator(
        serverAdministrator
    ) {}


ServerManager::ServerActivate::~ServerActivate() {}


RestApiInV1::JsonResponse ServerManager::ServerActivate::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.contains("server_id")) {
            QJsonObject responseObject;

            int serverIdInteger = object.value("server_id").toInt(-1);
            if (serverIdInteger > 0 && serverIdInteger <= 0xFFFF) {
                Server::ServerId serverId = static_cast<Server::ServerId>(serverIdInteger);
                bool success =  currentServerAdministrator->changeServerStatus(
                    serverId,
                    Server::Status::ACTIVE,
                    threadId
                );

                if (success) {
                    logWrite(QString("Activated server %1").arg(serverId), false);
                    responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "activation error");
                }
            } else {
                responseObject.insert("status", "invalid server ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* ServerManager::ServerDeactivate
*/

ServerManager::ServerDeactivate::ServerDeactivate(
        const QByteArray&    secret,
        ServerAdministrator* serverAdministrator
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServerAdministrator(
        serverAdministrator
    ) {}


ServerManager::ServerDeactivate::~ServerDeactivate() {}


RestApiInV1::JsonResponse ServerManager::ServerDeactivate::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.contains("server_id")) {
            QJsonObject responseObject;

            int serverIdInteger = object.value("server_id").toInt(-1);
            if (serverIdInteger > 0 && serverIdInteger <= 0xFFFF) {
                Server::ServerId serverId = static_cast<Server::ServerId>(serverIdInteger);
                bool success =  currentServerAdministrator->changeServerStatus(
                    serverId,
                    Server::Status::INACTIVE,
                    threadId
                );

                if (success) {
                    logWrite(QString("Deactivated server %1").arg(serverId), false);
                    responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "deactivation error");
                }
            } else {
                responseObject.insert("status", "invalid server ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* ServerManager::ServerStart
*/

ServerManager::ServerStart::ServerStart(
        const QByteArray&    secret,
        ServerAdministrator* serverAdministrator
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServerAdministrator(
        serverAdministrator
    ) {}


ServerManager::ServerStart::~ServerStart() {}


RestApiInV1::JsonResponse ServerManager::ServerStart::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.contains("server_id")) {
            QJsonObject responseObject;

            int serverIdInteger = object.value("server_id").toInt(-1);
            if (serverIdInteger > 0 && serverIdInteger <= 0xFFFF) {
                Server::ServerId serverId = static_cast<Server::ServerId>(serverIdInteger);
                bool success =  currentServerAdministrator->startServer(serverId, threadId);

                if (success) {
                    logWrite(QString("Started server %1").arg(serverId), false);
                    responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "activation error");
                }
            } else {
                responseObject.insert("status", "invalid server ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* ServerManager::ServerReassign
*/

ServerManager::ServerReassign::ServerReassign(
        const QByteArray& secret,
        ServerAdministrator*          serverAdministrator
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServerAdministrator(
        serverAdministrator
    ) {}


ServerManager::ServerReassign::~ServerReassign() {}


RestApiInV1::JsonResponse ServerManager::ServerReassign::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();

        QJsonObject responseObject;

        Server::ServerId fromServerId = Server::invalidServerId;
        bool             success      = true;
        unsigned         numberFields = 0;
        if (object.contains("from_server_id")) {
            int fromServerIdInteger = object.value("from_server_id").toInt(-1);
            if (fromServerIdInteger >= 0 && fromServerIdInteger <= 0xFFFF) {
                fromServerId = static_cast<Server::ServerId>(fromServerIdInteger);
            } else {
                responseObject.insert("status", "invalid from server ID");
                success = false;
            }

            ++numberFields;
        }

        Server::ServerId toServerId = Server::invalidServerId;
        if (object.contains("to_server_id")) {
            int toServerIdInteger = object.value("to_server_id").toInt(-1);
            if (toServerIdInteger >= 0 && toServerIdInteger <= 0xFFFF) {
                toServerId = static_cast<Server::ServerId>(toServerIdInteger);
            } else {
                responseObject.insert("status", "invalid to server ID");
                success = false;
            }

            ++numberFields;
        }

        ServerAdministrator::CustomerList customerList;
        if (object.contains("customers")) {
            QJsonValue customerListValue = object.value("customers");
            if (customerListValue.isArray()) {
                QJsonArray customerListArray = customerListValue.toArray();
                QJsonArray::const_iterator customerListIterator    = customerListArray.constBegin();
                QJsonArray::const_iterator customerListEndIterator = customerListArray.constEnd();
                while (success && customerListIterator != customerListEndIterator) {
                    const QJsonValue& entryValue       = *customerListIterator;
                    double            customerIdDouble = entryValue.toDouble(-1);
                    if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                        customerList.append(static_cast<CustomerCapabilities::CustomerId>(customerIdDouble));
                        ++customerListIterator;
                    } else {
                        success = false;
                    }
                }
            }

            ++numberFields;
        }

        if (success && static_cast<unsigned>(object.size()) <= numberFields) {
            if (fromServerId != Server::invalidServerId) {
                bool success = currentServerAdministrator->reassignWorkload(
                    fromServerId,
                    customerList,
                    toServerId,
                    threadId
                );

                if (success) {
                    if (toServerId != Server::invalidServerId) {
                        logWrite(
                            QString("Reassigned work from server %1 to %2").arg(fromServerId).arg(toServerId),
                            false
                        );
                    } else {
                        logWrite(QString("Reassigned work away from server %1").arg(fromServerId), false);
                    }

                    responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "failed");
                }
            } else {
                responseObject.insert("status", "invalid from server ID");
            }
        }

        response = RestApiInV1::JsonResponse(responseObject);
    }

    return response;
}

/***********************************************************************************************************************
* ServerManager::ServerRedistribute
*/

ServerManager::ServerRedistribute::ServerRedistribute(
        const QByteArray&    secret,
        ServerAdministrator* serverAdministrator
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServerAdministrator(
        serverAdministrator
    ) {}


ServerManager::ServerRedistribute::~ServerRedistribute() {}


RestApiInV1::JsonResponse ServerManager::ServerRedistribute::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             /* threadId */
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();

        if (object.contains("region_id")) {
            QJsonObject responseObject;

            int regionIdInteger = object.value("region_id").toInt(-1);
            if (regionIdInteger >= 0 && regionIdInteger <= 0xFFFF) {
                bool success = true; // TODO: DO NASTY THINGS HERE

                if (success) {
                    logWrite(QString("Redistributed workload, region %1").arg(regionIdInteger), false);
                    responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "failed");
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
* ServerManager
*/

const QString ServerManager::serverGetPath("/server/get");
const QString ServerManager::serverCreatePath("/server/create");
const QString ServerManager::serverModifyPath("/server/modify");
const QString ServerManager::serverDeletePath("/server/delete");
const QString ServerManager::serverListPath("/server/list");
const QString ServerManager::serverActivatePath("/server/activate");
const QString ServerManager::serverDeactivatePath("/server/deactivate");
const QString ServerManager::serverStartPath("/server/start");
const QString ServerManager::serverReassignPath("/server/reassign");
const QString ServerManager::serverRedistributePath("/server/redistribute");

ServerManager::ServerManager(
        RestApiInV1::Server* restApiServer,
        ServerAdministrator*             serverAdministrator,
        Regions*             regionDatabaseApi,
        const QByteArray&    secret,
        QObject*             parent
    ):QObject(
        parent
    ),serverGet(
        secret,
        serverAdministrator
    ),serverCreate(
        secret,
        serverAdministrator,
        regionDatabaseApi
    ),serverModify(
        secret,
        serverAdministrator,
        regionDatabaseApi
    ),serverDelete(
        secret,
        serverAdministrator
    ),serverList(
        secret,
        serverAdministrator
    ),serverActivate(
        secret,
        serverAdministrator
    ),serverDeactivate(
        secret,
        serverAdministrator
    ),serverStart(
        secret,
        serverAdministrator
    ),serverReassign(
        secret,
        serverAdministrator
    ),serverRedistribute(
        secret,
        serverAdministrator
    ) {
    restApiServer->registerHandler(&serverGet, RestApiInV1::Handler::Method::POST, serverGetPath);
    restApiServer->registerHandler(&serverCreate, RestApiInV1::Handler::Method::POST, serverCreatePath);
    restApiServer->registerHandler(&serverModify, RestApiInV1::Handler::Method::POST, serverModifyPath);
    restApiServer->registerHandler(&serverDelete, RestApiInV1::Handler::Method::POST, serverDeletePath);
    restApiServer->registerHandler(&serverList, RestApiInV1::Handler::Method::POST, serverListPath);
    restApiServer->registerHandler(&serverActivate, RestApiInV1::Handler::Method::POST, serverActivatePath);
    restApiServer->registerHandler(&serverDeactivate, RestApiInV1::Handler::Method::POST, serverDeactivatePath);
    restApiServer->registerHandler(&serverStart, RestApiInV1::Handler::Method::POST, serverStartPath);
    restApiServer->registerHandler(&serverReassign, RestApiInV1::Handler::Method::POST, serverReassignPath);
    restApiServer->registerHandler(&serverRedistribute, RestApiInV1::Handler::Method::POST, serverRedistributePath);
}


ServerManager::~ServerManager() {}


void ServerManager::setSecret(const QByteArray& newSecret) {
    serverGet.setSecret(newSecret);
    serverCreate.setSecret(newSecret);
    serverModify.setSecret(newSecret);
    serverDelete.setSecret(newSecret);
    serverList.setSecret(newSecret);
    serverActivate.setSecret(newSecret);
    serverDeactivate.setSecret(newSecret);
    serverStart.setSecret(newSecret);
    serverReassign.setSecret(newSecret);
    serverRedistribute.setSecret(newSecret);
}
