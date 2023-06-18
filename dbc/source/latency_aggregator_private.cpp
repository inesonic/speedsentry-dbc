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
* This header implements the \ref LatencyAggregator::Private class.
***********************************************************************************************************************/

#include <QObject>
#include <QThread>
#include <QString>
#include <QMutex>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlDriver>
#include <QSqlError>
#include <QVariant>
#include <QRandomGenerator>
#include <QPair>

#include <limits>

#include "log.h"
#include "database_manager.h"
#include "monitor.h"
#include "server.h"
#include "customer_capabilities.h"
#include "customers_capabilities.h"
#include "short_latency_entry.h"
#include "latency_entry.h"
#include "aggregated_latency_entry.h"
#include "latency_aggregator.h"
#include "latency_aggregator_private.h"

LatencyAggregator::Private::Private(
        DatabaseManager* databaseManager,
        QObject*         parent
    ):QThread(
        parent
    ),currentDatabaseManager(
        databaseManager
    ) {
    currentInputTableMaximumAge = 0;
    currentResamplePeriod       = 0;
    currentInputAggregated      = false;

    for (unsigned i=0 ; i<(sizeof(seed)/sizeof(std::uint64_t)) ; ++i) {
        seed[i] = QRandomGenerator::global()->generate64();
    }

    nextPrngValue    = 0;
    useNextPrngValue = false;
}


LatencyAggregator::Private::~Private() {}


const QString& LatencyAggregator::Private::inputTableName() const {
    return currentInputTableName;
}


const QString& LatencyAggregator::Private::outputTableName() const {
    return currentOutputTableName;
}


unsigned long LatencyAggregator::Private::inputTableMaximumAge() const {
    return currentInputTableMaximumAge;
}


unsigned long LatencyAggregator::Private::resamplePeriod() const {
    return currentResamplePeriod;
}


bool LatencyAggregator::Private::inputAlreadyAggregated() const {
    return currentInputAggregated;
}


void LatencyAggregator::Private::setParameters(
        const QString& inputTableName,
        const QString& outputTableName,
        unsigned long  inputTableMaximumAge,
        unsigned long  resamplePeriod,
        unsigned long  expungePeriod,
        bool           inputAggregated
    ) {
    accessMutex.lock();

    currentInputTableName       = inputTableName;
    currentOutputTableName      = outputTableName;
    currentInputTableMaximumAge = inputTableMaximumAge;
    currentResamplePeriod       = resamplePeriod;
    currentExpungePeriod        = expungePeriod;
    currentInputAggregated      = inputAggregated;

    accessMutex.unlock();
}


