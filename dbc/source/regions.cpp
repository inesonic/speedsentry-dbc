/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref Regions class.
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
#include "region.h"
#include "database_manager.h"
#include "regions.h"

Regions::Regions(DatabaseManager* databaseManager, QObject* parent):QObject(parent) {
    currentDatabaseManager = databaseManager;
}


Regions::~Regions() {}


Region Regions::getRegion(RegionId regionId, unsigned threadId) const {
    Region result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT region_name FROM regions WHERE region_id = %1").arg(regionId);
        success = query.exec(queryString);
        if (success) {
            if (query.first()) {
                int fieldNumber = query.record().indexOf("region_name");
                if (fieldNumber >= 0) {
                    result = Region(regionId, query.value(fieldNumber).toString());
                } else {
                    logWrite(
                        QString("Failed to get field index - Regions::getRegion: %1")
                        .arg(query.lastError().text()),
                        true
                    );
                }
            } else {
                // We're OK but there was no record for the requested region.
            }
        } else {
            logWrite(QString("Failed SELECT - Regions::getRegion: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(QString("Failed to open database - Regions::getRegion: %1").arg(database.lastError().text()), true);
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Region Regions::createRegion(const QString& regionName, unsigned threadId) {
    Region result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString = QString("INSERT INTO regions(region_name) VALUES ('%1')").arg(escape(regionName));
        success = query.exec(queryString);
        if (success) {
            QVariant regionId = query.lastInsertId();
            if (regionId.isValid()) {
                result = Region(regionId.toUInt(), regionName);
            } else {
                logWrite(
                    QString("Failed to get field index - Regions::createRegion: %1")
                    .arg(query.lastError().text()),
                    true
                );
            }
        } else {
            logWrite(QString("Failed INSERT - Regions::createRegion: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - Regions::createRegion: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


bool Regions::modifyRegion(const Region& region, unsigned threadId) {
    Region result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString = QString("UPDATE regions SET region_name = '%1' WHERE region_id = %2")
                              .arg(escape(region.regionName()))
                              .arg(region.regionId());

        success = query.exec(queryString);
        if (!success) {
            logWrite(QString("Failed UPDATE - Regions::modifyRegion: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - Regions::modifyRegion: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


bool Regions::deleteRegion(const Region& region, unsigned threadId) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString = QString("DELETE FROM regions WHERE region_id = %1").arg(region.regionId());
        success = query.exec(queryString);
        if (!success) {
            logWrite(QString("Failed DELETE - Regions::deleteRegion: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - Regions::deleteRegion: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


Regions::RegionHash Regions::getAllRegions(unsigned threadId) {
    RegionHash result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        success = query.exec(QString("SELECT * FROM regions"));
        if (success) {
            int regionIdField   = query.record().indexOf("region_id");
            int regionNameField = query.record().indexOf("region_name");

            if (regionIdField >= 0 && regionNameField >= 0) {
                while (success && query.next()) {
                    unsigned unsignedRegionId = query.value(regionIdField).toUInt(&success);
                    if (unsignedRegionId <= 0xFFFF) {
                        QString          regionName = query.value(regionNameField).toString();
                        Region::RegionId regionId   = static_cast<Region::RegionId>(unsignedRegionId);
                        result.insert(regionId, Region(regionId, regionName));
                    } else {
                        logWrite(QString("Invalid region ID -- Regions::getAllRegions"), true);
                        success = false;
                    }
                }

                if (!success) {
                    result = RegionHash();
                }
            } else {
                logWrite(
                    QString("Failed to get field index - Regions::getAllRegions: %1")
                    .arg(query.lastError().text()),
                    true
                );
            }
        } else {
            logWrite(QString("Failed SELECT - Regions::getAllRegions: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - Regions::getAllRegions: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}
