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
* This header implements the \ref ResourceManager class.
***********************************************************************************************************************/

#include <QObject>
#include <QByteArray>
#include <QBuffer>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_binary_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>
#include <rest_api_in_v1_inesonic_binary_rest_handler.h>

#include "log.h"
#include "resource.h"
#include "active_resources.h"
#include "resources.h"
#include "resource_plotter.h"
#include "plot_mailbox.h"
#include "resource_manager.h"

/***********************************************************************************************************************
* ResourceManager::ResourceAvailable
*/

ResourceManager::ResourceAvailable::ResourceAvailable(
        const QByteArray& secret,
        Resources*        resourceDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentResources(
        resourceDatabaseApi
    ) {}


ResourceManager::ResourceAvailable::~ResourceAvailable() {}


RestApiInV1::JsonResponse ResourceManager::ResourceAvailable::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 1 && object.contains("customer_id")) {
            QJsonObject responseObject;

            double customerIdDouble = object.value("customer_id").toDouble();
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                ActiveResources::CustomerId customerId = static_cast<ActiveResources::CustomerId>(customerIdDouble);
                ActiveResources     activeResources    = currentResources->hasResourceData(customerId, threadId);
                Resource::ValueType valueType          = activeResources.nextValidValueType();

                QJsonArray valueTypes;
                while (valueType != ActiveResources::invalidValueType) {
                    valueTypes.append(static_cast<int>(valueType));
                    valueType = activeResources.nextValidValueType(valueType + 1);
                }

                responseObject.insert("status", "OK");
                responseObject.insert("value_types", valueTypes);
            } else {
                responseObject.insert("status", "failed, invalid customer ID.");
            }

            response = RestApiInV1::JsonResponse(QJsonDocument(responseObject));
        }
    }

    return response;
}

/***********************************************************************************************************************
* ResourceManager::ResourceCreate
*/

ResourceManager::ResourceCreate::ResourceCreate(
        const QByteArray& secret,
        Resources*        resourceDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentResources(
        resourceDatabaseApi
    ) {}


ResourceManager::ResourceCreate::~ResourceCreate() {}


RestApiInV1::JsonResponse ResourceManager::ResourceCreate::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 4             &&
            object.contains("customer_id") &&
            object.contains("value_type")  &&
            object.contains("value")       &&
            object.contains("timestamp")      ) {
            QJsonObject responseObject;

            double customerIdDouble = object.value("customer_id").toDouble();
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                Resource::CustomerId customerId = static_cast<Resource::CustomerId>(customerIdDouble);

                int valueTypeInt = static_cast<Resource::ValueType>(object.value("value_type").toInt(-1));
                if (valueTypeInt >= 0 && valueTypeInt <= 255) {
                    Resource::ValueType valueType = static_cast<Resource::ValueType>(valueTypeInt);
                    Resource::Value value = object.value("value").toDouble();

                    double timestampDouble = object.value("timestamp").toDouble(-1);
                    if (timestampDouble > 0) {
                        unsigned long long timestamp = static_cast<unsigned long long>(timestampDouble);

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
                            responseObject.insert("status", "failed, could not add");
                        }
                    } else {
                        responseObject.insert("status", "failed, invalid timestamp.");
                    }
                } else {
                    responseObject.insert("status", "failed, invalid value type");
                }
            } else {
                responseObject.insert("status", "failed, invalid customer ID.");
            }

            response = RestApiInV1::JsonResponse(QJsonDocument(responseObject));
        }
    }

    return response;
}

/***********************************************************************************************************************
* ResourceManager::ResourceList
*/

ResourceManager::ResourceList::ResourceList(
        const QByteArray& secret,
        Resources*        resourceDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentResources(
        resourceDatabaseApi
    ) {}


ResourceManager::ResourceList::~ResourceList() {}


RestApiInV1::JsonResponse ResourceManager::ResourceList::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() >= 2 && object.contains("customer_id") && object.contains("value_type")) {
            double customerIdDouble = object.value("customer_id").toDouble();
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                Resource::CustomerId customerId = static_cast<Resource::CustomerId>(customerIdDouble);

                int valueTypeInt = static_cast<Resource::ValueType>(object.value("value_type").toInt(-1));
                if (valueTypeInt >= 0 && valueTypeInt <= 255) {
                    Resource::ValueType valueType = static_cast<Resource::ValueType>(valueTypeInt);

                    unsigned long long startTimestamp = 0;
                    unsigned long long endTimestamp   = 0;
                    unsigned           numberFields   = 2;
                    if (object.contains("start_timestamp")) {
                        double startTimestampDouble = object.value("start_timestamp").toDouble(-1);
                        if (startTimestampDouble >= 1.0) {
                            startTimestamp = static_cast<unsigned long long>(startTimestampDouble);
                            ++numberFields;
                        }
                    }

                    if (object.contains("end_timestamp")) {
                        double endTimestampDouble = object.value("end_timestamp").toDouble(-1);
                        if (endTimestampDouble >= 1.0) {
                            endTimestamp = static_cast<unsigned long long>(endTimestampDouble);
                            ++numberFields;
                        }
                    }

                    QJsonObject responseObject;

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
                        dataObject.insert("customer_id", static_cast<double>(customerId));
                        dataObject.insert("resources", convertToJson(resources, false, false));

                        responseObject.insert("status", "OK");
                        responseObject.insert("data", dataObject);
                    } else{
                        responseObject.insert("status", "failed");
                    }

                    response = RestApiInV1::JsonResponse(QJsonDocument(responseObject));
                }
            }
        }
    }

    return response;
}

/***********************************************************************************************************************
* ResourceManager::ResourcePurge
*/

