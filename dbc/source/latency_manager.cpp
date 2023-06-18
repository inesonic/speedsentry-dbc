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
* This header implements the \ref LatencyManager class.
***********************************************************************************************************************/

#include <QObject>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QImage>
#include <QBuffer>

#include <iomanip>

#include <rest_api_in_v1_response.h>
#include <rest_api_in_v1_binary_response.h>
#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "log.h"
#include "customer_capabilities.h"
#include "region.h"
#include "server.h"
#include "monitor.h"
#include "servers.h"
#include "monitors.h"
#include "latency_interface_manager.h"
#include "plot_mailbox.h"
#include "latency_plotter.h"
#include "latency_manager.h"

/***********************************************************************************************************************
* LatencyManager::LatencyRecord
*/

LatencyManager::LatencyRecord::LatencyRecord(
        const QByteArray&        secret,
        LatencyInterfaceManager* latencyInterfaceManager,
        Servers*                 serverDatabaseApi
    ):RestApiInV1::InesonicBinaryRestHandler(
        secret
    ),currentLatencyInterfaceManager(
        latencyInterfaceManager
    ),currentServers(
        serverDatabaseApi
    ) {
    Q_ASSERT(sizeof(Header) == 64);
    Q_ASSERT(sizeof(Entry) == 12);
}


LatencyManager::LatencyRecord::~LatencyRecord() {}


