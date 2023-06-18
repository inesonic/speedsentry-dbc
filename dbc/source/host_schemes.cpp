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
* This header implements the \ref HostSchemes class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QUrl>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include <cstdint>

#include "log.h"
#include "host_scheme.h"
#include "database_manager.h"
#include "host_schemes.h"

HostSchemes::HostSchemes(
        DatabaseManager* databaseManager,
        QObject*         parent
    ):QObject(
        parent
    ),currentDatabaseManager(
        databaseManager
    ) {}


HostSchemes::~HostSchemes() {}


HostScheme HostSchemes::getHostScheme(HostSchemes::HostSchemeId hostSchemeId, unsigned threadId) const {
    HostScheme result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT * FROM host_scheme WHERE host_scheme_id = %1").arg(hostSchemeId);

        success = query.exec(queryString);
        if (success) {
            if (query.first()) {
                result = convertQueryToHostScheme(query, &success);
            } else {
                // We're OK but there was no record for the requested region.
            }
        } else {
            logWrite(QString("Failed SELECT - HostSchemes::getHostScheme: ").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database HostSchemes::getHostScheme: ").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


HostScheme HostSchemes::createHostScheme(
        HostSchemes::CustomerId customerId,
        const QUrl&             url,
        unsigned                threadId
    ) const {
    HostScheme result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString urlString   = QString("%1://%2").arg(url.scheme(), url.authority());
        QString queryString = QString(
            "INSERT INTO host_scheme("
                "customer_id,"
                "host,"
                "ssl_expiration_timestamp"
            ") VALUES ("
                "%1,"
                "'%2',"
                "0"
            ")"
        ).arg(customerId).arg(escape(urlString));

        success = query.exec(queryString);
        if (success) {
            QVariant hostSchemeIdVariant = query.lastInsertId();
            if (hostSchemeIdVariant.isValid()) {
                unsigned long unsignedHostSchemeId = hostSchemeIdVariant.toUInt(&success);
                if (success) {
                    result = HostScheme(
                        static_cast<CustomerId>(unsignedHostSchemeId),
                        customerId,
                        QUrl(urlString),
                        0
                    );
                } else {
                    logWrite(QString("Invalid host/scheme ID, not integer - HostSchemes::createHostScheme."), true);
                }
            } else {
                logWrite(
                    QString("Failed to get field index - HostSchemes::createHostScheme: %1")
                    .arg(query.lastError().text()),
                    true
                );
            }
        } else {
            logWrite(
                QString("Failed INSERT - HostSchemes::createHostScheme: %1").arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - HostScheme::createHostScheme: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


bool HostSchemes::modifyHostScheme(const HostScheme& hostScheme, unsigned threadId) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString  = QString(
            "UPDATE host_scheme SET "
                "customer_id = %1, "
                "host = '%2', "
                "ssl_expiration_timestamp = '%3' "
            "WHERE "
                "host_scheme_id = %4"
        ).arg(hostScheme.customerId())
         .arg(escape(hostScheme.url().toString()))
         .arg(hostScheme.sslExpirationTimestamp())
         .arg(hostScheme.hostSchemeId());

        success = query.exec(queryString);
        if (!success) {
            logWrite(QString("Failed UPDATE - HostSchemes::modifyHostScheme: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - HostScheme::modifyHostScheme: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


bool HostSchemes::deleteHostScheme(const HostScheme& hostScheme, unsigned threadId) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString = QString(
            "DELETE FROM host_scheme WHERE host_scheme_id = %1"
        ).arg(hostScheme.hostSchemeId());

        success = query.exec(queryString);
        if (!success) {
            logWrite(QString("Failed DELETE - HostSchemes::deleteHostScheme: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - HostScheme::deleteHostScheme: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


bool HostSchemes::deleteCustomerHostSchemes(HostSchemes::CustomerId customerId, unsigned threadId) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString = QString("DELETE FROM host_scheme WHERE customer_id = %1").arg(customerId);
        success = query.exec(queryString);
        if (!success) {
            logWrite(
                QString("Failed DELETE - HostSchemes::deleteCustomerHostScheme: %1").arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - HostScheme::deleteCustomerHostSchemes: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


HostSchemes::HostSchemeHash HostSchemes::getHostSchemes(HostSchemes::CustomerId customerId, unsigned threadId) {
    HostSchemeHash result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT * FROM host_scheme");
        if (customerId != HostScheme::invalidCustomerId) {
            queryString += QString(" WHERE customer_id = %1").arg(customerId);
        }

        success = query.exec(queryString);
        if (success) {
            while (success && query.next()) {
                HostScheme hostScheme = convertQueryToHostScheme(query, &success);
                if (success) {
                    result.insert(hostScheme.hostSchemeId(), hostScheme);
                }
            }
        } else {
            logWrite(QString("Failed SELECT - HostSchemes::getHostScheme: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - HostScheme::getHostSchemes: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


HostScheme HostSchemes::convertQueryToHostScheme(const QSqlQuery& sqlQuery, bool* success) {
    HostScheme result;
    bool       ok = true;

    int hostSchemeIdField           = sqlQuery.record().indexOf("host_scheme_id");
    int customerIdField             = sqlQuery.record().indexOf("customer_id");
    int hostField                   = sqlQuery.record().indexOf("host");
    int sslExpirationTimestampField = sqlQuery.record().indexOf("ssl_expiration_timestamp");

    if (hostSchemeIdField >= 0           &&
        customerIdField >= 0             &&
        hostField >= 0                   &&
        sslExpirationTimestampField >= 0    ) {
        QString            host                   = sqlQuery.value(hostField).toString();
        HostSchemeId       hostSchemeId           = sqlQuery.value(hostSchemeIdField).toUInt(&ok);
        CustomerId         customerId             = HostScheme::invalidCustomerId;
        unsigned long long sslExpirationTimestamp = HostScheme::invalidSslExpirationTimestamp;

        if (ok) {
            customerId = sqlQuery.value(customerIdField).toUInt(&ok);
        }

        if (ok) {
            sslExpirationTimestamp = sqlQuery.value(sslExpirationTimestampField).toULongLong(&ok);
        }

        if (ok) {
            result = HostScheme(hostSchemeId, customerId, QUrl(host), sslExpirationTimestamp);
        }
    } else {
        logWrite(QString("Failed to get field index: %1").arg(sqlQuery.lastError().text()), true);
        ok = false;
    }

    if (success != nullptr) {
        *success = ok;
    }

    return result;
}
