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
* This header implements the \ref RegionManager class.
***********************************************************************************************************************/

#include <QObject>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "log.h"
#include "regions.h"
#include "region_manager.h"

/***********************************************************************************************************************
* RegionManager::RegionGet
*/

RegionManager::RegionGet::RegionGet(
        const QByteArray& secret,
        Regions*          regionDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentRegions(
        regionDatabaseApi
    ) {}


RegionManager::RegionGet::~RegionGet() {}


RestApiInV1::JsonResponse RegionManager::RegionGet::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 1 && object.contains("region_id")) {
            int  regionIdInteger = object.value("region_id").toInt(-1);
            if (regionIdInteger >= 0 && regionIdInteger <= 0xFFFF) {
                Region::RegionId regionId = static_cast<Region::RegionId>(regionIdInteger);
                Region region = currentRegions->getRegion(regionId, threadId);

                QJsonObject responseObject;
                if (region.isValid()) {
                    responseObject.insert("status", "OK");
                    responseObject.insert("region_id", regionId);
                    responseObject.insert("region_name", region.regionName());
                } else {
                    responseObject.insert("status", "invalid region ID");
                }

                response = RestApiInV1::JsonResponse(QJsonDocument(responseObject));
            }
        }
    }

    return response;
}

/***********************************************************************************************************************
* RegionManager::RegionCreate
*/

RegionManager::RegionCreate::RegionCreate(
        const QByteArray& secret,
        Regions*          regionDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentRegions(
        regionDatabaseApi
    ) {}


RegionManager::RegionCreate::~RegionCreate() {}


RestApiInV1::JsonResponse RegionManager::RegionCreate::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 1 && object.contains("region_name")) {
            QString regionName = object.value("region_name").toString();
            Region region      = currentRegions->createRegion(regionName, threadId);

            QJsonObject responseObject;
            if (region.isValid()) {
                logWrite(QString("Created new region %1 = %2").arg(region.regionName()).arg(region.regionId()), false);

                responseObject.insert("status", "OK");
                responseObject.insert("region_id", region.regionId());
                responseObject.insert("region_name", region.regionName());
            } else {
                responseObject.insert("status", "failed");
            }

            response = RestApiInV1::JsonResponse(QJsonDocument(responseObject));
        }
    }

    return response;
}

/***********************************************************************************************************************
* RegionManager::RegionModify
*/

RegionManager::RegionModify::RegionModify(
        const QByteArray& secret,
        Regions*          regionDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentRegions(
        regionDatabaseApi
    ) {}


RegionManager::RegionModify::~RegionModify() {}


RestApiInV1::JsonResponse RegionManager::RegionModify::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 2 && object.contains("region_id") && object.contains("region_name")) {
            int  regionIdInteger = object.value("region_id").toInt(-1);
            if (regionIdInteger >= 0 && regionIdInteger <= 0xFFFF) {
                Region::RegionId regionId   = static_cast<Region::RegionId>(regionIdInteger);
                QString          regionName = object.value("region_name").toString();


                QJsonObject responseObject;
                Region region = currentRegions->getRegion(regionId, threadId);
                if (region.isValid()) {
                    region.setRegionName(regionName);
                    bool success = currentRegions->modifyRegion(region, threadId);
                    if (success) {
                        logWrite(
                            QString("Modified region %1 -> %2").arg(region.regionId()).arg(region.regionName()),
                            false
                        );
                        responseObject.insert("status", "OK");
                    } else {
                        responseObject.insert("status", "modify failed");
                    }
                } else {
                    responseObject.insert("status", "invalid region ID");
                }

                response = RestApiInV1::JsonResponse(QJsonDocument(responseObject));
            }
        }
    }

    return response;
}

/***********************************************************************************************************************
* RegionManager::RegionDelete
*/

