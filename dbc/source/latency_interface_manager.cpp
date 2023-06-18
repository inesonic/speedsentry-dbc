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
* This header implements the \ref LatencyInterfaceManager class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <QSqlError>
#include <QSqlRecord>

#include <cstdint>

#include "log.h"
#include "database_manager.h"
#include "customer_capabilities.h"
#include "customers_capabilities.h"
#include "monitor.h"
#include "server.h"
#include "region.h"
#include "latency_interface.h"
#include "latency_aggregator.h"
#include "latency_interface_manager.h"

LatencyInterfaceManager::LatencyInterfaceManager(DatabaseManager* databaseManager, QObject* parent):QObject(parent) {
    currentDatabaseManager   = databaseManager;
    currentLatencyAggregator = new LatencyAggregator(databaseManager, this);
}


LatencyInterfaceManager::~LatencyInterfaceManager() {
    for (  QHash<RegionId, LatencyInterface*>::const_iterator it  = dataInterfacesByRegion.constBegin(),
                                                              end = dataInterfacesByRegion.constEnd()
         ; it != end
         ; ++it
        ) {
        LatencyInterface* dataInterface = it.value();
        delete dataInterface;
    }
}


LatencyInterface* LatencyInterfaceManager::getLatencyInterface(RegionId regionId) {
    QMutexLocker accessMutexLocker(&accessMutex);

    LatencyInterface* dataInterfaceForRegion = dataInterfacesByRegion.value(regionId, nullptr);
    if (dataInterfaceForRegion == nullptr) {
        dataInterfaceForRegion = new LatencyInterface(currentDatabaseManager, regionId);
        dataInterfaceForRegion->moveToThread(thread());

        dataInterfacesByRegion.insert(regionId, dataInterfaceForRegion);
    }

    return dataInterfaceForRegion;
}


