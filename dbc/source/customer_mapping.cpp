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
* This header implements the \ref CustomerMapping class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include <cstdint>

#include "log.h"
#include "database_manager.h"
#include "customer_mapping.h"

CustomerMapping::CustomerMapping(
        DatabaseManager* databaseManager,
        QObject*         parent
    ):QObject(
        parent
    ),currentDatabaseManager(
        databaseManager
    ) {}


CustomerMapping::~CustomerMapping() {}


bool CustomerMapping::updateMapping(
        CustomerMapping::CustomerId     customerId,
        const CustomerMapping::Mapping& mapping,
        unsigned                        threadId
    ) {
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

        QString queryString = QString("DELETE FROM customer_mapping WHERE customer_id = %1").arg(customerId);
        success = query.exec(queryString);
        if (success) {
            queryString = QString(
                "INSERT INTO customer_mapping(customer_id, server_id, primary_server) "
                "VALUES (:customer_id, :server_id, :primary_server)"
            );
            success = query.prepare(queryString);
            if (success) {
                Mapping::const_iterator mappingIterator    = mapping.constBegin();
                Mapping::const_iterator mappingEndIterator = mapping.constEnd();

                while (success && mappingIterator != mappingEndIterator) {
                    query.bindValue(":customer_id", customerId);
                    query.bindValue(":server_id", *mappingIterator);
                    query.bindValue(":primary_server", *mappingIterator == mapping.primaryServerId() ? true : false);

                    success = query.exec();
                    if (success) {
                        ++mappingIterator;
                    }
                }

                if (!success) {
                    logWrite(
                        QString("Failed to update customer mapping - customer id = %1, error = %2")
                        .arg(customerId)
                        .arg(query.lastError().text()),
                        true
                    );
                }
            } else {
                logWrite(
                    QString("Failed to prepare update of customer mapping - customer id = %1, error = %2")
                    .arg(customerId)
                    .arg(query.lastError().text()),
                    true
                );
            }
        } else {
            logWrite(
                QString("Failed to delete old customer mapping - customer id = %1, error = %2")
                .arg(customerId)
                .arg(query.lastError().text()),
                true
            );
        }

        if (supportsTransactions) {
            if (success) {
                success = database.commit();
                if (!success) {
                    logWrite(
                        QString("Failed to commit customer mapping - customer id = %1, error = %2")
                        .arg(customerId)
                        .arg(database.lastError().text()),
                        true
                    );
                }
            } else {
                bool rollbackSuccess = database.rollback();
                if (!rollbackSuccess) {
                    logWrite(
                        QString("Failed to rollback customer mapping - customer id = %1, error = %2")
                        .arg(customerId)
                        .arg(database.lastError().text()),
                        true
                    );
                }
            }
        }
    } else {
        logWrite(
            QString("Failed to open database - CustomerMapping::updateMapping: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


CustomerMapping::Mapping CustomerMapping::mapping(CustomerMapping::CustomerId customerId, unsigned threadId) {
    Mapping result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString(
            "SELECT server_id AS server_id, primary_server AS primary_server FROM customer_mapping "
            "WHERE customer_id = %1"
        ).arg(customerId);

        success = query.exec(queryString);
        if (success) {
            //int customerIdField = query.record().indexOf("customer_id");
            int serverIdField        = query.record().indexOf("server_id");
            int isPrimaryServerField = query.record().indexOf("primary_server");

            if (serverIdField >= 0 && isPrimaryServerField >= 0) {
                ServerSet servers;
                ServerId  primaryServerId = invalidServerId;
                while (success && query.next()) {
                    ServerId serverId        = query.value(serverIdField).toUInt(&success);
                    bool     isPrimaryServer = query.value(isPrimaryServerField).toBool();

                    if (success) {
                        if (isPrimaryServer) {
                            if (primaryServerId == invalidServerId) {
                                primaryServerId = serverId;
                            } else {
                                logWrite(
                                    QString("??? Warning: Multiple primary servers for customer %1")
                                    .arg(customerId),
                                    false
                                );
                            }
                        }

                        servers.insert(serverId);
                    }
                }

                result = Mapping(primaryServerId, servers);
            } else {
                logWrite(
                    QString("Failed to get field index - CustomerMapping::mapping: %1")
                    .arg(query.lastError().text()),
                    true
                );
            }
        } else {
            logWrite(
                QString("Failed SELECT - CustomerMapping::mapping: %1")
                .arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - CustomerMapping::mapping: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


CustomerMapping::MappingsByCustomerId CustomerMapping::mappings(
        CustomerMapping::ServerId serverId,
        unsigned                  threadId
    ) {
    MappingsByCustomerId result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT * FROM customer_mapping");
        if (serverId != invalidServerId) {
            queryString += QString(" WHERE server_id = %1").arg(serverId);
        }

        success = query.exec(queryString);
        if (success) {
            int customerIdField      = query.record().indexOf("customer_id");
            int serverIdField        = query.record().indexOf("server_id");
            int isPrimaryServerField = query.record().indexOf("primary_server");

            if (customerIdField >= 0 && serverIdField >= 0 && isPrimaryServerField >= 0) {
                while (success && query.next()) {
                    CustomerId customerId = query.value(customerIdField).toUInt(&success);
                    if (success && customerId != CustomerCapabilities::invalidCustomerId) {
                        ServerId serverId = query.value(serverIdField).toUInt(&success);
                        if (success && serverId != invalidServerId) {
                            bool isPrimaryServer = query.value(isPrimaryServerField).toBool();

                            result[customerId].insert(serverId);

                            if (isPrimaryServer) {
                                if (result[customerId].primaryServerId() == invalidServerId) {
                                    result[customerId].setPrimaryServer(serverId);
                                } else {
                                    logWrite(
                                        QString("??? Warning: Multiple primary servers for customer %1")
                                        .arg(customerId),
                                        false
                                    );
                                }
                            }
                        } else {
                            success = false;
                            logWrite(
                                QString("Invalid server ID - CustomerMapping::mappings, customer %1")
                                .arg(customerId),
                                true
                            );
                        }
                    } else {
                        success = false;
                        logWrite(
                            QString("Invalid server ID - CustomerMapping::mappings, customer %1")
                            .arg(customerId),
                            true
                        );
                    }
                }
            } else {
                success = false;
                logWrite(QString("Failed to get field index - CustomerMapping::mappings"), true);
            }
        } else {
            QSqlError lastError = query.lastError();
            logWrite(
                QString("Failed SELECT - CustomerMapping::mappings: %1").arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - CustomerMapping::mappings: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}
