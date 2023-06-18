/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref LatencyInterface class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QHash>
#include <QSet>
#include <QList>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include <cstdint>

#include "log.h"
#include "short_latency_entry.h"
#include "database_manager.h"
#include "latency_entry.h"
#include "latency_interface.h"

const unsigned      LatencyInterface::queueCheckInterval = 10;
const unsigned      LatencyInterface::numberCyclesBeforeForcedCommit = 30;
const unsigned long LatencyInterface::maximumNumberCachedEntries = 8000000;
const unsigned long LatencyInterface::maximumRowsPerTransaction = 100;
const unsigned      LatencyInterface::retryIntervalMilliseconds = 30000;

LatencyInterface::LatencyInterface(
        DatabaseManager* databaseManager,
        unsigned         connectionId,
        QObject*         parent
    ):QThread(
        parent
    ) {
    currentDatabaseManager = databaseManager;
    currentConnectionId    = connectionId;

    currentInProcessEntries = nullptr;
    currentIncomingEntries  = new LatencyEntryList;
    currentIncomingEntries->reserve(maximumNumberCachedEntries + maximumNumberCachedEntries / 2);

    shutdownRequested = false;
}


LatencyInterface::~LatencyInterface() {
    shutdownRequested = true;
    wait();
}


void LatencyInterface::addEntry(
        MonitorId           monitorId,
        ServerId            serverId,
        ZoranTimeStamp      zoranTimestamp,
        LatencyMicroseconds latencyMicroseconds
    ) {
    incomingEntriesMutex.lock();
    currentIncomingEntries->append(
        LatencyEntry(
            monitorId,
            serverId,
            zoranTimestamp,
            latencyMicroseconds
        )
    );
    incomingEntriesMutex.unlock();
}



void LatencyInterface::addEntry(const LatencyEntry& latencyEntry) {
    incomingEntriesMutex.lock();
    currentIncomingEntries->append(latencyEntry);
    incomingEntriesMutex.unlock();
}


void LatencyInterface::addEntries(const QList<LatencyEntry>& latencyEntries) {
    incomingEntriesMutex.lock();
    currentIncomingEntries->append(latencyEntries);
    incomingEntriesMutex.unlock();
}


void LatencyInterface::receivedEntries() {
    if (!isRunning()) {
        start();
    }
}


void LatencyInterface::run() {
    unsigned long numberQueuedEntries;

    do {
        unsigned cyclesRemaining = numberCyclesBeforeForcedCommit;
        do {
            sleep(queueCheckInterval);

            incomingEntriesMutex.lock();
            numberQueuedEntries = static_cast<unsigned long>(currentIncomingEntries->size());
            incomingEntriesMutex.unlock();

            --cyclesRemaining;
        } while (!shutdownRequested && numberQueuedEntries < maximumNumberCachedEntries && cyclesRemaining > 0);

        if (!shutdownRequested && numberQueuedEntries > 0) {
            performFlush();
        }

        incomingEntriesMutex.lock();
        numberQueuedEntries = static_cast<unsigned long>(currentIncomingEntries->size());
        incomingEntriesMutex.unlock();
    } while (numberQueuedEntries > 0);
}