bool LatencyAggregator::Private::deleteByCustomerId(
        const CustomersCapabilities::CustomerIdSet& customerIds,
        unsigned                                    threadId
    ) {
    QString inString;
    bool    first = true;
    for (  CustomersCapabilities::CustomerIdSet::const_iterator it  = customerIds.constBegin(),
                                                                end = customerIds.constEnd()
         ; it!=end
         ; ++it
        ) {
        CustomerCapabilities::CustomerId customerId = *it;
        if (first) {
            inString = QString::number(customerId);
            first    = false;
        } else {
            inString += "," + QString::number(customerId);
        }
    }

    QString queryString = (
          QString("DELETE FROM %1 WHERE monitor_id IN ")
        + QString("(SELECT monitor_id FROM monitor WHERE customer_id IN (%1))").arg(inString)
    );

    accessMutex.lock();

    QString inputTableName  = currentInputTableName;
    QString outputTableName = currentOutputTableName;

    accessMutex.unlock();

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        bool supportsTransactions;
        if (database.driver()->hasFeature(QSqlDriver::DriverFeature::Transactions)) {
            supportsTransactions = true;
            database.transaction();
        } else {
            supportsTransactions = false;
        }

        QSqlQuery query(database);
        bool success = query.exec(queryString.arg(inputTableName));
        if (!success) {
            logWrite(
                QString("Failed DELETE -- LatencyAggregator::deleteByCustomerId (1): %1")
                .arg(query.lastError().text()),
                true
            );
        } else {
            success = query.exec(queryString.arg(outputTableName));
            if (!success) {
                logWrite(
                    QString("Failed DELETE -- LatencyAggregator::deleteByCustomerId (2): %1")
                    .arg(query.lastError().text()),
                    true
                );
            }
        }

        if (supportsTransactions) {
            if (success) {
                success = database.commit();
                if (!success) {
                    logWrite(
                        QString("Failed commit - LatencyAggregator::deleteByCustomerId: ")
                        .arg(database.lastError().text()),
                        true
                    );
                }
            } else {
                bool rollbackSuccess = database.rollback();
                if (!rollbackSuccess) {
                    logWrite(
                        QString("Failed rollback - LatencyAggregator::deleteByCustomerId: ")
                        .arg(database.lastError().text()),
                        true
                    );
                }
            }
        }
    } else {
        logWrite(
            QString("Failed to open database - LatencyAggregator::deleteByCustomerId: ")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


void LatencyAggregator::Private::run() {
    accessMutex.lock();

    QString       inputTableName       = currentInputTableName;
    QString       outputTableName      = currentOutputTableName;
    unsigned long inputTableMaximumAge = currentInputTableMaximumAge;
    bool          inputAggregated      = currentInputAggregated;
    unsigned long expungePeriod        = currentExpungePeriod;

    accessMutex.unlock();

    unsigned long long currentTime      = QDateTime::currentSecsSinceEpoch();
    unsigned long long expungeThreshold = currentTime - expungePeriod;
    unsigned long long timeThreshold    = currentTime - inputTableMaximumAge;

    timeThreshold = timeThreshold - (timeThreshold % currentResamplePeriod);

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString("Database_%1").arg(outputTableName));
    bool success = database.isOpen();
    if (success) {
        bool supportsTransactions;
        if (database.driver()->hasFeature(QSqlDriver::DriverFeature::Transactions)) {
            supportsTransactions = true;
            database.transaction();
        } else {
            supportsTransactions = false;
        }

        QList<AggregatedLatencyEntry> aggregatedValues = generateAggregatedEntries(
            success,
            database,
            timeThreshold,
            currentResamplePeriod,
            inputTableName,
            inputAggregated
        );

        if (success) {
            success = writeAggregatedEntries(database, aggregatedValues, outputTableName);
            if (success) {
                success = deleteOldEntries(database, timeThreshold, inputTableName);
            }
        }

        if (supportsTransactions) {
            if (success) {
                success = database.commit();
                if (!success) {
                    logWrite(
                        QString("Failed commit - LatencyAggregator: ").arg(database.lastError().text()),
                        true
                    );
                }
            } else {
                success = database.rollback();
                if (!success) {
                    logWrite(
                        QString("Failed rollback - LatencyAggregator: ").arg(database.lastError().text()),
                        true
                    );
                }
            }
        }
    } else {
        logWrite(QString("Failed to open database - LatencyAggregator: ").arg(database.lastError().text()), true);
    }

    if (!inputAggregated) {
        deleteOldEntries(database, expungeThreshold, inputTableName);
    }

    deleteOldEntries(database, expungeThreshold, outputTableName);

    currentDatabaseManager->closeAndRelease(database);
}


QList<AggregatedLatencyEntry> LatencyAggregator::Private::generateAggregatedEntries(
        bool&              success,
        QSqlDatabase&      database,
        unsigned long long timeThreshold,
        unsigned long      resamplePeriod,
        const QString&     inputTableName,
        bool               inputAggregated
    ) {
    QList<AggregatedLatencyEntry> result;

    QSqlQuery query(database);
    query.setForwardOnly(true);

    QString queryString = QString(
        "SELECT * FROM %1 WHERE timestamp < %2 ORDER BY monitor_id ASC, server_id ASC, timestamp ASC"
    ).arg(inputTableName)
     .arg(LatencyEntry::toZoranTimestamp(timeThreshold));

    success = query.exec(queryString);
    if (success) {
        int monitorIdField       = -1;
        int serverIdField        = -1;
        int timestampField       = -1;
        int latencyField         = -1;
        int startTimestampField  = -1;
        int endTimestampField    = -1;
        int meanLatencyField     = -1;
        int varianceLatencyField = -1;
        int minimumLatencyField  = -1;
        int maximumLatencyField  = -1;
        int numberSamplesField   = -1;

        LatencyEntry::MonitorId           monitorId       = 0;
        LatencyEntry::ServerId            serverId        = 0;
        unsigned long long                timestamp       = 0;
        LatencyEntry::LatencyMicroseconds latency         = 0;
        unsigned long long                startTimestamp  = 0;
        unsigned long long                endTimestamp    = 0;
        double                            meanLatency     = 0;
        double                            varianceLatency = 0;
        double                            minimumLatency  = 0;
        double                            maximumLatency  = 0;
        unsigned long                     numberSamples   = 0;

        success = getFieldIndexes(
            query,
            inputAggregated,
            monitorIdField,
            serverIdField,
            timestampField,
            latencyField,
            startTimestampField,
            endTimestampField,
            meanLatencyField,
            varianceLatencyField,
            minimumLatencyField,
            maximumLatencyField,
            numberSamplesField
        );

        if (success) {
            LatencyEntry::MonitorId lastMonitorId        = Monitor::invalidMonitorId;
            LatencyEntry::ServerId  lastServerId         = Server::invalidServerId;
            unsigned long long      periodStartTimestamp = 0;
            unsigned long long      periodEndTimestamp   = 0;

            QList<ShortLatencyEntry>          shortValues;
            QList<WeightsAndMeans>            weightsAndMeans;
            double                            weightedSumMeanLatency     = 0;
            double                            weightedSumVarianceLatency = 0;
            LatencyEntry::LatencyMicroseconds aggregatedMinimumLatency   =
                std::numeric_limits<LatencyEntry::LatencyMicroseconds>::max();
            LatencyEntry::LatencyMicroseconds aggregatedMaximumLatency   = 0;
            unsigned long                     aggregatedNumberSamples    = 0;

            while (success && query.next()) {
                success = getRecord(
                    query,
                    inputAggregated,
                    monitorIdField,
                    serverIdField,
                    timestampField,
                    latencyField,
                    startTimestampField,
                    endTimestampField,
                    meanLatencyField,
                    varianceLatencyField,
                    minimumLatencyField,
                    maximumLatencyField,
                    numberSamplesField,
                    monitorId,
                    serverId,
                    timestamp,
                    latency,
                    startTimestamp,
                    endTimestamp,
                    meanLatency,
                    varianceLatency,
                    minimumLatency,
                    maximumLatency,
                    numberSamples
                );

                if (success) {
                    if (monitorId != lastMonitorId || serverId != lastServerId || endTimestamp >= periodEndTimestamp) {
                        if (lastMonitorId != Monitor::invalidMonitorId &&
                            lastServerId != Server::invalidServerId    &&
                            !shortValues.isEmpty()                        ) {
                            AggregatedLatencyEntry aggregatedEntry = generateEntry(
                                lastMonitorId,
                                lastServerId,
                                weightedSumMeanLatency,
                                weightedSumVarianceLatency,
                                shortValues,
                                weightsAndMeans,
                                periodStartTimestamp,
                                periodEndTimestamp,
                                aggregatedMinimumLatency,
                                aggregatedMaximumLatency,
                                aggregatedNumberSamples
                            );

                            shortValues.clear();
                            weightsAndMeans.clear();

                            weightedSumMeanLatency     = 0;
                            weightedSumVarianceLatency = 0;
                            aggregatedMinimumLatency   = std::numeric_limits<LatencyEntry::LatencyMicroseconds>::max();
                            aggregatedMaximumLatency   = 0;
                            aggregatedNumberSamples    = 0;

                            result.append(aggregatedEntry);
                        }

                        lastMonitorId = monitorId;
                        lastServerId  = serverId;

                        // Note that the algorithm relies on two conditions:
                        // - Entries have been sorted by the database first by time order.
                        // - Within the sort, all start/end values bound the actual sample time.

                        if (endTimestamp >= periodEndTimestamp || startTimestamp < periodStartTimestamp) {
                            periodStartTimestamp = endTimestamp - (endTimestamp % resamplePeriod);
                            periodEndTimestamp   = periodStartTimestamp + resamplePeriod;
                        }
                    }

                    if (inputAggregated) {
                        weightedSumMeanLatency     += numberSamples * meanLatency;
                        weightedSumVarianceLatency += numberSamples * varianceLatency;

                        if (minimumLatency < aggregatedMinimumLatency) {
                            aggregatedMinimumLatency = minimumLatency;
                        }

                        if (maximumLatency > aggregatedMaximumLatency) {
                            aggregatedMaximumLatency = maximumLatency;
                        }

                        weightsAndMeans.append(WeightsAndMeans(meanLatency, numberSamples));

                        aggregatedNumberSamples += numberSamples;
                    } else {
                        weightedSumMeanLatency += latency;

                        if (latency < aggregatedMinimumLatency) {
                            aggregatedMinimumLatency = latency;
                        }

                        if (latency > aggregatedMaximumLatency) {
                            aggregatedMaximumLatency = latency;
                        }

                        weightsAndMeans.append(latency);
                        ++aggregatedNumberSamples;
                    }

                    shortValues.append(ShortLatencyEntry(LatencyEntry::toZoranTimestamp(timestamp), latency));
                }
            }

            if (lastMonitorId != Monitor::invalidMonitorId &&
                lastServerId != Server::invalidServerId    &&
                !shortValues.isEmpty()                        ) {
                AggregatedLatencyEntry aggregatedEntry = generateEntry(
                    lastMonitorId,
                    lastServerId,
                    weightedSumMeanLatency,
                    weightedSumVarianceLatency,
                    shortValues,
                    weightsAndMeans,
                    periodStartTimestamp,
                    periodEndTimestamp,
                    aggregatedMinimumLatency,
                    aggregatedMaximumLatency,
                    aggregatedNumberSamples
                );

                result.append(aggregatedEntry);
            }
        } else {
            logWrite(
                QString("Failed to obtain field index values -- LatencyAggregator: %1")
                .arg(query.lastError().text()),
                true
            );
            success = false;
        }
    } else {
        logWrite(
            QString("Failed SELECT -- LatencyAggregator: %1")
            .arg(query.lastError().text()),
            true
        );
        success = false;
    }

    return result;
}


bool LatencyAggregator::Private::writeAggregatedEntries(
        QSqlDatabase&                        database,
        const QList<AggregatedLatencyEntry>& aggregatedEntries,
        const QString&                       outputTableName
    ) {
    QString queryString  = QString(
        "INSERT INTO %1 ("
            "monitor_id, "
            "server_id, "
            "timestamp, "
            "latency, "
            "start_timestamp, "
            "end_timestamp, "
            "mean_latency, "
            "variance_latency, "
            "minimum_latency, "
            "maximum_latency, "
            "number_samples"
        ") VALUES ("
            ":monitor_id, "
            ":server_id, "
            ":timestamp, "
            ":latency, "
            ":start_timestamp, "
            ":end_timestamp, "
            ":mean_latency, "
            ":variance_latency, "
            ":minimum_latency, "
            ":maximum_latency, "
            ":number_samples"
        ") ON CONFLICT DO NOTHING"
    ).arg(outputTableName);

    QSqlQuery query(database);
    bool success = query.prepare(queryString);
    if (success) {
        QList<AggregatedLatencyEntry>::const_iterator entryIterator    = aggregatedEntries.constBegin();
        QList<AggregatedLatencyEntry>::const_iterator entryEndIterator = aggregatedEntries.constEnd();
        while (success && entryIterator != entryEndIterator) {
            const AggregatedLatencyEntry& entry = *entryIterator;

            query.bindValue(":monitor_id", entry.monitorId());
            query.bindValue(":server_id", entry.serverId());
            query.bindValue(":timestamp", entry.zoranTimestamp());
            query.bindValue(":latency", entry.latencyMicroseconds());
            query.bindValue(":start_timestamp", entry.startZoranTimestamp());
            query.bindValue(":end_timestamp", entry.endZoranTimestamp());
            query.bindValue(":mean_latency", entry.meanLatency());
            query.bindValue(":variance_latency", entry.varianceLatency());
            query.bindValue(":minimum_latency", entry.minimumLatency());
            query.bindValue(":maximum_latency", entry.maximumLatency());
            query.bindValue(":number_samples", static_cast<unsigned>(entry.numberSamples()));

            success = query.exec();

            ++entryIterator;
        }

        if (!success) {
            logWrite(
                QString("Failed to execute INSERT -- LatencyAggregator: %1")
                .arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to prepare INSERT -- LatencyAggregator: %1")
            .arg(query.lastError().text()),
            true
        );
    }

    return success;
}


bool LatencyAggregator::Private::deleteOldEntries(
        QSqlDatabase&      database,
        unsigned long long timeThreshold,
        const QString&     inputTableName
    ) {
    QString   queryString  = QString("DELETE FROM %1 WHERE timestamp < %2")
                             .arg(inputTableName)
                             .arg(LatencyEntry::toZoranTimestamp(timeThreshold));
    QSqlQuery query(database);

    bool success = query.exec(queryString);
    if (!success) {
        logWrite(
            QString("Failed DELETE -- LatencyAggregator: %1")
            .arg(query.lastError().text()),
            true
        );
    }

    return success;
}


bool LatencyAggregator::Private::getFieldIndexes(
        const QSqlQuery& query,
        bool             inputIsAggregated,
        int&             monitorIdField,
        int&             serverIdField,
        int&             timestampField,
        int&             latencyField,
        int&             startTimestampField,
        int&             endTimestampField,
        int&             meanLatencyField,
        int&             varianceLatencyField,
        int&             minimumLatencyField,
        int&             maximumLatencyField,
        int&             numberSamplesField
    ) {
    monitorIdField = query.record().indexOf("monitor_id");
    serverIdField  = query.record().indexOf("server_id");
    timestampField = query.record().indexOf("timestamp");
    latencyField   = query.record().indexOf("latency");

    bool success = (
           monitorIdField >= 0
        && serverIdField >= 0
        && timestampField >= 0
        && latencyField >= 0
    );

    if (success && inputIsAggregated) {
        startTimestampField  = query.record().indexOf("start_timestamp");
        endTimestampField    = query.record().indexOf("end_timestamp");
        meanLatencyField     = query.record().indexOf("mean_latency");
        varianceLatencyField = query.record().indexOf("variance_latency");
        minimumLatencyField  = query.record().indexOf("minimum_latency");
        maximumLatencyField  = query.record().indexOf("maximum_latency");
        numberSamplesField   = query.record().indexOf("number_samples");

        success = (
               startTimestampField >= 0
            && endTimestampField >= 0
            && meanLatencyField >= 0
            && varianceLatencyField >= 0
            && minimumLatencyField >= 0
            && maximumLatencyField >= 0
            && numberSamplesField >= 0
        );
    }

    return success;
}


bool LatencyAggregator::Private::getRecord(
        const QSqlQuery&                   query,
        bool                               inputIsAggregated,
        int                                monitorIdField,
        int                                serverIdField,
        int                                timestampField,
        int                                latencyField,
        int                                startTimestampField,
        int                                endTimestampField,
        int                                meanLatencyField,
        int                                varianceLatencyField,
        int                                minimumLatencyField,
        int                                maximumLatencyField,
        int                                numberSamplesField,
        Monitor::MonitorId&                monitorId,
        Server::ServerId&                  serverId,
        unsigned long long&                timestamp,
        LatencyEntry::LatencyMicroseconds& latency,
        unsigned long long&                startTimestamp,
        unsigned long long&                endTimestamp,
        double&                            meanLatency,
        double&                            varianceLatency,
        double&                            minimumLatency,
        double&                            maximumLatency,
        unsigned long&                     numberSamples
    ) {
    bool success;
    monitorId = query.value(monitorIdField).toUInt(&success);
    if (success) {
        serverId = query.value(serverIdField).toUInt(&success);
        if (success) {
            timestamp = LatencyEntry::toUnixTimestamp(query.value(timestampField).toUInt(&success));
            if (success) {
                latency = query.value(latencyField).toUInt(&success);
                if (success && inputIsAggregated) {
                    startTimestamp = LatencyEntry::toUnixTimestamp(query.value(startTimestampField).toUInt(&success));
                    if (success) {
                        endTimestamp = LatencyEntry::toUnixTimestamp(query.value(endTimestampField).toUInt(&success));
                        if (success) {
                            meanLatency = query.value(meanLatencyField).toDouble(&success);
                            if (success) {
                                varianceLatency = query.value(varianceLatencyField).toDouble(&success);
                                if (success) {
                                    minimumLatency = query.value(minimumLatencyField).toUInt(&success);
                                    if (success) {
                                        maximumLatency = query.value(maximumLatencyField).toUInt(&success);
                                        if (success) {
                                            numberSamples = query.value(numberSamplesField).toUInt(&success);
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    startTimestamp = timestamp;
                    endTimestamp = timestamp;
                    meanLatency = latency / 1.0E6;
                    varianceLatency = 0;
                    minimumLatency = meanLatency;
                    maximumLatency = meanLatency;
                    numberSamples = 1;
                }
            }
        }
    }

    return success;
}


AggregatedLatencyEntry LatencyAggregator::Private::generateEntry(
        AggregatedLatencyEntry::MonitorId                         monitorId,
        AggregatedLatencyEntry::ServerId                          serverId,
        double                                                    weightedSumMeanLatency,
        double                                                    weightedSumVarianceLatency,
        const QList<ShortLatencyEntry>&                           rawValues,
        const QList<LatencyAggregator::Private::WeightsAndMeans>& weightsAndMeans,
        unsigned long long                                        periodStartTimestamp,
        unsigned long long                                        periodEndTimestamp,
        AggregatedLatencyEntry::LatencyMicroseconds               minimumLatency,
        AggregatedLatencyEntry::LatencyMicroseconds               maximumLatency,
        unsigned long                                             numberSamples
    ) {
    double aggregationMeanLatency = weightedSumMeanLatency / numberSamples;

    unsigned numberPoints = static_cast<unsigned>(weightsAndMeans.size());
    for (unsigned i=0 ; i<numberPoints ; ++i) {
        const WeightsAndMeans& wam  = weightsAndMeans.at(i);
        double                 d    = wam.meanValue() - aggregationMeanLatency;
        weightedSumVarianceLatency += wam.numberSamples() * d * d;
    }

    double aggregationVarianceLatency = weightedSumVarianceLatency / numberSamples;

    const ShortLatencyEntry& shortEntry = rawValues.at(prng() % rawValues.size());
    return AggregatedLatencyEntry(
        monitorId,
        serverId,
        shortEntry.zoranTimestamp(),
        shortEntry.latencyMicroseconds(),
        LatencyEntry::toZoranTimestamp(periodStartTimestamp),
        LatencyEntry::toZoranTimestamp(periodEndTimestamp),
        aggregationMeanLatency,
        aggregationVarianceLatency,
        minimumLatency,
        maximumLatency,
        numberSamples
    );
}


std::uint32_t LatencyAggregator::Private::prng() {
    std::uint32_t result;
    if (useNextPrngValue) {
        useNextPrngValue = false;
        result           = nextPrngValue;
    } else {
        std::uint64_t s0 = seed[0];
        std::uint64_t s1 = seed[1];
        std::uint64_t s2 = seed[2];
        std::uint64_t s3 = seed[3];

        std::uint64_t t = s1 << 17;
        std::uint64_t r = s0 + s3;

        s2 ^= s0;
        s3 ^= s1;
        s1 ^= s2;
        s0 ^= s3;

        s2 ^= t;
        s3 = (s3 << 45) | (s3 >> 19);

        seed[0] = s0;
        seed[1] = s1;
        seed[2] = s2;
        seed[3] = s3;

        nextPrngValue    = static_cast<std::uint32_t>(r >> 32);
        useNextPrngValue = true;

        result = static_cast<std::uint32_t>(r);
    }

    return result;
}