ResourceManager::ResourcePurge::ResourcePurge(
        const QByteArray& secret,
        Resources*        resourceDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentResources(
        resourceDatabaseApi
    ) {}


ResourceManager::ResourcePurge::~ResourcePurge() {}


RestApiInV1::JsonResponse ResourceManager::ResourcePurge::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject object = request.object();
        if (object.size() == 2 && object.contains("customer_id") && object.contains("timestamp")) {
            double customerIdDouble = object.value("customer_id").toDouble();
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                Resource::CustomerId customerId = static_cast<Resource::CustomerId>(customerIdDouble);

                double timestampDouble = object.value("timestamp").toDouble(-1);
                if (timestampDouble >= 1.0) {
                    unsigned long long timestamp = static_cast<unsigned long long>(timestampDouble);

                    currentResources->purgeResources(customerId, timestamp, threadId);

                    QJsonObject responseObject;
                    responseObject.insert("status", "OK");

                    response = RestApiInV1::JsonResponse(QJsonDocument(responseObject));
                }
            }
        }
    }

    return response;
}

/***********************************************************************************************************************
* ResourceManager::ResourcePlot
*/

ResourceManager::ResourcePlot::ResourcePlot(
        const QByteArray& secret,
        ResourcePlotter*  resourcePlotter
    ):RestApiInV1::InesonicBinaryRestHandler(
        secret
    ),currentResourcePlotter(
        resourcePlotter
    ) {}


ResourceManager::ResourcePlot::~ResourcePlot() {}


RestApiInV1::Response* ResourceManager::ResourcePlot::processAuthenticatedRequest(
        const QString&    /* path */,
        const QByteArray& request,
        unsigned          threadId
    ) {
    RestApiInV1::Response* response = nullptr;

    QJsonDocument document = QJsonDocument::fromJson(request);
    if (document.isObject()) {
        QJsonObject          responseObject;
        bool                 success        = true;
        QJsonObject          object         = document.object();
        Resource::CustomerId customerId     = CustomerCapabilities::invalidCustomerId;
        Resource::ValueType  valueType      = 0;
        unsigned             numberFields   = 0;
        unsigned long long   startTimestamp = 0;
        unsigned long long   endTimestamp   = std::numeric_limits<unsigned long long>::max();
        float                scaleFactor    = 1;
        unsigned             width          = ResourcePlotter::defaultWidth;
        unsigned             height         = ResourcePlotter::defaultHeight;
        QString              plotFormat("PNG");
        QString              title("Resource Over Time");
        QString              xAxisLabel("Date/Time");
        QString              yAxisLabel("Value");
        QString              dateFormat("MMM dd yyyy - hh:mm");
        QString              titleFont;
        QString              axisTitleFont;
        QString              axisLabelFont;

        if (object.contains("customer_id")) {
            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                customerId = static_cast<CustomerCapabilities::CustomerId>(customerIdDouble);
            }

            ++numberFields;
        }

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
            scaleFactor = object.value("scale_factor").toDouble(0);
            if (scaleFactor == 0) {
                success = false;
                responseObject.insert("status", "failed, invalid scale factor");
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

        if (numberFields == static_cast<unsigned>(object.size())) {
            if (success) {
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
                    response = new RestApiInV1::BinaryResponse(
                        QString("image/%1").arg(plotFormat).toLower().toUtf8(),
                        plotData
                    );
                }
            }

            if (!success) {
                response = new RestApiInV1::JsonResponse(responseObject);
            }
        }
    }

    if (response == nullptr) {
        response = new RestApiInV1::BinaryResponse(StatusCode::BAD_REQUEST);
    }

    return response;
}

/***********************************************************************************************************************
* ResourceManager
*/

const QString ResourceManager::resourceAvailablePath("/resource/available");
const QString ResourceManager::resourceCreatePath("/resource/create");
const QString ResourceManager::resourceListPath("/resource/list");
const QString ResourceManager::resourcePurgePath("/resource/purge");
const QString ResourceManager::resourcePlotPath("/resource/plot");

ResourceManager::ResourceManager(
        RestApiInV1::Server* restApiServer,
        Resources*           resourceDatabaseApi,
        ResourcePlotter*     resourcePlotter,
        const QByteArray&    secret,
        QObject*             parent
    ):QObject(
        parent
    ),resourceAvailable(
        secret,
        resourceDatabaseApi
    ),resourceCreate(
        secret,
        resourceDatabaseApi
    ),resourceList(
        secret,
        resourceDatabaseApi
    ),resourcePurge(
        secret,
        resourceDatabaseApi
    ),resourcePlot(
        secret,
        resourcePlotter
    ) {
    restApiServer->registerHandler(&resourceAvailable, RestApiInV1::Handler::Method::POST, resourceAvailablePath);
    restApiServer->registerHandler(&resourceCreate, RestApiInV1::Handler::Method::POST, resourceCreatePath);
    restApiServer->registerHandler(&resourceList, RestApiInV1::Handler::Method::POST, resourceListPath);
    restApiServer->registerHandler(&resourcePurge, RestApiInV1::Handler::Method::POST, resourcePurgePath);
    restApiServer->registerHandler(&resourcePlot, RestApiInV1::Handler::Method::POST, resourcePlotPath);
}


ResourceManager::~ResourceManager() {}


void ResourceManager::setSecret(const QByteArray& newSecret) {
    resourceAvailable.setSecret(newSecret);
    resourceCreate.setSecret(newSecret);
    resourceList.setSecret(newSecret);
    resourcePurge.setSecret(newSecret);
    resourcePlot.setSecret(newSecret);
}
