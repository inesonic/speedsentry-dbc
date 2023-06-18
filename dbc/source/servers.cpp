/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref Servers class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QAbstractSocket>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include <cstdint>

#include "log.h"
#include "region.h"
#include "server.h"
#include "database_manager.h"
#include "servers.h"

Servers::Servers(DatabaseManager* databaseManager, QObject* parent):QObject(parent) {
    currentDatabaseManager = databaseManager;
}


Servers::~Servers() {}


Server Servers::getServer(ServerId serverId, unsigned threadId) const {
    Server result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT * FROM servers WHERE server_id = %1").arg(serverId);

        success = query.exec(queryString);
        if (success) {
            if (query.first()) {
                result = convertQueryToServer(query, &success);
            } else {
                // We're OK but there was no record for the requested region.
            }
        } else {
            logWrite(QString("Failed SELECT - Servers::getServer: 1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(QString("Failed to open database - Servers::getServer: %1").arg(database.lastError().text()), true);
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Server Servers::getServer(const QString& identifier, unsigned threadId) const {
    Server result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT * FROM servers WHERE identifier = '%1'").arg(identifier);

        success = query.exec(queryString);
        if (success) {
            if (query.first()) {
                result = convertQueryToServer(query, &success);
            } else {
                // We're OK but there was no record for the requested region.
            }
        } else {
            logWrite(QString("Failed SELECT - Servers::getServer: 1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(QString("Failed to open database - Servers::getServer: %1").arg(database.lastError().text()), true);
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Server Servers::createServer(
        RegionId       regionId,
        const QString& identifier,
        Server::Status status,
        unsigned       threadId
    ) {
    Server result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString statusString = Server::toString(status);
        QString queryString  = QString(
            "INSERT INTO servers("
                "region_id,"
                "identifier,"
                "status,"
                "monitor_service_rate,"
                "cpu_loading,"
                "memory_loading"
            ") VALUES ("
                "%1,"
                "'%2',"
                "'%3',"
                "0.0,"
                "0.0,"
                "0.0"
            ")"
        ).arg(regionId).arg(identifier, statusString);

        success = query.exec(queryString);
        if (success) {
            QVariant serverIdVariant = query.lastInsertId();
            if (serverIdVariant.isValid()) {
                unsigned unsignedServerId = serverIdVariant.toUInt(&success);
                if (success) {
                    if (unsignedServerId <= 0xFFFF) {
                        ServerId serverId = static_cast<ServerId>(unsignedServerId);

                        result = Server(
                            serverId,
                            regionId,
                            identifier,
                            status,
                            0.0F,
                            0.0F,
                            0.0F
                        );
                    } else {
                        success = false;
                        logWrite(QString("Invalid server ID, out of range - Servers::createServer"), true);
                    }
                } else {
                    logWrite(QString("Invalid server ID, not integer- Servers::createServer"), true);
                }
            } else {
                logWrite(
                    QString("Failed to get field index - Servers::createServer: %1").arg(query.lastError().text()),
                    true
                );
            }
        } else {
            logWrite(QString("Failed INSERT - Servers::createServer: 1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(QString("Failed to open database - Servers::createServer: %1").arg(database.lastError().text()), true);
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


bool Servers::modifyServer(const Server& server, unsigned threadId) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString  = QString(
            "UPDATE servers SET "
                "region_id = %1, "
                "identifier = '%2', "
                "status = '%3', "
                "monitor_service_rate = %4, "
                "cpu_loading = %5, "
                "memory_loading = %6 "
            "WHERE "
                "server_id = %7"
        ).arg(server.regionId())
         .arg(server.identifier(), Server::toString(server.status()))
         .arg(server.monitorsPerSecond())
         .arg(server.cpuLoading())
         .arg(server.memoryLoading())
         .arg(server.serverId());

        success = query.exec(queryString);
        if (!success) {
            logWrite(QString("Failed UPDATE - Servers::modifyServer: 1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(QString("Failed to open database - Servers::modifyServer: %1").arg(database.lastError().text()), true);
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


bool Servers::deleteServer(const Server& server, unsigned threadId) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString = QString("DELETE FROM servers WHERE server_id = %1").arg(server.serverId());
        success = query.exec(queryString);
        if (!success) {
            logWrite(QString("Failed DELETE - Servers::deleteServer: 1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(QString("Failed to open database - Servers::deleteServer: %1").arg(database.lastError().text()), true);
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


Servers::ServerList Servers::getServers(RegionId regionId, Server::Status status, unsigned threadId) {
    ServerList result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT * FROM servers");
        if (status != Status::ALL_UNKNOWN) {
            QString statusString = Server::toString(status);

            if (regionId != Region::invalidRegionId) {
                queryString += QString(" WHERE region_id = %1 AND status = '%2'").arg(regionId).arg(statusString);
            } else {
                queryString += QString(" WHERE status = '%1'").arg(statusString);
            }
        } else if (regionId != Region::invalidRegionId) {
            queryString += QString(" WHERE region_id = %1").arg(regionId);
        }

        queryString += QString(" ORDER BY region_id ASC, monitor_service_rate DESC, server_id ASC");
        success = query.exec(queryString);
        if (success) {
            while (success && query.next()) {
                Server server = convertQueryToServer(query, &success);
                if (success) {
                    result.append(server);
                }
            }
        } else {
            logWrite(QString("Failed SELECT - Servers::getServer: 1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(QString("Failed to open database - Servers::getServers: %1").arg(database.lastError().text()), true);
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Servers::ServersById Servers::getServersById(unsigned threadId) {
    ServersById result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT * FROM servers");
        success = query.exec(queryString);
        if (success) {
            while (success && query.next()) {
                Server server = convertQueryToServer(query, &success);
                if (success) {
                    result.insert(server.serverId(), server);
                }
            }
        } else {
            logWrite(QString("Failed SELECT - Servers::getServersById: 1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - Servers::getServersById: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Server Servers::convertQueryToServer(const QSqlQuery& sqlQuery, bool* success) {
    Server result;
    bool   ok = true;

    int serverIdField           = sqlQuery.record().indexOf("server_id");
    int regionIdField           = sqlQuery.record().indexOf("region_id");
    int identifierField         = sqlQuery.record().indexOf("identifier");
    int statusField             = sqlQuery.record().indexOf("status");
    int monitorServiceRateField = sqlQuery.record().indexOf("monitor_service_rate");
    int cpuLoadingField         = sqlQuery.record().indexOf("cpu_loading");
    int memoryLoadingField      = sqlQuery.record().indexOf("memory_loading");

    if (serverIdField >= 0           &&
        regionIdField >= 0           &&
        identifierField >= 0         &&
        statusField >= 0             &&
        monitorServiceRateField >= 0 &&
        cpuLoadingField >= 0         &&
        memoryLoadingField >= 0         ) {
        ServerId      serverId           = Server::invalidServerId;
        RegionId      regionId           = Region::invalidRegionId;
        Status        status             = Status::ALL_UNKNOWN;
        float         monitorServiceRate = 0;
        float         cpuLoading         = 0;
        float         memoryLoading      = 0;
        QString       identifier;

        unsigned unsignedServerId = sqlQuery.value(serverIdField).toUInt(&ok);
        if (ok) {
            if (unsignedServerId > 0xFFFF) {
                ok = false;
                logWrite(QString("Invalid server ID: %1").arg(unsignedServerId), true);
            } else {
                serverId = static_cast<ServerId>(unsignedServerId);
            }
        }

        if (ok) {
            unsigned unsignedRegionId = sqlQuery.value(regionIdField).toUInt(&ok);
            if (ok) {
                if (unsignedRegionId > 0xFFFF) {
                    ok = false;
                    logWrite(QString("Invalid region ID: ").arg(unsignedRegionId), true);
                } else {
                    regionId = static_cast<RegionId>(unsignedRegionId);
                }
            }
        }

        if (ok) {
            identifier = sqlQuery.value(identifierField).toString();

            QString statusString = sqlQuery.value(statusField).toString().trimmed().toLower();
            status = Server::toStatus(statusString, &ok);
            if (!ok) {
                logWrite(QString("Invalid status, server ID %1: \"%2\"").arg(serverId).arg(statusString), true);
            }
        }

        if (ok) {
            monitorServiceRate = sqlQuery.value(monitorServiceRateField).toUInt(&ok);
        }

        if (ok) {
            cpuLoading = sqlQuery.value(cpuLoadingField).toFloat(&ok);
            if (cpuLoading < 0.0F || cpuLoading > 1.0F)  {
                ok = false;
            }
        }

        if (ok) {
            memoryLoading = sqlQuery.value(memoryLoadingField).toFloat(&ok);
            if (memoryLoading < 0.0F || memoryLoading > 1.0F) {
                ok = false;
            }
        }

        if (ok) {
            result = Server(
                serverId,
                regionId,
                identifier,
                status,
                monitorServiceRate,
                cpuLoading,
                memoryLoading
            );
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