LatencyInterfaceManager::LatencyEntryLists LatencyInterfaceManager::getLatencyEntries(
        CustomerCapabilities::CustomerId customerId,
        HostScheme::HostSchemeId         hostSchemeId,
        LatencyEntry::MonitorId          monitorId,
        Region::RegionId                 regionId,
        Server::ServerId                 serverId,
        unsigned long long               startTimestamp,
        unsigned long long               endTimestamp,
        unsigned                         threadId
    ) {
    LatencyEntryList           rawEntries;
    AggregatedLatencyEntryList aggregatedEntries;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        rawEntries = getRawEntries(
            success,
            database,
            customerId,
            hostSchemeId,
            monitorId,
            regionId,
            serverId,
            startTimestamp,
            endTimestamp
        );

        if (success) {
            aggregatedEntries = getAggregatedEntries(
                success,
                database,
                customerId,
                hostSchemeId,
                monitorId,
                regionId,
                serverId,
                startTimestamp,
                endTimestamp
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - LatencyInterfaceManager::getLatencyEntries: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);

    if (!success) {
        rawEntries.clear();
        aggregatedEntries.clear();
    }

    return LatencyEntryLists(rawEntries, aggregatedEntries);
}


AggregatedLatencyEntry LatencyInterfaceManager::getLatencyStatistics(
        CustomerCapabilities::CustomerId customerId,
        HostScheme::HostSchemeId         hostSchemeId,
        LatencyEntry::MonitorId          monitorId,
        Region::RegionId                 regionId,
        Server::ServerId                 serverId,
        unsigned long long               startTimestamp,
        unsigned long long               endTimestamp,
        unsigned                         threadId
    ) {
    AggregatedLatencyEntry result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        AggregatedLatencyEntry rawEntryStatistics = getRawEntryStatistics(
            success,
            database,
            customerId,
            hostSchemeId,
            monitorId,
            regionId,
            serverId,
            startTimestamp,
            endTimestamp
        );

        if (success) {
            AggregatedLatencyEntryList aggregatedEntries = getAggregatedEntries(
                success,
                database,
                customerId,
                hostSchemeId,
                monitorId,
                regionId,
                serverId,
                startTimestamp,
                endTimestamp
            );

            unsigned numberAggregations = static_cast<unsigned>(aggregatedEntries.size());
            if (numberAggregations == 0) {
                result = rawEntryStatistics;
            } else {
                aggregatedEntries.append(rawEntryStatistics);

                // Now find the combined mean and variance
                //
                // To do this, we use the expression:
                //
                //   v_c = \frac{ \sum_i n _ i \left [ v_i + \left ( \mu_i - \mu_c \right ) ^ 2 \right ] }
                //              { \sum_i n _ i }
                //
                // Where:
                //   v_c -   Is the population variance of the combined population
                //   n_i -   Is the size of sub-population i
                //   v_i -   Is the variance of sub-population i
                //   \mu_i - Is the average of sub-population i
                //   \mu_c - Is the average of the combined population.
                //
                // We calculate the combined mean by:
                //
                //    \mu _ c = \frac{ \sum_i n _ i \mu _ i }
                //                   { \sum_i n _ i }
                //
                // While we're at it, we also get our minimum and maximum values.

                double                            sum = 0;
                unsigned long                     nc  = 0;
                LatencyEntry::LatencyMicroseconds min = std::numeric_limits<LatencyEntry::LatencyMicroseconds>::max();
                LatencyEntry::LatencyMicroseconds max = 0;
                for (unsigned i=0 ; i<numberAggregations ; ++i) {
                    const AggregatedLatencyEntry& entry = aggregatedEntries.at(i);
                    sum += entry.meanLatency() * entry.numberSamples();
                    nc  += entry.numberSamples();

                    if (entry.maximumLatency() > max) {
                        max = entry.maximumLatency();
                    }

                    if (entry.minimumLatency() < min) {
                        min = entry.minimumLatency();
                    }
                }

                double muc = sum / nc;

                double numerator = 0;
                for (unsigned i=0 ; i<numberAggregations ; ++i) {
                    const AggregatedLatencyEntry& entry = aggregatedEntries.at(i);
                    unsigned ni    = entry.numberSamples();
                    double   mui   = entry.meanLatency();
                    double   vi    = entry.varianceLatency();
                    double   dmean = mui - muc;

                    numerator += ni * (vi + dmean * dmean);
                }

                double vc = numerator / nc;

                result = AggregatedLatencyEntry(
                    monitorId,
                    serverId,
                    0,
                    0,
                    LatencyEntry::toZoranTimestamp(startTimestamp),
                    LatencyEntry::toZoranTimestamp(endTimestamp),
                    muc,
                    vc,
                    min,
                    max,
                    nc
                );
            }
        }
    } else {
        logWrite(
            QString("Failed to open database - LatencyInterfaceManager::getLatencyStatistics: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


void LatencyInterfaceManager::addEntry(
        RegionId                          regionId,
        LatencyEntry::MonitorId           monitorId,
        LatencyEntry::ServerId            serverId,
        LatencyEntry::ZoranTimeStamp      zoranTimestamp,
        LatencyEntry::LatencyMicroseconds latencyMicroseconds
    ) {
    LatencyInterface* latencyInterface = getLatencyInterface(regionId);
    latencyInterface->addEntry(monitorId, serverId, zoranTimestamp, latencyMicroseconds);
    latencyInterface->receivedEntries();
}


void LatencyInterfaceManager::addEntry(RegionId regionId, const LatencyEntry& latencyEntry) {
    LatencyInterface* latencyInterface = getLatencyInterface(regionId);
    latencyInterface->addEntry(latencyEntry);
    latencyInterface->receivedEntries();
}


void LatencyInterfaceManager::addEntries(RegionId regionId, const LatencyInterface::LatencyEntryList& latencyEntries) {
    LatencyInterface* latencyInterface = getLatencyInterface(regionId);
    latencyInterface->addEntries(latencyEntries);
    latencyInterface->receivedEntries();
}


bool LatencyInterfaceManager::deleteByCustomerId(
        const CustomersCapabilities::CustomerIdSet& customerIds,
        unsigned                                    threadId
    ) {
    return currentLatencyAggregator->deleteByCustomerId(customerIds, threadId);
}


void LatencyInterfaceManager::setParameters(
        unsigned long  inputTableMaximumAge,
        unsigned long  resamplePeriod,
        unsigned long  expungePeriod,
        bool           inputAggregated
    ) {
    currentLatencyAggregator->setParameters(
        QString("latency_seconds"),
        QString("latency_aggregated"),
        inputTableMaximumAge,
        resamplePeriod,
        expungePeriod,
        inputAggregated
    );
}


LatencyInterfaceManager::LatencyEntryList LatencyInterfaceManager::getRawEntries(
        bool&                            success,
        QSqlDatabase&                    database,
        CustomerCapabilities::CustomerId customerId,
        HostScheme::HostSchemeId         hostSchemeId,
        LatencyEntry::MonitorId          monitorId,
        Region::RegionId                 regionId,
        Server::ServerId                 serverId,
        unsigned long long               startTimestamp,
        unsigned long long               endTimestamp
    ) {
    LatencyEntryList result;

    typedef LatencyEntry::MonitorId           MonitorId;
    typedef LatencyEntry::ServerId            ServerId;
    typedef LatencyEntry::ZoranTimeStamp      ZoranTimeStamp;
    typedef LatencyEntry::LatencyMicroseconds Latency;

    QSqlQuery query(database);
    query.setForwardOnly(true);

    QString queryString = buildQueryString(
        "latency_seconds",
        customerId,
        hostSchemeId,
        monitorId,
        regionId,
        serverId,
        startTimestamp,
        endTimestamp
    );
    queryString += QString(" ORDER BY timestamp ASC, monitor_id ASC, server_id ASC");

    success = query.exec(queryString);
    if (success) {
        int monitorIdField = query.record().indexOf("monitor_id");
        int serverIdField  = query.record().indexOf("server_id");
        int timestampField = query.record().indexOf("timestamp");
        int latencyField   = query.record().indexOf("latency");

        if (monitorIdField >= 0 &&
            serverIdField >= 0  &&
            timestampField >= 0 &&
            latencyField >= 0      ) {
            while (success && query.next()) {
                MonitorId monitorId = query.value(monitorIdField).toUInt(&success);
                if (success) {
                    ServerId serverId = query.value(serverIdField).toUInt(&success);
                    if (success) {
                        ZoranTimeStamp timestamp = query.value(timestampField).toUInt(&success);
                        if (success) {
                            Latency latency = query.value(latencyField).toUInt(&success);
                            if (success) {
                                LatencyEntry entry = LatencyEntry(monitorId, serverId, timestamp, latency);
                                result.append(entry);
                            } else {
                                logWrite(
                                    QString("Invalid latency - LatencyInterfaceManager::getLatencyEntries."),
                                    true
                                );
                            }
                        } else {
                            logWrite(QString("Invalid timestamp - LatencyInterfaceManager::getLatencyEntries."), true);
                        }
                    } else {
                        logWrite(QString("Invalid server ID - LatencyInterfaceManager::getLatencyEntries."), true);
                    }
                } else {
                    logWrite(QString("Invalid monitor ID - LatencyInterfaceManager::getLatencyEntries."), true);
                }
            }
        } else {
            logWrite(QString("Failed to get field indexes - LatencyInterfaceManager::getLatencyEntries."), true);
            success = false;
        }
    } else {
        logWrite(
            QString("Failed SELECT - LatencyInterfaceManager::getLatencyEntries: ").arg(database.lastError().text()),
            true
        );
    }

    return result;
}


LatencyInterfaceManager::AggregatedLatencyEntryList LatencyInterfaceManager::getAggregatedEntries(
        bool&                            success,
        QSqlDatabase&                    database,
        CustomerCapabilities::CustomerId customerId,
        HostScheme::HostSchemeId         hostSchemeId,
        LatencyEntry::MonitorId          monitorId,
        Region::RegionId                 regionId,
        Server::ServerId                 serverId,
        unsigned long long               startTimestamp,
        unsigned long long               endTimestamp
    ) {
    AggregatedLatencyEntryList result;

    typedef LatencyEntry::MonitorId           MonitorId;
    typedef LatencyEntry::ServerId            ServerId;
    typedef LatencyEntry::ZoranTimeStamp      ZoranTimeStamp;
    typedef LatencyEntry::LatencyMicroseconds Latency;

    QSqlQuery query(database);
    query.setForwardOnly(true);

    QString queryString = buildQueryString(
        "latency_aggregated",
        customerId,
        hostSchemeId,
        monitorId,
        regionId,
        serverId,
        startTimestamp,
        endTimestamp
    );
    queryString += QString(" ORDER BY start_timestamp ASC, monitor_id ASC, server_id ASC");

    success = query.exec(queryString);
    if (success) {
        int monitorIdField       = query.record().indexOf("monitor_id");
        int serverIdField        = query.record().indexOf("server_id");
        int timestampField       = query.record().indexOf("timestamp");
        int latencyField         = query.record().indexOf("latency");
        int startTimestampField  = query.record().indexOf("start_timestamp");
        int endTimestampField    = query.record().indexOf("end_timestamp");
        int meanLatencyField     = query.record().indexOf("mean_latency");
        int varianceLatencyField = query.record().indexOf("variance_latency");
        int minimumLatencyField  = query.record().indexOf("minimum_latency");
        int maximumLatencyField  = query.record().indexOf("maximum_latency");
        int numberSamplesField   = query.record().indexOf("number_samples");

        if (monitorIdField >= 0       &&
            serverIdField >= 0        &&
            timestampField >= 0       &&
            latencyField >= 0         &&
            startTimestampField >= 0  &&
            endTimestampField >= 0    &&
            meanLatencyField >= 0     &&
            varianceLatencyField >= 0 &&
            minimumLatencyField >= 0  &&
            maximumLatencyField >= 0  &&
            numberSamplesField >= 0      ) {
            while (success && query.next()) {
                MonitorId monitorId = query.value(monitorIdField).toUInt(&success);
                if (success) {
                    ServerId serverId = query.value(serverIdField).toUInt(&success);
                    if (success) {
                        ZoranTimeStamp timestamp = query.value(timestampField).toUInt(&success);
                        if (success) {
                            Latency latency = query.value(latencyField).toUInt(&success);
                            if (success) {
                                ZoranTimeStamp startTime = query.value(startTimestampField).toUInt(&success);
                                if (success) {
                                    ZoranTimeStamp endTime = query.value(endTimestampField).toUInt(&success);
                                    if (success) {
                                        double meanLatency = query.value(meanLatencyField).toDouble(&success);
                                        if (success) {
                                            double varianceLatency = query.value(varianceLatencyField)
                                                                          .toDouble(&success);
                                            if (success) {
                                                Latency minimumLatency = query.value(minimumLatencyField)
                                                                              .toUInt(&success);
                                                if (success) {
                                                    Latency maximumLatency = query.value(maximumLatencyField)
                                                                                  .toUInt(&success);
                                                    if (success) {
                                                        unsigned long numberSamples = query.value(numberSamplesField)
                                                                                           .toUInt(&success);
                                                        if (success) {
                                                            AggregatedLatencyEntry entry = AggregatedLatencyEntry(
                                                                monitorId,
                                                                serverId,
                                                                timestamp,
                                                                latency,
                                                                startTime,
                                                                endTime,
                                                                meanLatency,
                                                                varianceLatency,
                                                                minimumLatency,
                                                                maximumLatency,
                                                                numberSamples
                                                            );

                                                            result.append(entry);
                                                        } else {
                                                            logWrite(
                                                                QString(
                                                                    "Invalid number samples - "
                                                                    "LatencyInterfaceManager::getLatencyEntries."
                                                                ),
                                                                true
                                                            );
                                                        }
                                                    } else {
                                                        logWrite(
                                                            QString(
                                                                "Invalid maximum latency - "
                                                                "LatencyInterfaceManager::getLatencyEntries."
                                                            ),
                                                            true
                                                        );
                                                    }
                                                } else {
                                                    logWrite(
                                                        QString(
                                                            "Invalid minimum latency - "
                                                            "LatencyInterfaceManager::getLatencyEntries."
                                                        ),
                                                        true
                                                    );
                                                }
                                            } else {
                                                logWrite(
                                                    QString(
                                                        "Invalid variance latency - "
                                                        "LatencyInterfaceManager::getLatencyEntries."
                                                    ),
                                                    true
                                                );
                                            }
                                        } else {
                                            logWrite(
                                                QString(
                                                    "Invalid mean latency - "
                                                    "LatencyInterfaceManager::getLatencyEntries."
                                                ),
                                                true
                                            );
                                        }
                                    } else {
                                        logWrite(
                                            QString("Invalid end time - LatencyInterfaceManager::getLatencyEntries."),
                                            true
                                        );
                                    }
                                } else {
                                    logWrite(
                                        QString("Invalid start time - LatencyInterfaceManager::getLatencyEntries."),
                                        true
                                    );
                                }
                            } else {
                                logWrite(
                                    QString("Invalid latency - LatencyInterfaceManager::getLatencyEntries."),
                                    true
                                );
                            }
                        } else {
                            logWrite(
                                QString("Invalid timestamp - LatencyInterfaceManager::getLatencyEntries."),
                                true
                            );
                        }
                    } else {
                        logWrite(
                            QString("Invalid server ID - LatencyInterfaceManager::getLatencyEntries."),
                            true
                        );
                    }
                } else {
                    logWrite(
                        QString("Invalid monitor ID - LatencyInterfaceManager::getLatencyEntries."),
                        true
                    );
                }
            }

        } else {
            logWrite(QString("Failed to get field indexes - LatencyInterfaceManager::getLatencyEntries."), true);
            success = false;
        }
    } else {
        logWrite(
            QString("Failed SELECT - LatencyInterfaceManager::getLatencyEntries: %1").arg(query.lastError().text()),
            true
        );
    }

    return result;
}


AggregatedLatencyEntry LatencyInterfaceManager::getRawEntryStatistics(
        bool&                            success,
        QSqlDatabase&                    database,
        CustomerCapabilities::CustomerId customerId,
        HostScheme::HostSchemeId         hostSchemeId,
        LatencyEntry::MonitorId          monitorId,
        Region::RegionId                 regionId,
        Server::ServerId                 serverId,
        unsigned long long               startTimestamp,
        unsigned long long               endTimestamp
    ) {
    AggregatedLatencyEntry result;

    QSqlQuery query(database);
    query.setForwardOnly(true);

    QString queryString = buildQueryString(
        "latency_seconds",
        customerId,
        hostSchemeId,
        monitorId,
        regionId,
        serverId,
        startTimestamp,
        endTimestamp,
        QString(
            "AVG(latency) as average, "
            "VAR_POP(latency) AS variance, "
            "MIN(latency) AS minimum, "
            "MAX(latency) AS maximum, "
            "COUNT(latency) AS sample_size")
    );

    success = query.exec(queryString);
    if (success) {
        int averageField    = query.record().indexOf("average");
        int varianceField   = query.record().indexOf("variance");
        int minimumField    = query.record().indexOf("minimum");
        int maximumField    = query.record().indexOf("maximum");
        int sampleSizeField = query.record().indexOf("sample_size");

        if (averageField >= 0    &&
            varianceField >= 0   &&
            minimumField >= 0    &&
            maximumField >= 0    &&
            sampleSizeField >= 0    ) {
            if (query.first()) {
                double average = query.value(averageField).toDouble(&success);
                if (success) {
                    double variance = query.value(varianceField).toDouble(&success);
                    if (success) {
                        LatencyEntry::LatencyMicroseconds minimum = query.value(minimumField).toUInt(&success);
                        if (success) {
                            LatencyEntry::LatencyMicroseconds maximum = query.value(maximumField).toUInt(&success);
                            if (success) {
                                unsigned long sampleSize = query.value(sampleSizeField).toUInt(&success);
                                if (success) {
                                    result = AggregatedLatencyEntry(
                                        monitorId,
                                        serverId,
                                        0,
                                        0,
                                        LatencyEntry::toZoranTimestamp(startTimestamp),
                                        LatencyEntry::toZoranTimestamp(endTimestamp),
                                        average,
                                        variance,
                                        minimum,
                                        maximum,
                                        sampleSize
                                    );
                                } else {
                                    logWrite(
                                        QString(
                                            "Invalid sample size - LatencyInterfaceManager::getRawEntryStatistics."
                                        ),
                                        true
                                    );
                                }
                            } else {
                                logWrite(
                                    QString("Invalid minimum - LatencyInterfaceManager::getRawEntryStatistics."),
                                    true
                                );
                            }
                        } else {
                            logWrite(
                                QString("Invalid maximum - LatencyInterfaceManager::getRawEntryStatistics."),
                                true
                            );
                        }
                    } else {
                        logWrite(
                            QString("Invalid variance - LatencyInterfaceManager::getRawEntryStatistics."),
                            true
                        );
                    }
                } else {
                    logWrite(
                        QString("Invalid average - LatencyInterfaceManager::getRawEntryStatistics."),
                        true
                    );
                }
            } else {
                // No issue - Just no data
            }
        } else {
            logWrite(QString("Failed to get field indexes - LatencyInterfaceManager::getLatencyEntries."), true);
            success = false;
        }
    } else {
        logWrite(
            QString("Failed SELECT - LatencyInterfaceManager::getLatencyEntries: ").arg(database.lastError().text()),
            true
        );
    }

    return result;
}


QString LatencyInterfaceManager::buildQueryString(
        const QString&                   tableName,
        CustomerCapabilities::CustomerId customerId,
        HostScheme::HostSchemeId         hostSchemeId,
        LatencyEntry::MonitorId          monitorId,
        Region::RegionId                 regionId,
        Server::ServerId                 serverId,
        unsigned long long               startTimestamp,
        unsigned long long               endTimestamp,
        const QString&                   selectClause
    ) {
    QString result        = QString("SELECT %1 FROM %2").arg(selectClause).arg(tableName);
    bool    hasConstraint = false;
    if (monitorId != Monitor::invalidMonitorId) {
        hasConstraint  = true;
        result        += QString(" WHERE monitor_id = %1").arg(monitorId);
    } else if (hostSchemeId != HostScheme::invalidHostSchemeId) {
        hasConstraint  = true;
        result        += QString(" WHERE monitor_id IN (SELECT monitor_id FROM monitor WHERE host_scheme_id = %1)")
                         .arg(hostSchemeId);
    } else if (customerId != CustomerCapabilities::invalidCustomerId) {
        hasConstraint  = true;
        result        += QString(" WHERE monitor_id IN (SELECT monitor_id FROM monitor WHERE customer_id = %1)")
                         .arg(customerId);
    }

    if (serverId != Server::invalidServerId) {
        if (hasConstraint) {
            result += QString(" AND server_id = %1").arg(serverId);
        } else {
            hasConstraint  = true;
            result        += QString(" WHERE server_id = %1").arg(serverId);
        }
    } else if (regionId != Region::invalidRegionId) {
        QString subQuery = QString("SELECT server_id FROM servers WHERE region_id = %1").arg(regionId);
        if (hasConstraint) {
            result += QString(" AND server_id IN (%1)").arg(subQuery);
        } else {
            hasConstraint  = true;
            result        += QString(" WHERE server_id IN (%1)").arg(subQuery);
        }
    }

    if (startTimestamp > 0) {
        LatencyEntry::ZoranTimeStamp zoranTimestamp = toZoranTimestamp(startTimestamp);

        if (hasConstraint) {
            result += QString(" AND timestamp >= %1").arg(zoranTimestamp);
        } else {
            hasConstraint  = true;
            result        += QString(" WHERE timestamp >= %1").arg(zoranTimestamp);
        }
    }

    if (endTimestamp != std::numeric_limits<unsigned long long>::max()) {
        LatencyEntry::ZoranTimeStamp zoranTimestamp = toZoranTimestamp(endTimestamp);

        if (hasConstraint) {
            result += QString(" AND timestamp <= %1").arg(zoranTimestamp);
        } else {
            hasConstraint  = true;
            result        += QString(" WHERE timestamp <= %1").arg(zoranTimestamp);
        }
    }

    return result;
}


LatencyEntry::ZoranTimeStamp LatencyInterfaceManager::toZoranTimestamp(unsigned long long unixTimestamp) {
    unsigned long long result =   unixTimestamp < LatencyEntry::startOfZoranEpoch
                                ? 0
                                : unixTimestamp - LatencyEntry::startOfZoranEpoch;

    if (result > std::numeric_limits<LatencyEntry::ZoranTimeStamp>::max()) {
        result = std::numeric_limits<LatencyEntry::ZoranTimeStamp>::max();
    }

    return static_cast<LatencyEntry::ZoranTimeStamp>(result);
}