void LatencyInterface::performFlush() {
    incomingEntriesMutex.lock();

    currentInProcessEntries = currentIncomingEntries;
    currentIncomingEntries  = new LatencyEntryList;
    currentIncomingEntries->reserve(maximumNumberCachedEntries + maximumNumberCachedEntries / 2);

    incomingEntriesMutex.unlock();

    bool          success        = true;
    unsigned long entryBaseIndex = 0;
    unsigned long numberEntries  = static_cast<unsigned long>(currentInProcessEntries->size());

    QString queryString  = QString(
        "INSERT INTO latency_seconds (monitor_id, server_id, timestamp, latency) "
                             "VALUES (:monitor_id, :server_id, :timestamp, :latency) "
                    "ON CONFLICT DO NOTHING"
    );
    QString databaseName = QString("LatencyInterface%1").arg(currentConnectionId);

    do {
        QSqlDatabase database = currentDatabaseManager->getDatabase(databaseName);

        success = database.isOpen();
        if (!success) {
            logWrite(
                QString("Failed to open database while inserting new data: %1 -- retrying")
                .arg(database.lastError().text()),
                true
            );
        }

        while (success && entryBaseIndex < numberEntries) {
            unsigned long entriesRemaining             = numberEntries - entryBaseIndex;
            unsigned long numberEntriesThisTransaction = std::min(entriesRemaining, maximumRowsPerTransaction);
            bool          supportsTransactions;

            if (database.driver()->hasFeature(QSqlDriver::DriverFeature::Transactions)) {
                supportsTransactions = true;
                database.transaction();
            } else {
                supportsTransactions = false;
            }

            QSqlQuery query(database);

            QSet<MonitorId> validMonitorIds;
            success = query.exec("SELECT monitor_id FROM monitor");
            if (success) {
                int monitorIdField = query.record().indexOf("monitor_id");
                while (success && query.next()) {
                    validMonitorIds.insert(query.value(monitorIdField).toUInt());
                }

                QSet<ServerId>  validServerIds;
                success = query.exec("SELECT server_id FROM servers");
                if (success) {
                    int serverIdField = query.record().indexOf("server_id");
                    while (success && query.next()) {
                        validServerIds.insert(query.value(serverIdField).toUInt());
                    }

                    query.finish();
                    success = query.prepare(queryString);
                    if (success) {
                        unsigned offset     = static_cast<unsigned>(-1);
                        unsigned lastOffset = numberEntriesThisTransaction - 1;
                        do {
                            ++offset;
                            const LatencyEntry& latencyEntry = currentInProcessEntries->at(entryBaseIndex + offset);

                            LatencyEntry::MonitorId           monitorId           = latencyEntry.monitorId();
                            LatencyEntry::ServerId            serverId            = latencyEntry.serverId();
                            LatencyEntry::LatencyMicroseconds latencyMicroseconds = latencyEntry.latencyMicroseconds();

                            if (latencyMicroseconds <= LatencyEntry::maximumAllowedLatencyMicroseconds &&
                                validMonitorIds.contains(monitorId)                                    &&
                                validServerIds.contains(serverId)                                         ) {
                                query.bindValue(":monitor_id", QVariant(monitorId));
                                query.bindValue(":server_id", QVariant(serverId));
                                query.bindValue(":timestamp", QVariant(latencyEntry.zoranTimestamp()));
                                query.bindValue(":latency", QVariant(latencyMicroseconds));

                                success = query.exec();
                            }
                        } while (success && offset < lastOffset);

                        if (!success) {
                            logWrite(
                                QString("Failed query: \"%1\" during insertion: %2 -- retrying")
                                .arg(queryString, query.lastError().text()),
                                true
                            );
                        }
                    } else {
                        logWrite(
                            QString("Failed to prepare query: %1 -- retrying").arg(query.lastError().text()),
                            true
                        );
                    }
                }
            }

            if (supportsTransactions) {
                if (success) {
                    success = database.commit();
                    if (success) {
                        entryBaseIndex += numberEntriesThisTransaction;
                    } else {
                        logWrite(
                            QString("Failed commit while inserting new data: %1 -- retrying")
                            .arg(query.lastError().text()),
                            true
                        );
                    }
                } else {
                    success = database.rollback();
                    if (!success) {
                        logWrite(
                            QString("Failed rollback while inserting new data: %1 -- retrying")
                            .arg(query.lastError().text()),
                            true
                        );
                    }
                }
            }
        }

        currentDatabaseManager->closeAndRelease(database);

        if (success) {
            delete currentInProcessEntries;
            currentInProcessEntries = nullptr;
        } else {
            msleep(retryIntervalMilliseconds);
        }
    } while (!success);
}
