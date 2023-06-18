/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref Resources class.
***********************************************************************************************************************/

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>

#include <cstdint>

#include "log.h"
#include "customer_capabilities.h"
#include "active_resources.h"
#include "resource.h"
#include "database_manager.h"
#include "resources.h"

/***********************************************************************************************************************
* Resources
*/

Resources::Resources(
        DatabaseManager* databaseManager,
        QObject*         parent
    ):QThread(
        parent
    ),Cache<ActiveResources, Resource::CustomerId>(
        maximumCacheDepth
    ),currentDatabaseManager(
        databaseManager
    ) {
    currentMaximumResourceDataAge = 0;

    expungeTimer = new QTimer(this);
    expungeTimer->setSingleShot(false);
    connect(expungeTimer, &QTimer::timeout, this, &Resources::startExpunge);
}


Resources::~Resources() {}


ActiveResources Resources::hasResourceData(CustomerId customerId, unsigned threadId) {
    ActiveResources result;

    cacheMutex.lock();
    const ActiveResources* activeResources = getCacheEntry(customerId);
    if (activeResources == nullptr) {
        cacheMutex.unlock();
        QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
        bool success = database.isOpen();
        if (success) {
            QSqlQuery query(database);
            query.setForwardOnly(true);

            QString queryString = QString("SELECT DISTINCT value_type FROM resources WHERE customer_id = %1")
                                  .arg(customerId);
            success = query.exec(queryString);
            if (success) {
                int valueTypeField   = query.record().indexOf("value_type");
                bool hasResourceData = false;
                if (valueTypeField >= 0) {
                    while (success && query.next()) {
                        ValueType valueType = static_cast<ValueType>(query.value(valueTypeField).toUInt(&success));
                        if (success) {
                            if (!hasResourceData) {
                                hasResourceData = true;                                
                                result = ActiveResources(customerId);
                            }

                            result.setActive(valueType);
                        } else {
                            logWrite(QString("Invalid value type - Resources::hasResourceData"), true);
                        }
                    }
                } else {
                    logWrite(QString("Failed to get field values - Resources::hasResourceData"), true);
                }

                if (hasResourceData) {
                    QMutexLocker locker(&cacheMutex);
                    const ActiveResources* activeResources = getCacheEntry(customerId);
                    if (activeResources == nullptr) {
                        addToCache(result);
                    }
                }
            } else {
                logWrite(QString("Failed SELECT - Resources::hasResourceData"), true);
            }
        } else {
            logWrite(
                QString("Failed to open database - Resources::hasResourceData: %1")
                .arg(database.lastError().text()),
                true
            );
        }
    } else {
        result = *activeResources;
        cacheMutex.unlock();
    }

    return result;
}