RegionManager::RegionDelete::RegionDelete(
        const QByteArray& secret,
        Regions*          regionDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentRegions(
        regionDatabaseApi
    ) {}


RegionManager::RegionDelete::~RegionDelete() {}


RestApiInV1::JsonResponse RegionManager::RegionDelete::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 1 && object.contains("region_id")) {
            int  regionIdInteger = object.value("region_id").toInt(-1);
            if (regionIdInteger >= 0 && regionIdInteger <= 0xFFFF) {
                Region::RegionId regionId = static_cast<Region::RegionId>(regionIdInteger);
                Region region = currentRegions->getRegion(regionId, threadId);

                QJsonObject responseObject;
                if (region.isValid()) {
                    bool success = currentRegions->deleteRegion(region, threadId);
                    if (success) {
                        logWrite(
                            QString("Deleted region %1 -> %2").arg(region.regionId()).arg(region.regionName()),
                            false
                        );

                        responseObject.insert("status", "OK");
                    } else {
                        responseObject.insert("status", "delete failed");
                    }
                } else {
                    responseObject.insert("status", "invalid region ID");
                }

                response = RestApiInV1::JsonResponse(QJsonDocument(responseObject));
            }
        }
    }

    return response;
}

/***********************************************************************************************************************
* RegionManager::RegionList
*/

RegionManager::RegionList::RegionList(
        const QByteArray& secret,
        Regions*          regionDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentRegions(
        regionDatabaseApi
    ) {}


RegionManager::RegionList::~RegionList() {}


RestApiInV1::JsonResponse RegionManager::RegionList::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& /* request */,
        unsigned             threadId
    ) {
    Regions::RegionHash regions = currentRegions->getAllRegions(threadId);
    QJsonObject         regionsObject;
    for (Regions::RegionHash::const_iterator it=regions.constBegin(),end=regions.constEnd() ; it!=end ; ++it) {
        regionsObject.insert(QString::number(it->regionId()), it->regionName());
    }

    QJsonObject responseObject;
    responseObject.insert("status", "OK");
    responseObject.insert("regions", regionsObject);

    return RestApiInV1::JsonResponse(QJsonDocument(responseObject));
}

/***********************************************************************************************************************
* RegionManager
*/

const QString RegionManager::regionGetPath("/region/get");
const QString RegionManager::regionCreatePath("/region/create");
const QString RegionManager::regionModifyPath("/region/modify");
const QString RegionManager::regionDeletePath("/region/delete");
const QString RegionManager::regionListPath("/region/list");

RegionManager::RegionManager(
        RestApiInV1::Server* restApiServer,
        Regions*             regionDatabaseApi,
        const QByteArray&    secret,
        QObject*             parent
    ):QObject(
        parent
    ),regionGet(
        secret,
        regionDatabaseApi
    ),regionCreate(
        secret,
        regionDatabaseApi
    ),regionModify(
        secret,
        regionDatabaseApi
    ),regionDelete(
        secret,
        regionDatabaseApi
    ),regionList(
        secret,
        regionDatabaseApi
    ) {
    restApiServer->registerHandler(&regionGet, RestApiInV1::Handler::Method::POST, regionGetPath);
    restApiServer->registerHandler(&regionCreate, RestApiInV1::Handler::Method::POST, regionCreatePath);
    restApiServer->registerHandler(&regionModify, RestApiInV1::Handler::Method::POST, regionModifyPath);
    restApiServer->registerHandler(&regionDelete, RestApiInV1::Handler::Method::POST, regionDeletePath);
    restApiServer->registerHandler(&regionList, RestApiInV1::Handler::Method::POST, regionListPath);
}


RegionManager::~RegionManager() {}


void RegionManager::setSecret(const QByteArray& newSecret) {
    regionGet.setSecret(newSecret);
    regionCreate.setSecret(newSecret);
    regionModify.setSecret(newSecret);
    regionDelete.setSecret(newSecret);
    regionList.setSecret(newSecret);
}