RestApiInV1::Response* LatencyManager::LatencyRecord::processAuthenticatedRequest(
        const QString&    /* path */,
        const QByteArray& request,
        unsigned          threadId
    ) {
    RestApiInV1::Response* response = nullptr;

    /* Our raw data is formatted as follows:
     *
     * +---------------+---------------+----------------+--------------------------------------------------------+
     * | Start         | End           | Length (bytes) | Contains                                               |
     * +===============+===============+================+========================================================+
     * | 0x0000        | 0x0003        | 4              | Raw 4-byte IPv4 address.  Byte 0 is the LSB byte.      |
     * +---------------+---------------+----------------+--------------------------------------------------------+
     * | 0x0004        | 0x0013        | 16             | Raw 16-byte IPv6 address.  Big endian format.          |
     * +---------------+---------------+----------------+--------------------------------------------------------+
     * | 0x0014        | 0x0017        |  4             | The server's service rate in monitors per second.      |
     * |               |               |                | Value is in unsigned 24.8 little endian fixed point    |
     * |               |               |                | format.                                                |
     * +---------------+---------------+----------------+--------------------------------------------------------+
     * | 0x0018        | 0x0019        |  2             | CPU loading.  Value is encoded in unsigned 4.12 little |
     * |               |               |                | endian format.  Value is the average number of tasks   |
     * |               |               |                | in the active queue divided by the number of cores.    |
     * +---------------+---------------+----------------+--------------------------------------------------------+
     * | 0x001A        | 0x001B        |  2             | Fractional memory loading.  Value is in unsigned 0.16  |
     * |               |               |                | little endian format.                                  |
     * +---------------+---------------+----------------+--------------------------------------------------------+
     * | 0x001C        | 0x001C        |  1             | Server status: 0 = UNKNOWN, 1 = ACTIVE, 2 = INACTIVE,  |
     * |               |               |                | 3 = DEFUNCT.  See the definition of Server::Status for |
     * |               |               |                | mapping.                                               |
     * +---------------+---------------+----------------+--------------------------------------------------------+
     * | 0x001D        | 0x003F        |  35            | reserved/spare                                         |
     * +---------------+---------------+----------------+--------------------------------------------------------+
     * | 0x0040 + 12*N | 0x0043 + 12*N |  4             | Monitor ID for record N.                               |
     * +---------------+---------------+----------------+--------------------------------------------------------+
     * | 0x0044 + 12*N | 0x0047 + 12*N |  4             | Zoran timestamp for record N.                          |
     * +---------------+---------------+----------------+--------------------------------------------------------+
     * | 0x0048 + 12*N | 0x004B + 12*N |  4             | Latency in microseconds for record N.                  |
     * +---------------+---------------+----------------+--------------------------------------------------------+
     *
     * For details, see the structures Header and Entry.
     */

    unsigned long payloadSize     = static_cast<unsigned long>(request.size());
    unsigned long monitorDataSize = payloadSize - sizeof(Header);
    if (payloadSize >= sizeof(Header) && (monitorDataSize % sizeof(Entry) == 0)) {
        QJsonObject responseObject;

        const std::uint8_t* requestData = reinterpret_cast<const std::uint8_t*>(request.data());
        const Header*       header      = reinterpret_cast<const Header*>(requestData);

        QString identifier = QString::fromUtf8(reinterpret_cast<const char*>(header->identifier));
        Server  server     = currentServers->getServer(identifier, threadId);

        if (server.isValid()) {
            bool success = true;

            std::uint8_t   serverStatusValue    = header->serverStatusCode;
            Server::Status newServerStatus      = static_cast<Server::Status>(serverStatusValue);
            float          newCpuLoading        = static_cast<double>(header->cpuLoading) / 4096.0;
            float          newMemoryLoading     = static_cast<double>(header->memoryLoading) / 65536.0;
            float          newMonitorsPerSecond = static_cast<double>(header->monitorsPerSecond) / 256.0;

            if (serverStatusValue < static_cast<std::uint8_t>(Server::Status::NUMBER_VALUES)) {
                if (server.status() != newServerStatus                 ||
                    server.cpuLoading() != newCpuLoading               ||
                    server.memoryLoading() != newMemoryLoading         ||
                    server.monitorsPerSecond() != newMonitorsPerSecond    ) {
                    server.setStatus(newServerStatus);
                    server.setCpuLoading(newCpuLoading);
                    server.setMemoryLoading(newMemoryLoading);
                    server.setMonitorsPerSecond(newMonitorsPerSecond);

                    success = currentServers->modifyServer(server, threadId);
                    if (!success) {
                        responseObject.insert("status", "failed, could not update server status");
                    }
                }
            } else {
                responseObject.insert("status", "failed, invalid server status code");
            }

            if (success) {
                Server::ServerId  serverId         = server.serverId();
                Region::RegionId  regionId         = server.regionId();
                LatencyInterface* latencyInterface = currentLatencyInterfaceManager->getLatencyInterface(regionId);
                unsigned long     numberMonitors   = monitorDataSize / sizeof(Entry);
                const Entry*      entry            = reinterpret_cast<const Entry*>(requestData + sizeof(Header));

                for (unsigned long i=0 ; i<numberMonitors ; ++i) {
                    latencyInterface->addEntry(
                        entry->monitorId,
                        serverId,
                        entry->timestamp,
                        entry->latencyMicroseconds
                    );

                    ++entry;
                }

                latencyInterface->receivedEntries();

                QString message = QString(
                    "Received records from %1, status = %2, cpu = %3%, memory = %4%, m/s= %5, records = %6"
                ).arg(identifier, Server::toString(newServerStatus))
                 .arg(100.0 * newCpuLoading, 0, 'f', 2)
                 .arg(100.0 * newMemoryLoading, 0, 'f', 2)
                 .arg(newMonitorsPerSecond)
                 .arg(numberMonitors);

                logWrite(message, false);
            }

            if (success) {
                responseObject.insert("status", "OK");
            }
        } else {
            responseObject.insert("status", "failed, unknown server");
        }

        response = new RestApiInV1::JsonResponse(responseObject);
    } else {
        response = new RestApiInV1::JsonResponse(StatusCode::BAD_REQUEST);
    }

    return response;
}

/***********************************************************************************************************************
* LatencyManager::LatencyGet
*/