Resource Resources::recordResource(
        Resources::CustomerId customerId,
        Resources::ValueType  valueType,
        Resources::Value      value,
        unsigned long long    unixTimestamp,
        unsigned              threadId
    ) {
    Resource result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QString queryString = QString(
            "INSERT INTO resources (customer_id, value_type, value, timestamp1, timestamp2) "
            "VALUES (:customer_id, :value_type, :value, :timestamp1, :timestamp2)"
        );

        QSqlQuery query(database);
        query.setForwardOnly(true);

        success = query.prepare(queryString);
        if (success) {
            query.bindValue(":customer_id", static_cast<int>(customerId));
            query.bindValue(":value_type", static_cast<int>(valueType));
            query.bindValue(":value", value);
            query.bindValue(":timestamp1", static_cast<int>(unixTimestamp / 3600));
            query.bindValue(":timestamp2", static_cast<int>(unixTimestamp % 3600));

            success = query.exec();
            if (success) {
                result = Resource(customerId, valueType, value, unixTimestamp);

                QMutexLocker cacheMutexLocker(&cacheMutex);
                ActiveResources* activeResources = getCacheEntry(customerId);
                if (activeResources != nullptr) {
                    activeResources->setActive(valueType);
                }
            } else {
                logWrite(
                    QString("Failed INSERT into resources: %1")
                    .arg(query.lastError().text()),
                    true
                );
            }
        } else {
            logWrite(
                QString("Failed prepare INSERT into resources: %1")
                .arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database for resources: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Resources::ResourceList Resources::getResources(
        Resources::CustomerId customerId,
        Resources::ValueType  valueType,
        unsigned long long    startTimestamp,
        unsigned long long    endTimestamp,
        unsigned              threadId
    ) {
    ResourceList result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT * FROM resources WHERE customer_id = %1 AND value_type = %2")
                              .arg(customerId)
                              .arg(valueType);

        if (startTimestamp != 0) {
            queryString += QString(" AND (timestamp1 > %1 OR (timestamp1 = %2 AND timestamp2 >=%3))")
                           .arg(startTimestamp / 3600)
                           .arg(startTimestamp / 3600)
                           .arg(startTimestamp % 3600);
        }

        if (endTimestamp != 0) {
            queryString += QString(" AND (timestamp1 < %1 OR (timestamp1 = %2 AND timestamp2 <= %3))")
                           .arg(endTimestamp / 3600)
                           .arg(endTimestamp / 3600)
                           .arg(endTimestamp % 3600);
        }

        queryString += QString(" ORDER BY timestamp1 ASC");

        success = query.exec(queryString);
        if (success) {
            int customerIdField = query.record().indexOf("customer_id");
            int valueTypeField  = query.record().indexOf("value_type");
            int valueField      = query.record().indexOf("value");
            int timestamp1Field = query.record().indexOf("timestamp1");
            int timestamp2Field = query.record().indexOf("timestamp2");

            if (customerIdField >= 0 &&
                valueTypeField >= 0  &&
                valueField >= 0      &&
                timestamp1Field >= 0 &&
                timestamp2Field >= 0    ) {
                while (query.next()) {
                    CustomerId customerId = static_cast<CustomerId>(query.value(customerIdField).toInt(&success));
                    if (success) {
                        ValueType valueType = static_cast<ValueType>(query.value(valueTypeField).toInt(&success));
                        if (success) {
                            Value value = static_cast<Value>(query.value(valueField).toDouble(&success));
                            if (success) {
                                unsigned long long timestamp1 = query.value(timestamp1Field).toUInt(&success);
                                if (success) {
                                    unsigned long long timestamp2 = query.value(timestamp2Field).toUInt(&success);

                                    if (success) {
                                        unsigned long long timestamp = (3600 * timestamp1) + timestamp2;
                                        result.append(Resource(customerId, valueType, value, timestamp));
                                    } else {
                                        logWrite(QString("Invalid timestamp2 value - Resources::getResources\n"), true);
                                    }
                                } else {
                                    logWrite(QString("Invalid timestamp1 value - Resources::getResources\n"), true);
                                }
                            } else {
                                logWrite(QString("Invalid value - Resources::getResources\n"), true);
                            }
                        } else {
                            logWrite(QString("Invalid value type - Resources::getResources\n"), true);
                        }
                    } else {
                        logWrite(QString("Invalid customer ID - Resources::getResources\n"), true);
                    }
                }
            } else {
                logWrite(QString("Failed to get field values - Resources::getResources\n"), true);
            }
        } else {
            logWrite(
                QString("Failed SELECT - Resources::getResources: %1").arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - Resources::getResources: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


void Resources::purgeResources(Resources::CustomerId customerId, unsigned long long timestamp, unsigned threadId) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString = QString(
                                  "DELETE FROM resources "
                                  "WHERE (timestamp1 < %1 OR (timestamp1 = %2 AND timestamp2 < %3))"
                              ).arg(timestamp / 3600)
                               .arg(timestamp / 3600)
                               .arg(timestamp % 3600);

        if (customerId != CustomerCapabilities::invalidCustomerId) {
            queryString += QString(" AND customer_id = %1").arg(customerId);
        }

        success = query.exec(queryString);
        if (!success) {
            logWrite(QString("Failed DELETE - Resources::purgeResources: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - Resources::purgeResources: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
}


void Resources::setMaximumAge(unsigned long newMaximumAge) {
    currentMaximumResourceDataAge = newMaximumAge;

    if (newMaximumAge == 0) {
        if (expungeTimer->isActive()) {
            expungeTimer->stop();
        }
    } else {
        if (!expungeTimer->isActive()) {
            expungeTimer->start(expungeTimerPeriod);
        }
    }
}


Resources::CustomerId Resources::idFromValue(const ActiveResources& value) const {
    return value.customerId();
}


void Resources::run() {
    unsigned long long expungeThreshold = QDateTime::currentSecsSinceEpoch() - currentMaximumResourceDataAge;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(expungeThreadId));
    bool success = database.isOpen();
    if (success) {
        bool supportsTransactions;
        if (database.driver()->hasFeature(QSqlDriver::DriverFeature::Transactions)) {
            supportsTransactions = true;
            database.transaction();
        } else {
            supportsTransactions = false;
        }

        QList<Resource::CustomerId> updatedCustomers;
        QSqlQuery query(database);

        QString queryString = QString(
                                  "SELECT DISTINCT customer_id FROM resources "
                                  "WHERE timestamp1 < %1 OR (timestamp1 = %2 AND timestamp2 < %3)"
                              )
                              .arg(expungeThreshold / 3600)
                              .arg(expungeThreshold / 3600)
                              .arg(expungeThreshold % 3600);

        success = query.exec(queryString);
        if (success) {
            int customerIdField = query.record().indexOf("customer_id");
            if (customerIdField >= 0) {
                while (success && query.next()) {
                    Resource::CustomerId customerId = static_cast<Resource::CustomerId>(
                        query.value(customerIdField).toUInt(&success)
                    );

                    if (success) {
                        updatedCustomers.append(customerId);
                    }
                }

                if (!success) {
                    logWrite(QString("Failed to read entry -- Resources::run: %1").arg(query.lastError().text()), true);
                }
            } else {
                logWrite(QString("Invalid field ID -- Resources::run: %1").arg(query.lastError().text()), true);
                success = false;
            }
        } else {
            logWrite(QString("Failed SELECT DISTINCT -- Resources::run: %1").arg(query.lastError().text()), true);
        }

        if (success) {
            queryString = QString(
                              "DELETE FROM resources "
                              "WHERE timestamp1 < %1 OR (timestamp1 = %2 AND timestamp2 < %3)"
                          )
                          .arg(expungeThreshold / 3600)
                          .arg(expungeThreshold / 3600)
                          .arg(expungeThreshold % 3600);

            success = query.exec(queryString);
            if (!success) {
                logWrite(QString("Failed DELETE - Resources::run: %1").arg(query.lastError().text()), true);
            }
        }

        if (success) {
            cacheMutex.lock();

            for (  QList<Resource::CustomerId>::const_iterator it  = updatedCustomers.constBegin(),
                                                               end = updatedCustomers.constEnd()
                 ; it!=end
                 ; ++it
                ) {
                evictCacheEntry(*it);
            }

            cacheMutex.unlock();

            if (supportsTransactions) {
                success = database.commit();
                if (!success) {
                    logWrite(QString("Failed commit - Resources::run: %1").arg(database.lastError().text()), true);
                }
            }
        } else if (supportsTransactions) {
            success = database.rollback();
            if (!success) {
                logWrite(QString("Failed rollback - Resources::run: %1").arg(database.lastError().text()), true);
            }
        }
    }

    currentDatabaseManager->closeAndRelease(database);
}


void Resources::startExpunge() {
    start();
}

