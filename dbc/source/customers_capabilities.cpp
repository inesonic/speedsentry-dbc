/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref CustomersCapabilities class.
***********************************************************************************************************************/

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QHash>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include <cstdint>
#include <cstring>
#include <cmath>

#include "log.h"
#include "database_manager.h"
#include "cache.h"
#include "customer_capabilities.h"
#include "customers_capabilities.h"

CustomersCapabilities::CustomersCapabilities(
        DatabaseManager*  databaseManager,
        unsigned          maximumCacheDepth,
        QObject*          parent
    ):QObject(
        parent
    ),Cache<CustomerCapabilities, CustomerCapabilities::CustomerId>(
        maximumCacheDepth
    ),currentDatabaseManager(
        databaseManager
    ) {}


CustomersCapabilities::~CustomersCapabilities() {}


CustomerCapabilities CustomersCapabilities::getCustomerCapabilities(
        CustomerId customerId,
        bool       noCacheUpdate,
        unsigned   threadId
    ) {
    CustomerCapabilities result;

    cacheMutex.lock();
    const CustomerCapabilities* cacheResult = getCacheEntry(customerId);
    if (cacheResult == nullptr) {
        cacheMutex.unlock();
        QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
        bool success = database.isOpen();
        if (success) {
            QSqlQuery query(database);
            query.setForwardOnly(true);

            QString queryString = QString("SELECT * FROM customer_capabilities WHERE customer_id = %1").arg(customerId);
            success = query.exec(queryString);
            if (success) {
                if (query.first()) {
                    int numberMonitorsField  = query.record().indexOf("number_monitors");
                    int pollingIntervalField = query.record().indexOf("polling_interval");
                    int expirationDaysField  = query.record().indexOf("expiration_days");
                    int flagsField           = query.record().indexOf("flags");
                    if (numberMonitorsField >= 0  &&
                        pollingIntervalField >= 0 &&
                        expirationDaysField >= 0  &&
                        flagsField >= 0              ) {
                        unsigned numberMonitors = query.value(numberMonitorsField).toUInt(&success);
                        if (success && numberMonitors <= 0xFFFF) {
                            unsigned pollingInterval = query.value(pollingIntervalField).toUInt(&success);
                            if (success && pollingInterval <= 0xFFFF) {
                                unsigned expirationDays = query.value(expirationDaysField).toUInt(&success);
                                if (success) {
                                    unsigned flags = query.value(flagsField).toUInt(&success);
                                    if (success && flags <= 0xFFFF) {
                                        result = CustomerCapabilities(
                                            customerId,
                                            static_cast<unsigned short>(numberMonitors),
                                            static_cast<unsigned short>(pollingInterval),
                                            expirationDays,
                                            static_cast<CustomerCapabilities::Flags>(flags)
                                        );

                                        if (!noCacheUpdate) {
                                            cacheMutex.lock();
                                            addToCache(result);
                                            cacheMutex.unlock();
                                        }
                                    } else {
                                        success = false;
                                        logWrite(
                                            QString(
                                                "Failed - CustomerCapabilities::getCustomerCapabilities: "
                                                "flags = 0x%1, customer_id = %2"
                                            ).arg(flags, 0, 16)
                                             .arg(customerId),
                                            true
                                        );
                                    }
                                } else {
                                    logWrite(
                                        QString(
                                            "Failed - CustomerCapabilities::getCustomerCapabilities: "
                                            "expiration_days = %1, customer_id = %2"
                                        ).arg(expirationDays)
                                         .arg(customerId),
                                        true
                                    );
                                }
                            } else{
                                success = false;
                                logWrite(
                                    QString(
                                        "Failed - CustomerCapabilities::getCustomerCapabilities: "
                                        "polling_interval = %1, customer_id = %2"
                                    ).arg(pollingInterval)
                                     .arg(customerId),
                                    true
                                );
                            }
                        } else {
                            success = false;
                            logWrite(
                                QString(
                                    "Failed - CustomerCapabilities::getCustomerCapabilities: "
                                    "number_monitors = %1, customer_id = %2"
                                ).arg(numberMonitors)
                                 .arg(customerId),
                                true
                            );
                        }
                    } else {
                        success = false;
                        logWrite(
                            QString(
                                "Failed to get field index - CustomerCapabilities::getCustomerCapabilities: "
                            ).arg(query.lastError().text()),
                            true
                        );
                    }
                }
            } else {
                logWrite(
                    QString(
                        "Failed SELECT - CustomerCapabilities::getCustomerCapabilities: "
                    ).arg(query.lastError().text()),
                    true
                );
            }
        } else {
            logWrite(
                QString(
                    "Failed to open database - CustomerCapabilities::getCustomerCapabilities: "
                ).arg(database.lastError().text()),
                true
            );
        }

        currentDatabaseManager->closeAndRelease(database);
    } else {
        result = *cacheResult;
        cacheMutex.unlock();
    }

    return result;
}