LatencyManager::LatencyGet::LatencyGet(
        const QByteArray&        secret,
        LatencyInterfaceManager* latencyInterfaceManager,
        Servers*                 serverDatabaseApi,
        Monitors*                monitorDatabaseApi
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentLatencyInterfaceManager(
        latencyInterfaceManager
    ),currentServers(
        serverDatabaseApi
    ),currentMonitors(
        monitorDatabaseApi
    ) {}


LatencyManager::LatencyGet::~LatencyGet() {}


RestApiInV1::JsonResponse LatencyManager::LatencyGet::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response;

    if (request.isObject()) {
        QJsonObject                      responseObject;
        bool                             success        = true;
        QJsonObject                      object         = request.object();
        unsigned                         numberFields   = 0;
        CustomerCapabilities::CustomerId customerId     = CustomerCapabilities::invalidCustomerId;
        Monitor::MonitorId               monitorId      = Monitor::invalidMonitorId;
        Server::ServerId                 serverId       = Server::invalidServerId;
        Region::RegionId                 regionId       = Region::invalidRegionId;
        unsigned long long               startTimestamp = 0;
        unsigned long long               endTimestamp   = std::numeric_limits<unsigned long long>::max();

        if (object.contains("customer_id")) {
            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                customerId = static_cast<CustomerCapabilities::CustomerId>(customerIdDouble);
            } else {
                success = false;
                responseObject.insert("status", "failed, invalid customer ID");
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

        if (object.contains("server_id")) {
            if (success) {
                double serverIdDouble = object.value("server_id").toDouble(-1);
                if (serverIdDouble > 0 && serverIdDouble <= 0xFFFF) {
                    serverId = static_cast<Server::ServerId>(serverIdDouble);
                } else {
                    success = false;
                    responseObject.insert("status", "failed, invalid server ID");
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
                customerId,
                HostScheme::invalidHostSchemeId,
                monitorId,
                regionId,
                serverId,
                startTimestamp,
                endTimestamp,
                threadId
            );

            LatencyInterfaceManager::LatencyEntryList           rawEntries        = result.first;
            LatencyInterfaceManager::AggregatedLatencyEntryList aggregatedEntries = result.second;

            Servers::ServersById serversById    = currentServers->getServersById(threadId);
            Monitors::MonitorsById monitorsById = currentMonitors->getMonitorsById(threadId);

            responseObject.insert("status", "OK");
            responseObject.insert("recent", convertToJson(rawEntries, serversById, monitorsById, true, true, true));
            responseObject.insert(
                "aggregated",
                convertToJson(
                    aggregatedEntries,
                    serversById,
                    monitorsById,
                    true,
                    true,
                    true
                )
            );

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* LatencyManager::LatencyPurge
*/

LatencyManager::LatencyPurge::LatencyPurge(
        const QByteArray&        secret,
        LatencyInterfaceManager* latencyInterfaceManager
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentLatencyInterfaceManager(
        latencyInterfaceManager
    ) {}


LatencyManager::LatencyPurge::~LatencyPurge() {}


RestApiInV1::JsonResponse LatencyManager::LatencyPurge::processAuthenticatedRequest(
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
                customerIds.insert(static_cast<CustomerCapabilities::CustomerId>(customerIdDouble));
                ++index;
            } else {
                success = false;
            }
        }

        if (success) {
            if (static_cast<unsigned long>(customerIds.size()) == numberCustomerIds) {
                success = currentLatencyInterfaceManager->deleteByCustomerId(customerIds, threadId);
                if (success) {
                    responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "failed");
                }
            } else {
                responseObject.insert("status", "failed, duplicate customer ID");
            }
        } else {
            responseObject.insert("status", "failed, invalid customer ID");
        }

        response = RestApiInV1::JsonResponse(responseObject);
    }

    return response;
}

/***********************************************************************************************************************
* LatencyManager::LatencyPlot
*/

LatencyManager::LatencyPlot::LatencyPlot(
        const QByteArray& secret,
        LatencyPlotter*   latencyPlotter
    ):RestApiInV1::InesonicBinaryRestHandler(
        secret
    ),currentLatencyPlotter(
        latencyPlotter
    ) {}


LatencyManager::LatencyPlot::~LatencyPlot() {}


RestApiInV1::Response* LatencyManager::LatencyPlot::processAuthenticatedRequest(
        const QString&    /* path */,
        const QByteArray& request,
        unsigned          threadId
    ) {
    RestApiInV1::Response* response = nullptr;

    QJsonDocument document = QJsonDocument::fromJson(request);
    if (document.isObject()) {
        QJsonObject                      responseObject;
        bool                             success        = true;
        QJsonObject                      object         = document.object();
        CustomerCapabilities::CustomerId customerId     = CustomerCapabilities::invalidCustomerId;
        unsigned                         numberFields   = 0;
        HostScheme::HostSchemeId         hostSchemeId   = HostScheme::invalidHostSchemeId;
        Monitor::MonitorId               monitorId      = Monitor::invalidMonitorId;
        Server::ServerId                 serverId       = Server::invalidServerId;
        Region::RegionId                 regionId       = Region::invalidRegionId;
        unsigned long long               startTimestamp = 0;
        unsigned long long               endTimestamp   = std::numeric_limits<unsigned long long>::max();
        double                           minimumLatency = -1;
        double                           maximumLatency = -1;
        bool                             logScale       = false;
        unsigned                         width          = LatencyPlotter::defaultWidth;
        unsigned                         height         = LatencyPlotter::defaultHeight;
        QString                          plotType("history");
        QString                          plotFormat("PNG");
        QString                          title("Latency Over Time");
        QString                          xAxisLabel("Date/Time");
        QString                          yAxisLabel("Latency (seconds)");
        QString                          dateFormat("MMM dd yyyy - hh:mm");
        QString                          titleFont;
        QString                          axisTitleFont;
        QString                          axisLabelFont;

        if (object.contains("customer_id")) {
            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                customerId = static_cast<CustomerCapabilities::CustomerId>(customerIdDouble);
            }

            ++numberFields;
        }

        if (object.contains("server_id")) {
            int serverIdDouble = object.value("server_id").toInt(-1);
            if (serverIdDouble > 0 && serverIdDouble <= 0xFFFF) {
                serverId = static_cast<Server::ServerId>(serverIdDouble);
            }

            ++numberFields;
        }

        if (object.contains("plot_type")) {
            plotType = object.value("plot_type").toString().toLower();
            if (plotType == "histogram") {
                title = QString("Latency Histogram");
                xAxisLabel = QString("Latency (seconds)");
                yAxisLabel = QString("Counts");
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
                        serverId,
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
                        serverId,
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
                        response = new RestApiInV1::BinaryResponse(
                            QString("image/%1").arg(plotFormat).toLower().toUtf8(),
                            plotData
                        );
                    }
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
* LatencyManager::LatencyStatistics
*/

LatencyManager::LatencyStatistics::LatencyStatistics(
        const QByteArray&        secret,
        LatencyInterfaceManager* latencyInterfaceManager
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentLatencyInterfaceManager(
        latencyInterfaceManager
    ) {}


LatencyManager::LatencyStatistics::~LatencyStatistics() {}


RestApiInV1::JsonResponse LatencyManager::LatencyStatistics::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             threadId
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject                      responseObject;
        bool                             success        = true;
        QJsonObject                      object         = request.object();
        unsigned                         numberFields   = 0;
        CustomerCapabilities::CustomerId customerId     = CustomerCapabilities::invalidCustomerId;
        Monitor::MonitorId               monitorId      = Monitor::invalidMonitorId;
        Server::ServerId                 serverId       = Server::invalidServerId;
        Region::RegionId                 regionId       = Region::invalidRegionId;
        unsigned long long               startTimestamp = 0;
        unsigned long long               endTimestamp   = std::numeric_limits<unsigned long long>::max();

        if (object.contains("customer_id")) {
            double customerIdDouble = object.value("customer_id").toDouble(-1);
            if (customerIdDouble > 0 && customerIdDouble <= 0xFFFFFFFF) {
                customerId = static_cast<CustomerCapabilities::CustomerId>(customerIdDouble);
            } else {
                success = false;
                responseObject.insert("status", "failed, invalid customer ID");
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

        if (object.contains("server_id")) {
            if (success) {
                double serverIdDouble = object.value("server_id").toDouble(-1);
                if (serverIdDouble > 0 && serverIdDouble <= 0xFFFF) {
                    serverId = static_cast<Server::ServerId>(serverIdDouble);
                } else {
                    success = false;
                    responseObject.insert("status", "failed, invalid server ID");
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

        if (numberFields == static_cast<unsigned>(object.size()) && success) {
            AggregatedLatencyEntry result = currentLatencyInterfaceManager->getLatencyStatistics(
                customerId,
                HostScheme::invalidHostSchemeId,
                monitorId,
                regionId,
                serverId,
                startTimestamp,
                endTimestamp,
                threadId
            );

            if (result.numberSamples() > 0) {
                responseObject.insert("status", "OK");

                QJsonObject statisticsObject;
                statisticsObject.insert("mean", result.meanLatency() * 1.0E-6);
                statisticsObject.insert("variance", result.varianceLatency() * 1.0E-12);
                statisticsObject.insert("minimum", result.minimumLatency() * 1.0E-6);
                statisticsObject.insert("maximum", result.maximumLatency() * 1.0E-6);
                statisticsObject.insert("number_samples", static_cast<double>(result.numberSamples()));

                responseObject.insert("statistics", statisticsObject);
            } else {
                responseObject.insert("status", "failed");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* LatencyManager
*/

const QString LatencyManager::latencyRecordPath("/latency/record");
const QString LatencyManager::latencyGetPath("/latency/get");
const QString LatencyManager::latencyPurgePath("/latency/purge");
const QString LatencyManager::latencyPlotPath("/latency/plot");
const QString LatencyManager::latencyStatisticsPath("/latency/statistics");

LatencyManager::LatencyManager(
        RestApiInV1::Server*     restApiServer,
        LatencyInterfaceManager* latencyInterfaceManager,
        Servers*                 serverDatabaseApi,
        Monitors*                monitorDatabaseApi,
        LatencyPlotter*          latencyPlotter,
        const QByteArray&        secret,
        QObject*                 parent
    ):QObject(
        parent
    ),latencyRecord(
        secret,
        latencyInterfaceManager,
        serverDatabaseApi
    ),latencyGet(
        secret,
        latencyInterfaceManager,
        serverDatabaseApi,
        monitorDatabaseApi
    ),latencyPurge(
        secret,
        latencyInterfaceManager
    ),latencyPlot(
        secret,
        latencyPlotter
    ),latencyStatistics(
        secret,
        latencyInterfaceManager
    ) {
    restApiServer->registerHandler(&latencyRecord, RestApiInV1::Handler::Method::POST, latencyRecordPath);
    restApiServer->registerHandler(&latencyGet, RestApiInV1::Handler::Method::POST, latencyGetPath);
    restApiServer->registerHandler(&latencyPurge, RestApiInV1::Handler::Method::POST, latencyPurgePath);
    restApiServer->registerHandler(&latencyPlot, RestApiInV1::Handler::Method::POST, latencyPlotPath);
    restApiServer->registerHandler(&latencyStatistics, RestApiInV1::Handler::Method::POST, latencyStatisticsPath);
}


LatencyManager::~LatencyManager() {}


void LatencyManager::setSecret(const QByteArray& newSecret) {
    latencyRecord.setSecret(newSecret);
    latencyGet.setSecret(newSecret);
    latencyPurge.setSecret(newSecret);
    latencyPlot.setSecret(newSecret);
    latencyStatistics.setSecret(newSecret);
}