bool CustomersCapabilities::deleteCustomerCapabilities(CustomerId customerId, unsigned threadId) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString = QString("DELETE FROM customer_capabilities WHERE customer_id = %1")
                .arg(customerId);
        success = query.exec(queryString);

        if (!success) {
            logWrite(
                QString(
                    "Failed DELETE - CustomerCapabilities::deleteCustomerCapabilities: "
                ).arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString(
                "Failed to open database - CustomerCapabilities::deleteCustomerCapabilities: "
            ).arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);

    cacheMutex.lock();
    evictCacheEntry(customerId);
    cacheMutex.unlock();

    return success;
}


bool CustomersCapabilities::purgeCustomerCapabilities(
        const CustomersCapabilities::CustomerIdSet& customerIds,
        unsigned                                    threadId
    ) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString inString;
        bool    first = true;

        cacheMutex.lock();
        for (CustomerIdSet::const_iterator it=customerIds.constBegin(),end=customerIds.constEnd() ; it!=end ; ++it) {
            CustomerId customerId = *it;
            evictCacheEntry(customerId);

            if (first) {
                inString = QString::number(customerId);
                first    = false;
            } else {
                inString += "," + QString::number(customerId);
            }
        }

        QString queryString = QString("DELETE FROM customer_capabilities WHERE customer_id IN (%1)")
                .arg(inString);

        success = query.exec(queryString);
        cacheMutex.unlock();

        if (!success) {
            logWrite(
                QString(
                    "Failed DELETE - CustomerCapabilities::purgeCustomerCapabilities: "
                ).arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString(
                "Failed to open database - CustomerCapabilities::purgeCustomerCapabilities: "
            ).arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


bool CustomersCapabilities::updateCustomerCapabilities(
        const CustomerCapabilities& customerCapabilities,
        unsigned                    threadId
    ) {
    CustomerCapabilities storedCapabilities = getCustomerCapabilities(customerCapabilities.customerId(), true);

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        bool    doUpdate = storedCapabilities.isValid();
        QString queryString;
        if (doUpdate) {
            queryString = QString(
                "UPDATE customer_capabilities SET "
                    "number_monitors = %1, "
                    "polling_interval = %2, "
                    "expiration_days = %3, "
                    "flags = %4 "
                "WHERE customer_id = %5"
            ).arg(customerCapabilities.maximumNumberMonitors())
             .arg(customerCapabilities.pollingInterval())
             .arg(customerCapabilities.expirationDays())
             .arg(customerCapabilities.flags())
             .arg(customerCapabilities.customerId());
        } else {
            queryString = QString(
                "INSERT INTO customer_capabilities VALUES (%1, %2, %3, %4, %5)"
            ).arg(customerCapabilities.customerId())
             .arg(customerCapabilities.maximumNumberMonitors())
             .arg(customerCapabilities.pollingInterval())
             .arg(customerCapabilities.expirationDays())
             .arg(customerCapabilities.flags());
        }

        QSqlQuery query(database);
        success = query.exec(queryString);
        if (!success) {
            QSqlError lastError = query.lastError();
            if (doUpdate) {
                logWrite(
                    QString(
                        "Failed UPDATE - CustomerCapabilities::updateCustomerCapabilities: "
                    ).arg(query.lastError().text()),
                    true
                );
            } else {
                logWrite(
                    QString(
                        "Failed INSERT - CustomerCapabilities::updateCustomerCapabilities: "
                    ).arg(query.lastError().text()),
                    true
                );
            }
        } else {
            cacheMutex.lock();
            addToCache(customerCapabilities);
            cacheMutex.unlock();
        }
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


CustomersCapabilities::CapabilitiesByCustomerId CustomersCapabilities::getAllCustomerCapabilities(unsigned threadId) {
    CapabilitiesByCustomerId result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT * FROM customer_capabilities");
        success = query.exec(queryString);
        if (success) {
            int customerIdField      = query.record().indexOf("customer_id");
            int numberMonitorsField  = query.record().indexOf("number_monitors");
            int pollingIntervalField = query.record().indexOf("polling_interval");
            int expirationDaysField  = query.record().indexOf("expiration_days");
            int flagsField           = query.record().indexOf("flags");

            if (customerIdField >= 0      &&
                numberMonitorsField >= 0  &&
                pollingIntervalField >= 0 &&
                expirationDaysField >=0   &&
                flagsField >= 0              ) {
                while (success && query.next()) {
                    unsigned customerId = query.value(customerIdField).toUInt(&success);
                    if (success && customerId != 0) {
                        unsigned numberMonitors = query.value(numberMonitorsField).toUInt(&success);
                        if (success && numberMonitors <= 0xFFFF) {
                            unsigned pollingInterval = query.value(pollingIntervalField).toUInt(&success);
                            if (success && pollingInterval <= 0xFFFF) {
                                unsigned expirationDays = query.value(expirationDaysField).toUInt(&success);
                                if (success) {
                                    unsigned flags = query.value(flagsField).toUInt(&success);
                                    if (success && flags <= 0xFFFF) {
                                        CustomerCapabilities cc = CustomerCapabilities(
                                            customerId,
                                            static_cast<unsigned short>(numberMonitors),
                                            static_cast<unsigned short>(pollingInterval),
                                            expirationDays,
                                            static_cast<CustomerCapabilities::Flags>(flags)
                                        );

                                        result.insert(customerId, cc);
                                    } else {
                                        success = false;
                                        logWrite(
                                            QString(
                                                "Failed - CustomerCapabilities::getAllCustomerCapabilities: "
                                                "flags = 0x%1, "
                                                "customer_id = %2"
                                            ).arg(flags, 0, 16)
                                             .arg(customerId),
                                            true
                                        );
                                    }
                                } else {
                                    logWrite(
                                        QString(
                                            "Failed - CustomerCapabilities::getAllCustomerCapabilities: "
                                            "expiration_days = %1, "
                                            "customer_id = %2"
                                        ).arg(expirationDays)
                                         .arg(customerId),
                                        true
                                    );
                                }
                            } else {
                                success = false;
                                logWrite(
                                    QString(
                                        "Failed - CustomerCapabilities::getAllCustomerCapabilities: "
                                        "polling_interval = %1, "
                                        "customer_id = %2"
                                    ).arg(pollingInterval)
                                     .arg(customerId),
                                    true
                                );
                            }
                        } else {
                            success = false;
                            logWrite(
                                QString(
                                    "Failed - CustomerCapabilities::getAllCustomerCapabilities: "
                                    "number_monitors = %1, "
                                    "customer_id = %2"
                                ).arg(numberMonitors)
                                 .arg(customerId),
                                true
                            );
                        }
                    } else {
                        logWrite(
                            QString(
                                "Failed - CustomerCapabilities::getAllCustomerCapabilities: "
                                "customer_id = %1"
                            ).arg(customerId),
                            true
                        );
                    }
                }
            } else {
                logWrite(
                    QString(
                        "Failed to get field index - CustomerCapabilities::getAllCustomerCapabilities: %1"
                    ).arg(query.lastError().text()),
                    true
                );
            }
        } else {
            logWrite(
                QString(
                    "Query failed - CustomerCapabilities::getAllCustomerCapabilities: %1"
                ).arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString(
                "Failed to open database - CustomerCapabilities::getAllCustomerCapabilities: %1"
            ).arg(database.lastError().text()),
            true
        );
    }

    if (!success) {
        result = CapabilitiesByCustomerId();
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


CustomersCapabilities::CustomerId CustomersCapabilities::idFromValue(const CustomerCapabilities& value) const {
    return value.customerId();
}
